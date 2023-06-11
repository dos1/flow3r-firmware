#include "badge23/display.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#include "../../usermodule/uctx/uctx/ctx.h"

#include "flow3r_bsp.h"

static Ctx *the_ctx = NULL;
static volatile bool initialized = false;
static DMA_ATTR uint16_t ctx_framebuffer[FLOW3R_BSP_DISPLAY_WIDTH * FLOW3R_BSP_DISPLAY_HEIGHT];

void display_ctx_init() {
    the_ctx = ctx_new_for_framebuffer(
        ctx_framebuffer,
        FLOW3R_BSP_DISPLAY_WIDTH,
        FLOW3R_BSP_DISPLAY_HEIGHT,
        FLOW3R_BSP_DISPLAY_WIDTH * 2,
        CTX_FORMAT_RGB565_BYTESWAPPED
    );

	int32_t offset_x = FLOW3R_BSP_DISPLAY_WIDTH / 2;
	int32_t offset_y = FLOW3R_BSP_DISPLAY_HEIGHT / 2;
    // rotate by 180 deg and translate x and y by 120 px to have (0,0) at the center of the screen
    ctx_apply_transform(the_ctx,-1,0,offset_x,0,-1,offset_y,0,0,1);
}

static void display_loading_splash(void) {
    ctx_rgb(the_ctx, 0.157, 0.129, 0.167);
    ctx_rectangle(the_ctx, -120, -120, 240, 240);
    ctx_fill(the_ctx);
    
    ctx_move_to(the_ctx, 0, 0);
    ctx_rgb(the_ctx, 0.9, 0.9, 0.9);
    ctx_text_align(the_ctx, CTX_TEXT_ALIGN_CENTER);
    ctx_text_baseline(the_ctx, CTX_TEXT_BASELINE_ALPHABETIC);
    ctx_text(the_ctx, "Loading...");

    flow3r_bsp_display_send_fb(ctx_framebuffer);
}

void display_init() {
    flow3r_bsp_display_init();
    display_ctx_init();
    display_loading_splash();
    // Delay turning on backlight, otherwise we get a flash of some old
    // buffer..? Is the display RAMWR not synchronous..?
    vTaskDelay(100 / portTICK_PERIOD_MS);
    flow3r_bsp_display_set_backlight(100);

    memset(ctx_framebuffer, 0, FLOW3R_BSP_DISPLAY_WIDTH * FLOW3R_BSP_DISPLAY_HEIGHT * 2);

    initialized = true;
}

void display_update(){
    if (!initialized) {
        return;
    }
    flow3r_bsp_display_send_fb(ctx_framebuffer);
}

void display_set_backlight(uint8_t percent) {
    if (!initialized) {
        return;
    }
    flow3r_bsp_display_set_backlight(percent);
}

Ctx *display_global_ctx(void) {
    if (!initialized) {
        return NULL;
    }
    return the_ctx;
}