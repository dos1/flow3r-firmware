#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "esp_system.h"
#include "badge23/leds.h"
#include "badge23/lock.h"
#include "badge23_hwconfig.h"

static uint8_t leds_brightness = 69;;
static uint8_t leds_slew_rate = 255;
static bool leds_auto_update = 0;

static uint8_t gamma_red[256];
static uint8_t gamma_green[256];
static uint8_t gamma_blue[256];

#if defined(CONFIG_BADGE23_HW_GEN_P1)
#define LED_SPI_PORT
#elif defined(CONFIG_BADGE23_HW_GEN_P3) || defined(CONFIG_BADGE23_HW_GEN_P4) || defined(CONFIG_BADGE23_HW_GEN_P6)
#define LED_ASYNC_PORT
#else
#error "leds not implemented for this badge generation"
#endif

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

struct RGB led_target[40] = {0,};
struct RGB led_target_buffer[40] = {0,};
struct RGB led_hardware_value[40] = {0,};

struct HSV
{
    double H;
    double S;
    double V;
};

struct RGB HSVToRGB(struct HSV hsv) {
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

#ifdef LED_SPI_PORT
#include "driver/spi_master.h"
#include "badge23/apa102LEDStrip.h"

#define totalPixels 40
#define bytesPerPixel 4

static struct apa102LEDStrip leds;
static spi_device_handle_t spi_led;
static spi_transaction_t spiTransObject;
static esp_err_t ret;
static spi_bus_config_t buscfg;
static spi_device_interface_config_t devcfg;

#define maxSPIFrameInBytes 8000
#define maxSPIFrequency 10000000

void renderLEDs()
{
    spi_device_queue_trans(spi_led, &spiTransObject, portMAX_DELAY);
}

static int setupSPI()
{
    //Set up the Bus Config struct
    buscfg.miso_io_num=-1;
    buscfg.mosi_io_num=18;
    buscfg.sclk_io_num=8;
    buscfg.quadwp_io_num=-1;
    buscfg.quadhd_io_num=-1;
    buscfg.max_transfer_sz=maxSPIFrameInBytes;

    //Set up the SPI Device Configuration Struct
    devcfg.clock_speed_hz=maxSPIFrequency;
    devcfg.mode=0;
    devcfg.spics_io_num=-1;
    devcfg.queue_size=1;

    //Initialize the SPI driver
    ret=spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
    //Add SPI port to bus
    ret=spi_bus_add_device(SPI2_HOST, &devcfg, &spi_led);
    ESP_ERROR_CHECK(ret);
    return ret;
}

static void set_single_led(uint8_t index, uint8_t c[3]){
    setPixel(&leds, index, c);
}

static void _leds_init() {
    memset(active_leds, 0 , sizeof(active_leds));

    setupSPI();
    initLEDs(&leds, totalPixels, bytesPerPixel, 255);

    //Set up SPI tx/rx storage Object
    memset(&spiTransObject, 0, sizeof(spiTransObject));
    spiTransObject.length = leds._frameLength*8;
    spiTransObject.tx_buffer = leds.LEDs;


    //TaskHandle_t handle;
    //xTaskCreate(&leds_task, "LEDs player", 4096, NULL, configMAX_PRIORITIES - 2, &handle);
}
#endif

#ifdef LED_ASYNC_PORT
#include "../../espressif__led_strip/include/led_strip.h"
led_strip_t *led_strip_init(uint8_t channel, uint8_t gpio, uint16_t led_num);

#define LED_GPIO_NUM 14
#define LED_RMT_CHAN 0

led_strip_t * led_strip;

static void _leds_init(){
    memset(active_leds, 0 , sizeof(active_leds));
    led_strip = led_strip_init(LED_RMT_CHAN, LED_GPIO_NUM, 40);
}

void set_single_led(uint8_t index, uint8_t c[3]){
    index = ((39-index) + 1 + 32)%40;
    led_strip->set_pixel(led_strip, index, c[0], c[1], c[2]);
}

static void renderLEDs(){
    led_strip->refresh(led_strip, 1000);
}

#endif

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

void leds_update_hardware(){ 
    if(leds_auto_update) leds_update_target();
    xSemaphoreTake(mutex_LED, portMAX_DELAY);
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
    xSemaphoreGive(mutex_LED);
}

void leds_set_single_rgb(uint8_t index, uint8_t red, uint8_t green, uint8_t blue){
    led_target_buffer[index].R = red;
    led_target_buffer[index].G = green;
    led_target_buffer[index].B = blue;
}
 
void leds_set_single_hsv(uint8_t index, float hue, float sat, float val){
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

void leds_set_all_rgb(uint8_t red, uint8_t green, uint8_t blue){
    for(int i = 0; i<40; i++){
        leds_set_single_rgb(i, red, green, blue);
    }
}

void leds_set_all_hsv(float h, float s, float v){
    for(int i = 0; i<40; i++){
        leds_set_single_hsv(i, h, s, v);
    }
}

void leds_update(){
    leds_update_target();
    leds_update_hardware();
}

void leds_init(){
    for(uint16_t i = 0; i<256; i++){
        gamma_red[i] = i;
        gamma_green[i] = i;
        gamma_blue[i] = i;
    }
    _leds_init();
}

void leds_set_brightness(uint8_t b){
    leds_brightness = b;
}

uint8_t leds_get_brightness(){
    return leds_brightness;
}

void leds_set_slew_rate(uint8_t s){
    leds_slew_rate = s;
}

uint8_t leds_get_slew_rate(){
    return leds_slew_rate;
}

void leds_set_auto_update(bool on){
    leds_auto_update = on;
}

bool leds_get_auto_update(){
    return leds_auto_update;
}

void leds_set_gamma(float red, float green, float blue){
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
