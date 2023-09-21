#include "st3m_leds.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "flow3r_bsp.h"
#include "st3m_colors.h"

#define TAU360 0.017453292519943295

static const char *TAG = "st3m-leds";

typedef struct {
    uint8_t lut[256];
} st3m_leds_gamma_table_t;

typedef struct {
    uint16_t r;
    uint16_t g;
    uint16_t b;
} st3m_leds_rgb_t;

typedef struct {
    uint8_t brightness;
    uint8_t slew_rate;
    bool auto_update;
    uint8_t timer;

    st3m_leds_gamma_table_t gamma_red;
    st3m_leds_gamma_table_t gamma_green;
    st3m_leds_gamma_table_t gamma_blue;

    st3m_rgb_t target[40];
    st3m_rgb_t target_buffer[40];
    st3m_leds_rgb_t slew_output[40];
    st3m_rgb_t ret_prev[40];
} st3m_leds_state_t;

static st3m_leds_state_t state;

SemaphoreHandle_t mutex;
#define LOCK xSemaphoreTake(mutex, portMAX_DELAY)
#define UNLOCK xSemaphoreGive(mutex)

SemaphoreHandle_t mutex_incoming;
#define LOCK_INCOMING xSemaphoreTake(mutex_incoming, portMAX_DELAY)
#define UNLOCK_INCOMING xSemaphoreGive(mutex_incoming)

static void set_single_led(uint8_t index, st3m_rgb_t c) {
    index = (index + 29) % 40;
    flow3r_bsp_leds_set_pixel(index, c.r, c.g, c.b);
}

static uint16_t led_get_slew(uint16_t old, uint16_t new, uint16_t slew) {
    new = new << 8;
    if (slew == 255) return new;
    int16_t bonus = ((int16_t)slew) - 225;
    slew = 30 + (slew << 2) + ((slew * slew) >> 3);
    if (bonus > 0) {
        slew += 62 * bonus * bonus;
    }

    if (new > old + slew) {
        return old + slew;
    } else if (new > old) {
        return new;
    }
    if (new < old - slew) {
        return old - slew;
    } else if (new < old) {
        return new;
    }
    return old;
}

static void leds_update_target() {
    LOCK_INCOMING;

    for (int i = 0; i < 40; i++) {
        state.target[i].r = state.target_buffer[i].r;
        state.target[i].g = state.target_buffer[i].g;
        state.target[i].b = state.target_buffer[i].b;
    }

    UNLOCK_INCOMING;
}

void st3m_leds_update_hardware() {
    LOCK;

    if (state.auto_update) leds_update_target();

    bool is_different = false;

    for (int i = 0; i < 40; i++) {
        st3m_rgb_t ret = state.target[i];
        st3m_leds_rgb_t c;
        c.r = led_get_slew(state.slew_output[i].r, ret.r, state.slew_rate);
        c.g = led_get_slew(state.slew_output[i].g, ret.g, state.slew_rate);
        c.b = led_get_slew(state.slew_output[i].b, ret.b, state.slew_rate);
        state.slew_output[i] = c;

        c.r = ((uint32_t)c.r * state.brightness) >> 8;
        c.g = ((uint32_t)c.g * state.brightness) >> 8;
        c.b = ((uint32_t)c.b * state.brightness) >> 8;

        ret.r = state.gamma_red.lut[c.r >> 8];
        ret.g = state.gamma_red.lut[c.g >> 8];
        ret.b = state.gamma_red.lut[c.b >> 8];

        if ((ret.r != state.ret_prev[i].r) || (ret.g != state.ret_prev[i].g) ||
            (ret.b != state.ret_prev[i].b)) {
            set_single_led(i, ret);
            state.ret_prev[i] = ret;
            is_different = true;
        }
    }
    UNLOCK;

    if (is_different || (state.timer > 10)) {
        esp_err_t ret = flow3r_bsp_leds_refresh(portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "LED refresh failed: %s", esp_err_to_name(ret));
        }
        state.timer = 0;
    } else {
        state.timer += 1;
    }
}

void st3m_leds_set_single_rgba(uint8_t index, float red, float green,
                               float blue, float alpha) {
    float r, g, b;
    st3m_leds_get_single_rgb(index, &r, &g, &b);

    float nalpha = 1 - alpha;
    r = alpha * red + nalpha * r;
    g = alpha * green + nalpha * g;
    b = alpha * blue + nalpha * blue;
    st3m_leds_set_single_rgb(index, r, g, b);
}

