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

// Output, either speakers or headphones. Holds volume/mute state and limits,
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

    // Denormalized setting data that can be read back by user.
    st3m_audio_input_source_t source;
    int8_t headset_gain;

    uint8_t speaker_eq_on;

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

    .source = st3m_audio_input_source_none,
    .headset_gain = 0,
    .input_thru_vol = 0,
    .input_thru_vol_int = 0,
    .input_thru_mute = true,
    .function = st3m_audio_player_function_dummy,
};

// Returns whether we should be outputting audio through headphones. If not,
// audio should be output via speaker.
//
// Lock must be taken.
static bool _headphones_connected(void) {
    return state.jacksense.headphones || state.headphones_detection_override;
}

static void _update_jacksense() {
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

    st3m_audio_input_thru_set_volume_dB(-20);
    state.speaker_eq_on = true;
    _update_jacksense();
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
    memset(buffer_tx, 0, sizeof(buffer_tx));
    memset(buffer_rx, 0, sizeof(buffer_rx));
    size_t count;

    bool hwmute = flow3r_bsp_audio_has_hardware_mute();

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
        bool headphones = _headphones_connected();
        st3m_audio_player_function_t function = state.function;
        int32_t software_volume = headphones ? state.headphones.volume_software
                                             : state.speaker.volume_software;
        bool software_mute =
            headphones ? state.headphones.mute : state.speaker.mute;
        bool input_thru_mute = state.input_thru_mute;
        int32_t input_thru_vol_int = state.input_thru_vol_int;
        UNLOCK;

        (*function)(buffer_rx, buffer_tx, FLOW3R_BSP_AUDIO_DMA_BUFFER_SIZE * 2);
        for (uint16_t i = 0; i < FLOW3R_BSP_AUDIO_DMA_BUFFER_SIZE; i++) {
            st3m_scope_write(buffer_tx[2 * i] >> 2);
        }

        if (!hwmute && software_mute) {
            // Software muting needed. Only used on P1.
            for (int i = 0; i < (FLOW3R_BSP_AUDIO_DMA_BUFFER_SIZE * 2);
                 i += 2) {
                buffer_tx[i] = 0;
            }
        } else {
            for (int i = 0; i < (FLOW3R_BSP_AUDIO_DMA_BUFFER_SIZE * 2);
                 i += 1) {
                int32_t acc = buffer_tx[i];

                acc = (acc * software_volume) >> 15;

                if (!input_thru_mute) {
                    acc += (((int32_t)buffer_rx[i]) * input_thru_vol_int) >> 15;
                }
                buffer_tx[i] = acc;
            }
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
        _update_jacksense();
    }
}

// BSP wrappers that don't need locking.

void st3m_audio_headphones_line_in_set_hardware_thru(bool enable) {
    flow3r_bsp_audio_headphones_line_in_set_hardware_thru(enable);
}

void st3m_audio_speaker_line_in_set_hardware_thru(bool enable) {
    flow3r_bsp_audio_speaker_line_in_set_hardware_thru(enable);
}

void st3m_audio_line_in_set_hardware_thru(bool enable) {
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
GETTER(st3m_audio_input_source_t, audio_input_get_source, state.source)
GETTER(uint8_t, audio_headset_get_gain_dB, state.headset_gain)
GETTER(float, audio_input_thru_get_volume_dB, state.input_thru_vol)
GETTER(bool, audio_input_thru_get_mute, state.input_thru_mute)
GETTER(bool, audio_speaker_get_eq_on, state.speaker_eq_on)
#undef GETTER

// Locked global API functions.

void st3m_audio_headset_set_gain_dB(int8_t gain_dB) {
    gain_dB = flow3r_bsp_audio_headset_set_gain_dB(gain_dB);
    LOCK;
    state.headset_gain = gain_dB;
    UNLOCK;
}

void st3m_audio_input_set_source(st3m_audio_input_source_t source) {
    switch (source) {
        case st3m_audio_input_source_none:
            flow3r_bsp_audio_input_set_source(
                flow3r_bsp_audio_input_source_none);
            break;
        case st3m_audio_input_source_line_in:
            flow3r_bsp_audio_input_set_source(
                flow3r_bsp_audio_input_source_line_in);
            break;
        case st3m_audio_input_source_headset_mic:
            flow3r_bsp_audio_input_set_source(
                flow3r_bsp_audio_input_source_headset_mic);
            break;
        case st3m_audio_input_source_onboard_mic:
            flow3r_bsp_audio_input_set_source(
                flow3r_bsp_audio_input_source_onboard_mic);
            break;
    }
    LOCK;
    state.source = source;
    UNLOCK;
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

float st3m_audio_input_thru_set_volume_dB(float vol_dB) {
    if (vol_dB > 0) vol_dB = 0;
    LOCK;
    state.input_thru_vol_int = (int32_t)(32768. * expf(vol_dB * NAT_LOG_DB));
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
