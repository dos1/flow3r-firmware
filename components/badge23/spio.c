//special purpose input outputs
#include "driver/gpio.h"
#include "badge23_hwconfig.h"
#include "stdint.h"
#include "badge23/spio.h"
#include "badge23/lock.h"
#include "badge23/audio.h"

#include "driver/i2c.h"
#define I2C_MASTER_NUM 0
#define TIMEOUT_MS 1000

#if defined(CONFIG_BADGE23_HW_GEN_P1)

#define BADGE_LINK_LINE_OUT_TIP_ENABLE_PIN 5
#define BADGE_LINK_LINE_OUT_RING_ENABLE_PIN 6
#define BADGE_LINK_LINE_IN_TIP_ENABLE_PIN 3
#define BADGE_LINK_LINE_IN_RING_ENABLE_PIN 4

#elif defined(CONFIG_BADGE23_HW_GEN_P4) || defined(CONFIG_BADGE23_HW_GEN_P3)

#define BADGE_LINK_LINE_OUT_TIP_ENABLE_PIN 6
#define BADGE_LINK_LINE_OUT_RING_ENABLE_PIN 7
#define BADGE_LINK_LINE_IN_TIP_ENABLE_PIN 5
#define BADGE_LINK_LINE_IN_RING_ENABLE_PIN 4

//on ESP32
#define LEFT_BUTTON_LEFT 3
#define LEFT_BUTTON_MID 0

//on PORTEXPANDER
#define LEFT_BUTTON_RIGHT (0+8)
#define RIGHT_BUTTON_LEFT (6+8)
#define RIGHT_BUTTON_MID (5+8)
#define RIGHT_BUTTON_RIGHT (7+8)

#elif defined(CONFIG_BADGE23_HW_GEN_P6)

#define BADGE_LINK_LINE_OUT_RING_ENABLE_PIN 5
#define BADGE_LINK_LINE_OUT_TIP_ENABLE_PIN 6
#define BADGE_LINK_LINE_IN_TIP_ENABLE_PIN 4
#define BADGE_LINK_LINE_IN_RING_ENABLE_PIN 3

#define ENABLE_INVERTED 

//on ESP32
#define RIGHT_BUTTON_MID 3
#define LEFT_BUTTON_MID 0

//on PORTEXPANDER
#define LEFT_BUTTON_RIGHT (0+8)
#define RIGHT_BUTTON_LEFT (4+8)
#define LEFT_BUTTON_LEFT (7+8)
#define RIGHT_BUTTON_RIGHT (5+8)

#endif

static int8_t leftbutton = 0;
static int8_t rightbutton = 0;

static bool menu_button_left = 0;

static uint8_t badge_link_enabled = 0;

/* The MAX7321 doesn't have any input/output pin configuration methods. Instead,
 * it has a ~40k pullup resistor to VCC and a programmable shunt transistor to VEE.
 * Writing a byte to the IC closes each shunt with a LO at its index.
 * Reading a byte returns the state of each pin.
 * 
 * This means that changing a single output bit cannot be changed without some
 * information about the other pins. Also a output pin set to HI can read as LO
 * if there's an outside shunt.
 */
typedef struct{
    uint8_t address;
    uint8_t is_output_pin;  // mask for pins we wish to use as outputs
    uint8_t read_state;     //
    uint8_t output_state;   // goal output state
} max7321_t;

max7321_t port_expanders[2]; 

void _max7321_update(max7321_t *max){
    uint8_t rx = 0;
    uint8_t tx = (~(max->is_output_pin)) | max->output_state;

    xSemaphoreTake(mutex_i2c, portMAX_DELAY);
    esp_err_t tx_error = i2c_master_write_to_device(I2C_MASTER_NUM, max->address, &tx, sizeof(tx), TIMEOUT_MS / portTICK_PERIOD_MS);
    esp_err_t rx_error = i2c_master_read_from_device(I2C_MASTER_NUM, max->address, &rx, sizeof(rx), TIMEOUT_MS / portTICK_PERIOD_MS);
    xSemaphoreGive(mutex_i2c);
    max->read_state = rx;
}

void max7321s_update(){
    _max7321_update(&port_expanders[0]);
    _max7321_update(&port_expanders[1]);
}

bool max7321s_get_pin(uint8_t pin){
    if(pin>15) return 0;
    max7321_t * pe = &port_expanders[0];
    if(pin>7){
        pe = &port_expanders[1];
        pin -= 8;
    }
    return ((pe->read_state) >> pin) & 1;
}

