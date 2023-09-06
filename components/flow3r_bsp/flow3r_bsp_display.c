#include "flow3r_bsp.h"

#include <string.h>

#include "esp_log.h"
#include "sdkconfig.h"

#include "flow3r_bsp_gc9a01.h"

static const char *TAG = "flow3r-bsp-display";

flow3r_bsp_gc9a01_config_t gc9a01_config = {
    .reset_used = 0,
    .backlight_used = 1,

    .pin_sck = 41,
    .pin_mosi = 42,
    .pin_cs = 40,
    .pin_dc = 38,
    .pin_backlight = 46,

    .host = 2,
};

static flow3r_bsp_gc9a01_t gc9a01;
static uint8_t gc9a01_initialized = 0;

void flow3r_bsp_display_init(void) {
    ESP_LOGI(TAG, "gc9a01 initializing...");
    esp_err_t ret = flow3r_bsp_gc9a01_init(&gc9a01, &gc9a01_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gc9a01 init failed: %s", esp_err_to_name(ret));
    } else {
        gc9a01_initialized = 1;
        ESP_LOGI(TAG, "gc9a01 initialized");
    }
}

void flow3r_bsp_display_send_fb_osd(void *fb_data, int bits, void *osd_data,
                                    int osd_x0, int osd_y0, int osd_x1,
                                    int osd_y1) {
    if (!gc9a01_initialized) {
        return;
    }
    static bool had_error = false;

    esp_err_t ret = flow3r_bsp_gc9a01_blit_osd(&gc9a01, fb_data, bits, osd_data,
                                               osd_x0, osd_y0, osd_x1, osd_y1);
    if (ret != ESP_OK) {
        if (!had_error) {
            ESP_LOGE(TAG, "display blit failed: %s", esp_err_to_name(ret));
            had_error = true;
        }
    } else {
        if (had_error) {
            ESP_LOGI(TAG, "display blit success!");
            had_error = false;
        }
    }
}

void flow3r_bsp_display_send_fb(void *fb_data, int bits) {
    flow3r_bsp_display_send_fb_osd(fb_data, bits, NULL, 0, 0, 0, 0);
}

void flow3r_bsp_display_set_backlight(uint8_t percent) {
    if (!gc9a01_initialized) {
        return;
    }
    flow3r_bsp_gc9a01_backlight_set(&gc9a01, percent);
}
