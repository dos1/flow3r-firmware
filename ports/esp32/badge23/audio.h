#pragma once
#include <stdint.h>

#define SAMPLE_RATE 16000

void audio_init();
void play_bootsound();
void synth_set_freq(int i, float freq);
void synth_set_vol(int i, float vol);
void synth_set_bend(int i, float bend);
void synth_start(int i);
void synth_stop(int i);
void synth_fullstop(int i);
float synth_get_env(int i);
