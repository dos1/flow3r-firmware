#include "badge23/captouch.h"
#include "badge23/audio.h"
#include "badge23/leds.h"
#include "badge23/spio.h"
#include "badge23/lock.h"

#include "flow3r_bsp.h"
#include "st3m_gfx.h"
#include "st3m_fs.h"

#include "esp_log.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "freertos/timers.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

static const char *TAG = "espan";

#define I2C_MASTER_NUM              0                          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          400000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */

#if defined(CONFIG_BADGE23_HW_GEN_P3) || defined(CONFIG_BADGE23_HW_GEN_P4) || defined(CONFIG_BADGE23_HW_GEN_P6)
#define CONFIG_I2C_MASTER_SDA 2
#define CONFIG_I2C_MASTER_SCL 1

#elif defined(CONFIG_BADGE23_HW_GEN_P1)
#define CONFIG_I2C_MASTER_SDA 10
#define CONFIG_I2C_MASTER_SCL 9

#else
#error "i2c not implemented for this badge generation"
#endif

static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = CONFIG_I2C_MASTER_SDA,
        .scl_io_num = CONFIG_I2C_MASTER_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(i2c_master_port, &conf);

    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

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
        audio_update_jacksense();
        leds_update_hardware();
    }
}

void locks_init(){
    mutex_i2c = xSemaphoreCreateMutex();
    mutex_LED = xSemaphoreCreateMutex();
}

void os_app_early_init(void) {
    // Initialize display first as that gives us a nice splash screen.
    st3m_gfx_init();
    // Submit splash a couple of times to make sure we've fully flushed out the
    // initial framebuffer (both on-ESP and in-screen) noise before we turn on
    // the backlight.
    for (int i = 0; i < 4; i++) {
        st3m_gfx_splash("st3m loading...");
    }
    flow3r_bsp_display_set_backlight(100);

    st3m_fs_init();
}

void os_app_main(void)
{
    locks_init();
    ESP_LOGI(TAG, "Starting on %s...", flow3r_bsp_hw_name);
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized successfully");

    audio_init();
    leds_init();
    init_buttons();
    captouch_init();
    spio_badge_link_disable(255);

    captouch_force_calibration();

    xTaskCreatePinnedToCore(&io_fast_task, "iofast", 4096, NULL, configMAX_PRIORITIES-1, NULL, 0);
    xTaskCreatePinnedToCore(&io_slow_task, "ioslow", 4096, NULL, configMAX_PRIORITIES-2, NULL, 0);

    hw_init_done = 1;
}

uint8_t hardware_is_initialized(){
    return hw_init_done;
}

