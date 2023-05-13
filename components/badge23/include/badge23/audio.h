#pragma once
#include <stdint.h>

#define SAMPLE_RATE 16000
#define BADGE_MAX_VOLUME_DB 20
#define BADGE_MIN_VOLUME_DB (-80)
#define BADGE_VOLUME_LIMIT 30000

void audio_init();

void set_global_vol_dB(int8_t vol_dB);
uint16_t count_audio_sources();

uint16_t add_audio_source(void * render_data, void * render_function);
void remove_audio_source(uint16_t index);
