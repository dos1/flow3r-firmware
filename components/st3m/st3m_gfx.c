#include "st3m_gfx.h"
// Submit a filled ctx descriptor to the rasterization pipeline.

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
#include "st3m_version.h"

static EXT_RAM_BSS_ATTR uint16_t fb[240 * 240];

// Get a free drawlist ctx to draw into.
//
// ticks_to_wait can be used to limit the time to wait for a free ctx
// descriptor, or portDELAY_MAX can be specified to wait forever. If the timeout
// expires, NULL will be returned.
static Ctx *st3m_gfx_drawctx_free_get(TickType_t ticks_to_wait);

// Submit a filled ctx descriptor to the rasterization pipeline.
static void st3m_gfx_drawctx_pipe_put(void);

static const char *TAG = "st3m-gfx";

#define OVERLAY_WIDTH 240
#define OVERLAY_HEIGHT 240
#define OVERLAY_X 0
#define OVERLAY_Y 0

static st3m_gfx_mode _st3m_gfx_mode = st3m_gfx_default;

EXT_RAM_BSS_ATTR static uint8_t
    st3m_overlay_fb[OVERLAY_WIDTH * OVERLAY_HEIGHT * 4];
EXT_RAM_BSS_ATTR uint16_t st3m_overlay_backup[OVERLAY_WIDTH * OVERLAY_HEIGHT];

// drawlist ctx
static Ctx *user_ctx = NULL;

// RGB565_byteswapped framebuffer ctx - our global ctx
//   user_ctx gets replayed on this
static Ctx *fb_ctx = NULL;

// RGBA8 overlay ctx
static Ctx *overlay_ctx = NULL;

// corner pixel coordinates for overlay clip
static int _st3m_overlay_y1 = 0;
static int _st3m_overlay_x1 = 0;
static int _st3m_overlay_y0 = 0;
static int _st3m_overlay_x0 = 0;

static float smoothed_fps = 0.0f;
static QueueHandle_t user_ctx_freeq = NULL;
static QueueHandle_t user_ctx_rastq = NULL;

static st3m_counter_rate_t rast_rate;
static TaskHandle_t graphics_task;

//////////////////////////

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
            ESP_LOGI(TAG, "%s", desc);
            starved = 1;
        }
    }
}

void st3m_ctx_merge_overlay(uint16_t *fb, uint8_t *overlay, int ostride,
                            uint16_t *overlay_backup, int x0, int y0, int w,
                            int h);
void st3m_ctx_unmerge_overlay(uint16_t *fb, uint16_t *overlay_backup, int x0,
                              int y0, int w, int h);

float st3m_gfx_fps(void) { return smoothed_fps; }

void st3m_gfx_set_palette(uint8_t *pal_in, int count) {
    if (count > 256) count = 256;
    if (count < 0) count = 0;
    uint8_t *pal =
        ((uint8_t *)st3m_overlay_fb) + sizeof(st3m_overlay_fb) - 256 * 3;
    for (int i = 0; i < count * 3; i++) pal[i] = pal_in[i];
}

static void grayscale_palette(void) {
    uint8_t pal[256 * 3];
    for (int i = 0; i < 256; i++) {
        pal[i * 3 + 0] = i;
        pal[i * 3 + 1] = i;
        pal[i * 3 + 2] = i;
    }
    st3m_gfx_set_palette(pal, 256);
}

static void fire_palette(void) {
    uint8_t pal[256 * 3];
    for (int i = 0; i < 256; i++) {
        pal[i * 3 + 0] = i;
        pal[i * 3 + 1] = (i / 255.0) * (i / 255.0) * 255;
        pal[i * 3 + 2] = (i / 255.0) * (i / 255.0) * (i / 255.0) * 255;
    }
    st3m_gfx_set_palette(pal, 256);
}

static void ice_palette(void) {
    uint8_t pal[256 * 3];
    for (int i = 0; i < 256; i++) {
        pal[i * 3 + 0] = (i / 255.0) * (i / 255.0) * (i / 255.0) * 255;
        pal[i * 3 + 1] = (i / 255.0) * (i / 255.0) * 255;
        pal[i * 3 + 2] = i;
    }
    st3m_gfx_set_palette(pal, 256);
}

