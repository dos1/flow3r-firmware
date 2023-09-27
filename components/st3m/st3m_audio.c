#include "st3m_audio.h"
#include "st3m_scope.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "flow3r_bsp.h"
#include "flow3r_bsp_max98091.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

static const char *TAG = "st3m-audio";

#define TIMEOUT_MS 1000

static void _audio_player_task(void *data);
static void _jacksense_update_task(void *data);
static bool _headphones_connected(void);

// used for exp(vol_dB * NAT_LOG_DB)
#define NAT_LOG_DB 0.1151292546497023

// placeholder for "fake mute" -inf dB (we know floats can do that but we have
// trust issues when using NAN)
#define SILLY_LOW_VOLUME_DB (-10000.)

const static float headphones_maximum_volume_system_dB = 3;
const static float speaker_maximum_volume_system_dB = 14;

// Output, either speaker or headphones. Holds volume/mute state and limits,
// and calculated software volume.
//
// An output's apply function configures the actual physical output, ie. by
// programming the codec.
typedef struct st3m_audio_output {
    float volume;
    float volume_max;
    float volume_min;
    float volume_max_system;
    int32_t volume_software;
    bool mute;

    void (*apply)(struct st3m_audio_output *out);
} st3m_audio_output_t;

// All _output_* functions are not thread-safe, access must be synchronized via
// locks.

// Apply output settings to actual output channel, calculate software volume if
// output is active.
static void _output_apply(st3m_audio_output_t *out) { out->apply(out); }

static void _output_set_mute(st3m_audio_output_t *out, bool mute) {
    out->mute = mute;
    _output_apply(out);
}

static float _output_set_volume(st3m_audio_output_t *out, float vol_dB) {
    if (vol_dB > out->volume_max) {
        vol_dB = out->volume_max;
    }
    if (vol_dB < out->volume_min) {
        vol_dB = SILLY_LOW_VOLUME_DB;
    }
    out->volume = vol_dB;
    _output_apply(out);
    return vol_dB;
}

static float _output_adjust_volume(st3m_audio_output_t *out, float vol_dB) {
    if (out->volume < out->volume_min) {
        if (vol_dB > 0) {
            _output_set_volume(out, out->volume_min);
        }
    } else {
        _output_set_volume(out, out->volume + vol_dB);
    }
    return out->volume;
}

static float _output_set_maximum_volume(st3m_audio_output_t *out,
                                        float vol_dB) {
    if (vol_dB > out->volume_max_system) {
        vol_dB = out->volume_max_system;
    }
    if (vol_dB < out->volume_min) {
        vol_dB = out->volume_min;
    }
    out->volume_max = vol_dB;
    if (out->volume > out->volume_max) {
        out->volume = out->volume_max;
    }
    _output_apply(out);
    return vol_dB;
}

static float _output_set_minimum_volume(st3m_audio_output_t *out,
                                        float vol_dB) {
    if (vol_dB > out->volume_max) {
        vol_dB = out->volume_max;
    }
    if (vol_dB + 1 < SILLY_LOW_VOLUME_DB) {
        vol_dB = SILLY_LOW_VOLUME_DB + 1.;
    }
    out->volume_min = vol_dB;
    if (out->volume < out->volume_min) {
        out->volume = out->volume_min;
    }
    _output_apply(out);
    return vol_dB;
}

static float _output_get_volume_relative(st3m_audio_output_t *out) {
    float ret = out->volume;
    // fake mute
    if (ret < out->volume_min) return 0;
    float vol_range = out->volume_max - out->volume_min;
    // shift to above zero
    ret -= out->volume_min;
    // restrict to 0..1 range
    ret /= vol_range;
    // shift to 0.01 to 0.99 range to distinguish from fake mute
    return (ret * 0.99) + 0.01;
}

// Output apply function for headphones.
static void _audio_headphones_apply(st3m_audio_output_t *out) {
    bool mute = out->mute;
    float vol_dB = out->volume;
    if (out->volume < (SILLY_LOW_VOLUME_DB + 1)) mute = true;

    bool headphones = _headphones_connected();
    if (!headphones) {
        mute = true;
    }

    float hardware_volume_dB =
        flow3r_bsp_audio_headphones_set_volume(mute, vol_dB);

    // do the fine steps in software
    // note: synchronizing both hw and software volume changes is somewhat
    // tricky
    float software_volume_dB = vol_dB - hardware_volume_dB;
    if (software_volume_dB > 0) software_volume_dB = 0;
    out->volume_software =
        (int32_t)(32768 * expf(software_volume_dB * NAT_LOG_DB));
}

