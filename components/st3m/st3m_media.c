#include "st3m_media.h"
#include "st3m_audio.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#ifdef CONFIG_FLOW3R_CTX_FLAVOUR_FULL
static st3m_media *audio_media = NULL;

// XXX should be refactored to either be temporary SPIRAM allocation
// or a static shared global pcm queuing API
//
static int16_t audio_buffer[AUDIO_BUF_SIZE];

void st3m_media_audio_out(int16_t *rx, int16_t *tx, uint16_t len) {
    if (!audio_media) return;
    for (int i = 0; i < len; i++) {
        if ((audio_media->audio_r + 1 != audio_media->audio_w) &&
            (audio_media->audio_r + 1 - AUDIO_BUF_SIZE !=
             audio_media->audio_w)) {
            tx[i] = audio_media->audio_buffer[audio_media->audio_r++];
            if (audio_media->audio_r >= AUDIO_BUF_SIZE)
                audio_media->audio_r = 0;
        } else
            tx[i] = 0;
    }
}
int st3m_media_samples_queued(void) {
    if (!audio_media) return 0;
    if (audio_media->audio_r > audio_media->audio_w)
        return (AUDIO_BUF_SIZE - audio_media->audio_r) + audio_media->audio_w;
    return audio_media->audio_w - audio_media->audio_r;
}

void st3m_media_stop(void) {
    if (audio_media && audio_media->destroy) audio_media->destroy(audio_media);
    audio_media = 0;
    st3m_audio_set_player_function(st3m_audio_player_function_dummy);
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
    return audio_media->time;
}

void st3m_media_seek(float position) {
    if (!audio_media) return;
    audio_media->seek = position;
}

void st3m_media_seek_relative(float time) {
    if (!audio_media) return;
    st3m_media_seek((audio_media->position * audio_media->duration) + time);
}

void st3m_media_draw(Ctx *ctx) {
    if (audio_media && audio_media->draw)
        audio_media->draw(audio_media, ctx);
}

void st3m_media_think(float ms) {
    if (audio_media && audio_media->think) audio_media->think(audio_media, ms);
}

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

int st3m_media_load(const char *path) {
    if (strstr(path, ".mpg")) {
        st3m_media_stop();
        audio_media = st3m_media_load_mpg1(path);
    } else if (strstr(path, ".mod")) {
        st3m_media_stop();
        audio_media = st3m_media_load_mod(path);
    } else if (strstr(path, "mp3") || !strncmp (path, "http://", 7)) {
        st3m_media_stop();
        audio_media = st3m_media_load_mp3(path);
    }

    if (!audio_media) return 0;

    st3m_audio_set_player_function(st3m_media_audio_out);
    audio_media->audio_buffer = audio_buffer;
    audio_media->audio_r = 0;
    audio_media->audio_w = 1;

    return 1;
}

#endif
