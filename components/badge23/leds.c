#include "badge23/leds.h"
#include "portexpander.h"

#include "driver/spi_master.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_system.h"

typedef struct leds_cfg {
    int             leaf;
    size_t          size;
    size_t          position;
    size_t          slot;
    int             timer;
} leds_cfg_t;

static QueueHandle_t leds_queue = NULL;
static leds_cfg_t active_leds[11];
static void leds_task(void* arg);

#include "badge23/apa102LEDStrip.h"
static struct apa102LEDStrip leds;

//SPI Vars
static spi_device_handle_t spi_led;
static spi_transaction_t spiTransObject;
static esp_err_t ret;
static spi_bus_config_t buscfg;
static spi_device_interface_config_t devcfg;

#define totalPixels 40
#define bytesPerPixel 4
#define maxValuePerColour 255
#define maxSPIFrameInBytes 8000
#define maxSPIFrequency 10000000

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

static uint8_t black[] = {0,0,0};

static const uint8_t pl[][9] = {
                          {39,  0,  1,  2,  3,  4,  5,  6,  7},
                          {3,   4,  5,  6,  7,  8,  9, 10, 11},
                          {7,   8,  9, 10, 11, 12, 13, 14, 15},
                          {11, 12, 13, 14, 15, 16, 17, 18, 19},
                          {15, 16, 17, 18, 19, 20, 21, 22, 23},
                          {19, 20, 21, 22, 23, 24, 25, 26, 27},
                          {23, 24, 25, 26, 27, 28, 29, 30, 31},
                          {27, 28, 29, 30, 31, 32, 33, 34, 35},
                          {31, 32, 33, 34, 35, 36, 37, 38, 39},
                          {35, 36, 37, 38, 39, 0,   1,  2,  3}
                          };

static const int8_t pan_leds[][11][2] = {
                          {
                                {pl[0][4], pl[0][4]}, {pl[0][3], pl[0][5]}, {pl[0][2], pl[0][6]}, {pl[0][1], pl[0][7]}, {pl[0][0], pl[0][8]},
                                    {pl[4][0], pl[6][8]}, {pl[4][1], pl[6][7]}, {pl[4][2], pl[6][6]}, {pl[4][3], pl[6][5]}, {pl[4][4], pl[6][4]}
                                ,{-1, -1}
                          },
                          {
                                {pl[2][4], pl[0][4]}, {pl[2][3], pl[0][5]}, {pl[2][2], pl[0][6]}, {pl[2][1], pl[0][7]}, {pl[2][0], pl[0][8]},
                                    {pl[6][8], pl[6][0]}, {pl[6][6], pl[6][1]}, {pl[6][6], pl[6][2]}, {pl[6][5], pl[6][3]}, {pl[6][4], pl[6][4]}
                                ,{-1, -1}
                          },
                          {
                                {pl[2][4], pl[2][4]}, {pl[2][3], pl[2][5]}, {pl[2][2], pl[2][6]}, {pl[2][1], pl[2][7]}, {pl[2][0], pl[2][8]},
                                    {pl[6][0], pl[8][8]}, {pl[6][1], pl[8][7]}, {pl[6][2], pl[8][6]}, {pl[6][3], pl[8][5]}, {pl[6][4], pl[8][4]}
                                ,{-1, -1}
                          },
                          {
                                {pl[4][4], pl[2][4]}, {pl[4][3], pl[2][5]}, {pl[4][2], pl[2][6]}, {pl[4][1], pl[2][7]}, {pl[4][0], pl[2][8]},
                                    {pl[8][8], pl[8][0]}, {pl[8][6], pl[8][1]}, {pl[8][6], pl[8][2]}, {pl[8][5], pl[8][3]}, {pl[8][4], pl[8][4]}
                                ,{-1, -1}
                          },
                          {
                                {pl[4][4], pl[4][4]}, {pl[4][3], pl[4][5]}, {pl[4][2], pl[4][6]}, {pl[4][1], pl[4][7]}, {pl[4][0], pl[4][8]},
                                    {pl[8][0], pl[0][8]}, {pl[8][1], pl[0][7]}, {pl[8][2], pl[0][6]}, {pl[8][3], pl[0][5]}, {pl[8][4], pl[0][4]}
                                ,{-1, -1}
                          },
                          {
                                {pl[6][4], pl[4][4]}, {pl[6][3], pl[4][5]}, {pl[6][2], pl[4][6]}, {pl[6][1], pl[4][7]}, {pl[6][0], pl[4][8]},
                                    {pl[0][8], pl[0][0]}, {pl[0][6], pl[0][1]}, {pl[0][6], pl[0][2]}, {pl[0][5], pl[0][3]}, {pl[0][4], pl[0][4]}
                                ,{-1, -1}
                          },
                          {
                                {pl[6][4], pl[6][4]}, {pl[6][3], pl[6][5]}, {pl[6][2], pl[6][6]}, {pl[6][1], pl[6][7]}, {pl[6][0], pl[6][8]},
                                    {pl[0][0], pl[6][2]}, {pl[0][1], pl[2][7]}, {pl[0][2], pl[2][6]}, {pl[0][3], pl[2][5]}, {pl[0][4], pl[2][4]}
                                ,{-1, -1}
                          },
                          {
                                {pl[8][4], pl[6][4]}, {pl[8][3], pl[6][5]}, {pl[8][2], pl[6][6]}, {pl[8][1], pl[6][7]}, {pl[8][0], pl[6][8]},
                                    {pl[2][8], pl[2][0]}, {pl[2][6], pl[2][1]}, {pl[2][6], pl[2][2]}, {pl[2][5], pl[2][3]}, {pl[2][4], pl[2][4]}
                                ,{-1, -1}
                          },
                          {
                                {pl[8][4], pl[8][4]}, {pl[8][3], pl[8][5]}, {pl[8][2], pl[8][6]}, {pl[8][1], pl[8][7]}, {pl[8][0], pl[8][8]},
                                    {pl[2][0], pl[4][4]}, {pl[2][1], pl[4][7]}, {pl[2][2], pl[4][6]}, {pl[2][3], pl[4][5]}, {pl[2][4], pl[4][4]}
                                ,{-1, -1}
                          },
                          {
                                {pl[0][4], pl[8][4]}, {pl[0][3], pl[8][5]}, {pl[0][2], pl[8][6]}, {pl[0][1], pl[8][7]}, {pl[0][0], pl[8][8]},
                                    {pl[4][8], pl[4][0]}, {pl[4][6], pl[4][1]}, {pl[4][6], pl[4][2]}, {pl[4][5], pl[4][3]}, {pl[4][4], pl[4][4]}
                                ,{-1, -1}
                          },

                          };


