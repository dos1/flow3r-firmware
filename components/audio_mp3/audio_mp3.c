#ifndef __clang__
#pragma GCC optimize("O2")
#endif

#include <fcntl.h>
#include <st3m_audio.h>
#include <st3m_media.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "lwip/igmp.h"
#include "lwip/ip4.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"

#include "ctx.h"

#define MINIMP3_NONSTANDARD_BUT_LOGICAL
#define MINIMP3_NO_SIMD
#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"

typedef struct {
    st3m_media control;
    mp3dec_t mp3d;
    char *path;
    char *artist;
    char *title;
    int year;
    int started;
    int samplerate;
    int channels;
    uint8_t *data;
    size_t size;
    size_t count;

    int pos;
    int offset;
    int buffer_size;

    int file_size;
    FILE *file;

    int socket;

    int in_buffering;
    float scroll_pos;
} mp3_state;

static int has_data(mp3_state *mp3) {
    if (mp3->file) return 1;
    fd_set rfds;
    struct timeval tv = { 0, 0 };
    FD_ZERO(&rfds);
    FD_SET(mp3->socket, &rfds);
    if (select(mp3->socket + 1, &rfds, NULL, NULL, &tv) == 1)
        return FD_ISSET(mp3->socket, &rfds);
    return 0;
}

static void mp3_fetch_data(mp3_state *mp3) {
    if (mp3->pos) {
        memmove(mp3->data, &mp3->data[mp3->pos], mp3->count - mp3->pos);
        mp3->offset += mp3->pos;
        mp3->count -= mp3->pos;
        mp3->pos = 0;
    }

    // if incoming data-buffer falls below 16kb - do a full buffer fill
    if (!mp3->file && (mp3->count < 16 * 1024)) {
        mp3->in_buffering = 1;
    }

    if ((mp3->size - mp3->count > 0) && has_data(mp3)) {
        int desire_bytes = (mp3->size - mp3->count);
        if (desire_bytes > 2048) desire_bytes = 2048;
        if (desire_bytes) {
            int read_bytes;
            if (mp3->file)
                read_bytes =
                    fread(mp3->data + mp3->count, 1, desire_bytes, mp3->file);
            else
                read_bytes =
                    read(mp3->socket, mp3->data + mp3->count, desire_bytes);
            mp3->count += read_bytes;
        }
    }
}

static void mp3_draw(st3m_media *media, Ctx *ctx) {
    mp3_state *self = (void *)media;

    ctx_rectangle(ctx, -120, -120, 240, 240);
    ctx_gray(ctx, 0);
    ctx_fill(ctx);
    ctx_rgb(ctx, 1.0, 1.0, 1.0);
    ctx_rectangle(ctx, -120, 0, 240, 1);
    ctx_rectangle(ctx, -120 + self->offset * 240.0 / self->file_size, -32, 2,
                  64);
    ctx_fill(ctx);
    ctx_font_size(ctx, 24);
    ctx_text_align(ctx, CTX_TEXT_ALIGN_CENTER);

    int xpos = 0;
    int str_width = ctx_text_width(ctx, self->artist);
    if (str_width > 200) {
        xpos = ctx_sinf(self->scroll_pos) * (str_width - 200) / 2;
    }
    ctx_move_to(ctx, xpos, -40);
    ctx_text(ctx, self->artist);

    str_width = ctx_text_width(ctx, self->title);
    xpos = 0;
    if (str_width > 200) {
        xpos = ctx_sinf(self->scroll_pos) * (str_width - 200) / 2;
    }
    ctx_move_to(ctx, xpos, 64);
    ctx_text(ctx, self->title);

    ctx_font_size(ctx, 14);
    str_width = ctx_text_width(ctx, self->path);
    xpos = 0;
    if (str_width > 220) {
        xpos = ctx_sinf(self->scroll_pos) * (str_width - 220) / 2;
    }
    ctx_move_to(ctx, xpos, 14);
    ctx_gray(ctx, 0.6);
    ctx_text(ctx, self->path);

    if (!self->file) {
        ctx_rectangle(ctx, -100, 65, self->count * 200.0 / self->size, 55);
        if (self->in_buffering)
            ctx_rgba(ctx, 0.8, 0.2, 0.0, 1.0);
        else
            ctx_gray(ctx, 0.2);
        ctx_fill(ctx);
    }
}