void st3m_set_gfx_mode(st3m_gfx_mode mode) {
    memset(fb, 0, sizeof(fb));
    memset(st3m_overlay_fb, 0, sizeof(st3m_overlay_fb));

    switch ((int)mode) {
        case 8:
            fire_palette();
            break;
        case 9:
            ice_palette();
            mode = 8;
            break;
        case 10:
            grayscale_palette();
            mode = 8;
            break;
    }
    _st3m_gfx_mode = mode;
}
st3m_gfx_mode st3m_get_gfx_mode(void) { return _st3m_gfx_mode; }
uint8_t *st3m_gfx_fb(st3m_gfx_mode mode) {
    switch (_st3m_gfx_mode) {
        case st3m_gfx_default:
        case st3m_gfx_16bpp:
            return (uint8_t *)fb;
        case st3m_gfx_32bpp:
        case st3m_gfx_24bpp:
        case st3m_gfx_8bpp:
        case st3m_gfx_overlay:
            return st3m_overlay_fb;
    }
    return st3m_overlay_fb;
}

static void st3m_gfx_task(void *_arg) {
    (void)_arg;

    while (true) {
        int descno = 0;

        xQueueReceiveNotifyStarved(user_ctx_rastq, &descno,
                                   "rast task starved (user_ctx)!");

        ctx_set_textureclock(fb_ctx, ctx_textureclock(fb_ctx) + 1);

        st3m_overlay_ctx();

        switch (_st3m_gfx_mode) {
            case st3m_gfx_8bpp:
            case st3m_gfx_24bpp:
            case st3m_gfx_32bpp:
                flow3r_bsp_display_send_fb(st3m_overlay_fb, _st3m_gfx_mode);
                break;
            case st3m_gfx_overlay:
                flow3r_bsp_display_send_fb(st3m_overlay_fb, 32);
                break;
            case st3m_gfx_default:
                ctx_render_ctx(user_ctx, fb_ctx);
                if (_st3m_overlay_y1 != _st3m_overlay_y0)
                    st3m_ctx_merge_overlay(
                        fb,
                        st3m_overlay_fb + 4 * ((_st3m_overlay_y0 - OVERLAY_Y) *
                                                   OVERLAY_WIDTH +
                                               (_st3m_overlay_x0 - OVERLAY_X)),
                        OVERLAY_WIDTH * 4, st3m_overlay_backup,
                        _st3m_overlay_x0, _st3m_overlay_y0,
                        _st3m_overlay_x1 - _st3m_overlay_x0 + 1,
                        _st3m_overlay_y1 - _st3m_overlay_y0 + 1);
                flow3r_bsp_display_send_fb(fb, 16);
                if (_st3m_overlay_y1 != _st3m_overlay_y0)
                    st3m_ctx_unmerge_overlay(
                        fb, st3m_overlay_backup, _st3m_overlay_x0,
                        _st3m_overlay_y0,
                        _st3m_overlay_x1 - _st3m_overlay_x0 + 1,
                        _st3m_overlay_y1 - _st3m_overlay_y0 + 1);
                break;
            case st3m_gfx_16bpp:
                flow3r_bsp_display_send_fb(fb, 16);
                break;
        }
        ctx_drawlist_clear(user_ctx);

        xQueueSend(user_ctx_freeq, &descno, portMAX_DELAY);
        st3m_counter_rate_sample(&rast_rate);
        float rate = 1000000.0 / st3m_counter_rate_average(&rast_rate);
        smoothed_fps = smoothed_fps * 0.9 + 0.1 * rate;
    }
}

