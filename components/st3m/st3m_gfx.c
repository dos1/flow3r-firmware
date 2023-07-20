#include "st3m_gfx.h"

#include <string.h>

#include "esp_system.h"
#include "esp_log.h"
#include "esp_task.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "ctx_config.h"
#include "ctx.h"

#include "flow3r_bsp.h"
#include "st3m_counter.h"

static const char *TAG = "st3m-gfx";

// Framebuffer descriptors, containing framebuffer and ctx for each of the
// framebuffers.
//
// These live in SPIRAM, as we don't have enough space in SRAM/IRAM.
EXT_RAM_BSS_ATTR static st3m_framebuffer_desc_t framebuffer_descs[ST3M_GFX_NBUFFERS];

static st3m_ctx_desc_t dctx_descs[ST3M_GFX_NCTX];

// Queue of free framebuffer descriptors, written into by crtc once rendered,
// read from by rasterizer when new frame starts.
static QueueHandle_t framebuffer_freeq = NULL;
// Queue of framebuffer descriptors to blit out.
static QueueHandle_t framebuffer_blitq = NULL;

static st3m_counter_rate_t blit_rate;
static st3m_counter_timer_t blit_read_time;
static st3m_counter_timer_t blit_work_time;
static st3m_counter_timer_t blit_write_time;

static QueueHandle_t dctx_freeq = NULL;
static QueueHandle_t dctx_rastq = NULL;

static st3m_counter_rate_t rast_rate;
static st3m_counter_timer_t rast_read_fb_time;
static st3m_counter_timer_t rast_read_dctx_time;
static st3m_counter_timer_t rast_work_time;
static st3m_counter_timer_t rast_write_time;

static TaskHandle_t crtc_task;
static TaskHandle_t rast_task;

// Attempt to receive from a queue forever, but log an error if it takes longer
// than two seconds to get something.
static void xQueueReceiveNotifyStarved(QueueHandle_t q, void *dst, const char *desc) {
    uint8_t starved = 0;
    for (;;) {
        if (xQueueReceive(q, dst, pdMS_TO_TICKS(2000)) == pdTRUE) {
            return;
        }
        if (!starved) {
            ESP_LOGE(TAG, "%s", desc);
            starved = 1;
        }
    }
}

static void st3m_gfx_crtc_task(void *_arg) {
    (void)_arg;

    while (true) {
        int descno;

        int64_t start = esp_timer_get_time();
        xQueueReceiveNotifyStarved(framebuffer_blitq, &descno, "crtc task starved!");
        int64_t end = esp_timer_get_time();
        st3m_counter_timer_sample(&blit_read_time, end-start);

        start = esp_timer_get_time();
        flow3r_bsp_display_send_fb(framebuffer_descs[descno].buffer);
        end = esp_timer_get_time();
        st3m_counter_timer_sample(&blit_work_time, end-start);

        start = esp_timer_get_time();
        xQueueSend(framebuffer_freeq, &descno, portMAX_DELAY);
        end = esp_timer_get_time();
        st3m_counter_timer_sample(&blit_write_time, end-start);

        st3m_counter_rate_sample(&blit_rate);

        if (st3m_counter_rate_report(&blit_rate, 1)) {
            float rate = 1000000.0 / st3m_counter_rate_average(&blit_rate);
            float read = st3m_counter_timer_average(&blit_read_time) / 1000.0;
            float work = st3m_counter_timer_average(&blit_work_time) / 1000.0;
            float write = st3m_counter_timer_average(&blit_write_time) / 1000.0;
            // Mark variables as used even if debug is disabled.
            (void)rate; (void)read; (void)work; (void)write;
            ESP_LOGD(TAG, "blitting: %.3f/sec, read %.3fms, work %.3fms, write %.3fms", rate, read, work, write);
        }
    }
}

