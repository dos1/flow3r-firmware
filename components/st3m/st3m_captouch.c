#include "st3m_captouch.h"

#include "esp_err.h"
#include "esp_log.h"

#include "flow3r_bsp_captouch.h"
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include <string.h>

static const char *TAG = "st3m-captouch";

static SemaphoreHandle_t _mu = NULL;
static st3m_captouch_state_t _state = {};
static bool _request_calibration = false;
static bool _calibrating = false;

static inline void _pad_feed(st3m_petal_pad_state_t *pad, uint16_t data,
                             bool top) {
    ringbuffer_write(&pad->rb, data);
    int32_t thres = top ? 8000 : 12000;
    thres = pad->pressed_prev ? thres - 1000 : thres;  // some hysteresis
    pad->pressed = data > thres;
    pad->pressed_prev = pad->pressed;

    if ((!pad->press_event_new) || pad->fresh) {
        pad->press_event_new = pad->pressed;
        pad->fresh = false;
    }

    if (pad->pressed) {
        pad->pressure = data - thres;
    } else {
        pad->pressure = 0;
    }
}

static inline void _petal_process(st3m_petal_state_t *petal, bool top) {
    if (top) {
        petal->pressed =
            petal->base.pressed || petal->ccw.pressed || petal->cw.pressed;
        petal->pressure =
            (petal->base.pressure + petal->ccw.pressure + petal->cw.pressure) /
            3;
        int32_t left = ringbuffer_avg(&petal->ccw.rb);
        int32_t right = ringbuffer_avg(&petal->cw.rb);
        int32_t base = ringbuffer_avg(&petal->base.rb);
        petal->pos_distance = (left + right) / 2 - base;
        petal->pos_angle = right - left;
#if defined(CONFIG_FLOW3R_HW_GEN_P3)
        petal->pos_distance = -petal->pos_distance;
#endif
    } else {
        petal->pressed = petal->base.pressed || petal->tip.pressed;
        petal->pressure = (petal->base.pressure + petal->tip.pressure) / 2;
        int32_t base = ringbuffer_avg(&petal->base.rb);
        int32_t tip = ringbuffer_avg(&petal->tip.rb);
        petal->pos_distance = tip - base;
        petal->pos_angle = 0;
    }
}

static void _on_data(const flow3r_bsp_captouch_state_t *st) {
    xSemaphoreTake(_mu, portMAX_DELAY);

    for (size_t ix = 0; ix < 10; ix++) {
        bool top = (ix % 2) == 0;

        if (top) {
#if defined(CONFIG_FLOW3R_HW_GEN_P3)
            // Hack for P3 badges, pretend tip is base.
            _pad_feed(&_state.petals[ix].base, st->petals[ix].tip.raw, true);
#else
            _pad_feed(&_state.petals[ix].base, st->petals[ix].base.raw, true);
#endif
            _pad_feed(&_state.petals[ix].cw, st->petals[ix].cw.raw, true);
            _pad_feed(&_state.petals[ix].ccw, st->petals[ix].ccw.raw, true);
            _petal_process(&_state.petals[ix], true);
        } else {
            _pad_feed(&_state.petals[ix].base, st->petals[ix].base.raw, false);
            _pad_feed(&_state.petals[ix].tip, st->petals[ix].tip.raw, false);
            _petal_process(&_state.petals[ix], false);
        }
    }

    if (_request_calibration) {
        _request_calibration = false;
        flow3r_bsp_captouch_calibrate();
    }
    _calibrating = flow3r_bsp_captouch_calibrating();
    xSemaphoreGive(_mu);
}

void st3m_captouch_init(void) {
    assert(_mu == NULL);
    _mu = xSemaphoreCreateMutex();
    assert(_mu != NULL);

    esp_err_t ret = flow3r_bsp_captouch_init(_on_data);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Captouch init failed: %s", esp_err_to_name(ret));
    }
}

bool st3m_captouch_calibrating(void) {
    xSemaphoreTake(_mu, portMAX_DELAY);
    bool res = _calibrating || _request_calibration;
    xSemaphoreGive(_mu);
    return res;
}

void st3m_captouch_request_calibration(void) {
    xSemaphoreTake(_mu, portMAX_DELAY);
    _request_calibration = true;
    xSemaphoreGive(_mu);
}

static void _refresh_petal_events(uint8_t petal_ix) {
    bool top = (petal_ix % 2) == 0;
    st3m_petal_state_t *pt = &(_state.petals[petal_ix]);
    if (top) {
        pt->cw.press_event = pt->cw.press_event_new;
        pt->ccw.press_event = pt->ccw.press_event_new;
        pt->base.press_event = pt->base.press_event_new;
        pt->press_event =
            pt->base.press_event || pt->ccw.press_event || pt->cw.press_event;
        pt->cw.fresh = true;
        pt->ccw.fresh = true;
        pt->base.fresh = true;
    } else {
        pt->tip.press_event = pt->tip.press_event_new;
        pt->base.press_event = pt->base.press_event_new;
        pt->press_event = pt->tip.press_event || pt->base.press_event;
        pt->tip.fresh = true;
        pt->base.fresh = true;
    }
}

void st3m_captouch_refresh_petal_events(uint8_t petal_ix) {
    xSemaphoreTake(_mu, portMAX_DELAY);
    _refresh_petal_events(petal_ix);
    xSemaphoreGive(_mu);
}

void st3m_captouch_refresh_all_events() {
    xSemaphoreTake(_mu, portMAX_DELAY);
    for (uint8_t i = 0; i < 10; i++) {
        _refresh_petal_events(i);
    }
    xSemaphoreGive(_mu);
}

void st3m_captouch_get_all(st3m_captouch_state_t *dest) {
    xSemaphoreTake(_mu, portMAX_DELAY);
    memcpy(dest, &_state, sizeof(_state));
    xSemaphoreGive(_mu);
}

void st3m_captouch_get_petal(st3m_petal_state_t *dest, uint8_t petal_ix) {
    if (petal_ix > 9) {
        petal_ix = 9;
    }
    xSemaphoreTake(_mu, portMAX_DELAY);
    memcpy(dest, &_state.petals[petal_ix], sizeof(_state.petals[petal_ix]));
    xSemaphoreGive(_mu);
}
