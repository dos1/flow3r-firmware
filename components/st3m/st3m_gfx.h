#pragma once

#include "freertos/FreeRTOS.h"

// Each buffer  takes ~116kB SRAM. While one framebuffer is being blitted, the
// other one is being written to by the rasterizer.
#define ST3M_GFX_NBUFFERS 1

// A framebuffer descriptor, pointing at a framebuffer.
typedef struct {
	int num;
	uint16_t *buffer;
} st3m_framebuffer_desc_t;

void st3m_gfx_init(void);

st3m_framebuffer_desc_t *st3m_gfx_framebuffer_get(TickType_t ticks_to_wait);

void st3m_gfx_framebuffer_queue(st3m_framebuffer_desc_t *desc);