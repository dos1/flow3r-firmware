#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include "st3m_audio.h"
#include "st3m_scope.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

typedef struct _audio_source_t {
    void *render_data;
    int16_t (*render_function)(void *);
    uint16_t index;
    struct _audio_source_t *next;
} audio_source_t;

static audio_source_t *_audio_sources = NULL;

uint16_t bl00mbox_source_add(void *render_data, void *render_function) {
    // construct audio source struct
    audio_source_t *src = malloc(sizeof(audio_source_t));
    if (src == NULL) return 0;
    src->render_data = render_data;
    src->render_function = render_function;
    src->next = NULL;
    src->index = 0;

    // handle empty list special case
    if (_audio_sources == NULL) {
        _audio_sources = src;
        return 0;  // only nonempty lists from here on out!
    }

    // searching for lowest unused index
    audio_source_t *index_source = _audio_sources;
    while (1) {
        if (src->index == (index_source->index)) {
            src->index++;  // someone else has index already, try next
            index_source = _audio_sources;  // start whole list for new index
        } else {
            index_source = index_source->next;
        }
        if (index_source == NULL) {  // traversed the entire list
            break;
        }
    }

    audio_source_t *audio_source = _audio_sources;
    // append new source to linked list
    while (audio_source != NULL) {
        if (audio_source->next == NULL) {
            audio_source->next = src;
            break;
        } else {
            audio_source = audio_source->next;
        }
    }
    return src->index;
}

void bl00mbox_source_remove(uint16_t index) {
    audio_source_t *audio_source = _audio_sources;
    audio_source_t *start_gap = NULL;

    while (audio_source != NULL) {
        if (index == audio_source->index) {
            if (start_gap == NULL) {
                _audio_sources = audio_source->next;
            } else {
                start_gap->next = audio_source->next;
            }
            vTaskDelay(
                20 /
                portTICK_PERIOD_MS);  // give other tasks time to stop using
            free(audio_source);       // terrible hack tbh
            break;
        }
        start_gap = audio_source;
        audio_source = audio_source->next;
    }
}

uint16_t bl00mbox_sources_count() {
    uint16_t ret = 0;
    audio_source_t *audio_source = _audio_sources;
    while (audio_source != NULL) {
        audio_source = audio_source->next;
        ret++;
    }
    return ret;
}

void bl00mbox_player_function(int16_t *rx, int16_t *tx, uint16_t len) {
    int32_t acc[len];
    memset(acc, 0, len * sizeof(float));
    audio_source_t *audio_source = _audio_sources;
    while (audio_source != NULL) {
        for (uint16_t i = 0; i < len; i += 2) {
            acc[i] +=
                (*(audio_source->render_function))(audio_source->render_data);
        }
        audio_source = audio_source->next;
    }

    for (uint16_t i = 0; i < len; i += 2) {
        st3m_scope_write((acc[i]) >> 4);

        acc[i] = acc[i] >> 3;

        if (acc[i] > 32767) acc[i] = 32767;
        if (acc[i] < -32767) acc[i] = -32767;
        tx[i] = acc[i];
        tx[i + 1] = acc[i];
    }
}

void bl00mbox_init() {
    st3m_audio_set_player_function(bl00mbox_player_function);
}
