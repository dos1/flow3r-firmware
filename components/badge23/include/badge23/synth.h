#pragma once
#include <stdint.h>
#include <stdio.h>

#define TRAD_OSC_DECAY_STEP          0.01
#define TRAD_OSC_ATTACK_STEP          0.01
#define TRAD_OSC_MIN_ATTACK_ENV          0.01
#define TRAD_OSC_ATTACK_POP_BLOCK          16
typedef struct {
    //user variables
    float       freq;           //in hertz, negative frequencies for linFM allowed
    float       bend;
    float       vol;            //output volume
    float       env;
    uint8_t     env_phase;      //0: off, 1: attack, 2: hold, 3: decay
    uint8_t     skip_hold;
    float       gate;           //below what volume the oscillator stops and returns 0
    uint16_t    decay_steps;    //after how many sample rate steps the volume reduces
                                //by factor TRAD_OSC_DECAY_STEP, set 0 for no decay
    uint16_t    attack_steps;
    uint8_t     waveform;       //0: sine, 1: fast sine, 2: tri, 3: saw,
                                //4: square, 5: 33% pulse, 6: 25% pulse

    //internal data storage, not for user access
    float       counter;        //state of central sawtooth oscillator, [-1..1] typ.
    uint16_t    env_counter;
    int8_t      overflow_event; //set to -1 when counter underflows (below -1),
                                //set to +1 when counter overflows (above 1)
                                //not reset or used by anything so far
    uint8_t     undersampling_counter;
    uint16_t    noise_reg;
} trad_osc_t;

float run_trad_osc(trad_osc_t * osc);
void trad_osc_set_freq_semitone(trad_osc_t * osc, float bend);
void trad_osc_set_freq_Hz(trad_osc_t * osc, float freq);
void trad_osc_set_waveform(trad_osc_t * osc, uint8_t waveform);
void trad_osc_set_attack(trad_osc_t * osc, uint16_t attack);
void trad_osc_set_decay(trad_osc_t * osc, uint16_t decay);
void trad_env_stop(trad_osc_t * osc);
void trad_env_fullstop(trad_osc_t * osc);
void trad_env_start(trad_osc_t * osc);
