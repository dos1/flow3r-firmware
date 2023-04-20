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

// channel -> paddle
uint8_t top_map[] = {2, 2, 2, 0, 0, 8, 8, 8, 6, 6, 4, 4};
uint8_t bot_map[] = {1, 1, 3, 3, 5, 5, 7, 7, 9, 9};

static bool active_paddles[10];
void espan_handle_captouch(uint16_t pressed_top, uint16_t pressed_bot)
{

    bool paddles[10] = {0,};
    for(int i=0; i<12; i++) {
        if(pressed_top  & (1 << i)) {
            paddles[top_map[i]] = true;
        }
    }

    for(int i=0; i<10; i++) {
        if(pressed_bot  & (1 << i)) {
            paddles[bot_map[i]] = true;
        }
    }

    bool changed = false;
    for(int i=0; i<10; i++) {
        if(active_paddles[i] == false && paddles[i] == true) {
            if(!(i == 2 || i == 8)) synth_start(i);
            leds_animate(i);
            active_paddles[i] = true;
            changed = true;
        } else if(active_paddles[i] == true && paddles[i] == false) {
            active_paddles[i] = false;
            changed = true;
        if(!(i == 2 || i == 8)) synth_stop(i);
        }
    }

    if(changed) {
        //display_show(active_paddles);
    }
}

#define VIB_DEPTH 0.01
#define VIB 0
#define VIOLIN 1
#define VIOLIN_DECAY 10
#define VIOLIN_VOL_BOOST 0.004
#define VIOLIN_SENS_POW 2

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
    //play_bootsound();
    //not sure how slow captouch_get_cross is so duplicating edge detection here;
    bool prev_petals[10] = {0};
    //pitch bend as movement relative to inital touch pos to make intonation easier
    int petal_refs[10] = {0};
    int petal_tot_prev[10] = {0};
    int petal_min[10] = {0};
    int petal_max[10] = {0};
    int petal_dir_prev[10] = {0};
    int i = 0;
    void * asdasd = &i;
    while(1) {
        gpio_event_handler(asdasd);

        i = (i + 1) % 10;
        if(!(i == 2 || i == 8)) continue;
        if(VIB){
            if(active_paddles[i]){
                int x = 0;
                int y = 0;
                captouch_get_cross(i, &x, &y);
                int tot = (x >> 1) + (y >> 1);
                if(!prev_petals[i]){
                    prev_petals[i] = 1;
                    petal_refs[i] = tot;
                    synth_set_bend(i, 0);
                } else {
                    float bend = tot - petal_refs[i];
                    bend *= VIB_DEPTH;
                    synth_set_bend(i, bend);
                }
            } else {
                prev_petals[i] = 0;
            }
        }
        if(VIOLIN){
            if(active_paddles[i]){
                int x = 0;
                int y = 0;
                captouch_get_cross(i, &x, &y);
                int tot = (x >> 1) + (y >> 1);

                uint8_t dir = tot > petal_tot_prev[i];

                if(dir != petal_dir_prev[i]){
                    if(dir){
                        petal_min[i] = petal_tot_prev[i];
                    } else {
                        petal_max[i] = petal_tot_prev[i];
                    }
                    petal_refs[i] = tot;
                    float vol = (VIOLIN_VOL_BOOST) * (petal_max[i] - petal_min[i]);
                    for(int i = 1; i < VIOLIN_SENS_POW; i++){
                        vol *= vol;
                    }
                    vol += synth_get_env(i);
                    if(vol > 1.) vol = 1;
                    if(vol < 0.) vol = 0;
                    synth_set_vol(i, vol);
                    synth_start(i);
                }
                petal_tot_prev[i] = tot;
                petal_dir_prev[i] = dir;
            }
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
        //captouch_print_debug_info();
    }

    ESP_ERROR_CHECK(i2c_driver_delete(I2C_MASTER_NUM));
    ESP_LOGI(TAG, "I2C de-initialized successfully");
}
