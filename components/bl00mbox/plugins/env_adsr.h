#pragma once
#include "radspa.h"
#include "radspa_helpers.h"

typedef struct {
    uint32_t    env_counter;
    uint32_t    attack;
    uint32_t    decay;
    uint32_t    sustain;
    uint32_t    release;
    uint32_t    release_init_val;
    uint16_t    attack_prev_ms;
    uint16_t    decay_prev_ms;
    uint16_t    release_prev_ms;
    uint32_t    gate;
    uint32_t    velocity;
    uint8_t     env_phase;
    uint8_t     skip_hold;
    int16_t     trigger_prev;
} env_adsr_data_t;

extern radspa_descriptor_t env_adsr_desc;
radspa_t * env_adsr_create(uint32_t init_var);
void env_adsr_run(radspa_t * osc, uint16_t num_samples, uint32_t render_pass_id);