// Output apply function for speaker.
static void _audio_speaker_apply(st3m_audio_output_t *out) {
    bool mute = out->mute;
    float vol_dB = out->volume;
    if (out->volume < (SILLY_LOW_VOLUME_DB + 1)) mute = true;

    bool headphones = _headphones_connected();
    if (headphones) {
        mute = true;
    }

    float hardware_volume_dB =
        flow3r_bsp_audio_speaker_set_volume(mute, vol_dB);

    // do the fine steps in software
    // note: synchronizing both hw and software volume changes is somewhat
    // tricky
    float software_volume_dB = vol_dB - hardware_volume_dB;
    if (software_volume_dB > 0) software_volume_dB = 0;
    out->volume_software =
        (int32_t)(32768. * expf(software_volume_dB * NAT_LOG_DB));
}

// Global state structure. Guarded by state_mutex.
typedef struct {
    flow3r_bsp_audio_jacksense_state_t jacksense;

    // True if system should pretend headphones are plugged in.
    bool headphones_detection_override;

    // The two output channels.
    st3m_audio_output_t headphones;
    st3m_audio_output_t speaker;

    float headset_mic_gain_dB;
    int16_t headset_mic_gain_software;
    float onboard_mic_gain_dB;
    int16_t onboard_mic_gain_software;
    float line_in_gain_dB;
    int16_t line_in_gain_software;

    uint8_t speaker_eq_on;

    bool headset_mic_allowed;
    bool onboard_mic_allowed;
    bool line_in_allowed;
    bool onboard_mic_to_speaker_allowed;
    st3m_audio_input_source_t engine_source;
    st3m_audio_input_source_t engine_target_source;
    st3m_audio_input_source_t thru_source;
    st3m_audio_input_source_t thru_target_source;
    st3m_audio_input_source_t source;

    // Software-based audio pipe settings.
    int32_t input_thru_vol;
    int32_t input_thru_vol_int;
    bool input_thru_mute;

    // Main player function callback.
    st3m_audio_player_function_t function;
} st3m_audio_state_t;

SemaphoreHandle_t state_mutex;
#define LOCK xSemaphoreTakeRecursive(state_mutex, portMAX_DELAY)
#define UNLOCK xSemaphoreGiveRecursive(state_mutex)

static st3m_audio_state_t state = {
    .jacksense =
        {
            .headphones = false,
            .headset = false,
            .line_in = false,
        },
    .headphones_detection_override = false,
    .headphones =
        {
            .volume = 0,
            .mute = false,
            .volume_max = headphones_maximum_volume_system_dB,
            .volume_min = headphones_maximum_volume_system_dB - 70,
            .volume_max_system = headphones_maximum_volume_system_dB,
            .apply = _audio_headphones_apply,
        },
    .speaker =
        {
            .volume = 0,
            .mute = false,
            .volume_max = speaker_maximum_volume_system_dB,
            .volume_min = speaker_maximum_volume_system_dB - 60,
            .volume_max_system = speaker_maximum_volume_system_dB,
            .apply = _audio_speaker_apply,
        },

    .speaker_eq_on = true,

    .headset_mic_allowed = true,
    .onboard_mic_allowed = true,
    .line_in_allowed = true,
    .onboard_mic_to_speaker_allowed = false,
    .headset_mic_gain_dB = 0,
    .onboard_mic_gain_dB = 0,
    .line_in_gain_dB = 0,
    .headset_mic_gain_software = 256,
    .onboard_mic_gain_software = 256,
    .line_in_gain_software = 256,

    .input_thru_vol = 0,
    .input_thru_vol_int = 32768,
    .input_thru_mute = false, // deprecated

    .engine_target_source = st3m_audio_input_source_none,
    .engine_source = st3m_audio_input_source_none,
    .thru_source = st3m_audio_input_source_none,
    .thru_target_source = st3m_audio_input_source_none,
    .source = st3m_audio_input_source_none,
    .function = st3m_audio_player_function_dummy,
};

