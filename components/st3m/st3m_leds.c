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

static const char *TAG = "st3m-leds";

typedef struct {
    uint8_t lut[256];
} st3m_leds_gamma_table_t;

typedef struct {
    uint8_t brightness;
    uint8_t slew_rate;
    bool auto_update;

    st3m_leds_gamma_table_t gamma_red;
    st3m_leds_gamma_table_t gamma_green;
    st3m_leds_gamma_table_t gamma_blue;

    st3m_rgb_t target[40];
    st3m_rgb_t target_buffer[40];
    st3m_rgb_t hardware_value[40];
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

uint8_t led_get_slew(int16_t old, int16_t new, int16_t slew) {
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

    for (int i = 0; i < 40; i++) {
        st3m_rgb_t c = state.target[i];
        c.r = c.r * state.brightness / 255;
        c.g = c.g * state.brightness / 255;
        c.b = c.b * state.brightness / 255;

        c.r = state.gamma_red.lut[c.r];
        c.g = state.gamma_red.lut[c.g];
        c.b = state.gamma_red.lut[c.b];

        c.r = led_get_slew(state.hardware_value[i].r, c.r, state.slew_rate);
        c.g = led_get_slew(state.hardware_value[i].g, c.g, state.slew_rate);
        c.b = led_get_slew(state.hardware_value[i].b, c.b, state.slew_rate);
        state.hardware_value[i] = c;

        set_single_led(i, c);
    }
    UNLOCK;

    esp_err_t ret = flow3r_bsp_leds_refresh(portMAX_DELAY);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LED refresh failed: %s", esp_err_to_name(ret));
    }
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

void st3m_leds_set_single_hsv(uint8_t index, float hue, float sat, float val) {
    st3m_hsv_t hsv = {
        .h = hue,
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
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(100));  // 10 Hz
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
