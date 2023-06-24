#pragma once

#include "flow3r_bsp_i2c.h"

#include <stdint.h>

// Initialize badge display. An error will be reported if the initialization
// failed.
//
// Must be called exactly once from a task and cannot be called cocurrently with
// any other flow3r_bsp_display_* functions.
//
// Side effects: initializes singleton flow3r display object. All other
// flow3r_bsp_display_* functions operate on same object.
void flow3r_bsp_display_init(void);

// Send a full framebuffer of 240x240 16bpp pixels to the display. No-op if
// display hasn't been succesfully initialized.
//
// Transfer will be performed using DMA/interrupts and will block the calling
// FreeRTOS task until finished.
//
// This must not be called if another transfer is alraedy being performed. The
// user code should sequence access and make sure not more than one transfer is
// performed simultaneously.
void flow3r_bsp_display_send_fb(uint16_t *fb_data);

// Set display backlight, as integer percent value (from 0 to 100, clamped).
// No-op if display hasn't been succesfully initialized.
void flow3r_bsp_display_set_backlight(uint8_t percent);

// Currently same on all generations. Might change on future revisions.
#define FLOW3R_BSP_DISPLAY_WIDTH 240
#define FLOW3R_BSP_DISPLAY_HEIGHT 240

// Badge hardware generation name, human-readable.
const char *flow3r_bsp_hw_name;