// Returns whether we should be outputting audio through headphones. If not,
// audio should be output via speaker.
//
// Lock must be taken.
static bool _headphones_connected(void) {
    return state.jacksense.headphones || state.headphones_detection_override;
}

static void _audio_input_set_source(st3m_audio_input_source_t source) {
    LOCK;
    st3m_audio_input_source_t prev_source = state.source;
    UNLOCK;
    if (source == prev_source) return;

    flow3r_bsp_audio_input_source_t fsource;
    switch (source) {
        case st3m_audio_input_source_line_in:
            fsource = flow3r_bsp_audio_input_source_line_in;
            break;
        case st3m_audio_input_source_onboard_mic:
            fsource = flow3r_bsp_audio_input_source_onboard_mic;
            break;
        case st3m_audio_input_source_headset_mic:
            fsource = flow3r_bsp_audio_input_source_headset_mic;
            break;
        case st3m_audio_input_source_none:
            fsource = flow3r_bsp_audio_input_source_none;
            break;
        case st3m_audio_input_source_auto:
            fsource = flow3r_bsp_audio_input_source_none;
            break;
    }
    LOCK;
    state.source = source;
    UNLOCK;
    flow3r_bsp_audio_input_set_source(fsource);
}

static bool _check_engine_source_avail(st3m_audio_input_source_t source) {
    switch (source) {
        case st3m_audio_input_source_none:
            return true;
        case st3m_audio_input_source_auto:
            return true;
        case st3m_audio_input_source_line_in:
            return state.line_in_allowed && state.jacksense.line_in;
        case st3m_audio_input_source_headset_mic:
            return state.headset_mic_allowed && state.jacksense.headset;
        case st3m_audio_input_source_onboard_mic:
            return state.onboard_mic_allowed;
    }
    return false;
}

bool st3m_audio_input_engine_get_source_avail(
    st3m_audio_input_source_t source) {
    bool ret = false;
    if (source == st3m_audio_input_source_auto) {
        LOCK;
        ret =
            ret || _check_engine_source_avail(st3m_audio_input_source_line_in);
        ret = ret ||
              _check_engine_source_avail(st3m_audio_input_source_headset_mic);
        ret = ret ||
              _check_engine_source_avail(st3m_audio_input_source_onboard_mic);
        UNLOCK;
    } else {
        LOCK;
        ret = _check_engine_source_avail(source);
        UNLOCK;
    }
    return ret;
}

void _update_engine_source() {
    st3m_audio_input_source_t source;
    LOCK;
    source = state.engine_target_source;
    if (source == st3m_audio_input_source_auto) {
        if (_check_engine_source_avail(st3m_audio_input_source_line_in)) {
            source = st3m_audio_input_source_line_in;
        } else if (_check_engine_source_avail(
                       st3m_audio_input_source_headset_mic)) {
            source = st3m_audio_input_source_headset_mic;
        } else if (_check_engine_source_avail(
                       st3m_audio_input_source_onboard_mic)) {
            source = st3m_audio_input_source_onboard_mic;
        } else {
            source = st3m_audio_input_source_none;
        }
    }
    bool avail = _check_engine_source_avail(source);
    UNLOCK;
    source = avail ? source : st3m_audio_input_source_none;
    LOCK;
    bool return_early = state.source == source;
    state.engine_source = source;
    UNLOCK;
    if (return_early) return;

    if (avail) {
        switch (source) {
            case st3m_audio_input_source_none:
                break;
            case st3m_audio_input_source_auto:
                break;
            case st3m_audio_input_source_line_in:
                if (avail) {
                    _audio_input_set_source(st3m_audio_input_source_line_in);
                }
                break;
            case st3m_audio_input_source_headset_mic:
                if (avail) {
                    _audio_input_set_source(
                        st3m_audio_input_source_headset_mic);
                }
                break;
            case st3m_audio_input_source_onboard_mic:
                if (avail) {
                    _audio_input_set_source(
                        st3m_audio_input_source_onboard_mic);
                }
                break;
        }
    }
}

