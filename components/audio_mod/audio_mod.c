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
    sprintf(buf, "p:%i/%i l:%i lc:%i", self->pocketmod.pattern,
            self->pocketmod.num_patterns, self->pocketmod.line,
            self->pocketmod.loop_count);
    ctx_move_to(ctx, -90, 0);
    ctx_text(ctx, buf);
    ctx_fill(ctx);
}

static void mod_think(st3m_media *media, float ms_elapsed) {
    int samples_needed = (ms_elapsed / 1000.0) * 48000;
    if (samples_needed > 1000) samples_needed = 1000;

    float rendered[samples_needed * 2];
    mod_state *self = (void *)media;
    int rend = pocketmod_render(&self->pocketmod, rendered, sizeof(rendered));
    for (int i = 0; i < rend / 4; i++) {
        self->control.audio_buffer[self->control.audio_w++] =
            rendered[i] * 20000;
        if (self->control.audio_w >= AUDIO_BUF_SIZE) self->control.audio_w = 0;
    }
}

static void mod_destroy(st3m_media *media) {
    mod_state *self = (void *)media;
    if (self->data) free(self->data);
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
        printf("BOOO\n");
        if (self->data) free(self->data);
        free(self);
        return NULL;
    }

    return (st3m_media *)self;
}
