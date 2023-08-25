#pragma once

#include "freertos/FreeRTOS.h"

// clang-format off
#include "ctx_config.h"
#include "ctx.h"
// clang-format on

// Each buffer  takes ~116kB SPIRAM. While one framebuffer is being blitted, the
// other one is being written to by the rasterizer.
#define ST3M_GFX_NBUFFERS 1
// More ctx drawlists than buffers so that micropython doesn't get starved when
// pipeline runs in lockstep.
#define ST3M_GFX_NCTX 1

// A framebuffer descriptor, pointing at a framebuffer.
typedef struct {
    // The numeric ID of this descriptor.
    int num;
    // SPIRAM buffer.
    uint16_t buffer[240 * 240];
    Ctx *ctx;
} st3m_framebuffer_desc_t;

// Initialize the gfx subsystem of st3m, includng the rasterization and
// crtx/blitter pipeline.
void st3m_gfx_init(void);

// A drawlist ctx descriptor, pointing to a drawlist-backed Ctx.
typedef struct {
    // The numeric ID of this descriptor.
    int num;
    Ctx *ctx;
} st3m_ctx_desc_t;

// Get a free drawlist ctx to draw into.
//
// ticks_to_wait can be used to limit the time to wait for a free ctx
// descriptor, or portDELAY_MAX can be specified to wait forever. If the timeout
// expires, NULL will be returned.
st3m_ctx_desc_t *st3m_gfx_drawctx_free_get(TickType_t ticks_to_wait);

// Submit a filled ctx descriptor to the rasterization pipeline.
void st3m_gfx_drawctx_pipe_put(st3m_ctx_desc_t *desc);

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