void st3m_gfx_flow3r_logo(Ctx *ctx, float x, float y, float dim) {
    ctx_save(ctx);
    ctx_translate(ctx, x, y);
    ctx_scale(ctx, dim, dim);
    ctx_translate(ctx, -0.5f, -0.5f);
    ctx_linear_gradient(ctx, 0.18f, 0.5f, 0.95f, 0.5f);
    ctx_gradient_add_stop(ctx, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
    ctx_gradient_add_stop(ctx, 0.2f, 1.0f, 1.0f, 0.0f, 1.0f);
    ctx_gradient_add_stop(ctx, 0.4f, 0.0f, 1.0f, 0.0f, 1.0f);
    ctx_gradient_add_stop(ctx, 0.65f, 0.0f, 1.0f, 1.0f, 1.0f);
    ctx_gradient_add_stop(ctx, 0.8f, 0.0f, 0.0f, 1.0f, 1.0f);
    ctx_gradient_add_stop(ctx, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f);

    ctx_save(ctx);
    ctx_scale(ctx, 1 / 30.0f, 1 / 30.0f);
    ctx_translate(ctx, 0.0f, 10.0f);
    ctx_move_to(ctx, 6.185f, 0.0f);
    ctx_curve_to(ctx, 5.514f, 0.021f, 4.852f, 0.234f, 4.210f, 0.560f);
    ctx_curve_to(ctx, 2.740f, 1.360f, 1.969f, 2.589f, 1.939f, 4.269f);
    ctx_curve_to(ctx, 1.969f, 4.479f, 1.950f, 4.649f, 1.980f, 4.849f);
    ctx_curve_to(ctx, 1.980f, 5.089f, 1.890f, 5.209f, 1.660f, 5.269f);
    ctx_curve_to(ctx, 1.260f, 5.319f, 0.849f, 5.370f, 0.419f, 5.460f);
    ctx_curve_to(ctx, 0.129f, 5.530f, -0.0704f, 5.870f, 0.0195f, 6.060f);
    ctx_curve_to(ctx, 0.109f, 6.260f, 0.319f, 6.289f, 0.589f, 6.259f);
    ctx_curve_to(ctx, 0.999f, 6.209f, 1.370f, 6.199f, 1.740f, 6.119f);
    ctx_curve_to(ctx, 2.150f, 6.069f, 2.470f, 6.199f, 2.650f, 6.519f);
    ctx_curve_to(ctx, 2.950f, 6.989f, 3.379f, 7.379f, 3.859f, 7.699f);
    ctx_curve_to(ctx, 4.339f, 8.009f, 4.530f, 8.469f, 4.660f, 8.929f);
    ctx_curve_to(ctx, 4.880f, 9.589f, 4.429f, 10.16f, 3.760f, 10.240f);
    ctx_curve_to(ctx, 2.949f, 10.340f, 2.320f, 10.220f, 1.730f, 9.640f);
    ctx_curve_to(ctx, 1.380f, 9.300f, 1.090f, 8.889f, 0.900f, 8.439f);
    ctx_curve_to(ctx, 0.800f, 8.169f, 0.700f, 7.909f, 0.570f, 7.689f);
    ctx_curve_to(ctx, 0.520f, 7.589f, 0.379f, 7.539f, 0.279f, 7.590f);
    ctx_curve_to(ctx, 0.219f, 7.599f, 0.070f, 7.719f, 0.070f, 7.789f);
    ctx_curve_to(ctx, 0.150f, 8.229f, 0.240f, 8.659f, 0.390f, 9.019f);
    ctx_curve_to(ctx, 0.790f, 9.999f, 1.391f, 10.731f, 2.451f, 11.011f);
    ctx_curve_to(ctx, 3.028f, 11.163f, 3.616f, 11.365f, 4.301f, 11.269f);
    ctx_curve_to(ctx, 5.110f, 11.002f, 5.599f, 10.219f, 5.789f, 9.269f);
    ctx_curve_to(ctx, 5.969f, 8.500f, 5.430f, 8.019f, 4.960f, 7.529f);
    ctx_line_to(ctx, 4.650f, 7.289f);
    ctx_curve_to(ctx, 4.338f, 7.043f, 3.646f, 6.725f, 3.519f, 6.160f);
    ctx_curve_to(ctx, 3.889f, 6.080f, 4.260f, 6.000f, 4.630f, 6.00f);
    ctx_curve_to(ctx, 5.240f, 5.980f, 5.870f, 6.029f, 6.480f, 6.029f);
    ctx_curve_to(ctx, 6.820f, 6.059f, 7.120f, 5.990f, 7.330f, 5.720f);
    ctx_curve_to(ctx, 7.390f, 5.640f, 7.429f, 5.499f, 7.429f, 5.429f);
    ctx_curve_to(ctx, 7.379f, 5.269f, 7.260f, 5.109f, 7.150f, 5.089f);
    ctx_curve_to(ctx, 6.860f, 4.999f, 6.559f, 5.0f, 6.25f, 5.0f);
    ctx_curve_to(ctx, 5.330f, 5.02f, 4.410f, 5.099f, 3.490f, 5.109f);
    ctx_curve_to(ctx, 2.980f, 5.139f, 2.859f, 4.989f, 2.859f, 4.439f);
    ctx_curve_to(ctx, 2.889f, 3.239f, 3.519f, 2.330f, 4.429f, 1.640f);
    ctx_curve_to(ctx, 5.049f, 1.150f, 5.849f, 0.979f, 6.619f, 1.089f);
    ctx_curve_to(ctx, 7.379f, 1.199f, 8.070f, 1.489f, 8.380f, 2.339f);
    ctx_curve_to(ctx, 8.440f, 2.569f, 8.810f, 2.489f, 8.919f, 2.269f);
    ctx_curve_to(ctx, 9.089f, 1.979f, 9.129f, 1.700f, 8.949f, 1.380f);
    ctx_curve_to(ctx, 8.679f, 0.860f, 8.239f, 0.580f, 7.759f, 0.330f);
    ctx_curve_to(ctx, 7.234f, 0.080f, 6.707f, -0.0170f, 6.185f, 0.0f);
    ctx_fill(ctx);

    ctx_move_to(ctx, 24.390f, 3.050f);
    ctx_curve_to(ctx, 23.745f, 3.058f, 23.089f, 3.252f, 22.419f, 3.449f);
    ctx_curve_to(ctx, 22.349f, 3.459f, 22.340f, 3.630f, 22.320f, 3.740f);
    ctx_curve_to(ctx, 22.350f, 4.010f, 22.539f, 4.160f, 22.779f, 4.160f);
    ctx_curve_to(ctx, 23.389f, 4.150f, 24.029f, 4.100f, 24.619f, 4.130f);
    ctx_curve_to(ctx, 24.859f, 4.130f, 25.149f, 4.229f, 25.369f, 4.339f);
    ctx_curve_to(ctx, 25.619f, 4.479f, 25.799f, 4.799f, 25.689f, 5.019f);
    ctx_curve_to(ctx, 25.609f, 5.199f, 25.459f, 5.390f, 25.269f, 5.480f);
    ctx_curve_to(ctx, 25.009f, 5.580f, 24.700f, 5.590f, 24.400f, 5.660f);
    ctx_curve_to(ctx, 24.130f, 5.690f, 23.860f, 5.719f, 23.630f, 5.789f);
    ctx_curve_to(ctx, 23.400f, 5.859f, 23.280f, 6.010f, 23.310f, 6.210f);
    ctx_curve_to(ctx, 23.370f, 6.380f, 23.500f, 6.600f, 23.640f, 6.650f);
    ctx_curve_to(ctx, 23.860f, 6.760f, 24.070f, 6.810f, 24.310f, 6.810f);
    ctx_curve_to(ctx, 24.650f, 6.840f, 25.029f, 6.819f, 25.439f, 6.839f);
    ctx_curve_to(ctx, 25.779f, 6.859f, 26.040f, 7.000f, 26.150f, 7.330f);
    ctx_curve_to(ctx, 26.540f, 8.300f, 25.899f, 9.468f, 24.859f, 9.628f);
    ctx_curve_to(ctx, 24.449f, 9.688f, 24.0f, 9.619f, 23.600f, 9.669f);
    ctx_curve_to(ctx, 23.399f, 9.699f, 23.189f, 9.729f, 23.029f, 9.779f);
    ctx_curve_to(ctx, 22.799f, 9.839f, 22.640f, 10.200f, 22.720f, 10.330f);
    ctx_curve_to(ctx, 22.902f, 10.577f, 23.018f, 10.732f, 23.320f, 10.730f);
    ctx_curve_to(ctx, 23.940f, 10.720f, 24.580f, 10.680f, 25.150f, 10.570f);
    ctx_curve_to(ctx, 27.220f, 10.170f, 27.830f, 7.660f, 26.740f, 6.330f);
    ctx_curve_to(ctx, 26.540f, 6.120f, 26.519f, 5.920f, 26.619f, 5.630f);
    ctx_curve_to(ctx, 27.005f, 4.795f, 26.709f, 4.028f, 25.880f, 3.460f);
    ctx_curve_to(ctx, 25.388f, 3.152f, 24.892f, 3.044f, 24.390f, 3.050f);
    ctx_fill(ctx);

    ctx_move_to(ctx, 9.294f, 3.687f);
    ctx_curve_to(ctx, 9.198f, 3.690f, 9.092f, 3.7307f, 9.00f, 3.800f);
    ctx_curve_to(ctx, 8.739f, 4.010f, 8.740f, 4.3091f, 8.740f, 4.619f);
    ctx_curve_to(ctx, 8.780f, 6.289f, 8.789f, 7.999f, 8.589f, 9.669f);
    ctx_curve_to(ctx, 8.599f, 9.979f, 8.529f, 10.289f, 8.599f, 10.589f);
    ctx_curve_to(ctx, 8.659f, 10.819f, 8.789f, 11.049f, 8.919f, 11.269f);
    ctx_curve_to(ctx, 9.019f, 11.469f, 9.379f, 11.389f, 9.589f, 11.119f);
    ctx_curve_to(ctx, 9.700f, 10.399f, 9.747f, 9.754f, 9.800f, 8.970f);
    ctx_curve_to(ctx, 10.00f, 7.610f, 9.860f, 6.219f, 9.830f, 4.859f);
    ctx_curve_to(ctx, 9.790f, 4.529f, 9.680f, 4.199f, 9.570f, 3.869f);
    ctx_curve_to(ctx, 9.530f, 3.739f, 9.419f, 3.683f, 9.294f, 3.687f);
    ctx_fill(ctx);
    ctx_move_to(ctx, 30.595f, 5.626f);
    ctx_curve_to(ctx, 30.542f, 5.629f, 30.486f, 5.634f, 30.429f, 5.640f);
    ctx_curve_to(ctx, 30.252f, 5.654f, 29.879f, 5.879f, 29.589f, 6.019f);
    ctx_curve_to(ctx, 29.329f, 6.119f, 29.129f, 6.149f, 28.859f, 5.939f);
    ctx_curve_to(ctx, 28.549f, 5.699f, 28.159f, 5.889f, 28.109f, 6.269f);
    ctx_curve_to(ctx, 28.109f, 6.439f, 28.089f, 6.620f, 28.109f, 6.75f);
    ctx_curve_to(ctx, 28.199f, 7.729f, 28.319f, 8.669f, 28.199f, 9.609f);
    ctx_curve_to(ctx, 28.189f, 9.779f, 28.209f, 9.920f, 28.259f, 10.080f);
    ctx_curve_to(ctx, 28.309f, 10.170f, 28.39f, 10.300f, 28.5f, 10.320f);
    ctx_curve_to(ctx, 28.54f, 10.350f, 28.699f, 10.300f, 28.759f, 10.220f);
    ctx_curve_to(ctx, 28.899f, 9.960f, 29.110f, 9.699f, 29.140f, 9.419f);
    ctx_curve_to(ctx, 29.260f, 9.019f, 29.239f, 8.579f, 29.259f, 8.169f);
    ctx_curve_to(ctx, 29.289f, 7.819f, 29.400f, 7.609f, 29.640f, 7.369f);
    ctx_curve_to(ctx, 29.980f, 7.089f, 30.329f, 6.869f, 30.769f, 6.849f);
    ctx_curve_to(ctx, 31.009f, 6.859f, 31.320f, 6.849f, 31.550f, 6.789f);
    ctx_curve_to(ctx, 31.790f, 6.719f, 31.899f, 6.569f, 31.839f, 6.339f);
    ctx_curve_to(ctx, 31.773f, 5.973f, 31.395f, 5.593f, 30.595f, 5.626f);
    ctx_fill(ctx);
    ctx_move_to(ctx, 21.314f, 6.205f);
    ctx_curve_to(ctx, 21.242f, 6.202f, 21.158f, 6.230f, 21.060f, 6.300f);
    ctx_curve_to(ctx, 20.870f, 6.460f, 20.759f, 6.610f, 20.789f, 6.880f);
    ctx_curve_to(ctx, 20.829f, 7.220f, 20.879f, 7.550f, 20.849f, 7.900f);
    ctx_curve_to(ctx, 20.809f, 8.420f, 20.729f, 8.909f, 20.619f, 9.369f);
    ctx_curve_to(ctx, 20.599f, 9.469f, 20.340f, 9.640f, 20.210f, 9.660f);
    ctx_curve_to(ctx, 20.000f, 9.690f, 19.779f, 9.579f, 19.699f, 9.449f);
    ctx_curve_to(ctx, 19.549f, 9.329f, 19.55f, 9.089f, 19.5f, 8.929f);
    ctx_curve_to(ctx, 19.43f, 8.700f, 19.430f, 8.389f, 19.300f, 8.169f);
    ctx_curve_to(ctx, 19.210f, 8.039f, 19.027f, 7.841f, 18.845f, 7.859f);
    ctx_curve_to(ctx, 18.685f, 7.845f, 18.489f, 8.029f, 18.439f, 8.169f);
    ctx_curve_to(ctx, 18.349f, 8.349f, 18.389f, 8.620f, 18.349f, 8.830f);
    ctx_curve_to(ctx, 18.289f, 9.150f, 18.289f, 9.450f, 18.189f, 9.740f);
    ctx_curve_to(ctx, 18.129f, 9.810f, 17.969f, 9.929f, 17.839f, 9.949f);
    ctx_curve_to(ctx, 17.779f, 9.959f, 17.559f, 9.850f, 17.509f, 9.75f);
    ctx_curve_to(ctx, 17.199f, 9.200f, 17.050f, 8.600f, 17.050f, 7.990f);
    ctx_curve_to(ctx, 17.050f, 7.690f, 17.080f, 7.409f, 17.080f, 7.099f);
    ctx_curve_to(ctx, 17.090f, 6.860f, 16.930f, 6.670f, 16.650f, 6.640f);
    ctx_curve_to(ctx, 16.450f, 6.660f, 16.219f, 6.790f, 16.189f, 7.070f);
    ctx_curve_to(ctx, 16.069f, 8.010f, 16.049f, 8.970f, 16.439f, 9.880f);
    ctx_curve_to(ctx, 16.889f, 10.960f, 17.680f, 11.269f, 18.630f, 10.669f);
    ctx_curve_to(ctx, 18.980f, 10.450f, 19.250f, 10.420f, 19.570f, 10.550f);
    ctx_curve_to(ctx, 20.300f, 10.870f, 20.840f, 10.559f, 21.320f, 10.019f);
    ctx_line_to(ctx, 21.320f, 10.030f);
    ctx_curve_to(ctx, 21.430f, 9.819f, 21.610f, 9.590f, 21.640f, 9.310f);
    ctx_curve_to(ctx, 21.870f, 8.390f, 21.790f, 7.480f, 21.640f, 6.570f);
    ctx_curve_to(ctx, 21.630f, 6.478f, 21.529f, 6.212f, 21.314f, 6.205f);
    ctx_fill(ctx);

    ctx_move_to(ctx, 13.375f, 6.542f);
    ctx_curve_to(ctx, 12.980f, 6.538f, 12.576f, 6.627f, 12.199f, 6.820f);
    ctx_curve_to(ctx, 11.591f, 7.294f, 10.740f, 7.913f, 10.669f, 9.099f);
    ctx_curve_to(ctx, 10.691f, 10.877f, 12.662f, 11.652f, 14.699f, 10.650f);
    ctx_curve_to(ctx, 15.399f, 10.220f, 15.729f, 9.630f, 15.699f, 8.810f);
    ctx_curve_to(ctx, 15.654f, 7.408f, 14.557f, 6.556f, 13.375f, 6.542f);
    ctx_close_path(ctx);
    ctx_move_to(ctx, 13.357f, 7.556f);
    ctx_curve_to(ctx, 13.758f, 7.552f, 14.152f, 7.715f, 14.400f, 7.990f);
    ctx_curve_to(ctx, 14.720f, 8.360f, 14.769f, 9.240f, 14.509f, 9.650f);
    ctx_curve_to(ctx, 13.936f, 10.261f, 13.290f, 10.362f, 12.359f, 10.230f);
    ctx_curve_to(ctx, 11.899f, 10.120f, 11.610f, 9.709f, 11.720f, 9.25f);
    ctx_curve_to(ctx, 11.869f, 8.568f, 11.925f, 8.145f, 12.820f, 7.669f);
    ctx_curve_to(ctx, 12.992f, 7.594f, 13.175f, 7.558f, 13.357f, 7.556f);
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

    st3m_gfx_drawctx_free_get(portMAX_DELAY);

    ctx_save(user_ctx);

    // Draw background.
    ctx_rgb(user_ctx, 0, 0, 0);
    ctx_rectangle(user_ctx, -120, -120, 240, 240);
    ctx_fill(user_ctx);

    st3m_gfx_flow3r_logo(user_ctx, 0, -30, 150);

    int y = 20;

    ctx_gray(user_ctx, 1.0);
    ctx_text_align(user_ctx, CTX_TEXT_ALIGN_CENTER);
    ctx_text_baseline(user_ctx, CTX_TEXT_BASELINE_MIDDLE);
    ctx_font_size(user_ctx, 20.0);

    // Draw title, if any.
    if (tv->title != NULL) {
        ctx_move_to(user_ctx, 0, y);
        ctx_text(user_ctx, tv->title);
        y += 20;
    }

    ctx_font_size(user_ctx, 15.0);
    ctx_gray(user_ctx, 0.8);

    // Draw messages.
    const char **lines = tv->lines;
    if (lines != NULL) {
        while (*lines != NULL) {
            const char *text = *lines++;
            ctx_move_to(user_ctx, 0, y);
            ctx_text(user_ctx, text);
            y += 15;
        }
    }

    // Draw version.
    ctx_font_size(user_ctx, 15.0);
    ctx_gray(user_ctx, 0.6);
    ctx_move_to(user_ctx, 0, 100);
    ctx_text(user_ctx, st3m_version);

    ctx_restore(user_ctx);

    st3m_gfx_drawctx_pipe_put();
}

void st3m_gfx_init(void) {
    // Make sure we're not being re-initialized.

    st3m_counter_rate_init(&rast_rate);

    flow3r_bsp_display_init();

    // Create drawlist ctx queues.
    user_ctx_freeq = xQueueCreate(1 + 1, sizeof(int));
    assert(user_ctx_freeq != NULL);
    user_ctx_rastq = xQueueCreate(1 + 1, sizeof(int));
    assert(user_ctx_rastq != NULL);

    // Setup framebuffer descriptor.
    fb_ctx = ctx_new_for_framebuffer(
        fb, FLOW3R_BSP_DISPLAY_WIDTH, FLOW3R_BSP_DISPLAY_HEIGHT,
        FLOW3R_BSP_DISPLAY_WIDTH * 2, CTX_FORMAT_RGB565_BYTESWAPPED);
    assert(fb_ctx != NULL);
    // translate x and y by 120 px to have (0,0) at center of the screen
    int32_t offset_x = FLOW3R_BSP_DISPLAY_WIDTH / 2;
    int32_t offset_y = FLOW3R_BSP_DISPLAY_HEIGHT / 2;
    ctx_apply_transform(fb_ctx, 1, 0, offset_x, 0, 1, offset_y, 0, 0, 1);

    // Setup user_ctx descriptor.
    user_ctx =
        ctx_new_drawlist(FLOW3R_BSP_DISPLAY_WIDTH, FLOW3R_BSP_DISPLAY_HEIGHT);
    assert(user_ctx != NULL);
    ctx_set_texture_cache(user_ctx, fb_ctx);

    // Push descriptor to freeq.
    int i = 0;
    BaseType_t res = xQueueSend(user_ctx_freeq, &i, 0);
    assert(res == pdTRUE);

    // Start rasterization, compositing and scan-out
    res = xTaskCreate(st3m_gfx_task, "graphics", 8192, NULL,
                      ESP_TASK_PRIO_MIN + 1, &graphics_task);
    assert(res == pdPASS);
}

static Ctx *st3m_gfx_drawctx_free_get(TickType_t ticks_to_wait) {
    int descno;
    BaseType_t res = xQueueReceive(user_ctx_freeq, &descno, ticks_to_wait);
    if (res != pdTRUE) {
        return NULL;
    }
    return user_ctx;
}

static void st3m_gfx_drawctx_pipe_put(void) {
    int i = 0;
    xQueueSend(user_ctx_rastq, &i, portMAX_DELAY);
}

uint8_t st3m_gfx_drawctx_pipe_full(void) {
    return uxQueueSpacesAvailable(user_ctx_rastq) == 0;
}

void st3m_gfx_flush(void) {
    ESP_LOGW(TAG, "Pipeline flush/reset requested...");
    // Drain all workqs and freeqs.
    xQueueReset(user_ctx_freeq);
    xQueueReset(user_ctx_rastq);

    // Delay, making sure pipeline tasks have returned all used descriptors. One
    // second is enough to make sure we've processed everything.
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // And drain again.
    xQueueReset(user_ctx_freeq);

    int i = 0;
    BaseType_t res = xQueueSend(user_ctx_freeq, &i, 0);
    assert(res == pdTRUE);
    ESP_LOGW(TAG, "Pipeline flush/reset done.");
}

static int st3m_user_ctx_no = 0;  // always 0 - but this keeps pipelined
                                  // rendering working as a compile time opt-in
Ctx *st3m_ctx(TickType_t ticks_to_wait) {
    Ctx *ctx = st3m_gfx_drawctx_free_get(ticks_to_wait);
    if (!ctx) return NULL;
    return ctx;
}

void st3m_overlay_clear(void) {
    Ctx *ctx = st3m_overlay_ctx();
    ctx_save(ctx);
    ctx_compositing_mode(ctx, CTX_COMPOSITE_CLEAR);
    ctx_gray(ctx, 0);  // BUG(ctx) - alpha=0 should be allowed here
    ctx_rectangle(ctx, -120, -120, 240, 240);
    ctx_fill(ctx);
    ctx_restore(ctx);
}

Ctx *st3m_overlay_ctx(void) {
    if (!overlay_ctx) {
        Ctx *ctx = overlay_ctx = ctx_new_for_framebuffer(
            st3m_overlay_fb, OVERLAY_WIDTH, OVERLAY_HEIGHT, OVERLAY_WIDTH * 4,
            CTX_FORMAT_RGBA8);

        ctx_translate(ctx, 120 - OVERLAY_X, 120 - OVERLAY_Y);
        memset(st3m_overlay_fb, 0, sizeof(st3m_overlay_fb));
    }
    return overlay_ctx;
}

void st3m_ctx_end_frame(Ctx *ctx) {
    xQueueSend(user_ctx_rastq, &st3m_user_ctx_no, portMAX_DELAY);
}

void st3m_gfx_overlay_clip(int x0, int y0, int x1, int y1) {
    if (y1 < OVERLAY_Y) y1 = OVERLAY_Y;
    if (y1 > OVERLAY_Y + OVERLAY_HEIGHT) y1 = OVERLAY_Y + OVERLAY_HEIGHT;
    if (y0 < OVERLAY_Y) y0 = OVERLAY_Y;
    if (y0 > OVERLAY_Y + OVERLAY_HEIGHT) y0 = OVERLAY_Y + OVERLAY_HEIGHT;

    if (x1 < OVERLAY_X) x1 = OVERLAY_X;
    if (x1 > OVERLAY_X + OVERLAY_WIDTH) x1 = OVERLAY_X + OVERLAY_WIDTH;
    if (x0 < OVERLAY_X) x0 = OVERLAY_X;
    if (x0 > OVERLAY_X + OVERLAY_WIDTH) x0 = OVERLAY_X + OVERLAY_WIDTH;
    if ((x1 < x0) || (y1 < y0)) {
        _st3m_overlay_y1 = _st3m_overlay_y0 = _st3m_overlay_x1 =
            _st3m_overlay_x0 = 0;
    } else {
        _st3m_overlay_y1 = y1;
        _st3m_overlay_y0 = y0;
        _st3m_overlay_x1 = x1;
        _st3m_overlay_x0 = x0;
    }
}
