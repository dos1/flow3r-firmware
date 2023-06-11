#include "st3m_gfx.h"

#include <string.h>

#include "esp_system.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "flow3r_bsp.h"

// Actual framebuffers, efficiently accessible via DMA.
static DMA_ATTR uint16_t framebuffers[ST3M_GFX_NBUFFERS][FLOW3R_BSP_DISPLAY_WIDTH * FLOW3R_BSP_DISPLAY_HEIGHT];

static st3m_framebuffer_desc_t framebuffer_descs[ST3M_GFX_NBUFFERS];

// Queue of free framebuffer descriptors, written into by crtc once rendered,
// read from by rasterizer when new frame starts.
static QueueHandle_t framebuffer_freeq = NULL;

// Queue of framebuffer descriptors to blit out.
static QueueHandle_t framebuffer_blitq = NULL;

static void st3m_gfx_crtc_task(void *_arg) {
	(void)_arg;

	while (true) {
		int descno;
		xQueueReceive(framebuffer_blitq, &descno, portMAX_DELAY);

    	flow3r_bsp_display_send_fb(framebuffer_descs[descno].buffer);

		xQueueSend(framebuffer_freeq, &descno, portMAX_DELAY);
	}
}

void st3m_gfx_init(void) {
	// Make sure we're not being re-initialized.
	assert(framebuffer_freeq == NULL);

    flow3r_bsp_display_init();

	// Create framebuffer queues.
	framebuffer_freeq = xQueueCreate(2, sizeof(int));
	assert(framebuffer_freeq != NULL);
	framebuffer_blitq = xQueueCreate(2, sizeof(int));
	assert(framebuffer_blitq != NULL);

	// Zero out framebuffers and set up descriptors.
	for (int i = 0; i < ST3M_GFX_NBUFFERS; i++) {
		memset(&framebuffers[i], 0, FLOW3R_BSP_DISPLAY_WIDTH * FLOW3R_BSP_DISPLAY_HEIGHT * 2);
		st3m_framebuffer_desc_t *desc = &framebuffer_descs[i];
		desc->num = i;
		desc->buffer = framebuffers[i];

		// Push descriptor to freeq.
		BaseType_t res = xQueueSend(framebuffer_freeq, &i, 0);
		assert(res == pdTRUE);
	}

	// Start crtc.
	BaseType_t res = xTaskCreate(st3m_gfx_crtc_task, "crtc", 2048, NULL, configMAX_PRIORITIES - 2, NULL);
	assert(res == pdPASS);
}

st3m_framebuffer_desc_t *st3m_gfx_framebuffer_get(TickType_t ticks_to_wait) {
	int descno;
	BaseType_t res = xQueueReceive(framebuffer_freeq, &descno, ticks_to_wait);
	if (res != pdTRUE) {
		return NULL;
	}
	return &framebuffer_descs[descno];
}

void st3m_gfx_framebuffer_queue(st3m_framebuffer_desc_t *desc) {
	xQueueSend(framebuffer_blitq, &desc->num, portMAX_DELAY);
}