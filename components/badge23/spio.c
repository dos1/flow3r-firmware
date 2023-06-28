#include "badge23/spio.h"

static const char *TAG = "badge23-spio";

#include "esp_log.h"

#include "st3m_audio.h"
#include "flow3r_bsp_i2c.h"
#include "flow3r_bsp.h"

void update_button_state(){
    esp_err_t ret = flow3r_bsp_spio_update();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "update failed: %s", esp_err_to_name(ret));
    }
}

void init_buttons(){
    esp_err_t ret = flow3r_bsp_spio_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "init failed: %s", esp_err_to_name(ret));
        for (;;) {}
    }
}

bool spio_charger_state_get() {
    return flow3r_bsp_spio_charger_state_get();
}

bool spio_line_in_jacksense_get(){
    return flow3r_bsp_spio_jacksense_right_get();
}

static bool menu_button_left = false;

void spio_menu_button_set_left(bool left){
    menu_button_left = left;
}

int8_t spio_menu_button_get() {
    if (menu_button_left)
        return flow3r_bsp_spio_left_button_get();
    return flow3r_bsp_spio_right_button_get();
}

int8_t spio_application_button_get(){
    if (menu_button_left)
        return flow3r_bsp_spio_right_button_get();
    return flow3r_bsp_spio_left_button_get();
}

int8_t spio_left_button_get(){
    return flow3r_bsp_spio_left_button_get();
}

int8_t spio_right_button_get(){
    return flow3r_bsp_spio_right_button_get();
}

int8_t spio_menu_button_get_left(){
    return menu_button_left;
}

static uint8_t badge_link_enabled = 0;

uint8_t spio_badge_link_get_active(uint8_t pin_mask){
    return badge_link_enabled & pin_mask;
}

static int8_t spio_badge_link_set(uint8_t mask, bool state) {
    bool left_tip = (mask & BADGE_LINK_PIN_MASK_LINE_OUT_TIP) > 0;
    bool left_ring = (mask & BADGE_LINK_PIN_MASK_LINE_OUT_RING) > 0;
    bool right_tip = (mask & BADGE_LINK_PIN_MASK_LINE_IN_TIP) > 0;
    bool right_ring = (mask & BADGE_LINK_PIN_MASK_LINE_IN_RING) > 0;
    if(state) {
        if (left_tip || left_ring) {
            if(!st3m_audio_headphones_are_connected()) {
                left_tip = false;
                left_ring = false;
                ESP_LOGE(TAG, "cannot enable line out badge link without cable plugged in for safety reasons");
            }
        }
        if (left_tip) badge_link_enabled |= BADGE_LINK_PIN_MASK_LINE_OUT_TIP;
        if (left_ring) badge_link_enabled |= BADGE_LINK_PIN_MASK_LINE_OUT_RING;
        if (right_tip) badge_link_enabled |= BADGE_LINK_PIN_MASK_LINE_IN_TIP;
        if (right_ring) badge_link_enabled |= BADGE_LINK_PIN_MASK_LINE_IN_RING;
    } else {
        if (!left_tip) badge_link_enabled &= ~BADGE_LINK_PIN_MASK_LINE_OUT_TIP;
        if (!left_ring) badge_link_enabled &= ~BADGE_LINK_PIN_MASK_LINE_OUT_RING;
        if (!right_tip) badge_link_enabled &= ~BADGE_LINK_PIN_MASK_LINE_IN_TIP;
        if (!right_ring) badge_link_enabled &= ~BADGE_LINK_PIN_MASK_LINE_IN_RING;
    }

    flow3r_bsp_spio_badgelink_left_enable(left_tip, left_ring);
    flow3r_bsp_spio_badgelink_right_enable(right_tip, right_ring);
    return spio_badge_link_get_active(mask);
}

uint8_t spio_badge_link_disable(uint8_t pin_mask){
    return spio_badge_link_set(pin_mask, 0);
}

uint8_t spio_badge_link_enable(uint8_t pin_mask){
    return spio_badge_link_set(pin_mask, 1);
}