bool max7321s_set_pin(uint8_t pin, bool on){
    if(pin>15) return 0;
    max7321_t * pe = &port_expanders[0];
    if(pin>7){
        pe = &port_expanders[1];
        pin -= 8;
    }

    if(((pe->is_output_pin) >> pin) & 1){
        if(on){
            pe->output_state |= 1<<pin;
        } else {
            pe->output_state &= ~(1<<pin);
        }
        return 1;
    } else {
        return 0;
    }
}

void max7321s_set_pinmode_output(uint8_t pin, bool output){
    if(pin>15) return 0;
    max7321_t * pe = &port_expanders[0];
    if(pin>7){
        pe = &port_expanders[1];
        pin -= 8;
    }
    if(output){
        pe->is_output_pin |= (1<<pin);
    } else {
        pe->is_output_pin &= ~(1<<pin);
    }
}

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

max7321_t port_expanders[] = {  {0b01101110, 0, 255, 255}, 
                                {0b01101101, 0, 255, 255}  };

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
    
    max7321s_set_pinmode_output(RIGHT_BUTTON_RIGHT, 0);
    max7321s_set_pinmode_output(RIGHT_BUTTON_MID, 0);
    max7321s_set_pinmode_output(RIGHT_BUTTON_LEFT, 0);
    max7321s_set_pinmode_output(LEFT_BUTTON_RIGHT, 0);
}

int8_t process_button_state(bool r, bool m, bool l){
    if(!l){
        return BUTTON_PRESSED_LEFT;
    } else if(!m){
        return BUTTON_PRESSED_DOWN;
    } else if(!r){
        return BUTTON_PRESSED_RIGHT;
    } else {
        return BUTTON_NOT_PRESSED;
    }
}

void update_button_state(){
    max7321s_update();
    uint8_t rr = max7321s_get_pin(RIGHT_BUTTON_RIGHT);
    uint8_t rm = max7321s_get_pin(RIGHT_BUTTON_MID);
    uint8_t rl = max7321s_get_pin(RIGHT_BUTTON_LEFT);
    uint8_t lr = max7321s_get_pin(LEFT_BUTTON_RIGHT);
    uint8_t ll = gpio_get_level(LEFT_BUTTON_LEFT);
    uint8_t lm = gpio_get_level(LEFT_BUTTON_MID);

    int8_t new_rightbutton = process_button_state(rr, rm, rl);
    int8_t new_leftbutton = process_button_state(lr, lm, ll);
    if(new_rightbutton != rightbutton){
        //TODO: CALLBACK button_state_has_changed_to(new_rightbutton)
        //note: consider menubutton/application button config option
    }
    if(new_leftbutton != leftbutton){
        //TODO: CALLBACK button_state_has_changed_to(new_leftbutton)
    }
    rightbutton = new_rightbutton;
    leftbutton = new_leftbutton;
}
#elif defined(CONFIG_BADGE23_HW_GEN_P6)
max7321_t port_expanders[] = {  {0b01101110, 0, 255, 255}, 
                                {0b01101101, 0, 255, 255}  };

static void _init_buttons(){
    //configure all buttons as pullup
    gpio_config_t cfg = {
        .pin_bit_mask = 1 << RIGHT_BUTTON_MID,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
    cfg.pin_bit_mask = 1;
    cfg.pull_up_en = GPIO_PULLUP_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&cfg));
    
    max7321s_set_pinmode_output(RIGHT_BUTTON_RIGHT, 0);
    max7321s_set_pinmode_output(LEFT_BUTTON_LEFT, 0);
    max7321s_set_pinmode_output(RIGHT_BUTTON_LEFT, 0);
    max7321s_set_pinmode_output(LEFT_BUTTON_RIGHT, 0);
}

int8_t process_button_state(bool r, bool m, bool l){
    if(!l){
        return BUTTON_PRESSED_LEFT;
    } else if(!m){
        return BUTTON_PRESSED_DOWN;
    } else if(!r){
        return BUTTON_PRESSED_RIGHT;
    } else {
        return BUTTON_NOT_PRESSED;
    }
}