static void mp3_think(st3m_media *media, float ms_elapsed) {
    mp3_state *self = (void *)media;

    if (self->file && self->control.seek == 0) {
        rewind(self->file);
        self->offset = 0;
        self->control.time = 0;
        self->control.position = 0;
        self->control.seek = -1;
        mp3dec_init(&self->mp3d);
    }

    mp3_fetch_data(self);

    self->scroll_pos += ms_elapsed / 1000.0;

    if (self->in_buffering) {
        if (self->size - self->count > 0) return;
        self->in_buffering = 0;
    }

    if (self->control.paused) return;

    if (self->file) {
        self->control.position = self->offset;
        if (self->offset + 512 >= self->file_size)
            self->control.position = self->file_size;
    }

    if (!self->started) {
        self->started = 1;
        mp3_think(media, 100);
    }
    int samples_needed =
        ((AUDIO_BUF_SIZE - st3m_media_samples_queued()) / 2) - 2400;

    int samples;
    mp3dec_frame_info_t info = {
        0,
    };
    if (samples_needed > 0 &&
        ((self->offset + 512 < self->file_size) || (!self->file))) {
        do {
            int16_t rendered[MINIMP3_MAX_SAMPLES_PER_FRAME];
            samples =
                mp3dec_decode_frame(&self->mp3d, self->data + self->pos,
                                    self->count - self->pos, rendered, &info);
            self->samplerate = info.hz;
            self->channels = info.channels;
            if (info.frame_bytes > samples) {
                printf("[[%s]]\n", self->data + self->pos);
            }
            self->pos += info.frame_bytes;
            self->control.time += samples / (float)self->samplerate;

            if (self->samplerate != 48000) {
                int phase = 0;
                int fraction = ((48000.0 / self->samplerate) - 1.0) * 65536;
                if (info.channels == 1)
                    for (int i = 0; i < samples; i++) {
                    again1:
                        self->control.audio_buffer[self->control.audio_w++] =
                            rendered[i] / 2;
                        if (self->control.audio_w >= AUDIO_BUF_SIZE)
                            self->control.audio_w = 0;
                        phase += fraction;
                        if (phase > 65536) {
                            phase -= 65536;
                            phase -= fraction;
                            goto again1;
                        }
                    }
                else if (info.channels == 2) {
                    int phase = 0;
                    for (int i = 0; i < samples; i++) {
                    again2:
                        self->control.audio_buffer[self->control.audio_w++] =
                            rendered[i * 2] / 2;
                        if (self->control.audio_w >= AUDIO_BUF_SIZE)
                            self->control.audio_w = 0;
                        self->control.audio_buffer[self->control.audio_w++] =
                            rendered[i * 2 + 1] / 2;
                        if (self->control.audio_w >= AUDIO_BUF_SIZE)
                            self->control.audio_w = 0;

                        phase += fraction;
                        if (phase > 65536) {
                            phase -= 65536;
                            phase -= fraction;
                            goto again2;
                        }
                    }
                }
            } else {
                if (info.channels == 1)
                    for (int i = 0; i < samples; i++) {
                        self->control.audio_buffer[self->control.audio_w++] =
                            rendered[i];
                        if (self->control.audio_w >= AUDIO_BUF_SIZE)
                            self->control.audio_w = 0;
                        self->control.audio_buffer[self->control.audio_w++] =
                            rendered[i];
                        if (self->control.audio_w >= AUDIO_BUF_SIZE)
                            self->control.audio_w = 0;
                    }
                else if (info.channels == 2) {
                    for (int i = 0; i < samples; i++) {
                        self->control.audio_buffer[self->control.audio_w++] =
                            rendered[i * 2];
                        if (self->control.audio_w >= AUDIO_BUF_SIZE)
                            self->control.audio_w = 0;
                        self->control.audio_buffer[self->control.audio_w++] =
                            rendered[i * 2 + 1];
                        if (self->control.audio_w >= AUDIO_BUF_SIZE)
                            self->control.audio_w = 0;
                    }
                }
            }

            samples_needed -= (samples);
        } while (samples_needed > 0);
    }
}

