#include "st3m_gfx.h"

#include <string.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_task.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// clang-format off
#include "ctx_config.h"
#include "ctx.h"
// clang-format on

#include "flow3r_bsp.h"
#include "st3m_counter.h"

static const char *TAG = "st3m-gfx";

// Framebuffer descriptors, containing framebuffer and ctx for each of the
// framebuffers.
//
// These live in SPIRAM, as we don't have enough space in SRAM/IRAM.
EXT_RAM_BSS_ATTR static st3m_framebuffer_desc_t
    framebuffer_descs[ST3M_GFX_NBUFFERS];

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
static void xQueueReceiveNotifyStarved(QueueHandle_t q, void *dst,
                                       const char *desc) {
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
        xQueueReceiveNotifyStarved(framebuffer_blitq, &descno,
                                   "crtc task starved!");
        int64_t end = esp_timer_get_time();
        st3m_counter_timer_sample(&blit_read_time, end - start);

        start = esp_timer_get_time();
        flow3r_bsp_display_send_fb(framebuffer_descs[descno].buffer);
        end = esp_timer_get_time();
        st3m_counter_timer_sample(&blit_work_time, end - start);

        start = esp_timer_get_time();
        xQueueSend(framebuffer_freeq, &descno, portMAX_DELAY);
        end = esp_timer_get_time();
        st3m_counter_timer_sample(&blit_write_time, end - start);

        st3m_counter_rate_sample(&blit_rate);

        if (st3m_counter_rate_report(&blit_rate, 1)) {
            float rate = 1000000.0 / st3m_counter_rate_average(&blit_rate);
            float read = st3m_counter_timer_average(&blit_read_time) / 1000.0;
            float work = st3m_counter_timer_average(&blit_work_time) / 1000.0;
            float write = st3m_counter_timer_average(&blit_write_time) / 1000.0;
            // Mark variables as used even if debug is disabled.
            (void)rate;
            (void)read;
            (void)work;
            (void)write;
            ESP_LOGD(
                TAG,
                "blitting: %.3f/sec, read %.3fms, work %.3fms, write %.3fms",
                rate, read, work, write);
        }
    }
}

static void st3m_gfx_rast_task(void *_arg) {
    (void)_arg;

    while (true) {
        int fb_descno, dctx_descno;
        int64_t start = esp_timer_get_time();
        xQueueReceiveNotifyStarved(framebuffer_freeq, &fb_descno,
                                   "rast task starved (freeq)!");
        st3m_framebuffer_desc_t *fb = &framebuffer_descs[fb_descno];
        int64_t end = esp_timer_get_time();
        st3m_counter_timer_sample(&rast_read_fb_time, end - start);

        start = esp_timer_get_time();
        xQueueReceiveNotifyStarved(dctx_rastq, &dctx_descno,
                                   "rast task starved (dctx)!");
        st3m_ctx_desc_t *draw = &dctx_descs[dctx_descno];
        end = esp_timer_get_time();
        st3m_counter_timer_sample(&rast_read_dctx_time, end - start);

        // Render drawctx into fbctx.
        start = esp_timer_get_time();
        ctx_render_ctx(draw->ctx, fb->ctx);
        ctx_drawlist_clear(draw->ctx);
        end = esp_timer_get_time();
        st3m_counter_timer_sample(&rast_work_time, end - start);

        start = esp_timer_get_time();
        xQueueSend(dctx_freeq, &dctx_descno, portMAX_DELAY);
        xQueueSend(framebuffer_blitq, &fb_descno, portMAX_DELAY);
        end = esp_timer_get_time();
        st3m_counter_timer_sample(&rast_write_time, end - start);

        st3m_counter_rate_sample(&rast_rate);

        if (st3m_counter_rate_report(&rast_rate, 1)) {
            float rate = 1000000.0 / st3m_counter_rate_average(&rast_rate);
            float read_fb =
                st3m_counter_timer_average(&rast_read_fb_time) / 1000.0;
            float read_dctx =
                st3m_counter_timer_average(&rast_read_dctx_time) / 1000.0;
            float work = st3m_counter_timer_average(&rast_work_time) / 1000.0;
            float write = st3m_counter_timer_average(&rast_write_time) / 1000.0;
            // Mark variables as used even if debug is disabled.
            (void)rate;
            (void)read_fb;
            (void)read_dctx;
            (void)work;
            (void)write;
            ESP_LOGD(TAG,
                     "rasterization: %.3f/sec, read fb %.3fms, read dctx "
                     "%.3fms, work %.3fms, write %.3fms",
                     rate, read_fb, read_dctx, work, write);
        }
    }
}

