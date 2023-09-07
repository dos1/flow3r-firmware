#pragma once

#include "freertos/FreeRTOS.h"

// clang-format off
#include "ctx_config.h"
#include "ctx.h"
// clang-format on

typedef enum {
    st3m_gfx_default = 0,
    // bitmask flag over base bpp to turn on OSD, only 16bpp for now will
    // become available for other bitdepths as grayscale rather than color
    // overlays.
    st3m_gfx_direct_ctx = 128,
    st3m_gfx_osd = 256,
    // shallower pipeline, in the future might mean immediate mode
    st3m_gfx_low_latency = 512,
    st3m_gfx_unset = 1024,
    st3m_gfx_force = 8192,
    st3m_gfx_2x = 2048,
    st3m_gfx_3x = 4096,
    st3m_gfx_4x = 6144,
    // 4 and 8bpp modes use the configured palette, the palette resides
    // in video ram and is lost upon mode change
    st3m_gfx_1bpp = 1,
    st3m_gfx_2bpp = 2,
    st3m_gfx_4bpp = 4,
    // a flag for modes >4bpp requesting that ctx calls are direct, this is
    // slower since micropython cannot run in parallell with rasterization.
    st3m_gfx_8bpp = 8,
    st3m_gfx_8bpp_osd = 8 + st3m_gfx_osd,
    st3m_gfx_rgb332 = 9,
    st3m_gfx_sepia = 10,
    st3m_gfx_cool = 11,
    st3m_gfx_palette = 15,
    st3m_gfx_8bpp_direct_ctx = 8 + st3m_gfx_direct_ctx,
    st3m_gfx_8bpp_low_latency = 8 + st3m_gfx_low_latency,
    st3m_gfx_8bpp_osd_low_latency = 8 + st3m_gfx_osd + st3m_gfx_low_latency,
    // 16bpp modes have the lowest blit overhead - no osd for now
    st3m_gfx_16bpp = 16,
    st3m_gfx_16bpp_osd = 16 + st3m_gfx_osd,
    st3m_gfx_16bpp_low_latency = 16 + st3m_gfx_low_latency,
    st3m_gfx_16bpp_direct_ctx = 16 + st3m_gfx_direct_ctx,
    st3m_gfx_16bpp_direct_ctx_osd = 16 + st3m_gfx_direct_ctx + st3m_gfx_osd,
    // for pixel poking 24bpp might be a little faster than 32bpp
    // for now there is no ctx drawing support in 24bpp mode.
    st3m_gfx_24bpp = 24,
    st3m_gfx_24bpp_osd = 24 + st3m_gfx_osd,
    st3m_gfx_24bpp_direct_ctx = 24 + st3m_gfx_direct_ctx,
    st3m_gfx_24bpp_low_latency = 24 + st3m_gfx_low_latency,
    st3m_gfx_32bpp = 32,
    // 32bpp modes - are faster at doing compositing, for solid text/fills
    // 16bpp is probably faster.
    st3m_gfx_32bpp_osd = 32 + st3m_gfx_osd,
    st3m_gfx_32bpp_low_latency = 32 + st3m_gfx_low_latency,
    st3m_gfx_32bpp_direct_ctx = 32 + st3m_gfx_direct_ctx,
} st3m_gfx_mode;

// sets the system graphics mode, this is the mode you get to
// when calling st3m_gfx_set_mode(st3m_gfx_default);
void st3m_gfx_set_default_mode(st3m_gfx_mode mode);

// sets the current graphics mode
st3m_gfx_mode st3m_gfx_set_mode(st3m_gfx_mode mode);

// gets the current graphics mode
st3m_gfx_mode st3m_gfx_get_mode(void);

// returns a ctx for drawing at the specified mode/target
// should be paired with a st3m_ctx_end_frame
// normal values are st3m_gfx_default and st3m_gfx_osd for base framebuffer
// and overlay drawing context.
Ctx *st3m_gfx_ctx(st3m_gfx_mode mode);

// get the framebuffer associated with graphics mode
// if you ask for st3m_gfx_default you get the current modes fb
// and if you ask for st3m_gfx_osd you get the current modes overlay fb
uint8_t *st3m_gfx_fb(st3m_gfx_mode mode, int *width, int *height, int *stride);

// get the bits per pixel for a given mode
int st3m_gfx_bpp(st3m_gfx_mode mode);

// sets the palette, pal_in is an array with 3 uint8_t's per entry,
// support values for count is 1-256, used only in 4bpp and 8bpp
// graphics modes.
void st3m_gfx_set_palette(uint8_t *pal_in, int count);

// specifies the corners of the clipping rectangle
// for compositing overlay
void st3m_gfx_overlay_clip(int x0, int y0, int x1, int y1);

// returns a running average of fps
float st3m_gfx_fps(void);

// temporary, signature compatible
// with ctx_end_frame()
void st3m_gfx_end_frame(Ctx *ctx);

// Initialize the gfx subsystem of st3m, includng the rasterization and
// crtx/blitter pipeline.
void st3m_gfx_init(void);

// Returns true if we right now cannot accept another frame
uint8_t st3m_gfx_pipe_full(void);

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
