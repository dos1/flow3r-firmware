#pragma once
#include <stdint.h>
#include <stdio.h>

//#include "radspa.h"
//#include "bl00mbox.h"

#define SAMPLE_RATE 48000

#define TRAD_OSC_DECAY_STEP          0.01
#define TRAD_OSC_ATTACK_POP_BLOCK          16

#define TRAD_ENV_PHASE_OFF 0
#define TRAD_ENV_PHASE_ATTACK 1
#define TRAD_ENV_PHASE_DECAY 2
#define TRAD_ENV_PHASE_SUSTAIN 3
#define TRAD_ENV_PHASE_RELEASE 4

#define TRAD_OSC_WAVE_SINE 0
#define TRAD_OSC_WAVE_FAKE_SINE 1
#define TRAD_OSC_WAVE_TRI 2
#define TRAD_OSC_WAVE_SAW 3
#define TRAD_OSC_WAVE_SQUARE 4
#define TRAD_OSC_WAVE_PULSE 5
#define TRAD_OSC_WAVE_BLIP 6


typedef struct {
    uint32_t    env_counter;
    uint32_t    attack;
    uint32_t    decay;
    uint32_t    sustain;
    uint32_t    release;
    uint8_t     env_phase;
    uint8_t     skip_hold;
} trad_env_t;

typedef struct {
    //user variables

    //internal data storage, not for user access
    uint64_t    freq;           //in hertz, negative frequencies for linFM allowed
    uint64_t    bend;
    uint32_t    vol;            //output volume
    uint8_t     waveform;       //0: sine, 1: fast sine, 2: tri, 3: saw,
                                //4: square, 5: 33% pulse, 6: 25% pulse

    uint64_t    counter;     //state of central sawtooth oscillator.
    uint64_t    counter_prev;     //previous state of central sawtooth oscillator.
    int8_t      overflow_event; //set to -1 when counter underflows (below -1),
                                //set to +1 when counter overflows (above 1)
                                //not reset or used by anything so far
    uint8_t     undersampling_counter;
    int16_t       prev_output;           // for undersampling
    uint16_t    noise_reg;
    trad_env_t env;
} trad_osc_t;

int16_t run_trad_osc(trad_osc_t * osc);
void trad_osc_set_freq_semitone(trad_osc_t * osc, float bend);
void trad_osc_set_freq_Hz(trad_osc_t * osc, float freq);
void trad_osc_set_waveform(trad_osc_t * osc, uint8_t waveform);
void trad_osc_set_attack_ms(trad_osc_t * osc, float ms);
void trad_osc_set_decay_ms(trad_osc_t * osc, float ms);
void trad_osc_set_sustain(trad_osc_t * osc, float sus);
void trad_osc_set_release_ms(trad_osc_t * osc, float ms);
void trad_env_stop(trad_osc_t * osc);
void trad_env_fullstop(trad_osc_t * osc);
void trad_env_start(trad_osc_t * osc);

void trad_osc_set_vol(trad_osc_t * osc, float volume);
