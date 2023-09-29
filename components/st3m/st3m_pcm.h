#pragma once

#include <stdbool.h>
#include <stdint.h>

// audio rendering function used by st3m_audio
bool st3m_pcm_audio_render(int16_t *rx, int16_t *tx, uint16_t len);

// query how many pcm samples have been queued for output
int st3m_pcm_queued(void);

// returns the internal PCM buffer length, this return the maximum number
// of samples that can be queued - the buffer might internally be larger.
int st3m_pcm_queue_length(void);

// queue signed 16bit samples, supports hz is up to 48000, ch 1 for mono
// or 2 for stereo
void st3m_pcm_queue_s16(int hz, int ch, int count, int16_t *data);

// queue 32bit float samples, supports hz is up to 48000 ch 1 for mono or 2
// for stereo
void st3m_pcm_queue_float(int hz, int ch, int count, float *data);

// set audio volume (0.0 - 1.0)
void st3m_pcm_set_volume(float volume);

// get audio volume
float st3m_pcm_get_volume(void);