static void st3m_gfx_rast_task(void *_arg) {
    (void)_arg;

    while (true) {
        int fb_descno, dctx_descno;
        int64_t start = esp_timer_get_time();
        xQueueReceiveNotifyStarved(framebuffer_freeq, &fb_descno, "rast task starved (freeq)!");
        st3m_framebuffer_desc_t *fb = &framebuffer_descs[fb_descno];
        int64_t end = esp_timer_get_time();
        st3m_counter_timer_sample(&rast_read_fb_time, end-start);

        start = esp_timer_get_time();
        xQueueReceiveNotifyStarved(dctx_rastq, &dctx_descno, "rast task starved (dctx)!");
        st3m_ctx_desc_t *draw = &dctx_descs[dctx_descno];
        end = esp_timer_get_time();
        st3m_counter_timer_sample(&rast_read_dctx_time, end-start);

        // Render drawctx into fbctx.
        start = esp_timer_get_time();
        ctx_render_ctx(draw->ctx, fb->ctx);
        ctx_drawlist_clear(draw->ctx);
        end = esp_timer_get_time();
        st3m_counter_timer_sample(&rast_work_time, end-start);

        start = esp_timer_get_time();
        xQueueSend(dctx_freeq, &dctx_descno, portMAX_DELAY);
        xQueueSend(framebuffer_blitq, &fb_descno, portMAX_DELAY);
        end = esp_timer_get_time();
        st3m_counter_timer_sample(&rast_write_time, end-start);

        st3m_counter_rate_sample(&rast_rate);


        if (st3m_counter_rate_report(&rast_rate, 1)) {
            float rate = 1000000.0 / st3m_counter_rate_average(&rast_rate);
            float read_fb = st3m_counter_timer_average(&rast_read_fb_time) / 1000.0;
            float read_dctx = st3m_counter_timer_average(&rast_read_dctx_time) / 1000.0;
            float work = st3m_counter_timer_average(&rast_work_time) / 1000.0;
            float write = st3m_counter_timer_average(&rast_write_time) / 1000.0;
            // Mark variables as used even if debug is disabled.
            (void)rate; (void)read_fb; (void)read_dctx; (void)work; (void)write;
            ESP_LOGD(TAG, "rasterization: %.3f/sec, read fb %.3fms, read dctx %.3fms, work %.3fms, write %.3fms", rate, read_fb, read_dctx, work, write);
        }
    }
}

void st3m_gfx_splash(const char *text) {
    const char *lines[] = {
        text, NULL,
    };
    st3m_gfx_textview_t tv = {
        .title = NULL,
        .lines = lines,
    };
    st3m_gfx_show_textview(&tv);
}

void st3m_gfx_show_textview(st3m_gfx_textview_t *tv) {
    if (tv == NULL) {
        return;
    }

    st3m_ctx_desc_t *target = st3m_gfx_drawctx_free_get(portMAX_DELAY);

	// Draw background.
    ctx_rgb(target->ctx, 0, 0, 0);
    ctx_rectangle(target->ctx, -120, -120, 240, 240);
    ctx_fill(target->ctx);

    int y = -30;

	ctx_gray(target->ctx, 1.0);
    ctx_text_align(target->ctx, CTX_TEXT_ALIGN_CENTER);
    ctx_text_baseline(target->ctx, CTX_TEXT_BASELINE_MIDDLE);
    ctx_font_size(target->ctx, 20.0);

	// Draw title, if any.
    if (tv->title != NULL) {
        ctx_move_to(target->ctx, 0, y);
        ctx_text(target->ctx, tv->title);
        y += 20;
    }

    ctx_font_size(target->ctx, 15.0);
	ctx_gray(target->ctx, 0.8);

    // Draw messages.
    const char **lines = tv->lines;
    if (lines != NULL) {
        while (*lines != NULL) {
            const char *text = *lines++;
            ctx_move_to(target->ctx, 0, y);
            ctx_text(target->ctx, text);
            y += 15;
        }
    }

    st3m_gfx_drawctx_pipe_put(target);
}

