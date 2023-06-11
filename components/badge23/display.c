#include "badge23/display.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"


#include "../../usermodule/uctx/uctx/ctx.h"

#include "flow3r_bsp.h"
#include "st3m_gfx.h"


typedef struct {
    Ctx *ctx;
    st3m_framebuffer_desc_t *desc;
} ctx_target_t;

static void new_ctx_target(ctx_target_t *target, st3m_framebuffer_desc_t *desc) {
    Ctx *ctx = ctx_new_for_framebuffer(
        desc->buffer,
        FLOW3R_BSP_DISPLAY_WIDTH,
        FLOW3R_BSP_DISPLAY_HEIGHT,
        FLOW3R_BSP_DISPLAY_WIDTH * 2,
        CTX_FORMAT_RGB565_BYTESWAPPED
    );
    assert(ctx != NULL);

	int32_t offset_x = FLOW3R_BSP_DISPLAY_WIDTH / 2;
	int32_t offset_y = FLOW3R_BSP_DISPLAY_HEIGHT / 2;
    // rotate by 180 deg and translate x and y by 120 px to have (0,0) at the center of the screen
    ctx_apply_transform(ctx,-1,0,offset_x,0,-1,offset_y,0,0,1);

    target->ctx = ctx;
    target->desc = desc;
}

static ctx_target_t targets[ST3M_GFX_NBUFFERS];

static void display_loading_splash(ctx_target_t *target) {
    ctx_rgb(target->ctx, 0.157, 0.129, 0.167);
    ctx_rectangle(target->ctx, -120, -120, 240, 240);
    ctx_fill(target->ctx);
    
    ctx_move_to(target->ctx, 0, 0);
    ctx_rgb(target->ctx, 0.9, 0.9, 0.9);
    ctx_text_align(target->ctx, CTX_TEXT_ALIGN_CENTER);
    ctx_text_baseline(target->ctx, CTX_TEXT_BASELINE_ALPHABETIC);
    ctx_text(target->ctx, "Loading...");
}

static ctx_target_t *next_target(void) {
    st3m_framebuffer_desc_t *desc = st3m_gfx_framebuffer_get(portMAX_DELAY);
    if (targets[desc->num].ctx == NULL) {
        new_ctx_target(&targets[desc->num], desc);
    }

    return &targets[desc->num];
}

static bool initialized = false;
static ctx_target_t *sync_target = NULL;

void display_init() {
    // HACK: needed until we have new async gfx api.
    assert(ST3M_GFX_NBUFFERS == 1);

    st3m_gfx_init();
    for (int i = 0; i < ST3M_GFX_NBUFFERS; i++) {
        targets[i].ctx = NULL;
        targets[i].desc = NULL;
    }

	ctx_target_t *tgt = next_target();
    display_loading_splash(tgt);
    st3m_gfx_framebuffer_queue(tgt->desc);

	sync_target = next_target();
    flow3r_bsp_display_set_backlight(100);
    initialized = true;
}


void display_update(){
    if (!initialized) {
        return;
    }

    st3m_gfx_framebuffer_queue(sync_target->desc);
    ctx_target_t *tgt = next_target();
    assert(tgt == sync_target);
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
    return sync_target->ctx;
}