#ifndef __clang__
#pragma GCC optimize("O2")
#endif

#include <fcntl.h>
#include <st3m_audio.h>
#include <st3m_media.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ctx.h"

#define POCKETMOD_IMPLEMENTATION
#include "pocketmod.h"

typedef struct {
    st3m_media control;
    pocketmod_context pocketmod;
    uint8_t *data;
    size_t size;
    char *path;
    float scroll_pos;
} mod_state;

static void mod_draw(st3m_media *media, Ctx *ctx) {
    mod_state *self = (void *)media;

    ctx_rectangle(ctx, -120, -120, 240, 240);
    ctx_gray(ctx, 0);
    ctx_fill(ctx);
    // ctx_arc(ctx, 0, 0, 10, 10);
    ctx_rgb(ctx, 1.0, 1.0, 1.0);
    ctx_font_size(ctx, 20);
    char buf[100];
    sprintf(buf, "pat:%i/%i line:%i", self->pocketmod.pattern,
            self->pocketmod.num_patterns, self->pocketmod.line);
    ctx_text_align(ctx, CTX_TEXT_ALIGN_CENTER);
    ctx_move_to(ctx, 0, -20);
    ctx_text(ctx, buf);
    ctx_fill(ctx);

    ctx_font_size(ctx, 14);
    int xpos = 0;
    int str_width = ctx_text_width(ctx, self->path);
    if (str_width > 220) {
        xpos = ctx_sinf(self->scroll_pos) * (str_width - 220) / 2;
    }
    ctx_move_to(ctx, xpos, 14);
    ctx_gray(ctx, 0.6);
    ctx_text(ctx, self->path);
}

static void mod_think(st3m_media *media, float ms_elapsed) {
    mod_state *self = (void *)media;
    if (self->control.paused) return;

    int samples_needed = (ms_elapsed / 1000.0) * 48000;
    if (samples_needed > 1000) samples_needed = 1000;

    float rendered[samples_needed * 2];
    int rend = pocketmod_render(&self->pocketmod, rendered, sizeof(rendered));
    st3m_pcm_queue_float(48000, 2, rend / (sizeof(float) * 2), rendered);

    if (self->control.duration == 0) {
        self->control.duration = self->pocketmod.num_patterns + 1;
    }
    if (self->pocketmod.pattern > self->control.duration)
        self->control.duration = self->pocketmod.pattern + 1;
    self->control.position = self->pocketmod.pattern;
    if (self->pocketmod.loop_count)
        self->control.duration = self->control.position;

    self->scroll_pos += ms_elapsed / 1000.0;
}

static void mod_destroy(st3m_media *media) {
    mod_state *self = (void *)media;
    if (self->data) free(self->data);
    if (self->path) free(self->path);
    free(self);
}

static int file_get_contents(const char *path, uint8_t **contents,
                             size_t *length) {
    FILE *file;
    long size;
    long remaining;
    uint8_t *buffer;
    file = fopen(path, "rb");
    if (!file) {
        return -1;
    }
    fseek(file, 0, SEEK_END);
    size = remaining = ftell(file);

    if (length) {
        *length = size;
    }
    rewind(file);
    buffer = malloc(size + 2);
    if (!buffer) {
        fclose(file);
        return -1;
    }
    remaining -= fread(buffer, 1, remaining, file);
    if (remaining) {
        fclose(file);
        free(buffer);
        return -1;
    }
    fclose(file);
    *contents = (unsigned char *)buffer;
    buffer[size] = 0;
    return 0;
}

st3m_media *st3m_media_load_mod(const char *path) {
    mod_state *self = (mod_state *)malloc(sizeof(mod_state));
    memset(self, 0, sizeof(mod_state));
    self->control.draw = mod_draw;
    self->control.think = mod_think;
    self->control.destroy = mod_destroy;
    file_get_contents(path, &self->data, &self->size);
    if (!self->data ||
        !pocketmod_init(&self->pocketmod, self->data, self->size, 48000)) {
        if (self->data) free(self->data);
        free(self);
        return NULL;
    }
    self->path = strdup(path);
    self->scroll_pos = 0;
    return (st3m_media *)self;
}