void st3m_audio_input_engine_set_source(st3m_audio_input_source_t source) {
    LOCK;
    state.engine_target_source = source;
    UNLOCK;
}

static bool _check_thru_source_avail(st3m_audio_input_source_t source) {
    bool avail = _check_engine_source_avail(source);
    if ((source == st3m_audio_input_source_onboard_mic) &&
        (!state.onboard_mic_to_speaker_allowed) &&
        (!state.jacksense.headphones)) {
        avail = false;
    }

    if ((state.engine_source != st3m_audio_input_source_none) &&
        (state.engine_source != source)) {
        avail = false;
    }
    return avail;
}

bool st3m_audio_input_thru_get_source_avail(st3m_audio_input_source_t source) {
    bool ret = false;
    if (source == st3m_audio_input_source_auto) {
        LOCK;
        ret = ret || _check_thru_source_avail(st3m_audio_input_source_line_in);
        ret = ret ||
              _check_thru_source_avail(st3m_audio_input_source_headset_mic);
        ret = ret ||
              _check_thru_source_avail(st3m_audio_input_source_onboard_mic);
        UNLOCK;
    } else {
        LOCK;
        ret = _check_thru_source_avail(source);
        UNLOCK;
    }
    return ret;
}

void _update_thru_source() {
    st3m_audio_input_source_t source;

    LOCK;
    source = state.thru_target_source;
    if (source == st3m_audio_input_source_auto) {
        if (state.engine_source != st3m_audio_input_source_none) {
            source = state.engine_source;
        } else if (_check_thru_source_avail(st3m_audio_input_source_line_in)) {
            source = st3m_audio_input_source_line_in;
        } else if (_check_thru_source_avail(
                       st3m_audio_input_source_headset_mic)) {
            source = st3m_audio_input_source_headset_mic;
        } else if (_check_thru_source_avail(
                       st3m_audio_input_source_onboard_mic)) {
            source = st3m_audio_input_source_onboard_mic;
        } else {
            source = st3m_audio_input_source_none;
        }
    }

    bool avail = _check_thru_source_avail(source);
    UNLOCK;
    source = avail ? source : st3m_audio_input_source_none;

    bool return_flag = false;
    LOCK;
    if (state.engine_source != st3m_audio_input_source_none) {
        if (state.engine_source == source) {
            state.thru_source = state.engine_source;
        } else {
            state.thru_source = st3m_audio_input_source_none;
        }
        return_flag = true;
    }
    state.thru_source = source;
    if (state.thru_source == state.source) {
        return_flag = true;
    }
    UNLOCK;
    if (return_flag) return;

    switch (source) {
        case st3m_audio_input_source_none:
            if (avail) {
                _audio_input_set_source(st3m_audio_input_source_none);
            }
            break;
        case st3m_audio_input_source_line_in:
            if (avail) {
                _audio_input_set_source(st3m_audio_input_source_line_in);
            }
            break;
        case st3m_audio_input_source_headset_mic:
            if (avail) {
                _audio_input_set_source(st3m_audio_input_source_headset_mic);
            }
            break;
        case st3m_audio_input_source_onboard_mic:
            if (avail) {
                _audio_input_set_source(st3m_audio_input_source_onboard_mic);
            }
            break;
        case st3m_audio_input_source_auto:
            // should not be possible
            break;
    }
}

void st3m_audio_input_thru_set_source(st3m_audio_input_source_t source) {
    LOCK;
    state.thru_target_source = source;
    UNLOCK;
}

void _update_sink() {
    flow3r_bsp_audio_jacksense_state_t st;
    flow3r_bsp_audio_read_jacksense(&st);
    static bool _speaker_eq_on_prev = false;
    // Update volume to trigger mutes if needed. But only do that if the
    // jacks actually changed.
    LOCK;
    if ((memcmp(&state.jacksense, &st,
                sizeof(flow3r_bsp_audio_jacksense_state_t)) != 0) ||
        (_speaker_eq_on_prev != state.speaker_eq_on)) {
        memcpy(&state.jacksense, &st,
               sizeof(flow3r_bsp_audio_jacksense_state_t));
        _output_apply(&state.speaker);
        _output_apply(&state.headphones);
        bool _speaker_eq = (!_headphones_connected()) && state.speaker_eq_on;
        flow3r_bsp_max98091_set_speaker_eq(_speaker_eq);
        _speaker_eq_on_prev = state.speaker_eq_on;
    }
    UNLOCK;
}

