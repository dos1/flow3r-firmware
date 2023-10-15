#ifndef __clang__
#pragma GCC optimize("O3")
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <st3m_audio.h>
#include <st3m_gfx.h>
#include <st3m_media.h>

#include "ctx.h"

#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"

typedef struct {
    st3m_media control;

    plm_t *plm;
    uint8_t *frame_data;
    int width;
    int height;
    int frame_drop;
    int sample_rate;
    int frame_no;
    int old_mode;
    // last decoded frame contained chroma samples
    // this allows us to take a grayscale fast-path
    unsigned had_chroma : 1;
    unsigned color : 1;
    unsigned video : 1;
    unsigned audio : 1;
    unsigned loop : 1;
    float scale;
} mpg1_state;

static void mpg1_on_video(plm_t *player, plm_frame_t *frame, void *user);
static void mpg1_on_audio(plm_t *player, plm_samples_t *samples, void *user);

static void mpg1_set(st3m_media *media, const char *key, float value) {
    mpg1_state *self = (void *)media;
    if (strcmp(key, "scale") == 0) {
        self->scale = value;
    } else if (strcmp(key, "grayscale") == 0) {
        self->color = !value;
    }
}

static float mpg1_get(st3m_media *media, const char *key) {
    mpg1_state *self = (void *)media;
    if (strcmp(key, "scale") == 0) {
        return self->scale;
    } else if (strcmp(key, "grayscale") == 0) {
        return !self->color;
    }
    return -1;
}

static void mpg1_think(st3m_media *media, float ms_elapsed) {
    mpg1_state *self = (void *)media;
    float elapsed_time = ms_elapsed / 1000.0;
    double seek_to = -1;

    if (self->control.seek >= 0.0) {
        seek_to = self->control.seek * self->control.duration;
        self->control.seek = -1;
    }

    if (elapsed_time > 1.0 / 25.0) {
        elapsed_time = 1.0 / 25.0;
    }

    if (self->control.paused) elapsed_time = 0;

    // Seek or advance decode
    if (seek_to != -1) {
        // XXX : clear queued audio
        plm_seek(self->plm, seek_to, FALSE);
    } else {
        plm_decode(self->plm, elapsed_time);
    }

    self->control.time = plm_get_time(self->plm);
    self->control.position = self->control.time;
}

static inline int memcpy_chroma(uint8_t *restrict target, uint8_t *restrict src,
                                int count) {
    int ret = 0;
    // TODO : do 4bytes at a time
    for (int i = 0; i < count; i++) {
        uint8_t val = src[i];
        target[i] = val;
        ret = (ret | (val != 128));
    }
    return ret;
}

static void mpg1_on_video(plm_t *mpeg, plm_frame_t *frame, void *user) {
    mpg1_state *self = (mpg1_state *)user;

    self->frame_no++;

    self->width = frame->y.width;
    self->height = frame->y.height;
    memcpy(self->frame_data, frame->y.data, frame->y.width * frame->y.height);

    if (self->color) {
        void *dst_cb = self->frame_data + frame->y.width * frame->y.height;
        void *dst_cr = dst_cb + (frame->y.width / 2) * (frame->y.height / 2);
        int chroma_len = (frame->y.width / 2) * (frame->y.height / 2);
        if (((self->frame_no & 63) == 0)) self->had_chroma = 0;
        if ((!self->had_chroma) &&
            ((self->frame_no < 60) || ((self->frame_no & 63) == 0))) {
            /* copy u and v components */
            self->had_chroma |=
                memcpy_chroma(dst_cb, frame->cb.data, chroma_len) |
                memcpy_chroma(dst_cr, frame->cr.data, chroma_len);
        } else if (self->had_chroma) {
            memcpy(dst_cb, frame->cb.data, chroma_len);
            memcpy(dst_cr, frame->cr.data, chroma_len);
        }
    }
}

