#include "st3m_captouch.h"

#include "esp_err.h"
#include "esp_log.h"

#include "flow3r_bsp_captouch.h"
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include <string.h>

// note: at some point there should be a calibration config load/save api
// that also allows users to set their own threshold. before doing that it
// would be more useful to start with the existing dynamic calibration data
// tho :D

#define TOP_PETAL_THRESHOLD 8000
#define BOTTOM_PETAL_THRESHOLD 12000
#define PETAL_HYSTERESIS 1000

static const char *TAG = "st3m-captouch";

static SemaphoreHandle_t _mu = NULL;
static st3m_captouch_state_t _state = {};
static bool _request_calibration = false;
static bool _calibrating = false;

static inline void _pad_feed(st3m_petal_pad_state_t *pad, uint16_t data,
                             uint8_t index) {
    bool top = (index % 2) == 0;
    int32_t thres = top ? (TOP_PETAL_THRESHOLD) : (BOTTOM_PETAL_THRESHOLD);
    thres = pad->pressed_prev ? thres - (PETAL_HYSTERESIS)
                              : thres;  // some hysteresis
    pad->raw = data;
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

// roughly matches the behavior of the legacy api. someday we should have more
// meaningful output units.
#define POS_AMPLITUDE 40000
#define POS_AMPLITUDE_SHIFT 2
#define POS_DIV_MIN 1000
static inline void _petal_process(st3m_petal_state_t *petal, uint8_t index) {
    bool top = (index % 2) == 0;
    int32_t thres = top ? (TOP_PETAL_THRESHOLD) : (BOTTOM_PETAL_THRESHOLD);
    thres =
        petal->pressed ? thres - (PETAL_HYSTERESIS) : thres;  // some hysteresis
    int32_t distance;
    int32_t angle;
    if (top) {
        petal->pressure =
            (petal->base.pressure + petal->ccw.pressure + petal->cw.pressure) /
            3;
        int32_t raw = petal->base.raw + petal->ccw.raw + petal->cw.raw;
        petal->pressed = raw > thres;
        int32_t left = petal->ccw.raw;
        int32_t right = petal->cw.raw;
        int32_t base = petal->base.raw;
        int32_t tip = (left + right) >> 1;
        distance = tip - base;
        distance *= (POS_AMPLITUDE) >> (POS_AMPLITUDE_SHIFT);
        distance /= ((tip + base) >> (POS_AMPLITUDE_SHIFT)) + (POS_DIV_MIN);
        angle = right - left;
        angle *= (POS_AMPLITUDE) >> (POS_AMPLITUDE_SHIFT);
        angle /= ((right + left) >> (POS_AMPLITUDE_SHIFT)) + (POS_DIV_MIN);
#if defined(CONFIG_FLOW3R_HW_GEN_P3)
        distance = -pos_distance;
#endif
    } else {
        petal->pressure = (petal->base.pressure + petal->tip.pressure) / 2;
        int32_t raw = petal->base.raw + petal->tip.raw;
        if (index == 5) raw *= 2;
        petal->pressed = raw > thres;
        int32_t base = petal->base.raw;
        int32_t tip = petal->tip.raw;
        if (index == 5) {
            distance =
                petal->pressed ? (tip * 5) / 4 - 40000 : 0;  // bad hack pt2
        } else {
            distance = tip - base;
            distance *= (POS_AMPLITUDE) >> (POS_AMPLITUDE_SHIFT);
            distance /= ((tip + base) >> (POS_AMPLITUDE_SHIFT)) + (POS_DIV_MIN);
        }
        angle = 0;
    }
    if ((!petal->press_event_new) || petal->fresh) {
        petal->press_event_new = petal->pressed;
        petal->fresh = false;
    }
    // moved filter behind the nonlinearity to get more consistent response
    // times, also replaced comb with pole for slightly lower lag with similar
    // noise. maybe switch to higher order in the future for even lower lag but
    // not sure if that's a good idea. graphical display looks good for now, so
    // we're leaving fine tuning for when the mapping is a bit more polished.
    int8_t f_div_pow = 4;
    // widescreen ratio for better graphics
    int8_t f_mult_old = 9;
    int8_t f_mult_new = (1 << f_div_pow) - f_mult_old;
    petal->pos_distance =
        (f_mult_old * petal->pos_distance + f_mult_new * distance) >> f_div_pow;
    petal->pos_angle =
        (f_mult_old * petal->pos_angle + f_mult_new * angle) >> f_div_pow;
}

static void _on_data(const flow3r_bsp_captouch_state_t *st) {
    xSemaphoreTake(_mu, portMAX_DELAY);

    for (size_t ix = 0; ix < 10; ix++) {
        bool top = (ix % 2) == 0;

        if (top) {
#if defined(CONFIG_FLOW3R_HW_GEN_P3)
            // Hack for P3 badges, pretend tip is base.
            _pad_feed(&_state.petals[ix].base, st->petals[ix].tip.raw, ix);
#else
            _pad_feed(&_state.petals[ix].base, st->petals[ix].base.raw, ix);
#endif
            _pad_feed(&_state.petals[ix].cw, st->petals[ix].cw.raw, ix);
            _pad_feed(&_state.petals[ix].ccw, st->petals[ix].ccw.raw, ix);
            _petal_process(&_state.petals[ix], ix);
        } else {
            _pad_feed(&_state.petals[ix].base, st->petals[ix].base.raw, ix);
            _pad_feed(&_state.petals[ix].tip, st->petals[ix].tip.raw, ix);
            _petal_process(&_state.petals[ix], ix);
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
        pt->press_event = pt->press_event_new;
        pt->cw.fresh = true;
        pt->ccw.fresh = true;
        pt->base.fresh = true;
        pt->fresh = true;
    } else {
        pt->tip.press_event = pt->tip.press_event_new;
        pt->base.press_event = pt->base.press_event_new;
        pt->press_event = pt->press_event_new;
        pt->tip.fresh = true;
        pt->base.fresh = true;
        pt->fresh = true;
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
