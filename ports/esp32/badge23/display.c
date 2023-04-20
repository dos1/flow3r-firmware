#include "display.h"
#include "components/gc9a01/gc9a01.h"

#include "esp_log.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <freertos/queue.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
//#include <audio.h>

#include "scope.h"
#include "esp_system.h"

uint16_t *pixels;
#include "decode_image.h"

typedef struct leds_cfg {
    bool active_paddles[10];
} display_cfg_t;

static QueueHandle_t display_queue = NULL;
static void display_task(TimerHandle_t aaaaa);
//static void display_task(void* arg);

static void _display_init() {
    GC9A01_Init();

#if 0
    uint16_t Color;
    for(;;)
    {
        Color=rand();
        GC9A01_FillRect(0,0,239,239,Color);
        GC9A01_Update();
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
#endif

    //decode_image(&pixels);
    GC9A01_Screen_Load(0,0,240,240,pixels);
    GC9A01_Update();

    /*
    display_queue = xQueueCreate(1, sizeof(display_cfg_t));
    TaskHandle_t handle;
    xTaskCreate(&display_task, "Display", 4096, NULL, configMAX_PRIORITIES - 3, &handle);
    */
    TimerHandle_t aa = xTimerCreate("Display", pdMS_TO_TICKS(100), pdTRUE, (void *) 0, *display_task);
    if( xTimerStart(aa, 0 ) != pdPASS )
    {
    }
}

//static void display_task(void* arg) {
static void display_task(TimerHandle_t aaaaa) {
    display_cfg_t  display_;
        // printf("hewo!");

    //static const int paddle_pos[10][2] = {{120, 240}, {190, 217}, {234, 157}, {234, 82}, {190, 22}, {120, 0}, {49, 22}, {5, 82}, {5, 157}, {49, 217}};
    uint16_t line[240];
    //while(true) {
        /*
        printf("waiting...\n");
        xQueueReceive(display_queue, &display_, portMAX_DELAY);
        printf("go...\n");

        bool any_active = false;
        for(int i=0; i<10; i++) {
            any_active |= display_.active_paddles[i];
        }

        if(any_active) {
        */
        // printf("hewwo!");
        if(1) {
            uint32_t t0 = esp_log_timestamp();
            /*
            for(int y=0; y<240; y++) {
                for(int x=0; x<240; x++) {
                    uint16_t Color=0;
                    for(int i=0; i<10; i++) {
                        if(display_.active_paddles[i]) {
                            int x_d = x - paddle_pos[i][0];
                            int y_d = y - paddle_pos[i][1];

                            int dist = (x_d * x_d + y_d * y_d) / 64;
                            Color += dist;
                        }
                    }
                    line[x] = Color;
                }
                memcpy(&ScreenBuff[y * 240], line, sizeof(line));
            }
            */
            begin_scope_read();
            for(int y=0; y<240; y++){
                read_line_from_scope(&(line[0]), y);
                memcpy(&ScreenBuff[y * 240], line, sizeof(line));
            }
            end_scope_read();
            uint32_t td = esp_log_timestamp() - t0;
            // printf("it took %lu\n", td);

        } else {
            GC9A01_Screen_Load(0,0,240,240,pixels);
        }
        GC9A01_Update();
    //}
}

void display_init() { _display_init(); }

void display_show(bool active_paddles[10]) {
    display_cfg_t display = {0,};

    memcpy(display.active_paddles, active_paddles, sizeof(active_paddles[0]) * 10);
    xQueueOverwrite(display_queue, &display);
}
