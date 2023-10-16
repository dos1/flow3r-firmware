/*
 * Copyright 2019, 2021, 2023 Øyvind Kolås <pippin@gimp.org>
 *
 *  plays a gif animation on loop using stb_image, originally written as
 *  a l0dable for card10
 *
 */

#include <st3m_gfx.h>
#include <st3m_media.h>

#include "stb_image.h"

/* we need more defs, copied from internals */
typedef uint32_t stbi__uint32;
typedef int16_t stbi__int16;

typedef struct {
    stbi__uint32 img_x, img_y;
    int img_n, img_out_n;

    stbi_io_callbacks io;
    void *io_user_data;

    int read_from_callbacks;
    int buflen;
    stbi_uc buffer_start[128];
    int callback_already_read;

    stbi_uc *img_buffer, *img_buffer_end;
    stbi_uc *img_buffer_original, *img_buffer_original_end;
} stbi__context;

typedef struct {
    stbi__int16 prefix;
    stbi_uc first;
    stbi_uc suffix;
} stbi__gif_lzw;

typedef struct {
    int w, h;
    stbi_uc *out;  // output buffer (always 4 components)
    stbi_uc
        *background;  // The current "background" as far as a gif is concerned
    stbi_uc *history;
    int flags, bgindex, ratio, transparent, eflags;
    stbi_uc pal[256][4];
    stbi_uc lpal[256][4];
    stbi__gif_lzw codes[8192];
    stbi_uc *color_table;
    int parse, step;
    int lflags;
    int start_x, start_y;
    int max_x, max_y;
    int cur_x, cur_y;
    int line_size;
    int delay;
} stbi__gif;

stbi_uc *stbi__gif_load_next(stbi__context *s, stbi__gif *g, int *comp,
                             int req_comp, stbi_uc *two_back);

void stbi__start_callbacks(stbi__context *s, stbi_io_callbacks *c, void *user);

/////////////////////////////////////////////////////

typedef struct {
    st3m_media control;
    char *path;
    stbi__context s;
    stbi__gif g;

    int width;
    int height;
    int delay;
    int file_size;
    FILE *file;
    uint8_t *pixels;
} gif_state;

static int read_cb(void *user, char *data, int size) {
    gif_state *gif = user;
    return fread(data, 1, size, gif->file);
}
static void skip_cb(void *user, int n) {
    gif_state *gif = user;
    long pos = ftell(gif->file);
    fseek(gif->file, pos + n, SEEK_SET);
}

static int eof_cb(void *user) {
    gif_state *gif = user;
    long pos = ftell(gif->file);
    if (pos >= gif->file_size) return 1;
    return 0;
}

static stbi_io_callbacks clbk = { read_cb, skip_cb, eof_cb };

static void gif_stop(gif_state *gif);

static int gif_init(gif_state *gif, const char *path) {
    if (gif->path) {
        gif_stop(gif);
    }
    gif->file = fopen(path, "rb");
    if (!gif->file) return -1;
    gif->width = -1;
    gif->height = -1;

    fseek(gif->file, 0, SEEK_END);
    gif->file_size = ftell(gif->file);
    fseek(gif->file, 0, SEEK_SET);
    memset(&gif->s, 0, sizeof(gif->s));
    memset(&gif->g, 0, sizeof(gif->g));
    stbi__start_callbacks(&gif->s, &clbk, (void *)gif);
    gif->path = strdup(path);
    return 0;
}

static int gif_load_frame(gif_state *gif) {
    int c;
    if (!gif->path) return -1;

    gif->pixels = stbi__gif_load_next(&gif->s, &gif->g, &c, 4, 0);
    if (gif->pixels == (uint8_t *)&gif->s) {
        gif->pixels = NULL;
        gif_stop(gif);
        return -1;
    }
    if (gif->width < 0) gif->width = gif->g.w;
    if (gif->height < 0) gif->height = gif->g.h;

    return gif->g.delay;
}

static void gif_stop(gif_state *gif) {
    if (!gif->path) return;
    free(gif->path);
    gif->path = NULL;
    fclose(gif->file);
    gif->file = NULL;
    if (gif->g.out) {
        free(gif->g.out);
        gif->g.out = NULL;
    }
    if (gif->g.history) {
        free(gif->g.history);
        gif->g.history = NULL;
    }
    if (gif->g.background) {
        free(gif->g.background);
        gif->g.background = NULL;
    }
}

static void gif_draw(st3m_media *media, Ctx *ctx) {
    gif_state *gif = (gif_state *)media;
    char *path = strdup(gif->path);
    if (gif->delay <= 0) {
        gif->delay = gif_load_frame(gif);
        if (gif->delay < 0) {
            gif_init(gif, path);
            gif->delay = gif_load_frame(gif);
        } else if (gif->delay == 0)
            gif->delay = 100;
    }
    free(path);
    if (!gif->pixels) return;
    ctx_save(ctx);
    ctx_rectangle(ctx, -120, -120, 240, 240);
    ctx_translate(ctx, -120, -120);
    float scale = ctx_width(ctx) * 1.0 / gif->width;
    float scaleh = ctx_height(ctx) * 1.0 / gif->height;
    if (scaleh < scale) scale = scaleh;
    ctx_translate(ctx, (ctx_width(ctx) - gif->width * scale) / 2.0,
                  (ctx_height(ctx) - gif->height * scale) / 2.0);
    ctx_scale(ctx, scale, scale);
    ctx_image_smoothing(ctx, 0);
    ctx_define_texture(ctx, "!video", gif->width, gif->height, gif->width * 4,
                       CTX_FORMAT_RGBA8, gif->pixels, NULL);
    ctx_compositing_mode(ctx, CTX_COMPOSITE_COPY);
    ctx_fill(ctx);
    ctx_restore(ctx);
}

static void gif_think(st3m_media *media, float ms_elapsed) {
    gif_state *gif = (gif_state *)media;
    if (st3m_media_is_playing()) gif->delay -= ms_elapsed * 10;
}

static void gif_destroy(st3m_media *media) {
    gif_state *gif = (gif_state *)media;
    gif_stop(gif);
    free(media);
}

st3m_media *st3m_media_load_gif(const char *path) {
    gif_state *gif = (gif_state *)malloc(sizeof(gif_state));
    memset(gif, 0, sizeof(gif_state));
    gif->control.duration = -1;
    gif->control.draw = gif_draw;
    gif->control.think = gif_think;
    gif->control.destroy = gif_destroy;
    if (gif_init(gif, path) != 0) {
        free(gif);
        return NULL;
    }

    return (st3m_media *)gif;
}
