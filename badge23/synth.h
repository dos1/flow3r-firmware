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
    uint16_t    noise_reg;
} trad_osc_t;

//#define KS_BUFFER_SIZE (SAMPLE_RATE)/20
#define KS_BUFFER_SIZE 800

typedef struct {
    //user variables
    float freq;                 //frequency in hertz, negative frequencies are rectified,
                                //minimum freq determined by KS_BUFFER_SIZE
    float feedback;             //feedback value, will be compensated with frequency
                                //for equal decay across spectrum, [-1..1] without
    
    //internal data storage, not for user access
    float tape[KS_BUFFER_SIZE]; //the delay chain
    float real_feedback;        //compensated feedback value
} ks_osc_t; //karplus strong

float trad_osc(trad_osc_t * osc);
