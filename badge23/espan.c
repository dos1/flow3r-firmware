#include "captouch.h"
#include "audio.h"
#include "leds.h"
#include "../../py/mphal.h"
#include "display.h"

#include "esp_log.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

static const char *TAG = "espan";

#define I2C_MASTER_NUM              0                          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          400000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */

#define CONFIG_I2C_MASTER_SDA 10
#define CONFIG_I2C_MASTER_SCL 9

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

#define CAPTOUCH_POLLING_PERIOD 10

void os_app_main(void)
{
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized successfully");

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    set_global_vol_dB(0);

    audio_init();
    leds_init();
    //display_init();
    captouch_init();

    mp_hal_stdout_tx_str("task inits done\n\r");
    while(1) {
        manual_captouch_readout(1);
        vTaskDelay((CAPTOUCH_POLLING_PERIOD) / portTICK_PERIOD_MS);
        manual_captouch_readout(0);
        vTaskDelay((CAPTOUCH_POLLING_PERIOD) / portTICK_PERIOD_MS);
    }

    ESP_ERROR_CHECK(i2c_driver_delete(I2C_MASTER_NUM));
    ESP_LOGI(TAG, "I2C de-initialized successfully");
}