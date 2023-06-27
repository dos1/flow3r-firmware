#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "esp_system.h"

#include "flow3r_bsp.h"
#include "st3m_leds.h"

#include "esp_log.h"

static const char *TAG = "st3m-leds";

static uint8_t leds_brightness = 69;;
static uint8_t leds_slew_rate = 255;
static bool leds_auto_update = 0;

static uint8_t gamma_red[256];
static uint8_t gamma_green[256];
static uint8_t gamma_blue[256];

typedef struct leds_cfg {
    int             leaf;
    size_t          size;
    size_t          position;
    size_t          slot;
    int             timer;
} leds_cfg_t;

static leds_cfg_t active_leds[11];

struct RGB
{
    unsigned char R;
    unsigned char G;
    unsigned char B;
};

static struct RGB led_target[40] = {0,};
static struct RGB led_target_buffer[40] = {0,};
static struct RGB led_hardware_value[40] = {0,};

SemaphoreHandle_t mutex;
#define LOCK xSemaphoreTake(mutex, portMAX_DELAY)
#define UNLOCK xSemaphoreGive(mutex)

struct HSV
{
    double H;
    double S;
    double V;
};

static struct RGB HSVToRGB(struct HSV hsv) {
    double r = 0, g = 0, b = 0;

    if (hsv.S == 0)
    {
        r = hsv.V;
        g = hsv.V;
        b = hsv.V;
    }
    else
    {
        int i;
        double f, p, q, t;

        if (hsv.H == 360)
            hsv.H = 0;
        else
            hsv.H = hsv.H / 60;

        i = (int)trunc(hsv.H);
        f = hsv.H - i;

        p = hsv.V * (1.0 - hsv.S);
        q = hsv.V * (1.0 - (hsv.S * f));
        t = hsv.V * (1.0 - (hsv.S * (1.0 - f)));

        switch (i)
        {
            case 0:
                r = hsv.V;
                g = t;
                b = p;
                break;

            case 1:
                r = q;
                g = hsv.V;
                b = p;
                break;

            case 2:
                r = p;
                g = hsv.V;
                b = t;
                break;

            case 3:
                r = p;
                g = q;
                b = hsv.V;
                break;

            case 4:
                r = t;
                g = p;
                b = hsv.V;
                break;

            default:
                r = hsv.V;
                g = p;
                b = q;
                break;
        }

    }

    struct RGB rgb;
    rgb.R = r * 255;
    rgb.G = g * 255;
    rgb.B = b * 255;

    return rgb;
}

static void set_single_led(uint8_t index, uint8_t c[3]){
    index = ((39-index) + 1 + 32)%40;
    flow3r_bsp_leds_set_pixel(index, c[0], c[1], c[2]);
}

static void renderLEDs(){
    esp_err_t ret = flow3r_bsp_leds_refresh(portMAX_DELAY);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LED refresh failed: %s", esp_err_to_name(ret));
    }
}

uint8_t led_get_slew(int16_t old, int16_t new, int16_t slew){
    if(new > old + slew){
        return old + slew;
    } else if(new > old) {
        return new;
    }
    if(new < old - slew){
        return old - slew;
    } else if(new < old) {
        return new;
    }
    return old;
}

static void leds_update_target(){
    for(int i = 0; i < 40; i++){
        led_target[i].R = led_target_buffer[i].R;
        led_target[i].G = led_target_buffer[i].G;
        led_target[i].B = led_target_buffer[i].B;
    }
}

void st3m_leds_update_hardware(){ 
    if(leds_auto_update) leds_update_target();
    LOCK;
    for(int i = 0; i < 40; i++){
        uint8_t c[3];
        c[0] = led_target[i].R * leds_brightness/255;
        c[1] = led_target[i].G * leds_brightness/255;
        c[2] = led_target[i].B * leds_brightness/255;
        c[0] = gamma_red[c[0]];
        c[1] = gamma_green[c[1]];
        c[2] = gamma_blue[c[2]];
        c[0] = led_get_slew(led_hardware_value[i].R, c[0], leds_slew_rate);
        c[1] = led_get_slew(led_hardware_value[i].G, c[1], leds_slew_rate);
        c[2] = led_get_slew(led_hardware_value[i].B, c[2], leds_slew_rate);
        led_hardware_value[i].R = c[0];
        led_hardware_value[i].G = c[1];
        led_hardware_value[i].B = c[2];
        int8_t index = i + 3 % 40;
        set_single_led(index, c);
    }
    renderLEDs();
    UNLOCK;
}

void st3m_leds_set_single_rgb(uint8_t index, uint8_t red, uint8_t green, uint8_t blue){
    led_target_buffer[index].R = red;
    led_target_buffer[index].G = green;
    led_target_buffer[index].B = blue;
}
 
void st3m_leds_set_single_hsv(uint8_t index, float hue, float sat, float val){
    struct RGB rgb;
    struct HSV hsv;
    hsv.H = hue;
    hsv.S = sat;
    hsv.V = val;
    
    rgb = HSVToRGB(hsv);

    led_target_buffer[index].R = rgb.R;
    led_target_buffer[index].G = rgb.G;
    led_target_buffer[index].B = rgb.B;
}

void st3m_leds_set_all_rgb(uint8_t red, uint8_t green, uint8_t blue){
    for(int i = 0; i<40; i++){
        st3m_leds_set_single_rgb(i, red, green, blue);
    }
}

void st3m_leds_set_all_hsv(float h, float s, float v){
    for(int i = 0; i<40; i++){
        st3m_leds_set_single_hsv(i, h, s, v);
    }
}

void st3m_leds_update(){
    leds_update_target();
    st3m_leds_update_hardware();
}

void st3m_leds_init(){
    mutex = xSemaphoreCreateMutex();
    assert(mutex != NULL);

    for(uint16_t i = 0; i<256; i++){
        gamma_red[i] = i;
        gamma_green[i] = i;
        gamma_blue[i] = i;
    }

    esp_err_t ret = flow3r_bsp_leds_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LED initialization failed: %s", esp_err_to_name(ret));
        abort();
    }

    ESP_LOGI(TAG, "LEDs initialized");
}

void st3m_leds_set_brightness(uint8_t b){
    leds_brightness = b;
}

uint8_t st3m_leds_get_brightness(){
    return leds_brightness;
}

void st3m_leds_set_slew_rate(uint8_t s){
    leds_slew_rate = s;
}

uint8_t st3m_leds_get_slew_rate(){
    return leds_slew_rate;
}

void st3m_leds_set_auto_update(bool on){
    leds_auto_update = on;
}

bool st3m_leds_get_auto_update(){
    return leds_auto_update;
}

void st3m_leds_set_gamma(float red, float green, float blue){
    for(uint16_t i = 0; i<256; i++){
        if(i == 0){
            gamma_red[i] = 0;
            gamma_green[i] = 0;
            gamma_blue[i] = 0;
        }
        float step = ((float) i) / 255.;
        gamma_red[i] = (uint8_t) (254.*(pow(step, red))+1);
        gamma_green[i] = (uint8_t) (254.*(pow(step, green))+1);
        gamma_blue[i] = (uint8_t) (254.*(pow(step, blue))+1);
    }
}
