#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "esp_system.h"
#include "badge23/leds.h"
#include "badge23_hwconfig.h"

#if defined(CONFIG_BADGE23_HW_GEN_P1)
#define LED_SPI_PORT

#elif defined(CONFIG_BADGE23_HW_GEN_P4)
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

void set_single_led(uint8_t index, uint8_t c[3]){
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


    TaskHandle_t handle;
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

void leds_set_single_rgb(uint8_t index, uint8_t red, uint8_t green, uint8_t blue){
    uint8_t c[3];
    c[0] = red;
    c[1] = green;
    c[2] = blue;
    set_single_led(index, c);
}
 
void leds_set_single_hsv(uint8_t index, float hue, float sat, float val){
    struct RGB rgb;
    struct HSV hsv;
    hsv.H = hue;
    hsv.S = sat;
    hsv.V = val;
    
    rgb = HSVToRGB(hsv);

    uint8_t c[3];
    c[0] = rgb.R;
    c[1] = rgb.G;
    c[2] = rgb.B;
    set_single_led(index, c);
}

void leds_update(){
    vTaskDelay(10 / portTICK_PERIOD_MS); //do we...
    renderLEDs();
    vTaskDelay(10 / portTICK_PERIOD_MS); //...need these?
}

void leds_init() { _leds_init(); }
