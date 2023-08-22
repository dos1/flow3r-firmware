#pragma once

#include <ctx.h>
#include <stdbool.h>
#include <stdint.h>

#define AUDIO_BUF_SIZE (8192)

typedef struct _st3m_media st3m_media;

struct _st3m_media {
    // set a tunable, to a numeric value - available tunables depends on
    // decoder
    void (*set)(st3m_media *media, const char *key, float value);
    // get a property/or tunable, defaulting to -1 for nonexisting keys
    float (*get)(st3m_media *media, const char *key);
    // get a string property/metadata, NULL if not existing or a string
    // to be freed
    char *(*get_string)(st3m_media *media, const char *key);

    // free resources used by this media instance
    void (*destroy)(st3m_media *media);

    // draw the current frame / visualization / metadata screen
    void (*draw)(st3m_media *media, Ctx *ctx);

    // do decoding work corresponding to passed time
    void (*think)(st3m_media *media, float ms);

    // pointer to global pcm output buffer
    int16_t *audio_buffer;
    // playback head
    int audio_r;
    // queuing/writing head
    int audio_w;

    // Duration of media in seconds or -1 for infinite/streaming media
    // at worst approximation of some unit, set by decoder.
    float duration;

    // currently played back position - set by decoder
    float position;

    // currently played back position, in seconds - set by decoder
    float time;  // time might be precise even when duration is not
                 // until first play through of some media, when time
                 // duration should also be set exact.

    // decoder should seek to this relative if not -1, and set it to -1
    float seek;

    // if set to 1 playback is momentarily stopped but can be resumed,
    // this is toggled by st3m_media_play | st3m_media_pause
    int paused;
};

// stops the currently playing media item
void st3m_media_stop(void);
// set a new media item
int st3m_media_load(const char *path_or_uri);

// decode current media item ms ahead (unless paused)
void st3m_media_think(float ms);

// draw the codecs own view of itself / its meta data - progress
// for video or animations formats this should draw the media-content
// other codecs can find mixes of debug visualizations.
void st3m_media_draw(Ctx *ctx);

// controls whether we are playing
void st3m_media_pause(void);
void st3m_media_play(void);
int st3m_media_is_playing(void);

// get duration in seconds
float st3m_media_get_duration(void);
// get current playback time in seconds
float st3m_media_get_time(void);
// get current playback position relative to overall duration
float st3m_media_get_position(void);
// seek to a position relative to overall duration
void st3m_media_seek(float position);
// seek a relative amount of seconds forward or with negative values back
void st3m_media_seek_relative(float seconds_jump);

// get decoder specific string or NULL if not existing, free returned value
//  common values:
//     "title"  "artist"
char *st3m_media_get_string(const char *key);
// get a decoder specific numeric value, defaulting to -1 for nonexisting values
float st3m_media_get(const char *key);
// set a decoder specific floating point value
// example posible/or already used values:
//    "scale"       0.0 - 1.0  - how large part of the screen to take up
//    "grayscale"   0 or 1     - drop color bits for performance
//    "smoothing"   0 or 1     - enable smooth texture scaling
void st3m_media_set(const char *key, float value);

// API for use in implementations
// query how manu audio samples have been queued in the pcm output buffer
int st3m_media_samples_queued(void);
