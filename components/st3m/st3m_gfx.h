#pragma once

#include "freertos/FreeRTOS.h"

// clang-format off
#include "ctx_config.h"
#include "ctx.h"
// clang-format on

Ctx *st3m_overlay_ctx(void);

Ctx *st3m_ctx(TickType_t ticks_to_wait);
void st3m_ctx_end_frame(Ctx *ctx);  // temporary, signature compatible
                                    // with ctx_end_frame()

// Initialize the gfx subsystem of st3m, includng the rasterization and
// crtx/blitter pipeline.
void st3m_gfx_init(void);

// A drawlist ctx descriptor, pointing to a drawlist-backed Ctx.
typedef struct {
    // The numeric ID of this descriptor.
    int num;
    Ctx *ctx;
} st3m_ctx_desc_t;

// Returns true if the rasterizaiton pipeline submission would block.
uint8_t st3m_gfx_drawctx_pipe_full(void);

// Flush any in-flight pipelined work, resetting the free ctx/framebuffer queues
// to their initial state. This should be called if there has been any drawlist
// ctx dropped (ie. drawctx_free_get was called but then drawctx_pipe_put
// wasn't, for exaple if Micropython restarted).
//
// This causes a graphical disturbance and shouldn't be called during normal
// operation.
void st3m_gfx_flush(void);

typedef struct {
    const char *title;
    const char **lines;
} st3m_gfx_textview_t;

void st3m_gfx_show_textview(st3m_gfx_textview_t *tv);

// Display some text as a splash message. This should be used early on in the
// badge boot process to provide feedback on the rest of the software stack
// coming up
//
// The splash screen is rendered the same way as if submitted by the normal
// drawctx pipe, which means they will get overwritten the moment a proper
// rendering loop starts.
void st3m_gfx_splash(const char *text);

// Draw the flow3r multi-coloured logo at coordinates x,y and with given
// dimension (approx. bounding box size).
void st3m_gfx_flow3r_logo(Ctx *ctx, float x, float y, float dim);

// Set the number of pixels to draw of the overlay screen, more pixels
// adds overhead to every frame, when set to 0 - no composite overhead
void st3m_gfx_set_overlay_height(int height);
