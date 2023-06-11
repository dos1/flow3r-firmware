#include "badge23/captouch.h"
#include "badge23/audio.h"
#include "badge23/leds.h"
#include "badge23/display.h"
#include "badge23/spio.h"
#include "badge23_hwconfig.h"
#include "badge23/lock.h"

#include "esp_log.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include <freertos/timers.h>

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

static QueueHandle_t i2c_queue = NULL;
static QueueHandle_t slow_system_status_queue = NULL;
static uint8_t dummy_data;

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
#define SLOW_SYSTEM_STATUS_PERIOD 100
static uint8_t hw_init_done = 0;

void i2c_timer(TimerHandle_t data){
    xQueueSend(i2c_queue, &dummy_data, 0);
}

void i2c_task(void * data){
    while(1){
        xQueueReceive(i2c_queue, &dummy_data, portMAX_DELAY);
        captouch_read_cycle();
        update_button_state();
    }
}

void slow_system_status_timer(TimerHandle_t data){
    xQueueSend(slow_system_status_queue, &dummy_data, 0);
}

void slow_system_status_task(void * data){
    while(1){
        xQueueReceive(slow_system_status_queue, &dummy_data, portMAX_DELAY);
        //read out stuff like jack detection, battery status, usb connection etc.
        audio_update_jacksense();
        leds_update_hardware();
    }
}

void locks_init(){
    mutex_i2c = xSemaphoreCreateMutex();
    mutex_LED = xSemaphoreCreateMutex();
}

void os_app_main(void)
{
    // Initialize display first as that gives us a nice splash screen.
    display_init();

    locks_init();
    ESP_LOGI(TAG, "Starting on %s...", badge23_hw_name);
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized successfully");

    audio_init();
    leds_init();
    init_buttons();
    captouch_init();
    spio_badge_link_disable(255);

    captouch_force_calibration();


    i2c_queue = xQueueCreate(1,1);

    TaskHandle_t i2c_task_handle;
    xTaskCreatePinnedToCore(&i2c_task, "I2C task", 4096, NULL, configMAX_PRIORITIES-1, &i2c_task_handle, 0);

    TimerHandle_t i2c_timer_handle = xTimerCreate("I2C timer", pdMS_TO_TICKS(CAPTOUCH_POLLING_PERIOD), pdTRUE, (void *) 0, *i2c_timer);
    if( xTimerStart(i2c_timer_handle, 0 ) != pdPASS) ESP_LOGI(TAG, "I2C timer initialization failed");

    slow_system_status_queue = xQueueCreate(1,1);

    TaskHandle_t slow_system_status_task_handle;
    xTaskCreatePinnedToCore(&slow_system_status_task, "slow system status task", 4096, NULL, configMAX_PRIORITIES-2, &slow_system_status_task_handle, 0);

    TimerHandle_t slow_system_status_timer_handle = xTimerCreate("slow system status timer", pdMS_TO_TICKS(SLOW_SYSTEM_STATUS_PERIOD), pdTRUE, (void *) 0, *slow_system_status_timer);
    if( xTimerStart(slow_system_status_timer_handle, 0 ) != pdPASS) ESP_LOGI(TAG, "I2C task initialization failed");

    hw_init_done = 1;
}

uint8_t hardware_is_initialized(){
    return hw_init_done;
}

