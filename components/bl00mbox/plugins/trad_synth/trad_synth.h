#pragma once
#include <math.h>
#include "radspa.h"

/* provides traditional synthesizer functionality distributed over several plugins.
 */

/* plugin: trad_osc
 * oscillator that can generate sine, square, saw and triangle waves in the audio band. uses trad_wave.
 */

typedef struct {
    uint32_t counter;
    int16_t prev_pitch;
    int32_t incr;
} trad_osc_data_t;

extern radspa_descriptor_t trad_osc_desc;
radspa_t * trad_osc_create(uint32_t init_var);
void trad_osc_run(radspa_t * osc, uint16_t num_samples, uint32_t render_pass_id);

typedef struct {
    uint32_t    env_counter;
    uint32_t    attack;
    uint32_t    decay;
    uint32_t    sustain;
    uint32_t    release;
    uint16_t    attack_prev_ms;
    uint16_t    decay_prev_ms;
    uint16_t    release_prev_ms;
    uint32_t    gate;
    uint32_t    velocity;
    uint8_t     env_phase;
    uint8_t     skip_hold;
    int16_t     trigger;
} trad_env_data_t;

extern radspa_descriptor_t trad_env_desc;
radspa_t * trad_env_create(uint32_t init_var);
void trad_env_run(radspa_t * osc, uint16_t num_samples, uint32_t render_pass_id);

