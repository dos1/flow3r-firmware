#include "badge23/display.h"
#include "gc9a01.h"

#include "esp_log.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <freertos/queue.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "badge23/scope.h"
#include "esp_system.h"

#include "../../usermodule/uctx/uctx/ctx.h"

Ctx *the_ctx = NULL;

uint8_t scope_active = 0;

uint16_t *pixels;

typedef struct leds_cfg {
    bool active_paddles[10];
} display_cfg_t;

static QueueHandle_t display_queue = NULL;
static void display_task(TimerHandle_t dummy);
static TimerHandle_t display_animation_timer;

static void _display_init() {
    GC9A01_Init();
    GC9A01_Update();
    display_animation_timer = xTimerCreate("display animation timer", pdMS_TO_TICKS(100), pdTRUE, (void *) 0, *display_task);
}


void display_ctx_init() {
	the_ctx = ctx_new_for_framebuffer(
        ScreenBuff,
        GC9A01_Width,
        GC9A01_Height,
        GC9A01_Width * 2,
        CTX_FORMAT_RGB565_BYTESWAPPED
    );
	
	// rotate by 180 deg and translate x and y by 120 px to have (0,0) at the center of the screen
	ctx_apply_transform(the_ctx,-1,0,120,0,-1,120,0,0,1);
}

void display_update(){
    if(scope_active) return;
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
    if(!scope_active) return;
    scope_write_to_framebuffer(&ScreenBuff);
    GC9A01_Update();
}

//static void display_task(void* arg) {
static void display_task(TimerHandle_t dummy) {
    display_draw_scope();
}

void display_init() { 
	_display_init(); 
	display_ctx_init();
}

void display_scope_start(){
    scope_active = 1;
    vTaskDelay(pdMS_TO_TICKS(100)); //hack: wait until last display update has stopped
    if(xTimerStart(display_animation_timer, pdMS_TO_TICKS(100)) != pdPASS)
    {   // timer startup has failed
        scope_active = 0;
    }
}

void display_scope_stop(){
    if(!scope_active) return; //nothing to do
    if(xTimerStop(display_animation_timer, pdMS_TO_TICKS(1000)) != pdPASS)
    {
        // not sure how to handle this, any ideas? just repeat query? throw error?
    }
    scope_active = 0;
}
