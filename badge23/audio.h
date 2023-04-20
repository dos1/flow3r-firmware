#pragma once
#include <stdint.h>

#define SAMPLE_RATE 16000
#define BADGE_MAX_VOLUME_DB 20
#define BADGE_MIN_VOLUME_DB (-80)
#define BADGE_VOLUME_LIMIT 30000

void audio_init();
void play_bootsound();
void synth_set_freq(int i, float freq);
void synth_set_vol(int i, float vol);
void synth_set_bend(int i, float bend);
void synth_start(int i);
void synth_stop(int i);
void synth_fullstop(int i);
float synth_get_env(int i);

void set_global_vol_dB(int8_t vol_dB);
void set_extra_synth(void * synth);
void clear_extra_synth();
uint16_t count_audio_sources();

uint16_t add_audio_source(void * render_data, void * render_function);
void remove_audio_source(uint16_t index);