static void _update_routing() {
    // order important
    _update_sink();
    _update_engine_source();
    _update_thru_source();
}

void st3m_audio_player_function_dummy(int16_t *rx, int16_t *tx, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        tx[i] = 0;
    }
}

void st3m_audio_init(void) {
    state_mutex = xSemaphoreCreateRecursiveMutex();
    assert(state_mutex != NULL);
    state.function = st3m_audio_player_function_dummy;

    flow3r_bsp_audio_init();

    _update_routing();
    _output_apply(&state.speaker);
    _output_apply(&state.headphones);
    bool _speaker_eq = (!_headphones_connected()) && state.speaker_eq_on;
    flow3r_bsp_max98091_set_speaker_eq(_speaker_eq);

    xTaskCreate(&_audio_player_task, "audio", 10000, NULL,
                configMAX_PRIORITIES - 1, NULL);
    xTaskCreate(&_jacksense_update_task, "jacksense", 2048, NULL, 8, NULL);
    ESP_LOGI(TAG, "Audio task started");
}

static void _audio_player_task(void *data) {
    (void)data;

    int16_t buffer_tx[FLOW3R_BSP_AUDIO_DMA_BUFFER_SIZE * 2];
    int16_t buffer_rx[FLOW3R_BSP_AUDIO_DMA_BUFFER_SIZE * 2];
    int16_t buffer_rx_dummy[FLOW3R_BSP_AUDIO_DMA_BUFFER_SIZE * 2];
    memset(buffer_tx, 0, sizeof(buffer_tx));
    memset(buffer_rx, 0, sizeof(buffer_rx));
    memset(buffer_rx_dummy, 0, sizeof(buffer_rx));
    size_t count;

    st3m_audio_input_source_t source_prev = st3m_audio_input_source_none;

    while (true) {
        count = 0;
        esp_err_t ret =
            flow3r_bsp_audio_read(buffer_rx, sizeof(buffer_rx), &count, 1000);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "audio_read: %s", esp_err_to_name(ret));
            abort();
        }
        if (count != sizeof(buffer_rx)) {
            ESP_LOGE(TAG, "audio_read: count (%d) != length (%d)\n", count,
                     sizeof(buffer_rx));
            continue;
        }

        LOCK;
        st3m_audio_input_source_t source = state.source;
        st3m_audio_input_source_t engine_source = state.engine_source;
        st3m_audio_input_source_t thru_source = state.thru_source;
        bool headphones = _headphones_connected();
        st3m_audio_player_function_t function = state.function;
        int32_t software_volume = headphones ? state.headphones.volume_software
                                             : state.speaker.volume_software;
        bool input_thru_mute = state.input_thru_mute;
        int32_t input_thru_vol_int = state.input_thru_vol_int;
        UNLOCK;

        // <RX SIGNAL PREPROCESSING>

        int16_t rx_gain = 256;  // unity
        uint8_t rx_chan = 3;    // stereo = 0; left = 1; right = 2; off = 3;
        if (source != source_prev) {
            // state change: throw away buffer
            source_prev = source;
            memset(buffer_rx, 0, sizeof(buffer_rx));
        } else if (source == st3m_audio_input_source_headset_mic) {
            // headset has its own gain thing going on, leave at unity
            rx_chan = 1;
        } else if (source == st3m_audio_input_source_line_in) {
            LOCK;
            int16_t gain = state.line_in_gain_software;
            UNLOCK;
            rx_gain = gain;
            rx_chan = 0;
        } else if (source == st3m_audio_input_source_onboard_mic) {
            LOCK;
            int16_t gain = state.onboard_mic_gain_software;
            UNLOCK;
            rx_gain = gain;
            rx_chan = 1;
        }

        if (rx_chan == 0) {
            // keep stereo image
            for (uint16_t i = 0; i < FLOW3R_BSP_AUDIO_DMA_BUFFER_SIZE * 2;
                 i++) {
                buffer_rx[i] = (((int32_t)buffer_rx[i]) * rx_gain) >> 8;
            }
        } else if (rx_chan < 3) {
            // mix one of the input channels to both rx stereo chans (easier
            // mono sources)
            for (uint16_t i = 0; i < FLOW3R_BSP_AUDIO_DMA_BUFFER_SIZE * 2;
                 i++) {
                uint16_t j = (i / 2) * 2 + rx_chan - 1;
                buffer_rx[i] = (((int32_t)buffer_rx[j]) * rx_gain) >> 8;
            }
        }

        int16_t *engine_rx;

        if (engine_source == st3m_audio_input_source_none) {
            engine_rx = buffer_rx_dummy;
        } else {
            engine_rx = buffer_rx;
        }

        // <ACTUAL ENGINE CALL>

        (*function)(engine_rx, buffer_tx, FLOW3R_BSP_AUDIO_DMA_BUFFER_SIZE * 2);

        // </ACTUAL ENGINE CALL>

        for (uint16_t i = 0; i < FLOW3R_BSP_AUDIO_DMA_BUFFER_SIZE; i++) {
            st3m_scope_write(buffer_tx[2 * i] >> 2);
        }

        for (int i = 0; i < (FLOW3R_BSP_AUDIO_DMA_BUFFER_SIZE * 2); i += 1) {
            int32_t acc = buffer_tx[i];

            acc = (acc * software_volume) >> 15;

            if ((thru_source != st3m_audio_input_source_none) &&
                ((engine_source == thru_source) ||
                 (engine_source == st3m_audio_input_source_none)) &&
                (!input_thru_mute)) {
                acc += (((int32_t)buffer_rx[i]) * input_thru_vol_int) >> 15;
            }
            buffer_tx[i] = acc;
        }

        flow3r_bsp_audio_write(buffer_tx, sizeof(buffer_tx), &count, 1000);
        if (count != sizeof(buffer_tx)) {
            ESP_LOGE(TAG, "audio_write: count (%d) != length (%d)\n", count,
                     sizeof(buffer_tx));
            abort();
        }
    }
}

