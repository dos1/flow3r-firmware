#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "rec_fatal.h"
#include "st3m_gfx.h"

void rec_fatal(const char *msg) {
    for (;;) {
        Ctx *ctx = st3m_ctx(st3m_gfx_default);

        // Draw background.
        ctx_rgb(ctx, 0.29, 0.0, 0.0);
        ctx_rectangle(ctx, -120, -120, 240, 240);
        ctx_fill(ctx);

        ctx_font_size(ctx, 15.0);
        ctx_gray(ctx, 0.8);
        ctx_text_align(ctx, CTX_TEXT_ALIGN_CENTER);
        ctx_text_baseline(ctx, CTX_TEXT_BASELINE_MIDDLE);
        ctx_move_to(ctx, 0, 0);
        ctx_text(ctx, msg);

        st3m_ctx_end_frame(ctx);

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
