#include "st3m_io.h"

static const char *TAG = "st3m-io";

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "flow3r_bsp.h"
#include "flow3r_bsp_i2c.h"
#include "st3m_audio.h"

static bool _app_button_left = true;
static SemaphoreHandle_t _mu = NULL;

static void _update_button_state() {
    esp_err_t ret = flow3r_bsp_spio_update();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "update failed: %s", esp_err_to_name(ret));
    }
}

bool st3m_io_charger_state_get() { return flow3r_bsp_spio_charger_state_get(); }

void st3m_io_app_button_configure(bool left) {
    xSemaphoreTake(_mu, portMAX_DELAY);
    _app_button_left = left;
    xSemaphoreGive(_mu);
}

bool st3m_io_app_button_is_left(void) {
    xSemaphoreTake(_mu, portMAX_DELAY);
    bool res = _app_button_left;
    xSemaphoreGive(_mu);
    return res;
}

st3m_tripos st3m_io_app_button_get() {
    if (st3m_io_app_button_is_left()) {
        return flow3r_bsp_spio_left_button_get();
    } else {
        return flow3r_bsp_spio_right_button_get();
    }
}

st3m_tripos st3m_io_os_button_get() {
    if (st3m_io_app_button_is_left()) {
        return flow3r_bsp_spio_right_button_get();
    } else {
        return flow3r_bsp_spio_left_button_get();
    }
}

static uint8_t badge_link_enabled = 0;

uint8_t st3m_io_badge_link_get_active(uint8_t pin_mask) {
    return badge_link_enabled & pin_mask;
}

static int8_t st3m_io_badge_link_set(uint8_t mask, bool state) {
    bool left_tip = (mask & BADGE_LINK_PIN_MASK_LINE_OUT_TIP) > 0;
    bool left_ring = (mask & BADGE_LINK_PIN_MASK_LINE_OUT_RING) > 0;
    bool right_tip = (mask & BADGE_LINK_PIN_MASK_LINE_IN_TIP) > 0;
    bool right_ring = (mask & BADGE_LINK_PIN_MASK_LINE_IN_RING) > 0;

    // Apply request to badge_link_enabled.
    if (state) {
        if (left_tip || left_ring) {
            if (!st3m_audio_headphones_are_connected()) {
                left_tip = false;
                left_ring = false;
                ESP_LOGE(TAG,
                         "cannot enable line out badge link without cable "
                         "plugged in for safety reasons");
            }
        }
        if (left_tip) badge_link_enabled |= BADGE_LINK_PIN_MASK_LINE_OUT_TIP;
        if (left_ring) badge_link_enabled |= BADGE_LINK_PIN_MASK_LINE_OUT_RING;
        if (right_tip) badge_link_enabled |= BADGE_LINK_PIN_MASK_LINE_IN_TIP;
        if (right_ring) badge_link_enabled |= BADGE_LINK_PIN_MASK_LINE_IN_RING;
    } else {
        if (!left_tip) badge_link_enabled &= ~BADGE_LINK_PIN_MASK_LINE_OUT_TIP;
        if (!left_ring)
            badge_link_enabled &= ~BADGE_LINK_PIN_MASK_LINE_OUT_RING;
        if (!right_tip) badge_link_enabled &= ~BADGE_LINK_PIN_MASK_LINE_IN_TIP;
        if (!right_ring)
            badge_link_enabled &= ~BADGE_LINK_PIN_MASK_LINE_IN_RING;
    }

    // Convert badge_link_enabled back to {left,right}_{tip,ring}, but this
    // time as requested state.
    left_tip = (badge_link_enabled & BADGE_LINK_PIN_MASK_LINE_OUT_TIP) > 0;
    left_ring = (badge_link_enabled & BADGE_LINK_PIN_MASK_LINE_OUT_RING) > 0;
    right_tip = (badge_link_enabled & BADGE_LINK_PIN_MASK_LINE_IN_TIP) > 0;
    right_ring = (badge_link_enabled & BADGE_LINK_PIN_MASK_LINE_IN_RING) > 0;

    flow3r_bsp_spio_badgelink_left_enable(left_tip, left_ring);
    flow3r_bsp_spio_badgelink_right_enable(right_tip, right_ring);
    return st3m_io_badge_link_get_active(mask);
}

uint8_t st3m_io_badge_link_disable(uint8_t pin_mask) {
    return st3m_io_badge_link_set(pin_mask, 0);
}

uint8_t st3m_io_badge_link_enable(uint8_t pin_mask) {
    return st3m_io_badge_link_set(pin_mask, 1);
}

static void _task(void *data) {
    TickType_t last_wake = xTaskGetTickCount();
    while (1) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(10));  // 100 Hz
        _update_button_state();
    }
}

void st3m_io_init(void) {
    assert(_mu == NULL);
    _mu = xSemaphoreCreateMutex();
    assert(_mu != NULL);

    esp_err_t ret = flow3r_bsp_spio_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spio init failed: %s", esp_err_to_name(ret));
        for (;;) {
        }
    }

    st3m_io_badge_link_disable(BADGE_LINK_PIN_MASK_ALL);

    xTaskCreate(&_task, "io", 4096, NULL, configMAX_PRIORITIES - 1, NULL);
    ESP_LOGI(TAG, "IO task started");
}
