#include "badge23/captouch.h"
#include "badge23/spio.h"

#include "flow3r_bsp.h"
#include "st3m_gfx.h"
#include "st3m_fs.h"
#include "st3m_audio.h"
#include "st3m_leds.h"

#include "bl00mbox.h"

#include "esp_log.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "freertos/timers.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

static const char *TAG = "espan";

static uint8_t hw_init_done = 0;

static void io_fast_task(void * data){
    TickType_t last_wake = xTaskGetTickCount();
    while(1) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(10)); // 100 Hz
        captouch_read_cycle();
        update_button_state();
    }
}

// read out stuff like jack detection, battery status, usb connection etc.
static void io_slow_task(void * data){
    TickType_t last_wake = xTaskGetTickCount();
    while(1) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(100)); // 10 Hz
        st3m_audio_update_jacksense();
    }
}

void badge23_main(void)
{
    ESP_LOGI(TAG, "Starting on %s...", flow3r_bsp_hw_name);

    init_buttons();
    captouch_init();
    spio_badge_link_disable(255);
    st3m_audio_set_player_function(bl00mbox_player_function);

    captouch_force_calibration();

    xTaskCreatePinnedToCore(&io_fast_task, "iofast", 4096, NULL, configMAX_PRIORITIES-1, NULL, 0);
    xTaskCreatePinnedToCore(&io_slow_task, "ioslow", 4096, NULL, configMAX_PRIORITIES-2, NULL, 0);

    hw_init_done = 1;
}

uint8_t hardware_is_initialized(){
    return hw_init_done;
}