void st3m_gfx_flow3r_logo(Ctx *ctx, float x, float y, float dim) {
    ctx_save(ctx);
    ctx_translate(ctx, x, y);
    ctx_scale(ctx, dim, dim);
    ctx_translate(ctx, -0.5, -0.5);
    ctx_linear_gradient(ctx, 0.18, 0.5, 0.95, 0.5);
    ctx_gradient_add_stop(ctx, 0.0, 255, 0, 0, 1.0);
    ctx_gradient_add_stop(ctx, 0.2, 255, 255, 0, 1.0);
    ctx_gradient_add_stop(ctx, 0.4, 0, 255, 0, 1.0);
    ctx_gradient_add_stop(ctx, 0.6, 0, 255, 255, 1.0);
    ctx_gradient_add_stop(ctx, 0.8, 0, 0, 255, 1.0);
    ctx_gradient_add_stop(ctx, 1.0, 255, 0, 255, 1.0);

    ctx_save(ctx);
    ctx_scale(ctx, 1 / 30.0, 1 / 30.0);
    ctx_translate(ctx, 0, 10);
    ctx_move_to(ctx, 6.1855469, 0.0);
    ctx_curve_to(ctx, 5.514239, 0.021906716, 4.8521869, 0.2342972, 4.2109375,
                 0.56054688);
    ctx_curve_to(ctx, 2.740939, 1.3605461, 1.9694531, 2.5895329, 1.9394531,
                 4.2695312);
    ctx_curve_to(ctx, 1.9694531, 4.479531, 1.9504688, 4.6496096, 1.9804688,
                 4.8496094);
    ctx_curve_to(ctx, 1.9804688, 5.0896091, 1.890156, 5.2095313, 1.6601562,
                 5.2695312);
    ctx_curve_to(ctx, 1.2601567, 5.3195312, 0.84992144, 5.3709376, 0.41992188,
                 5.4609375);
    ctx_curve_to(ctx, 0.12992217, 5.5309374, -0.07046866, 5.8705471, 0.01953125,
                 6.0605469);
    ctx_curve_to(ctx, 0.10953116, 6.2605467, 0.31984402, 6.2897656, 0.58984375,
                 6.2597656);
    ctx_curve_to(ctx, 0.99984334, 6.2097657, 1.3702347, 6.1991405, 1.7402344,
                 6.1191406);
    ctx_curve_to(ctx, 2.150234, 6.0691407, 2.4703908, 6.1995316, 2.6503906,
                 6.5195312);
    ctx_curve_to(ctx, 2.9503903, 6.9895308, 3.3793755, 7.3792191, 3.859375,
                 7.6992188);
    ctx_curve_to(ctx, 4.3393745, 8.0092184, 4.5301564, 8.469688, 4.6601562,
                 8.9296875);
    ctx_curve_to(ctx, 4.880156, 9.5896868, 4.429765, 10.160234, 3.7597656,
                 10.240234);
    ctx_curve_to(ctx, 2.9497664, 10.340234, 2.3204682, 10.220624, 1.7304688,
                 9.640625);
    ctx_curve_to(ctx, 1.3804691, 9.3006253, 1.0903904, 8.8894527, 0.90039062,
                 8.4394531);
    ctx_curve_to(ctx, 0.80039073, 8.1694534, 0.70031237, 7.9094529, 0.5703125,
                 7.6894531);
    ctx_curve_to(ctx, 0.52031255, 7.5894532, 0.37929677, 7.5398438, 0.27929688,
                 7.5898438);
    ctx_curve_to(ctx, 0.21929693, 7.5998437, 0.0703125, 7.7190626, 0.0703125,
                 7.7890625);
    ctx_curve_to(ctx, 0.15031242, 8.2290621, 0.24062515, 8.6595316, 0.390625,
                 9.0195312);
    ctx_curve_to(ctx, 0.7906246, 9.9995303, 1.3911729, 10.731719, 2.4511719,
                 11.011719);
    ctx_curve_to(ctx, 3.0284683, 11.163189, 3.6164729, 11.365113, 4.3007812,
                 11.269531);
    ctx_curve_to(ctx, 5.11028, 11.002334, 5.5990627, 10.21953, 5.7890625,
                 9.2695312);
    ctx_curve_to(ctx, 5.9690623, 8.499532, 5.430937, 8.0192964, 4.9609375,
                 7.5292969);
    ctx_line_to(ctx, 4.6503906, 7.2890625);
    ctx_curve_to(ctx, 4.3385878, 7.0431923, 3.6466633, 6.7251222, 3.5195312,
                 6.1601562);
    ctx_curve_to(ctx, 3.8895309, 6.0801563, 4.2608597, 6.0002344, 4.6308594,
                 6.00);
    ctx_curve_to(ctx, 5.2408588, 5.9802344, 5.8704694, 6.0292969, 6.4804688,
                 6.0292969);
    ctx_curve_to(ctx, 6.8204684, 6.0592968, 7.1200783, 5.9907029, 7.3300781,
                 5.7207031);
    ctx_curve_to(ctx, 7.3900781, 5.6407032, 7.4296875, 5.4996874, 7.4296875,
                 5.4296875);
    ctx_curve_to(ctx, 7.3796876, 5.2696877, 7.2603905, 5.1098437, 7.1503906,
                 5.0898438);
    ctx_curve_to(ctx, 6.8603909, 4.9998438, 6.5599997, 5, 6.25, 5);
    ctx_curve_to(ctx, 5.3300009, 5.02, 4.4102335, 5.099375, 3.4902344,
                 5.109375);
    ctx_curve_to(ctx, 2.9802349, 5.139375, 2.859375, 4.9894526, 2.859375,
                 4.4394531);
    ctx_curve_to(ctx, 2.889375, 3.2394543, 3.5196884, 2.3306243, 4.4296875,
                 1.640625);
    ctx_curve_to(ctx, 5.0496869, 1.1506255, 5.8491414, 0.97984386, 6.6191406,
                 1.0898438);
    ctx_curve_to(ctx, 7.3791399, 1.1998436, 8.0708597, 1.4898446, 8.3808594,
                 2.3398438);
    ctx_curve_to(ctx, 8.4408593, 2.5698435, 8.809922, 2.489531, 8.9199219,
                 2.2695312);
    ctx_curve_to(ctx, 9.0899217, 1.9795315, 9.1292186, 1.7008591, 8.9492188,
                 1.3808594);
    ctx_curve_to(ctx, 8.679219, 0.8608599, 8.2397651, 0.58007787, 7.7597656,
                 0.33007812);
    ctx_curve_to(ctx, 7.2347661, 0.080703374, 6.7076753, -0.017038557,
                 6.1855469, 0);
    ctx_fill(ctx);

    ctx_move_to(ctx, 24.390625, 3.0507812);
    ctx_curve_to(ctx, 23.745257, 3.0585757, 23.089296, 3.2523439, 22.419922,
                 3.4492188);
    ctx_curve_to(ctx, 22.349922, 3.4592187, 22.340312, 3.6302345, 22.320312,
                 3.7402344);
    ctx_curve_to(ctx, 22.350312, 4.0102341, 22.539297, 4.1601562, 22.779297,
                 4.1601562);
    ctx_curve_to(ctx, 23.389296, 4.1501563, 24.029141, 4.1008594, 24.619141,
                 4.1308594);
    ctx_curve_to(ctx, 24.85914, 4.1308594, 25.149141, 4.2298439, 25.369141,
                 4.3398438);
    ctx_curve_to(ctx, 25.61914, 4.4798436, 25.799453, 4.7995315, 25.689453,
                 5.0195312);
    ctx_curve_to(ctx, 25.609453, 5.1995311, 25.459531, 5.3904688, 25.269531,
                 5.4804688);
    ctx_curve_to(ctx, 25.009532, 5.5804686, 24.70039, 5.5901563, 24.400391,
                 5.6601562);
    ctx_curve_to(ctx, 24.130391, 5.6901562, 23.860859, 5.7190626, 23.630859,
                 5.7890625);
    ctx_curve_to(ctx, 23.40086, 5.8590624, 23.280547, 6.0109377, 23.310547,
                 6.2109375);
    ctx_curve_to(ctx, 23.370547, 6.3809373, 23.500625, 6.6003907, 23.640625,
                 6.6503906);
    ctx_curve_to(ctx, 23.860625, 6.7603905, 24.070547, 6.8105469, 24.310547,
                 6.8105469);
    ctx_curve_to(ctx, 24.650547, 6.8405468, 25.029454, 6.8198438, 25.439453,
                 6.8398438);
    ctx_curve_to(ctx, 25.779453, 6.8598437, 26.040391, 7.0000785, 26.150391,
                 7.3300781);
    ctx_curve_to(ctx, 26.54039, 8.3000772, 25.899374, 9.4689064, 24.859375,
                 9.6289062);
    ctx_curve_to(ctx, 24.449375, 9.6889062, 23.999609, 9.6199219, 23.599609,
                 9.6699219);
    ctx_curve_to(ctx, 23.39961, 9.6999218, 23.189297, 9.7292969, 23.029297,
                 9.7792969);
    ctx_curve_to(ctx, 22.799297, 9.8392968, 22.640703, 10.200078, 22.720703,
                 10.330078);
    ctx_curve_to(ctx, 22.902298, 10.577502, 23.018785, 10.732769, 23.320312,
                 10.730469);
    ctx_curve_to(ctx, 23.940312, 10.720469, 24.580391, 10.680312, 25.150391,
                 10.570312);
    ctx_curve_to(ctx, 27.220389, 10.170313, 27.830233, 7.6600768, 26.740234,
                 6.3300781);
    ctx_curve_to(ctx, 26.540235, 6.1200783, 26.519141, 5.9208591, 26.619141,
                 5.6308594);
    ctx_curve_to(ctx, 27.005002, 4.7951465, 26.709984, 4.028671, 25.880859,
                 3.4609375);
    ctx_curve_to(ctx, 25.388766, 3.1521857, 24.892578, 3.0447189, 24.390625,
                 3.0507812);
    ctx_fill(ctx);

    ctx_move_to(ctx, 9.2949219, 3.6875);
    ctx_curve_to(ctx, 9.1981593, 3.6903882, 9.0928905, 3.7307813, 9.0097656,
                 3.8007812);
    ctx_curve_to(ctx, 8.7397659, 4.010781, 8.7402344, 4.3091409, 8.7402344,
                 4.6191406);
    ctx_curve_to(ctx, 8.7802343, 6.289139, 8.7898435, 7.9999235, 8.5898438,
                 9.6699219);
    ctx_curve_to(ctx, 8.5998437, 9.9799216, 8.5296094, 10.289844, 8.5996094,
                 10.589844);
    ctx_curve_to(ctx, 8.6596093, 10.819844, 8.789922, 11.049531, 8.9199219,
                 11.269531);
    ctx_curve_to(ctx, 9.0199218, 11.469531, 9.379844, 11.38914, 9.5898438,
                 11.119141);
    ctx_curve_to(ctx, 9.7007251, 10.399672, 9.7475324, 9.7547508, 9.8007812,
                 8.9707031);
    ctx_curve_to(ctx, 10.000781, 7.6107045, 9.8600781, 6.2193736, 9.8300781,
                 4.859375);
    ctx_curve_to(ctx, 9.7900782, 4.5293753, 9.6803124, 4.1991403, 9.5703125,
                 3.8691406);
    ctx_curve_to(ctx, 9.5309375, 3.7397658, 9.4193309, 3.6837866, 9.2949219,
                 3.6875);
    ctx_fill(ctx);
    ctx_move_to(ctx, 30.595703, 5.6269531);
    ctx_curve_to(ctx, 30.542372, 5.6291567, 30.486805, 5.6346368, 30.429688,
                 5.640625);
    ctx_curve_to(ctx, 30.252992, 5.6540603, 29.879843, 5.8795314, 29.589844,
                 6.0195312);
    ctx_curve_to(ctx, 29.329844, 6.1195311, 29.129375, 6.1494529, 28.859375,
                 5.9394531);
    ctx_curve_to(ctx, 28.549375, 5.6994534, 28.159375, 5.8895316, 28.109375,
                 6.2695312);
    ctx_curve_to(ctx, 28.109375, 6.4395311, 28.089375, 6.6200001, 28.109375,
                 6.75);
    ctx_curve_to(ctx, 28.199375, 7.729999, 28.319219, 8.6693759, 28.199219,
                 9.609375);
    ctx_curve_to(ctx, 28.189219, 9.7793748, 28.209766, 9.9200783, 28.259766,
                 10.080078);
    ctx_curve_to(ctx, 28.309766, 10.170078, 28.39, 10.300313, 28.5, 10.320312);
    ctx_curve_to(ctx, 28.54, 10.350312, 28.699766, 10.300703, 28.759766,
                 10.220703);
    ctx_curve_to(ctx, 28.899765, 9.9607034, 29.110625, 9.6999216, 29.140625,
                 9.4199219);
    ctx_curve_to(ctx, 29.260625, 9.0199223, 29.239766, 8.5799215, 29.259766,
                 8.1699219);
    ctx_curve_to(ctx, 29.289766, 7.8199222, 29.400625, 7.6091404, 29.640625,
                 7.3691406);
    ctx_curve_to(ctx, 29.980625, 7.0891409, 30.329532, 6.8696094, 30.769531,
                 6.8496094);
    ctx_curve_to(ctx, 31.009531, 6.8596094, 31.320781, 6.8490624, 31.550781,
                 6.7890625);
    ctx_curve_to(ctx, 31.790781, 6.7190626, 31.899844, 6.5698435, 31.839844,
                 6.3398438);
    ctx_curve_to(ctx, 31.773174, 5.9731724, 31.395669, 5.5939, 30.595703,
                 5.6269531);
    ctx_fill(ctx);
    ctx_move_to(ctx, 21.314453, 6.2050781);
    ctx_curve_to(ctx, 21.242777, 6.2025999, 21.158536, 6.2301133, 21.060547,
                 6.3007812);
    ctx_curve_to(ctx, 20.870547, 6.4607811, 20.759063, 6.6108596, 20.789062,
                 6.8808594);
    ctx_curve_to(ctx, 20.829062, 7.220859, 20.879609, 7.550391, 20.849609,
                 7.9003906);
    ctx_curve_to(ctx, 20.809609, 8.4203901, 20.729141, 8.9091411, 20.619141,
                 9.3691406);
    ctx_curve_to(ctx, 20.599141, 9.4691405, 20.340937, 9.6401563, 20.210938,
                 9.6601562);
    ctx_curve_to(ctx, 20.000938, 9.6901562, 19.779219, 9.5792186, 19.699219,
                 9.4492188);
    ctx_curve_to(ctx, 19.549219, 9.3292189, 19.55, 9.0896873, 19.5, 8.9296875);
    ctx_curve_to(ctx, 19.43, 8.6996877, 19.430781, 8.3899217, 19.300781,
                 8.1699219);
    ctx_curve_to(ctx, 19.210781, 8.039922, 19.027123, 7.8411207, 18.845703,
                 7.859375);
    ctx_curve_to(ctx, 18.685194, 7.8452773, 18.489453, 8.029922, 18.439453,
                 8.1699219);
    ctx_curve_to(ctx, 18.349453, 8.3499217, 18.389609, 8.6200783, 18.349609,
                 8.8300781);
    ctx_curve_to(ctx, 18.289609, 9.1500778, 18.289453, 9.4502347, 18.189453,
                 9.7402344);
    ctx_curve_to(ctx, 18.129453, 9.8102343, 17.969844, 9.9292188, 17.839844,
                 9.9492188);
    ctx_curve_to(ctx, 17.779844, 9.9592187, 17.559766, 9.8499999, 17.509766,
                 9.75);
    ctx_curve_to(ctx, 17.199766, 9.2000006, 17.050781, 8.6002338, 17.050781,
                 7.9902344);
    ctx_curve_to(ctx, 17.050781, 7.6902347, 17.080078, 7.4096091, 17.080078,
                 7.0996094);
    ctx_curve_to(ctx, 17.090078, 6.8596096, 16.93039, 6.670625, 16.650391,
                 6.640625);
    ctx_curve_to(ctx, 16.450391, 6.660625, 16.219453, 6.7903128, 16.189453,
                 7.0703125);
    ctx_curve_to(ctx, 16.069453, 8.0103116, 16.049454, 8.9708603, 16.439453,
                 9.8808594);
    ctx_curve_to(ctx, 16.889453, 10.960858, 17.68086, 11.269921, 18.630859,
                 10.669922);
    ctx_curve_to(ctx, 18.980859, 10.449922, 19.250313, 10.420781, 19.570312,
                 10.550781);
    ctx_curve_to(ctx, 20.300312, 10.870781, 20.840313, 10.559531, 21.320312,
                 10.019531);
    ctx_line_to(ctx, 21.320312, 10.029297);
    ctx_curve_to(ctx, 21.430312, 9.8192971, 21.610625, 9.5905466, 21.640625,
                 9.3105469);
    ctx_curve_to(ctx, 21.870625, 8.3905478, 21.790625, 7.4803116, 21.640625,
                 6.5703125);
    ctx_curve_to(ctx, 21.630335, 6.4783061, 21.529481, 6.2125128, 21.314453,
                 6.2050781);
    ctx_fill(ctx);

    ctx_move_to(ctx, 13.375, 6.5429688);
    ctx_curve_to(ctx, 12.980782, 6.5385938, 12.576718, 6.6278127, 12.199219,
                 6.8203125);
    ctx_curve_to(ctx, 11.591586, 7.2942806, 10.740412, 7.9135672, 10.669922,
                 9.0996094);
    ctx_curve_to(ctx, 10.691491, 10.877171, 12.662612, 11.652379, 14.699219,
                 10.650391);
    ctx_curve_to(ctx, 15.399218, 10.220391, 15.729219, 9.6305461, 15.699219,
                 8.8105469);
    ctx_curve_to(ctx, 15.654219, 7.4080483, 14.557655, 6.5560937, 13.375,
                 6.5429688);
    ctx_close_path(ctx);
    ctx_move_to(ctx, 13.357422, 7.5566406);
    ctx_curve_to(ctx, 13.758239, 7.5527734, 14.152891, 7.7152346, 14.400391,
                 7.9902344);
    ctx_curve_to(ctx, 14.72039, 8.360234, 14.769765, 9.240391, 14.509766,
                 9.6503906);
    ctx_curve_to(ctx, 13.936796, 10.26144, 13.290525, 10.362348, 12.359375,
                 10.230469);
    ctx_curve_to(ctx, 11.899375, 10.120469, 11.610703, 9.7099995, 11.720703,
                 9.25);
    ctx_curve_to(ctx, 11.869196, 8.5683815, 11.925992, 8.1457089, 12.820312,
                 7.6699219);
    ctx_curve_to(ctx, 12.992187, 7.5949219, 13.175232, 7.5583984, 13.357422,
                 7.5566406);
    ctx_fill(ctx);
    ctx_restore(ctx);

    ctx_restore(ctx);
}