static void _jacksense_update_task(void *data) {
    (void)data;

    TickType_t last_wake = xTaskGetTickCount();
    while (1) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(50));  // 20 Hz
        _update_routing();
    }
}

// BSP wrappers that don't need locking.

void st3m_audio_headphones_line_in_set_hardware_thru(bool enable) {
    return;
    flow3r_bsp_audio_headphones_line_in_set_hardware_thru(enable);
}

void st3m_audio_speaker_line_in_set_hardware_thru(bool enable) {
    return;
    flow3r_bsp_audio_speaker_line_in_set_hardware_thru(enable);
}

void st3m_audio_line_in_set_hardware_thru(bool enable) {
    return;
    flow3r_bsp_audio_line_in_set_hardware_thru(enable);
}

// Locked global state getters.

#define GETTER(ty, name, accessor) \
    ty st3m_##name(void) {         \
        LOCK;                      \
        ty res = accessor;         \
        UNLOCK;                    \
        return res;                \
    }

GETTER(bool, audio_headset_is_connected, state.jacksense.headset)
GETTER(bool, audio_headphones_is_connected,
       state.jacksense.headphones || state.headphones_detection_override)
GETTER(float, audio_headphones_get_volume_dB, state.headphones.volume)
GETTER(float, audio_speaker_get_volume_dB, state.speaker.volume)
GETTER(float, audio_headphones_get_minimum_volume_dB,
       state.headphones.volume_min)
GETTER(float, audio_speaker_get_minimum_volume_dB, state.speaker.volume_min)
GETTER(float, audio_headphones_get_maximum_volume_dB,
       state.headphones.volume_max)
GETTER(float, audio_speaker_get_maximum_volume_dB, state.speaker.volume_max)
GETTER(bool, audio_headphones_get_mute, state.headphones.mute)
GETTER(bool, audio_speaker_get_mute, state.speaker.mute)
GETTER(st3m_audio_input_source_t, audio_input_engine_get_source,
       state.engine_source)
GETTER(st3m_audio_input_source_t, audio_input_thru_get_source,
       state.thru_source)
GETTER(st3m_audio_input_source_t, audio_input_engine_get_target_source,
       state.engine_target_source)
GETTER(st3m_audio_input_source_t, audio_input_thru_get_target_source,
       state.thru_target_source)