void st3m_gfx_init(void) {
    // Make sure we're not being re-initialized.
    assert(framebuffer_freeq == NULL);

    st3m_counter_rate_init(&blit_rate);
    st3m_counter_timer_init(&blit_read_time);
    st3m_counter_timer_init(&blit_work_time);
    st3m_counter_timer_init(&blit_write_time);

    st3m_counter_rate_init(&rast_rate);
    st3m_counter_timer_init(&rast_read_fb_time);
    st3m_counter_timer_init(&rast_read_dctx_time);
    st3m_counter_timer_init(&rast_work_time);
    st3m_counter_timer_init(&rast_write_time);

    flow3r_bsp_display_init();

    // Create framebuffer queues.
    framebuffer_freeq = xQueueCreate(ST3M_GFX_NBUFFERS+1, sizeof(int));
    assert(framebuffer_freeq != NULL);
    framebuffer_blitq = xQueueCreate(ST3M_GFX_NBUFFERS+1, sizeof(int));
    assert(framebuffer_blitq != NULL);

    // Create drawlist ctx queues.
    dctx_freeq = xQueueCreate(ST3M_GFX_NCTX+1, sizeof(int));
    assert(dctx_freeq != NULL);
    dctx_rastq = xQueueCreate(ST3M_GFX_NCTX+1, sizeof(int));
    assert(dctx_rastq != NULL);


    for (int i = 0; i < ST3M_GFX_NBUFFERS; i++) {
        // Setup framebuffer descriptor.
        st3m_framebuffer_desc_t *fb_desc = &framebuffer_descs[i];
        fb_desc->num = i;
        fb_desc->ctx = ctx_new_for_framebuffer(
            fb_desc->buffer,
            FLOW3R_BSP_DISPLAY_WIDTH,
            FLOW3R_BSP_DISPLAY_HEIGHT,
            FLOW3R_BSP_DISPLAY_WIDTH * 2,
            CTX_FORMAT_RGB565_BYTESWAPPED
        );
        assert(fb_desc->ctx != NULL);
        // Rotate by 180 deg and translate x and y by 120 px to have (0,0) at the center of the screen
        int32_t offset_x = FLOW3R_BSP_DISPLAY_WIDTH / 2;
        int32_t offset_y = FLOW3R_BSP_DISPLAY_HEIGHT / 2;
        ctx_apply_transform(fb_desc->ctx, -1, 0, offset_x, 0, -1, offset_y, 0, 0, 1);

        // Push descriptor to freeq.
        BaseType_t res = xQueueSend(framebuffer_freeq, &i, 0);
        assert(res == pdTRUE);
    }

    for (int i = 0; i < ST3M_GFX_NCTX; i++) {
        // Setup dctx descriptor.
        st3m_ctx_desc_t *dctx_desc = &dctx_descs[i];
        dctx_desc->num = i;
        dctx_desc->ctx = ctx_new_drawlist(FLOW3R_BSP_DISPLAY_WIDTH, FLOW3R_BSP_DISPLAY_HEIGHT);
        assert(dctx_desc->ctx != NULL);

        // Push descriptor to freeq.
        BaseType_t res = xQueueSend(dctx_freeq, &i, 0);
        assert(res == pdTRUE);
    }

    // Start crtc.
    BaseType_t res = xTaskCreate(st3m_gfx_crtc_task, "crtc", 4096, NULL, ESP_TASK_PRIO_MIN+3, &crtc_task);
    assert(res == pdPASS);

    // Start rast.
    res = xTaskCreate(st3m_gfx_rast_task, "rast", 8192, NULL, ESP_TASK_PRIO_MIN+1, &rast_task);
    assert(res == pdPASS);
}

st3m_ctx_desc_t *st3m_gfx_drawctx_free_get(TickType_t ticks_to_wait) {
    int descno;
    BaseType_t res = xQueueReceive(dctx_freeq, &descno, ticks_to_wait);
    if (res != pdTRUE) {
        return NULL;
    }
    return &dctx_descs[descno];
}

void st3m_gfx_drawctx_pipe_put(st3m_ctx_desc_t *desc) {
    xQueueSend(dctx_rastq, &desc->num, portMAX_DELAY);
}

uint8_t st3m_gfx_drawctx_pipe_full(void) {
    return uxQueueSpacesAvailable(dctx_rastq) == 0;
}

void st3m_gfx_flush(void) {
    ESP_LOGW(TAG, "Pipeline flush/reset requested...");
    // Drain all workqs and freeqs.
    xQueueReset(dctx_freeq);
    xQueueReset(dctx_rastq);
    xQueueReset(framebuffer_freeq);
    xQueueReset(framebuffer_blitq);

    // Delay, making sure pipeline tasks have returned all used descriptors. One
    // second is enough to make sure we've processed everything.
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // And drain again.
    xQueueReset(framebuffer_freeq);
    xQueueReset(dctx_freeq);

    // Now, re-submit all descriptors to freeqs.
    for (int i = 0; i < ST3M_GFX_NBUFFERS; i++) {
        BaseType_t res = xQueueSend(framebuffer_freeq, &i, 0);
        assert(res == pdTRUE);
    }
    for (int i = 0; i < ST3M_GFX_NCTX; i++) {
        BaseType_t res = xQueueSend(dctx_freeq, &i, 0);
        assert(res == pdTRUE);
    }
    ESP_LOGW(TAG, "Pipeline flush/reset done.");
}