static void mpg1_on_audio(plm_t *mpeg, plm_samples_t *samples, void *user) {
    st3m_pcm_queue_float(((mpg1_state *)user)->sample_rate, 2, samples->count,
                         samples->interleaved);
}

static void mpg1_draw(st3m_media *media, Ctx *ctx) {
    mpg1_state *mpg1 = (mpg1_state *)media;

    {
        float dim = 240 * mpg1->scale;

        if (mpg1->video) {
            float scale = dim / mpg1->width;
            float scaleh = dim / mpg1->height;
            if (scaleh < scale) scale = scaleh;
            {
                if (mpg1->frame_no < 20) {  // ensure we've filled at least some
                                            // complete frames with background
                    ctx_rectangle(ctx, -120, -120, 240, 240);
                    ctx_gray(ctx, 0.0);
                    ctx_fill(ctx);
                }
                ctx_translate(ctx, -dim / 2, -dim / 2);
                ctx_translate(ctx, (dim - mpg1->width * scale) / 2.0,
                              (dim - mpg1->height * scale) / 2.0);
                ctx_scale(ctx, scale, scale);
                ctx_rectangle(ctx, 0, 2, dim, dim - 1);
                ctx_define_texture(
                    ctx, "!video", mpg1->width, mpg1->height, mpg1->width,
                    mpg1->had_chroma ? CTX_FORMAT_YUV420 : CTX_FORMAT_GRAY8,
                    mpg1->frame_data, NULL);
                ctx_image_smoothing(ctx, false);
                ctx_compositing_mode(ctx, CTX_COMPOSITE_COPY);
                ctx_fill(ctx);
            }
        } else {
            ctx_rgb(ctx, 0.2, 0.3, 0.4);
            ctx_fill(ctx);
        }
    }
}

static void mpg1_destroy(st3m_media *media) {
    mpg1_state *self = (void *)media;
    st3m_gfx_set_mode(self->old_mode);
    plm_destroy(self->plm);
    free(self->frame_data);
    free(self);
}

st3m_media *st3m_media_load_mpg1(const char *path) {
    mpg1_state *self = (mpg1_state *)malloc(sizeof(mpg1_state));
    memset(self, 0, sizeof(mpg1_state));
    self->control.draw = mpg1_draw;
    self->control.think = mpg1_think;
    self->control.destroy = mpg1_destroy;
    self->control.get = mpg1_get;
    self->control.set = mpg1_set;

    self->old_mode = st3m_gfx_get_mode();
    st3m_gfx_set_mode(16 | st3m_gfx_osd | st3m_gfx_EXPERIMENTAL_think_per_draw |
                      st3m_gfx_2x);
    self->plm = plm_create_with_filename(path);
    self->color = 1;
    self->had_chroma = 0;
    self->scale = 1.0;
    self->audio = 1;
    self->video = 1;
    self->loop = 0;
    self->width = 0;
    self->height = 0;
    self->frame_drop = 1;
    if ((!self->plm) || (plm_get_width(self->plm) == 0)) {
        printf("Couldn't open %s", path);
        free(self);
        return NULL;
    }

    self->sample_rate = plm_get_samplerate(self->plm);
    self->control.duration = plm_get_duration(self->plm);

    plm_set_video_decode_callback(self->plm, mpg1_on_video, self);
    plm_set_audio_decode_callback(self->plm, mpg1_on_audio, self);
    plm_set_video_enabled(self->plm, self->video);

    plm_set_loop(self->plm, self->loop);
    plm_set_audio_enabled(self->plm, self->audio);
    plm_set_audio_stream(self->plm, 0);

    if (plm_get_num_audio_streams(self->plm) > 0) {
        plm_set_audio_lead_time(self->plm, 0.05);
    }

    self->frame_data =
        (uint8_t *)malloc(plm_get_width(self->plm) * plm_get_height(self->plm) *
                          2);  // XXX : this is not quite right

    mpg1_think((st3m_media *)self, 0);  // the frame is constructed in think
    return (st3m_media *)self;
}
