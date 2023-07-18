#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "rec_fatal.h"
#include "st3m_gfx.h"

void rec_fatal(const char *msg) {
    for (;;) {
        st3m_ctx_desc_t *target = st3m_gfx_drawctx_free_get(portMAX_DELAY);

        // Draw background.
        ctx_rgb(target->ctx, 0.29, 0.0, 0.0);
        ctx_rectangle(target->ctx, -120, -120, 240, 240);
        ctx_fill(target->ctx);

        ctx_font_size(target->ctx, 15.0);
        ctx_gray(target->ctx, 0.8);
        ctx_text_align(target->ctx, CTX_TEXT_ALIGN_CENTER);
        ctx_text_baseline(target->ctx, CTX_TEXT_BASELINE_MIDDLE);
        ctx_move_to(target->ctx, 0, 0);
        ctx_text(target->ctx, msg);

        st3m_gfx_drawctx_pipe_put(target);

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