static void mp3_destroy(st3m_media *media) {
    mp3_state *self = (void *)media;
    if (self->data) free(self->data);
    if (self->file) fclose(self->file);
    if (self->socket) {
        shutdown(self->socket, SHUT_RDWR);
        close(self->socket);
    }
    if (self->path) free(self->path);
    if (self->title) free(self->title);
    if (self->artist) free(self->artist);
    free(self);
}

typedef struct {
    char tag[3];
    char artist[30];
    char title[30];
    char year[4];
    char pad[128];
} id3tag_t;

st3m_media *st3m_media_load_mp3(const char *path) {
    mp3_state *self = (mp3_state *)malloc(sizeof(mp3_state));
    id3tag_t id3;
    memset(self, 0, sizeof(mp3_state));
    self->control.draw = mp3_draw;
    self->control.think = mp3_think;
    self->control.destroy = mp3_destroy;
    self->samplerate = 44100;
    self->buffer_size = 32 * 1024;
    self->scroll_pos = 0;

    if (!strncmp(path, "http://", 7)) {
        int port = 80;
        char *hostname = strdup(path + 7);
        char *rest = NULL;
        self->buffer_size = 96 * 1024;
        rest = strchr(hostname, '/') + 1;
        strchr(hostname, '/')[0] = 0;
        if (strchr(hostname, ':')) {
            port = atoi(strchr(hostname, ':') + 1);
            strchr(hostname, ':')[0] = 0;
        }

        struct hostent *host;
        struct sockaddr_in addr;
        self->socket = socket(PF_INET, SOCK_STREAM, 0);
        if (self->socket < 0) {
            free(hostname);
            free(self);
            return NULL;
        }
        int flag = 1;
        setsockopt(self->socket, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        host = gethostbyname(hostname);
        if (!host) {
            free(self);
            free(hostname);
            return NULL;
        }
        addr.sin_addr.s_addr = ((long unsigned int **)host->h_addr_list)[0][0];

        if (connect(self->socket, (struct sockaddr *)&addr, sizeof(addr)) ==
            0) {
            char s[1024];
            sprintf(s, "GET /%s HTTP/1.1\r\n", rest);
            write(self->socket, s, strlen(s));
            sprintf(s, "Range: bytes=0-\r\n");
            write(self->socket, s, strlen(s));
            if (hostname) {
                sprintf(s, "Host: %s\r\n", hostname);
                write(self->socket, s, strlen(s));
            }
            sprintf(s, "User-Agent: flow3r\r\n");
            write(self->socket, s, strlen(s));
            sprintf(s, "\r\n");
            write(self->socket, s, strlen(s));
            fsync(self->socket);

            self->data = malloc(self->buffer_size);
            self->size = self->buffer_size;

            mp3dec_init(&self->mp3d);
            self->control.duration = -1;
            free(hostname);
            self->in_buffering = 1;
            self->path = strdup(path);
            return (st3m_media *)self;
        }
        free(hostname);

        free(self);
        return NULL;
    }

    self->file = fopen(path, "r");
    fseek(self->file, 0, SEEK_END);
    self->file_size = ftell(self->file);
    fseek(self->file, self->file_size - 128, SEEK_SET);
    fread(&id3, 128, 1, self->file);
    if (id3.tag[0] == 'T' && id3.tag[1] == 'A' && id3.tag[2] == 'G') {
        self->title = strndup(id3.title, 30);
        while (self->title[strlen(self->title) - 1] == ' ')
            self->title[strlen(self->title) - 1] = 0;
        self->artist = strndup(id3.artist, 30);
        while (self->artist[strlen(self->artist) - 1] == ' ')
            self->artist[strlen(self->artist) - 1] = 0;
        self->year = atoi(id3.year);
    } else {
        self->artist = strdup("-");
        self->title = strdup(strrchr(path, '/') + 1);
    }
    self->path = strdup(path);
    rewind(self->file);

    self->data = malloc(self->buffer_size);
    self->size = self->buffer_size;
    if (!self->file) {
        free(self);
        return NULL;
    }
    mp3dec_init(&self->mp3d);
    self->control.duration = self->file_size;
    return (st3m_media *)self;
}
