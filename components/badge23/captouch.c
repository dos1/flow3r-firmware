#include "badge23/captouch.h"

#include "esp_err.h"
#include "esp_log.h"

#include "flow3r_bsp_captouch.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include <string.h>

static const char *TAG = "st3m-captouch";

// A simple, non-concurrent ringbuffer. I feel like I've already implemented
// this once in this codebase.
//
// TODO(q3k): unify/expose as common st3m API.
typedef struct {
    size_t write_ix;
    bool wrapped;
    uint16_t buf[4];
} ringbuffer_t;

// Size of ringbuffer, in elements.
static inline size_t ringbuffer_size(const ringbuffer_t *rb) {
    return sizeof(rb->buf) / sizeof(uint16_t);
}

// Write to ringbuffer.
static void ringbuffer_write(ringbuffer_t *rb, uint16_t data) {
    rb->buf[rb->write_ix] = data;
    rb->write_ix++;
    if (rb->write_ix >= ringbuffer_size(rb)) {
        rb->write_ix = 0;
        rb->wrapped = true;
    }
}

// Get ringbuffer average (mean), or 0 if no values have yet been inserted.
static uint16_t ringbuffer_avg(const ringbuffer_t *rb) {
    int32_t res = 0;
    if (rb->wrapped) {
        for (size_t i = 0; i < ringbuffer_size(rb); i++) {
            res += rb->buf[i];
        }
        res /= ringbuffer_size(rb);
        return res;
    }
    if (rb->write_ix == 0) {
        return 0;
    }
    for (size_t i = 0; i < rb->write_ix; i++) {
        res += rb->buf[i];
    }
    res /= rb->write_ix;
    return res;
}

// Get last inserted value, or 0 if no value have yet been inserted.
static uint16_t ringbuffer_last(const ringbuffer_t *rb) {
    if (rb->write_ix == 0) {
        if (rb->wrapped) {
            return rb->buf[ringbuffer_size(rb) - 1];
        }
        return 0;
    }
    return rb->buf[rb->write_ix - 1];
}

// TODO(q3k): expose these as user structs?

typedef struct {
    ringbuffer_t rb;
    bool pressed;
} st3m_captouch_petal_pad_t;

typedef struct {
    st3m_captouch_petal_pad_t base;
    st3m_captouch_petal_pad_t cw;
    st3m_captouch_petal_pad_t ccw;
    bool pressed;
} st3m_captouch_petal_top_t;

typedef struct {
    st3m_captouch_petal_pad_t base;
    st3m_captouch_petal_pad_t tip;
    bool pressed;
} st3m_captouch_petal_bottom_t;

typedef struct {
    flow3r_bsp_captouch_state_t raw;

    st3m_captouch_petal_top_t top[5];
    st3m_captouch_petal_bottom_t bottom[5];
} st3m_captouch_state_t;

static SemaphoreHandle_t _mu = NULL;
static st3m_captouch_state_t _state = {};
static bool _request_calibration = false;
static bool _calibrating = false;

static inline void _pad_feed(st3m_captouch_petal_pad_t *pad, uint16_t data,
                             bool top) {
    ringbuffer_write(&pad->rb, data);
    int32_t thres = top ? 8000 : 12000;
    pad->pressed = data > thres;
}

static void _on_data(const flow3r_bsp_captouch_state_t *st) {
    xSemaphoreTake(_mu, portMAX_DELAY);
    memcpy(&_state, st, sizeof(flow3r_bsp_captouch_state_t));
    for (size_t i = 0; i < 5; i++) {
        _pad_feed(&_state.top[i].base, _state.raw.petals[i * 2].base.raw, true);
        _pad_feed(&_state.top[i].cw, _state.raw.petals[i * 2].cw.raw, true);
        _pad_feed(&_state.top[i].ccw, _state.raw.petals[i * 2].ccw.raw, true);
        _state.top[i].pressed = _state.top[i].base.pressed ||
                                _state.top[i].cw.pressed ||
                                _state.top[i].ccw.pressed;
    }
    for (size_t i = 0; i < 5; i++) {
        _pad_feed(&_state.bottom[i].base, _state.raw.petals[i * 2 + 1].base.raw,
                  false);
        _pad_feed(&_state.bottom[i].tip, _state.raw.petals[i * 2 + 1].tip.raw,
                  false);
        _state.bottom[i].pressed =
            _state.bottom[i].base.pressed || _state.bottom[i].tip.pressed;
    }
    if (_request_calibration) {
        _request_calibration = false;
        flow3r_bsp_captouch_calibrate();
    }
    _calibrating = flow3r_bsp_captouch_calibrating();
    xSemaphoreGive(_mu);
}

