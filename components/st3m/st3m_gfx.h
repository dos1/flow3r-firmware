#pragma once

#include "freertos/FreeRTOS.h"

// clang-format off
#include "ctx_config.h"
#include "ctx.h"
// clang-format on

// There are three separate graphics modes that can be set, on application
// exit RGBA8_over_RGB565_BYTESWAPPED should be restored.
//
// The two other modes cause a scan-out of without compositing/clipping
typedef enum {
    st3m_gfx_default = 0,
    st3m_gfx_osd = 1,
    st3m_gfx_4bpp = 4,

    st3m_gfx_8bpp = 8,
    st3m_gfx_8bpp_osd,

    st3m_gfx_16bpp = 16,
    st3m_gfx_16bpp_osd,

    st3m_gfx_24bpp = 24,

    st3m_gfx_32bpp = 32,
    st3m_gfx_32bpp_osd
} st3m_gfx_mode;

// sets the current graphics mode
void st3m_gfx_set_mode(st3m_gfx_mode mode);

st3m_gfx_mode st3m_gfx_get_mode(void);

uint8_t *st3m_gfx_fb(st3m_gfx_mode mode);

void st3m_gfx_set_palette(uint8_t *pal_in, int count);

// specifies the corners of the clipping rectangle
// for compositing overlay
void st3m_gfx_overlay_clip(int x0, int y0, int x1, int y1);

// returns a running average of fps
float st3m_gfx_fps(void);

// returns a ctx for drawing at the specified mode/target
Ctx *st3m_ctx(st3m_gfx_mode mode);

void st3m_ctx_end_frame(Ctx *ctx);  // temporary, signature compatible
                                    // with ctx_end_frame()

// Initialize the gfx subsystem of st3m, includng the rasterization and
// crtx/blitter pipeline.
void st3m_gfx_init(void);

// Returns true if the rasterizaiton pipeline submission would block.
uint8_t st3m_gfx_drawctx_pipe_full(void);

// Flush any in-flight pipelined work, resetting the free ctx/framebuffer queues
// to their initial state. This should be called if there has been any drawlist
// ctx dropped (ie. drawctx_free_get was called but then drawctx_pipe_put
// wasn't, for exaple if Micropython restarted).
//
// This causes a graphical disturbance and shouldn't be called during normal
// operation. wait_ms is waited for drawlits to clear.
void st3m_gfx_flush(int wait_ms);

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