void st3m_leds_set_single_rgb(uint8_t index, float red, float green,
                              float blue) {
    if (red > 1.0) red /= 255.0;
    if (green > 1.0) green /= 255.0;
    if (blue > 1.0) blue /= 255.0;

    LOCK_INCOMING;
    state.target_buffer[index].r = (uint8_t)(red * 255);
    state.target_buffer[index].g = (uint8_t)(green * 255);
    state.target_buffer[index].b = (uint8_t)(blue * 255);
    UNLOCK_INCOMING;
}

void st3m_leds_get_single_rgb(uint8_t index, float *red, float *green,
                              float *blue) {
    LOCK_INCOMING;
    *red = ((float)state.target_buffer[index].r) / 255;
    *green = ((float)state.target_buffer[index].g) / 255;
    *blue = ((float)state.target_buffer[index].b) / 255;
    UNLOCK_INCOMING;
}

void st3m_leds_set_single_hsv(uint8_t index, float hue, float sat, float val) {
    st3m_hsv_t hsv = {
        .h = hue * TAU360,
        .s = sat,
        .v = val,
    };
    LOCK_INCOMING;
    state.target_buffer[index] = st3m_hsv_to_rgb(hsv);
    UNLOCK_INCOMING;
}

void st3m_leds_set_all_rgb(float red, float green, float blue) {
    for (int i = 0; i < 40; i++) {
        st3m_leds_set_single_rgb(i, red, green, blue);
    }
}

void st3m_leds_set_all_rgba(float red, float green, float blue, float alpha) {
    for (int i = 0; i < 40; i++) {
        st3m_leds_set_single_rgba(i, red, green, blue, alpha);
    }
}

void st3m_leds_set_all_hsv(float h, float s, float v) {
    for (int i = 0; i < 40; i++) {
        st3m_leds_set_single_hsv(i, h, s, v);
    }
}

void st3m_leds_update() { leds_update_target(); }

static void _leds_task(void *_data) {
    (void)_data;

    TickType_t last_wake = xTaskGetTickCount();
    while (true) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(20));  // 50 Hz
        st3m_leds_update_hardware();
    }
}

void st3m_leds_init() {
    mutex = xSemaphoreCreateMutex();
    assert(mutex != NULL);
    mutex_incoming = xSemaphoreCreateMutex();
    assert(mutex_incoming != NULL);

    memset(&state, 0, sizeof(state));
    state.brightness = 69;
    state.slew_rate = 255;
    state.auto_update = false;
    state.timer = 255;

    for (uint16_t i = 0; i < 256; i++) {
        state.gamma_red.lut[i] = i;
        state.gamma_green.lut[i] = i;
        state.gamma_blue.lut[i] = i;
    }

    esp_err_t ret = flow3r_bsp_leds_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LED initialization failed: %s", esp_err_to_name(ret));
        abort();
    }

    xTaskCreate(_leds_task, "leds", 4096, NULL, ESP_TASK_PRIO_MIN + 4, NULL);

    ESP_LOGI(TAG, "LED task started");
}

void st3m_leds_set_brightness(uint8_t b) {
    LOCK;
    state.brightness = b;
    UNLOCK;
}

uint8_t st3m_leds_get_brightness() {
    LOCK;
    uint8_t res = state.brightness;
    UNLOCK;
    return res;
}

void st3m_leds_set_slew_rate(uint8_t s) {
    LOCK;
    state.slew_rate = s;
    UNLOCK;
}

uint8_t st3m_leds_get_slew_rate() {
    LOCK;
    uint8_t res = state.slew_rate;
    UNLOCK;
    return res;
}

void st3m_leds_set_auto_update(bool on) {
    LOCK;
    state.auto_update = on;
    UNLOCK;
}

bool st3m_leds_get_auto_update() {
    LOCK;
    bool res = state.auto_update;
    UNLOCK;
    return res;
}

void st3m_leds_set_gamma(float red, float green, float blue) {
    LOCK;
    for (uint16_t i = 0; i < 256; i++) {
        if (i == 0) {
            state.gamma_red.lut[i] = 0;
            state.gamma_green.lut[i] = 0;
            state.gamma_blue.lut[i] = 0;
        }
        float step = ((float)i) / 255.;
        state.gamma_red.lut[i] = (uint8_t)(254. * (powf(step, red)) + 1);
        state.gamma_green.lut[i] = (uint8_t)(254. * (powf(step, green)) + 1);
        state.gamma_blue.lut[i] = (uint8_t)(254. * (powf(step, blue)) + 1);
    }
    UNLOCK;
}