GETTER(st3m_audio_input_source_t, audio_input_get_source, state.source)
GETTER(float, audio_headset_mic_get_gain_dB, state.headset_mic_gain_dB)
GETTER(float, audio_onboard_mic_get_gain_dB, state.onboard_mic_gain_dB)
GETTER(float, audio_line_in_get_gain_dB, state.line_in_gain_dB)
GETTER(float, audio_input_thru_get_volume_dB, state.input_thru_vol)
GETTER(bool, audio_input_thru_get_mute, state.input_thru_mute)
GETTER(bool, audio_speaker_get_eq_on, state.speaker_eq_on)
GETTER(bool, audio_headset_mic_get_allowed, state.headset_mic_allowed)
GETTER(bool, audio_onboard_mic_get_allowed, state.onboard_mic_allowed)
GETTER(bool, audio_line_in_get_allowed, state.line_in_allowed)
GETTER(bool, audio_onboard_mic_to_speaker_get_allowed,
       state.onboard_mic_to_speaker_allowed)
#undef GETTER

// Locked global API functions.

float st3m_audio_headset_mic_set_gain_dB(float gain_dB) {
    if (gain_dB > 42) gain_dB = 42;
    if (gain_dB < -9) gain_dB = -9;
    int8_t hw_gain = flow3r_bsp_audio_headset_set_gain_dB(gain_dB);
    int16_t software_gain = 256. * expf((gain_dB - hw_gain) * NAT_LOG_DB);
    LOCK;
    state.headset_mic_gain_dB = gain_dB;
    state.headset_mic_gain_software = software_gain;
    UNLOCK;
    return gain_dB;
}

float st3m_audio_line_in_set_gain_dB(float gain_dB) {
    if (gain_dB > 30) gain_dB = 30;
    if (gain_dB < -24) gain_dB = -24;
    int16_t software_gain = 256. * expf(gain_dB * NAT_LOG_DB);
    LOCK;
    state.line_in_gain_dB = gain_dB;
    state.line_in_gain_software = software_gain;
    UNLOCK;
    return gain_dB;
}

float st3m_audio_onboard_mic_set_gain_dB(float gain_dB) {
    if (gain_dB > 42) gain_dB = 42;
    if (gain_dB < 0) gain_dB = 0;
    int16_t software_gain = 256. * expf(gain_dB * NAT_LOG_DB);
    LOCK;
    state.onboard_mic_gain_dB = gain_dB;
    state.onboard_mic_gain_software = software_gain;
    UNLOCK;
    return gain_dB;
}

void st3m_audio_speaker_set_eq_on(bool enabled) {
    LOCK;
    state.speaker_eq_on = enabled;
    UNLOCK;
}

void st3m_audio_input_thru_set_mute(bool mute) {
    LOCK;
    state.input_thru_mute = mute;
    UNLOCK;
}

void st3m_audio_headset_mic_set_allowed(bool allowed) {
    LOCK;
    state.headset_mic_allowed = allowed;
    UNLOCK;
}

void st3m_audio_onboard_mic_set_allowed(bool allowed) {
    LOCK;
    state.onboard_mic_allowed = allowed;
    UNLOCK;
}

void st3m_audio_line_in_set_allowed(bool allowed) {
    LOCK;
    state.line_in_allowed = allowed;
    UNLOCK;
}

void st3m_audio_onboard_mic_to_speaker_set_allowed(bool allowed) {
    LOCK;
    state.onboard_mic_to_speaker_allowed = allowed;
    UNLOCK;
}

float st3m_audio_input_thru_set_volume_dB(float vol_dB) {
    if (vol_dB > 0) vol_dB = 0;
    int16_t vol = 32768. * expf(vol_dB * NAT_LOG_DB);
    LOCK;
    state.input_thru_vol_int = vol;
    state.input_thru_vol = vol_dB;
    UNLOCK;
    return vol_dB;
}

void st3m_audio_set_player_function(st3m_audio_player_function_t fun) {
    LOCK;
    state.function = fun;
    UNLOCK;
}

bool st3m_audio_headphones_are_connected(void) {
    LOCK;
    bool res = _headphones_connected();
    UNLOCK;
    return res;
}

