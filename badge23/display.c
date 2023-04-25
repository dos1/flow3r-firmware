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

#include "scope.h"
#include "esp_system.h"

uint16_t *pixels;

typedef struct leds_cfg {
    bool active_paddles[10];
} display_cfg_t;

static QueueHandle_t display_queue = NULL;
static void display_task(TimerHandle_t aaaaa);
//static void display_task(void* arg);

static void _display_init() {
    GC9A01_Init();
    //    GC9A01_Screen_Load(0,0,240,240,pixels);
    GC9A01_Update();

    /*
    display_queue = xQueueCreate(1, sizeof(display_cfg_t));
    TaskHandle_t handle;
    xTaskCreate(&display_task, "Display", 4096, NULL, configMAX_PRIORITIES - 3, &handle);
    */
    
    /* SCOPE TASK
    TimerHandle_t aa = xTimerCreate("Display", pdMS_TO_TICKS(100), pdTRUE, (void *) 0, *display_task);
    if( xTimerStart(aa, 0 ) != pdPASS )
    {
    }
    */
}

void display_update(){
    GC9A01_Update();
}

void display_draw_pixel(uint8_t x, uint8_t y, uint16_t col){
    GC9A01_DrawPixel(x, y, col);
}

uint16_t display_get_pixel(uint8_t x, uint8_t y){
    return GC9A01_GetPixel(x,y);
}

void display_fill(uint16_t col){
    GC9A01_FillRect(0, 0, 240, 240, col);
}

void display_draw_scope(){
    //display_cfg_t  display_;
    uint16_t line[240];
    /*
    printf("waiting...\n");
    xQueueReceive(display_queue, &display_, portMAX_DELAY);
    printf("go...\n");
    */
    //uint32_t t0 = esp_log_timestamp();
    begin_scope_read();

    for(int y=0; y<240; y++){
        read_line_from_scope(&(line[0]), y);
        memcpy(&ScreenBuff[y * 240], line, sizeof(line));
    }
    end_scope_read();

    //uint32_t td = esp_log_timestamp() - t0;
    // printf("it took %lu\n", td);
    display_update();

}
//static void display_task(void* arg) {
static void display_task(TimerHandle_t aaaaa) {
    display_draw_scope();
}

void display_init() { _display_init(); }

