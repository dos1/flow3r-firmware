#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c.h"

#include "badge23_portexpander.h"

#include "doomgeneric/doomgeneric.h"


static const char *TAG = "main";

#define I2C_MASTER_NUM              0                          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          400000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define CONFIG_I2C_MASTER_SDA 2
#define CONFIG_I2C_MASTER_SCL 1

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

void app_main(void) {
    i2c_master_init();
    portexpander_init();
    char *args[] = {
        "doomgeneric",
        "-iwad", "doom1.wad",
        NULL,
    };
    doomgeneric_Create(3, args);
    for (;;) {
        doomgeneric_Tick();
    }
}
