//special purpose input outputs
#include "driver/gpio.h"
#include "badge23_hwconfig.h"
#include "stdint.h"
#include "badge23/spio.h"
#include "badge23/lock.h"

static int8_t leftbutton = 0;
static int8_t rightbutton = 0;

#if defined(CONFIG_BADGE23_HW_GEN_P1)

#define RIGHT_BUTTON_LEFT 37
#define RIGHT_BUTTON_MID 0
#define RIGHT_BUTTON_RIGHT 35

#define LEFT_BUTTON_LEFT 7
#define LEFT_BUTTON_MID 6
#define LEFT_BUTTON_RIGHT 5

static void _init_buttons(){
    //configure all buttons as pullup
    uint64_t mask = 0;
    mask |= (1ULL << RIGHT_BUTTON_LEFT);
    mask |= (1ULL << RIGHT_BUTTON_RIGHT);
    mask |= (1ULL << LEFT_BUTTON_LEFT);
    mask |= (1ULL << LEFT_BUTTON_MID);
    mask |= (1ULL << LEFT_BUTTON_RIGHT);
    gpio_config_t cfg = {
        .pin_bit_mask = mask,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
    cfg.pin_bit_mask = 1;
    cfg.pull_up_en = GPIO_PULLUP_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&cfg));
}

void update_button_state(){
    if(!gpio_get_level(RIGHT_BUTTON_LEFT)){
        rightbutton = BUTTON_PRESSED_LEFT;
    } else if(!gpio_get_level(RIGHT_BUTTON_MID)){
        rightbutton = BUTTON_PRESSED_DOWN;
    } else if(!gpio_get_level(RIGHT_BUTTON_RIGHT)){
        rightbutton = BUTTON_PRESSED_RIGHT;
    } else {
        rightbutton = BUTTON_NOT_PRESSED;
    }

    if(!gpio_get_level(LEFT_BUTTON_LEFT)){
        leftbutton = BUTTON_PRESSED_LEFT;
    } else if(!gpio_get_level(LEFT_BUTTON_MID)){
        leftbutton = BUTTON_PRESSED_DOWN;
    } else if(!gpio_get_level(LEFT_BUTTON_RIGHT)){
        leftbutton = BUTTON_PRESSED_RIGHT;
    } else {
        leftbutton = BUTTON_NOT_PRESSED;
    }
}

#elif defined(CONFIG_BADGE23_HW_GEN_P3) || defined(CONFIG_BADGE23_HW_GEN_P4)

#include "driver/i2c.h"
#define I2C_MASTER_NUM 0
#define TIMEOUT_MS 1000

//on ESP32
#define LEFT_BUTTON_LEFT 3
#define LEFT_BUTTON_MID 0

//on PORTEXPANDER
#define LEFT_BUTTON_RIGHT 0
#define RIGHT_BUTTON_LEFT 6
#define RIGHT_BUTTON_MID 5
#define RIGHT_BUTTON_RIGHT 7

static void _init_buttons(){
    //configure all buttons as pullup
    gpio_config_t cfg = {
        .pin_bit_mask = 1 << LEFT_BUTTON_LEFT,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
    cfg.pin_bit_mask = 1;
    cfg.pull_up_en = GPIO_PULLUP_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&cfg));
    printf("nya\n");
}

void update_button_state(){
    uint8_t port;

    xSemaphoreTake(mutex_i2c, portMAX_DELAY);
    esp_err_t ret = i2c_master_read_from_device(I2C_MASTER_NUM, 0b1101101, &port, sizeof(port), TIMEOUT_MS / portTICK_PERIOD_MS);
    xSemaphoreGive(mutex_i2c);

    uint8_t rr = port & (1ULL << RIGHT_BUTTON_RIGHT);
    uint8_t rm = port & (1ULL << RIGHT_BUTTON_MID);
    uint8_t rl = port & (1ULL << RIGHT_BUTTON_LEFT);
    uint8_t lr = port & (1ULL << LEFT_BUTTON_RIGHT);
    uint8_t ll = gpio_get_level(LEFT_BUTTON_LEFT);
    uint8_t lm = gpio_get_level(LEFT_BUTTON_MID);

    if(!rl){
        rightbutton = BUTTON_PRESSED_LEFT;
    } else if(!rm){
        rightbutton = BUTTON_PRESSED_DOWN;
    } else if(!rr){
        rightbutton = BUTTON_PRESSED_RIGHT;
    } else {
        rightbutton = BUTTON_NOT_PRESSED;
    }

    if(!ll){
        leftbutton = BUTTON_PRESSED_LEFT;
    } else if(!lm){
        leftbutton = BUTTON_PRESSED_DOWN;
    } else if(!lr){
        leftbutton = BUTTON_PRESSED_RIGHT;
    } else {
        leftbutton = BUTTON_NOT_PRESSED;
    }
}

#else
#error "spio not implemented for this badge generation"
#endif

void init_buttons(){
    _init_buttons();
}

//#define ALWAYS_UPDATE_BUTTON
int8_t get_button_state(bool left){
#ifdef ALWAYS_UPDATE_BUTTON
    update_button_state();
#endif
    if(left) return leftbutton;
    return rightbutton;
}

static bool menu_button_left = 0;

void spio_menu_button_set_left(bool left){
    menu_button_left = 1;
}

int8_t spio_menu_button_get(){
    return get_button_state(menu_button_left);
}

int8_t spio_application_button_get(){
    return get_button_state(!menu_button_left);
}

int8_t spio_left_button_get(){
    return get_button_state(1);
}

int8_t spio_right_button_get(){
    return get_button_state(0);
}

int8_t spio_menu_button_get_left(){
    return menu_button_left;
}