void st3m_gfx_splash(const char *text) {
    const char *lines[] = {
        text,
        NULL,
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

    st3m_gfx_flow3r_logo(target->ctx, 0, -30, 150);

    int y = 20;

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
    framebuffer_freeq = xQueueCreate(ST3M_GFX_NBUFFERS + 1, sizeof(int));
    assert(framebuffer_freeq != NULL);
    framebuffer_blitq = xQueueCreate(ST3M_GFX_NBUFFERS + 1, sizeof(int));
    assert(framebuffer_blitq != NULL);

    // Create drawlist ctx queues.
    dctx_freeq = xQueueCreate(ST3M_GFX_NCTX + 1, sizeof(int));
    assert(dctx_freeq != NULL);
    dctx_rastq = xQueueCreate(ST3M_GFX_NCTX + 1, sizeof(int));
    assert(dctx_rastq != NULL);

    for (int i = 0; i < ST3M_GFX_NBUFFERS; i++) {
        // Setup framebuffer descriptor.
        st3m_framebuffer_desc_t *fb_desc = &framebuffer_descs[i];
        fb_desc->num = i;
        fb_desc->ctx = ctx_new_for_framebuffer(
            fb_desc->buffer, FLOW3R_BSP_DISPLAY_WIDTH,
            FLOW3R_BSP_DISPLAY_HEIGHT, FLOW3R_BSP_DISPLAY_WIDTH * 2,
            CTX_FORMAT_RGB565_BYTESWAPPED);
        assert(fb_desc->ctx != NULL);
        // Rotate by 180 deg and translate x and y by 120 px to have (0,0) at
        // the center of the screen
        int32_t offset_x = FLOW3R_BSP_DISPLAY_WIDTH / 2;
        int32_t offset_y = FLOW3R_BSP_DISPLAY_HEIGHT / 2;
        ctx_apply_transform(fb_desc->ctx, -1, 0, offset_x, 0, -1, offset_y, 0,
                            0, 1);

        // Push descriptor to freeq.
        BaseType_t res = xQueueSend(framebuffer_freeq, &i, 0);
        assert(res == pdTRUE);
    }

    for (int i = 0; i < ST3M_GFX_NCTX; i++) {
        // Setup dctx descriptor.
        st3m_ctx_desc_t *dctx_desc = &dctx_descs[i];
        dctx_desc->num = i;
        dctx_desc->ctx = ctx_new_drawlist(FLOW3R_BSP_DISPLAY_WIDTH,
                                          FLOW3R_BSP_DISPLAY_HEIGHT);
        assert(dctx_desc->ctx != NULL);

        // Push descriptor to freeq.
        BaseType_t res = xQueueSend(dctx_freeq, &i, 0);
        assert(res == pdTRUE);
    }

    // Start crtc.
    BaseType_t res = xTaskCreate(st3m_gfx_crtc_task, "crtc", 4096, NULL,
                                 ESP_TASK_PRIO_MIN + 3, &crtc_task);
    assert(res == pdPASS);

    // Start rast.
    res = xTaskCreate(st3m_gfx_rast_task, "rast", 8192, NULL,
                      ESP_TASK_PRIO_MIN + 1, &rast_task);
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
