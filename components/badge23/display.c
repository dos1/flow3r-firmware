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

#include "esp_system.h"

#include "../../usermodule/uctx/uctx/ctx.h"

volatile Ctx *the_ctx = NULL;


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

void display_init() {
    GC9A01_Init();
    GC9A01_Update();
    display_ctx_init();
}