void update_button_state(){
    max7321s_update();
    uint8_t rr = max7321s_get_pin(RIGHT_BUTTON_RIGHT);
    uint8_t ll = max7321s_get_pin(LEFT_BUTTON_LEFT);
    uint8_t rl = max7321s_get_pin(RIGHT_BUTTON_LEFT);
    uint8_t lr = max7321s_get_pin(LEFT_BUTTON_RIGHT);
    uint8_t rm = gpio_get_level(RIGHT_BUTTON_MID);
    uint8_t lm = gpio_get_level(LEFT_BUTTON_MID);

    int8_t new_rightbutton = process_button_state(rr, rm, rl);
    int8_t new_leftbutton = process_button_state(lr, lm, ll);
    if(new_rightbutton != rightbutton){
        //TODO: CALLBACK button_state_has_changed_to(new_rightbutton)
        //note: consider menubutton/application button config option
    }
    if(new_leftbutton != leftbutton){
        //TODO: CALLBACK button_state_has_changed_to(new_leftbutton)
    }
    rightbutton = new_rightbutton;
    leftbutton = new_leftbutton;
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

uint8_t spio_badge_link_get_active(uint8_t pin_mask){
    return badge_link_enabled & pin_mask;
}

#if defined(CONFIG_BADGE23_HW_GEN_P1)

static uint8_t spio_badge_link_set(uint8_t pin_mask, uint8_t state){
    return 0; // no badge link here (yet)
}

#elif defined(CONFIG_BADGE23_HW_GEN_P3) || defined(CONFIG_BADGE23_HW_GEN_P4) || defined(CONFIG_BADGE23_HW_GEN_P6)

#define USER_WARNINGS_ENABLED

static int8_t spio_badge_link_set(uint8_t pin_mask, bool state){
    if(state) {
        if((pin_mask & BADGE_LINK_PIN_MASK_LINE_OUT_RING) || (pin_mask & BADGE_LINK_PIN_MASK_LINE_OUT_TIP)){
            if(!audio_headphones_are_connected()) {
                pin_mask &= ~BADGE_LINK_PIN_MASK_LINE_OUT_RING;
                pin_mask &= ~BADGE_LINK_PIN_MASK_LINE_OUT_TIP;
#ifdef USER_WARNINGS_ENABLED
                printf("cannot enable line out badge link without cable plugged in for safety reasons\n");
            } else {
                printf("badge link enabled on line out. please make sure not to have headphones or sound sources connected before transmitting data.\n");
#endif
            }
        }
    }

#ifdef ENABLE_INVERTED
    uint8_t hw_state = state;
#else
    uint8_t hw_state = !state;
#endif
    if(pin_mask & BADGE_LINK_PIN_MASK_LINE_IN_RING) max7321s_set_pinmode_output(BADGE_LINK_LINE_IN_RING_ENABLE_PIN, 1);
    if(pin_mask & BADGE_LINK_PIN_MASK_LINE_IN_TIP) max7321s_set_pinmode_output(BADGE_LINK_LINE_IN_TIP_ENABLE_PIN, 1);
    if(pin_mask & BADGE_LINK_PIN_MASK_LINE_OUT_RING) max7321s_set_pinmode_output(BADGE_LINK_LINE_OUT_RING_ENABLE_PIN, 1);
    if(pin_mask & BADGE_LINK_PIN_MASK_LINE_OUT_TIP) max7321s_set_pinmode_output(BADGE_LINK_LINE_OUT_TIP_ENABLE_PIN, 1);

    if(pin_mask & BADGE_LINK_PIN_MASK_LINE_IN_RING) max7321s_set_pin(BADGE_LINK_LINE_IN_RING_ENABLE_PIN, hw_state);
    if(pin_mask & BADGE_LINK_PIN_MASK_LINE_IN_TIP) max7321s_set_pin(BADGE_LINK_LINE_IN_TIP_ENABLE_PIN, hw_state);
    if(pin_mask & BADGE_LINK_PIN_MASK_LINE_OUT_RING) max7321s_set_pin(BADGE_LINK_LINE_OUT_RING_ENABLE_PIN, hw_state);
    if(pin_mask & BADGE_LINK_PIN_MASK_LINE_OUT_TIP) max7321s_set_pin(BADGE_LINK_LINE_OUT_TIP_ENABLE_PIN, hw_state);

    max7321s_update();
    badge_link_enabled = (badge_link_enabled & (~pin_mask)) | (pin_mask & (hw_state ? 255 : 0));
    return spio_badge_link_get_active(pin_mask);
}
#endif

uint8_t spio_badge_link_disable(uint8_t pin_mask){
    return spio_badge_link_set(pin_mask, 0);
}

uint8_t spio_badge_link_enable(uint8_t pin_mask){
    return spio_badge_link_set(pin_mask, 1);
}
