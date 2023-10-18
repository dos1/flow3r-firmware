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
#include "freertos/semphr.h"
#include "freertos/task.h"
#define ST3M_PCM_BUF_SIZE (16384)

static st3m_media *media_item = NULL;
#ifdef CONFIG_FLOW3R_CTX_FLAVOUR_FULL

static TaskHandle_t media_task;
static bool media_pending_destroy = false;
static SemaphoreHandle_t media_lock;

static void st3m_media_task(void *_arg) {
    (void)_arg;
    TickType_t wake_time = xTaskGetTickCount();
    TickType_t last_think = wake_time;
    while (!media_pending_destroy) {
        TickType_t ticks;
        vTaskDelayUntil(&wake_time, 20 / portTICK_PERIOD_MS);
        if (media_item->think) {
            if (xSemaphoreTake(media_lock, portMAX_DELAY)) {
                bool seeking = media_item->seek != -1;
                ticks = xTaskGetTickCount();
                media_item->think(media_item,
                                  (ticks - last_think) * portTICK_PERIOD_MS);
                last_think = ticks;
                if (seeking && media_item->seek == -1) {
                    // don't count seeking time into delta
                    last_think = xTaskGetTickCount();
                }
                xSemaphoreGive(media_lock);
            }
        }
        // don't starve lower priority tasks if think takes a long time
        if ((xTaskGetTickCount() - wake_time) * portTICK_PERIOD_MS > 10) {
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }
    if (media_item->destroy) media_item->destroy(media_item);
    media_item = NULL;
    vSemaphoreDelete(media_lock);
    media_pending_destroy = false;
    vTaskDelete(NULL);
}

void st3m_media_stop(void) {
    if (!media_item) return;
    media_pending_destroy = true;
    while (media_pending_destroy) {
        vTaskDelay(10);
    }
}

void st3m_media_pause(void) {
    if (!media_item) return;
    media_item->paused = 1;
}

void st3m_media_play(void) {
    if (!media_item) return;
    media_item->paused = 0;
}

bool st3m_media_is_playing(void) {
    if (!media_item) return 0;
    return !media_item->paused;
}

bool st3m_media_has_video(void) {
    if (!media_item) return false;
    return media_item->has_video;
}

bool st3m_media_has_audio(void) {
    if (!media_item) return false;
    return media_item->has_audio;
}

bool st3m_media_is_visual(void) {
    if (!media_item) return false;
    return media_item->is_visual;
}

float st3m_media_get_duration(void) {
    if (!media_item) return 0;
    return media_item->duration;
}

float st3m_media_get_position(void) {
    if (!media_item) return 0;
    return media_item->position;
}

float st3m_media_get_time(void) {
    if (!media_item) return 0;
    if (media_item->time <= 0) return media_item->time;
    return media_item->time - st3m_pcm_queued() / 48000.0 / 2.0;
}

void st3m_media_seek(float position) {
    if (!media_item) return;
    if (position < 0.0) position = 0.0f;
    media_item->seek = position;
}

void st3m_media_seek_relative(float time) {
    if (!media_item) return;
    st3m_media_seek((media_item->position * media_item->duration) + time);
}

void st3m_media_set_volume(float volume) { st3m_pcm_set_volume(volume); }

float st3m_media_get_volume(void) { return st3m_pcm_get_volume(); }

void st3m_media_draw(Ctx *ctx) {
    if (media_item && media_item->draw) {
        if (xSemaphoreTake(media_lock, portMAX_DELAY)) {
            media_item->draw(media_item, ctx);
            xSemaphoreGive(media_lock);
        }
    }
}

void st3m_media_think(float ms) { (void)ms; }

char *st3m_media_get_string(const char *key) {
    if (!media_item) return NULL;
    if (!media_item->get_string) return NULL;
    return media_item->get_string(media_item, key);
}

static float _st3m_media_zoom = 1.0f;
static float _st3m_media_cx = 0.0f;
static float _st3m_media_cy = 0.0f;

float st3m_media_get(const char *key) {
    if (!strcmp(key, "zoom")) return _st3m_media_zoom;
    if (!strcmp(key, "cx")) return _st3m_media_cx;
    if (!strcmp(key, "cy")) return _st3m_media_cy;
    if (!media_item || !media_item->get) return -1.0f;
    return media_item->get(media_item, key);
}

void st3m_media_set(const char *key, float value) {
    if (!strcmp(key, "zoom")) {
        _st3m_media_zoom = value;
        return;
    }
    if (!strcmp(key, "cx")) {
        _st3m_media_cx = value;
        return;
    }
    if (!strcmp(key, "cy")) {
        _st3m_media_cy = value;
        return;
    }
    if (!media_item || !media_item->set) return;
    return media_item->set(media_item, key, value);
}

st3m_media *st3m_media_load_mpg1(const char *path);
st3m_media *st3m_media_load_mod(const char *path);
st3m_media *st3m_media_load_gif(const char *path);
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

bool st3m_media_load(const char *path, bool paused) {
    struct stat statbuf;
    if (!strncmp(path, "http://", 7)) {
        st3m_media_stop();
        media_item = st3m_media_load_mp3(path);
    } else if (stat(path, &statbuf)) {
        st3m_media_stop();
        media_item = st3m_media_load_txt(path);
    } else if (strstr(path, ".mp3") == strrchr(path, '.')) {
        st3m_media_stop();
        media_item = st3m_media_load_mp3(path);
    } else if (strstr(path, ".mpg")) {
        st3m_media_stop();
        media_item = st3m_media_load_mpg1(path);
    } else if (strstr(path, ".gif")) {
        st3m_media_stop();
        media_item = st3m_media_load_gif(path);
    } else if ((strstr(path, ".mod") == strrchr(path, '.'))) {
        st3m_media_stop();
        media_item = st3m_media_load_mod(path);
    } else if ((strstr(path, ".json") == strrchr(path, '.')) ||
               (strstr(path, ".txt") == strrchr(path, '.')) ||
               (strstr(path, "/README") == strrchr(path, '/')) ||
               (strstr(path, ".toml") == strrchr(path, '.')) ||
               (strstr(path, ".py") == strrchr(path, '.'))) {
        st3m_media_stop();
        media_item = st3m_media_load_txt(path);
    }

    if (!media_item) {
        media_item = st3m_media_load_txt(path);
    }

    media_item->volume = 4096;
    media_item->paused = paused;

    media_lock = xSemaphoreCreateMutex();

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