void captouch_init(void) {
    assert(_mu == NULL);
    _mu = xSemaphoreCreateMutex();
    assert(_mu != NULL);

    esp_err_t ret = flow3r_bsp_captouch_init(_on_data);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Captouch init failed: %s", esp_err_to_name(ret));
    }
}

void captouch_print_debug_info(void) {
    // Deprecated no-op, will be removed.
}

void captouch_read_cycle(void) {
    // Deprecated no-op, will be removed.
    // (now handled by interrupt)
}

void captouch_set_calibration_afe_target(uint16_t target) {
    // Deprecated no-op, will be removed.
}

void captouch_force_calibration() {
    xSemaphoreTake(_mu, portMAX_DELAY);
    _request_calibration = true;
    xSemaphoreGive(_mu);
}

uint8_t captouch_calibration_active() {
    xSemaphoreTake(_mu, portMAX_DELAY);
    bool res = _calibrating || _request_calibration;
    xSemaphoreGive(_mu);
    return res;
}

void read_captouch_ex(captouch_state_t *state) {
    memset(state, 0, sizeof(captouch_state_t));
    xSemaphoreTake(_mu, portMAX_DELAY);
    for (size_t i = 0; i < 5; i++) {
        bool base = _state.top[i].base.pressed;
        bool cw = _state.top[i].cw.pressed;
        bool ccw = _state.top[i].ccw.pressed;
        state->petals[i * 2].pads.base_pressed = base;
        state->petals[i * 2].pads.cw_pressed = cw;
        state->petals[i * 2].pads.ccw_pressed = ccw;
        state->petals[i * 2].pressed = base || cw || ccw;
    }
    for (size_t i = 0; i < 5; i++) {
        bool base = _state.bottom[i].base.pressed;
        bool tip = _state.bottom[i].tip.pressed;
        state->petals[i * 2 + 1].pads.base_pressed = base;
        state->petals[i * 2 + 1].pads.tip_pressed = tip;
        state->petals[i * 2 + 1].pressed = base || tip;
    }
    xSemaphoreGive(_mu);
}

uint16_t read_captouch(void) {
    xSemaphoreTake(_mu, portMAX_DELAY);
    uint16_t res = 0;
    for (size_t i = 0; i < 5; i++) {
        if (_state.top[i].pressed) res |= (1 << (i * 2));
    }
    for (size_t i = 0; i < 5; i++) {
        if (_state.bottom[i].pressed) res |= (1 << (i * 2 + 1));
    }
    xSemaphoreGive(_mu);
    return res;
}

void captouch_set_petal_pad_threshold(uint8_t petal, uint8_t pad,
                                      uint16_t thres) {
    // Deprecated no-op, will be removed.
}

uint16_t captouch_get_petal_pad_raw(uint8_t petal, uint8_t pad) {
    // Deprecated no-op, will be removed.
    return 0;
}

uint16_t captouch_get_petal_pad_calib_ref(uint8_t petal, uint8_t pad) {
    // Deprecated no-op, will be removed.
    return 0;
}

uint16_t captouch_get_petal_pad(uint8_t petal, uint8_t pad) {
    // Deprecated no-op, will be removed.
    return 0;
}

int32_t captouch_get_petal_phi(uint8_t petal) {
    bool top = (petal % 2) == 0;
    if (top) {
        size_t ix = petal / 2;
        xSemaphoreTake(_mu, portMAX_DELAY);
        int32_t left = ringbuffer_avg(&_state.top[ix].ccw.rb);
        int32_t right = ringbuffer_avg(&_state.top[ix].cw.rb);
        xSemaphoreGive(_mu);
        return left - right;
    } else {
        return 0;
    }
}

int32_t captouch_get_petal_rad(uint8_t petal) {
    bool top = (petal % 2) == 0;
    if (top) {
        size_t ix = petal / 2;
        xSemaphoreTake(_mu, portMAX_DELAY);
        int32_t left = ringbuffer_avg(&_state.top[ix].ccw.rb);
        int32_t right = ringbuffer_avg(&_state.top[ix].cw.rb);
        int32_t base = ringbuffer_avg(&_state.top[ix].base.rb);
        xSemaphoreGive(_mu);
        return (left + right) / 2 - base;
    } else {
        size_t ix = (petal - 1) / 2;
        xSemaphoreTake(_mu, portMAX_DELAY);
        int32_t tip = ringbuffer_avg(&_state.bottom[ix].tip.rb);
        int32_t base = ringbuffer_avg(&_state.bottom[ix].base.rb);
        xSemaphoreGive(_mu);
        return tip - base;
    }
}