static void _leds_init() {
    leds_queue = xQueueCreate(10, sizeof(leds_cfg_t));
    memset(active_leds, 0 , sizeof(active_leds));

    setupSPI();
    initLEDs(&leds, totalPixels, bytesPerPixel, 255);

    //Set up SPI tx/rx storage Object
    memset(&spiTransObject, 0, sizeof(spiTransObject));
    spiTransObject.length = leds._frameLength*8;
    spiTransObject.tx_buffer = leds.LEDs;

    portexpander_set_leds(true);

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));

    TaskHandle_t handle;
    //xTaskCreate(&leds_task, "LEDs player", 4096, NULL, configMAX_PRIORITIES - 2, &handle);
}

static bool leds_active(void)
{
    for(size_t i=0; i<sizeof(active_leds) / sizeof(active_leds[0]); i++) {
        if(active_leds[i].size != active_leds[i].position) {
            return true;
        }
    }
    return false;
}


static void leds_task(void* arg) {
    leds_cfg_t  leds_;

    static int timer = 0;

    while(true) {
        if(xQueueReceive(leds_queue, &leds_, 1)) {
            leds_.position = 0;
            active_leds[leds_.slot] = leds_;
        }

        if(leds_active()) {
            for(int i=0; i<40; i++) {
                setPixel(&leds, i, black);
            }

            for(size_t i=0; i<sizeof(active_leds) / sizeof(active_leds[0]); i++) {
                if(active_leds[i].size == active_leds[i].position) continue;

                int leaf = active_leds[i].leaf;
                int position = active_leds[i].position;

                uint8_t c[] = {0,0,0};
                if(leaf % 2 == 0)
                    c[0] = 32;
                else
                    c[2] = 32;

                struct RGB rgb = HSVToRGB((struct HSV){leaf/9.*360,1,1});
                c[0] = rgb.R / 4;
                c[1] = rgb.G / 4;
                c[2] = rgb.B / 4;

                if(pan_leds[leaf][position][0] >= 0)
                    setPixel(&leds, pan_leds[leaf][position][0], c);
                if(pan_leds[leaf][position][1] >= 0)
                    setPixel(&leds, pan_leds[leaf][position][1], c);

                if(active_leds[i].timer-- == 0) {
                    active_leds[i].timer = 2;
                    active_leds[i].position += 1;
                }

            }

            renderLEDs();
            timer = 30;
        }

        static int phase = 0;

        if(!leds_active()) {
            // background anim

            if(timer-- == 0) {
                timer = 60;
                for(int i=0; i<40; i++) {
                    setPixel(&leds, i, black);
                }

                if(phase++ == 360) {
                    phase = 0;
                }

                uint8_t c[] = {0,0,0};

                for(int i=0; i<40; i++) {
                    struct RGB rgb = HSVToRGB((struct HSV){(phase+i*3) % 360,1,0.125});
                    c[0] = rgb.R / 1;
                    c[1] = rgb.G / 1;
                    c[2] = rgb.B / 1;
                    setPixel(&leds, i, c);
                }

                renderLEDs();
            }
        }
    }
}

void leds_set_single_rgb(uint8_t index, uint8_t red, uint8_t green, uint8_t blue){
    uint8_t c[3];
    c[0] = red;
    c[1] = green;
    c[2] = blue;
    setPixel(&leds, index, c);
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
    setPixel(&leds, index, c);
}

void leds_update(){
    vTaskDelay(10 / portTICK_PERIOD_MS);
    renderLEDs();
    vTaskDelay(10 / portTICK_PERIOD_MS);
}

void leds_init() { _leds_init(); }

void leds_animate(int leaf) {
    leds_cfg_t leds = {0,};

    leds.leaf = leaf;
    leds.slot = leaf;
    leds.size = 11;
    xQueueSend(leds_queue, &leds, 0);
}
