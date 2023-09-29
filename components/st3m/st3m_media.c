#include "st3m_media.h"
#include "st3m_audio.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "esp_log.h"
#include "esp_task.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define ST3M_PCM_BUF_SIZE (16384)

static st3m_media *audio_media = NULL;
#ifdef CONFIG_FLOW3R_CTX_FLAVOUR_FULL

static TaskHandle_t media_task;
static bool media_pending_destroy = false;

static void st3m_media_task(void *_arg) {
    (void)_arg;
    TickType_t waketime = xTaskGetTickCount();
    while (!media_pending_destroy) {
        TickType_t lastwake = waketime;
        vTaskDelayUntil(&waketime, 20 / portTICK_PERIOD_MS);
        if (audio_media->think)
            audio_media->think(audio_media,
                               (waketime - lastwake) * portTICK_PERIOD_MS);
    }
    if (audio_media->destroy) audio_media->destroy(audio_media);
    audio_media = NULL;
    media_pending_destroy = false;
    vTaskDelete(NULL);
}

void st3m_media_stop(void) {
    if (!audio_media) return;
    media_pending_destroy = true;
    while (media_pending_destroy) {
        vTaskDelay(10);
    }
}

void st3m_media_pause(void) {
    if (!audio_media) return;
    audio_media->paused = 1;
}

void st3m_media_play(void) {
    if (!audio_media) return;
    audio_media->paused = 0;
}

int st3m_media_is_playing(void) {
    if (!audio_media) return 0;
    return !audio_media->paused;
}

float st3m_media_get_duration(void) {
    if (!audio_media) return 0;
    return audio_media->duration;
}

float st3m_media_get_position(void) {
    if (!audio_media) return 0;
    return audio_media->position;
}

float st3m_media_get_time(void) {
    if (!audio_media) return 0;
    if (audio_media->time <= 0) return audio_media->time;
    return audio_media->time - st3m_pcm_queued() / 48000.0 / 2.0;
}

void st3m_media_seek(float position) {
    if (!audio_media) return;
    audio_media->seek = position;
}

void st3m_media_seek_relative(float time) {
    if (!audio_media) return;
    st3m_media_seek((audio_media->position * audio_media->duration) + time);
}

void st3m_media_set_volume(float volume) { st3m_pcm_set_volume(volume); }

float st3m_media_get_volume(void) { return st3m_pcm_get_volume(); }

void st3m_media_draw(Ctx *ctx) {
    if (audio_media && audio_media->draw) audio_media->draw(audio_media, ctx);
}

void st3m_media_think(float ms) { (void)ms; }

char *st3m_media_get_string(const char *key) {
    if (!audio_media) return NULL;
    if (!audio_media->get_string) return NULL;
    return audio_media->get_string(audio_media, key);
}

float st3m_media_get(const char *key) {
    if (!audio_media || !audio_media->get_string) return -1.0f;
    return audio_media->get(audio_media, key);
}

void st3m_media_set(const char *key, float value) {
    if (!audio_media || !audio_media->set) return;
    return audio_media->set(audio_media, key, value);
}

st3m_media *st3m_media_load_mpg1(const char *path);
st3m_media *st3m_media_load_mod(const char *path);
st3m_media *st3m_media_load_mp3(const char *path);
st3m_media *st3m_media_load_txt(const char *path);
st3m_media *st3m_media_load_bin(const char *path);

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

int st3m_media_load(const char *path, bool paused) {
    struct stat statbuf;
#if 1
    if (!strncmp(path, "http://", 7)) {
        st3m_media_stop();
        audio_media = st3m_media_load_mp3(path);
    } else if (stat(path, &statbuf)) {
        st3m_media_stop();
        audio_media = st3m_media_load_txt(path);
    } else if (strstr(path, ".mp3") == strrchr(path, '.')) {
        st3m_media_stop();
        audio_media = st3m_media_load_mp3(path);
    } else
#endif
#if 1
        if (strstr(path, ".mpg")) {
        st3m_media_stop();
        audio_media = st3m_media_load_mpg1(path);
    } else
#endif
#if 1
        if ((strstr(path, ".mod") == strrchr(path, '.'))) {
        st3m_media_stop();
        audio_media = st3m_media_load_mod(path);
    } else
#endif
        if ((strstr(path, ".json") == strrchr(path, '.')) ||
            (strstr(path, ".txt") == strrchr(path, '.')) ||
            (strstr(path, "/README") == strrchr(path, '/')) ||
            (strstr(path, ".toml") == strrchr(path, '.')) ||
            (strstr(path, ".py") == strrchr(path, '.'))) {
        st3m_media_stop();
        audio_media = st3m_media_load_txt(path);
    }

    if (!audio_media) {
        audio_media = st3m_media_load_txt(path);
    }

    audio_media->volume = 4096;
    audio_media->paused = paused;

    BaseType_t res =
        xTaskCreatePinnedToCore(st3m_media_task, "media", 16384, NULL,
                                ESP_TASK_PRIO_MIN + 3, &media_task, 1);

    return res == pdPASS;
}

typedef struct {
    st3m_media control;
    char *data;
    size_t size;
    float scroll_pos;
    char *path;
} txt_state;

static void txt_destroy(st3m_media *media) {
    txt_state *self = (void *)media;
    if (self->data) free(self->data);
    if (self->path) free(self->path);
    free(self);
}

static void txt_draw(st3m_media *media, Ctx *ctx) {
    txt_state *self = (void *)media;
    ctx_rectangle(ctx, -120, -120, 240, 240);
    ctx_gray(ctx, 0);
    ctx_fill(ctx);
    ctx_gray(ctx, 1.0);
    ctx_move_to(ctx, -85, -70);
    ctx_font(ctx, "mono");
    ctx_font_size(ctx, 13.0);
    // ctx_text (ctx, self->path);
    ctx_text(ctx, self->data);
}

static void txt_think(st3m_media *media, float ms_elapsed) {
    // txt_state *self = (void*)media;
}

st3m_media *st3m_media_load_txt(const char *path) {
    txt_state *self = (txt_state *)malloc(sizeof(txt_state));
    memset(self, 0, sizeof(txt_state));
    self->control.draw = txt_draw;
    self->control.think = txt_think;
    self->control.destroy = txt_destroy;
    file_get_contents(path, (void *)&self->data, &self->size);
    if (!self->data) {
        self->data = malloc(strlen(path) + 64);
        sprintf(self->data, "40x - %s", path);
        self->size = strlen((char *)self->data);
    }
    self->path = strdup(path);
    return (void *)self;
}

st3m_media *st3m_media_load_bin(const char *path) {
    txt_state *self = (txt_state *)malloc(sizeof(txt_state));
    memset(self, 0, sizeof(txt_state));
    self->control.draw = txt_draw;
    self->control.destroy = txt_destroy;
    file_get_contents(path, (void *)&self->data, &self->size);
    if (!self->data) {
        self->data = malloc(strlen(path) + 64);
        sprintf(self->data, "40x - %s", path);
        self->size = strlen(self->data);
    }
    self->path = strdup(path);
    return (void *)self;
}

#endif