bool st3m_audio_line_in_is_connected(void) {
    LOCK;
    bool res = state.jacksense.line_in;
    UNLOCK;
    return res;
}

// Locked output getters/setters.

#define LOCKED0(body) \
    LOCK;             \
    body;             \
    UNLOCK
#define LOCKED(ty, body) \
    LOCK;                \
    ty res = body;       \
    UNLOCK;              \
    return res

void st3m_audio_headphones_set_mute(bool mute) {
    LOCKED0(_output_set_mute(&state.headphones, mute));
}

void st3m_audio_speaker_set_mute(bool mute) {
    LOCKED0(_output_set_mute(&state.speaker, mute));
}

float st3m_audio_speaker_set_volume_dB(float vol_dB) {
    LOCKED(float, _output_set_volume(&state.speaker, vol_dB));
}

float st3m_audio_headphones_set_volume_dB(float vol_dB) {
    LOCKED(float, _output_set_volume(&state.headphones, vol_dB));
}

void st3m_audio_headphones_detection_override(bool enable) {
    LOCK;
    state.headphones_detection_override = enable;
    _output_apply(&state.headphones);
    _output_apply(&state.speaker);
    UNLOCK;
}

float st3m_audio_headphones_adjust_volume_dB(float vol_dB) {
    LOCKED(float, _output_adjust_volume(&state.headphones, vol_dB));
}

float st3m_audio_speaker_adjust_volume_dB(float vol_dB) {
    LOCKED(float, _output_adjust_volume(&state.speaker, vol_dB));
}

float st3m_audio_headphones_set_maximum_volume_dB(float vol_dB) {
    LOCKED(float, _output_set_maximum_volume(&state.headphones, vol_dB));
}

float st3m_audio_headphones_set_minimum_volume_dB(float vol_dB) {
    LOCKED(float, _output_set_minimum_volume(&state.headphones, vol_dB));
}

float st3m_audio_speaker_set_maximum_volume_dB(float vol_dB) {
    LOCKED(float, _output_set_maximum_volume(&state.speaker, vol_dB));
}

float st3m_audio_speaker_set_minimum_volume_dB(float vol_dB) {
    LOCKED(float, _output_set_minimum_volume(&state.speaker, vol_dB));
}

float st3m_audio_speaker_get_volume_relative(void) {
    LOCKED(float, _output_get_volume_relative(&state.speaker));
}

float st3m_audio_headphones_get_volume_relative() {
    LOCKED(float, _output_get_volume_relative(&state.headphones));
}

// Automatic output detection wrappers. We need a recursive mutex here to make
// sure we don't race between output detection and applying the function to the
// current output.

#define DISPATCH_TY_TY(ty, ty2, name)                \
    ty st3m_audio_##name(ty2 arg) {                  \
        ty res;                                      \
        LOCK;                                        \
        if (_headphones_connected()) {               \
            res = st3m_audio_headphones_##name(arg); \
        } else {                                     \
            res = st3m_audio_speaker_##name(arg);    \
        }                                            \
        UNLOCK;                                      \
        return res;                                  \
    }

#define DISPATCH_TY_VOID(ty, name)                \
    ty st3m_audio_##name(void) {                  \
        ty res;                                   \
        LOCK;                                     \
        if (_headphones_connected()) {            \
            res = st3m_audio_headphones_##name(); \
        } else {                                  \
            res = st3m_audio_speaker_##name();    \
        }                                         \
        UNLOCK;                                   \
        return res;                               \
    }

#define DISPATCH_VOID_TY(ty, name)             \
    void st3m_audio_##name(ty arg) {           \
        LOCK;                                  \
        if (_headphones_connected()) {         \
            st3m_audio_headphones_##name(arg); \
        } else {                               \
            st3m_audio_speaker_##name(arg);    \
        }                                      \
        UNLOCK;                                \
    }

DISPATCH_TY_TY(float, float, adjust_volume_dB)
DISPATCH_TY_TY(float, float, set_volume_dB)
DISPATCH_TY_VOID(float, get_volume_dB)
DISPATCH_VOID_TY(bool, set_mute)
DISPATCH_TY_VOID(bool, get_mute)
DISPATCH_TY_VOID(float, get_volume_relative)
