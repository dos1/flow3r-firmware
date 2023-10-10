#pragma once
#include "radspa.h"
#include "radspa_helpers.h"

typedef struct {
    uint32_t    env_counter;
    uint32_t    env_counter_prev;
    int32_t     env_pre_gain;
    int16_t     attack_prev_ms;
    uint32_t    attack_raw;
    int16_t     decay_prev_ms;
    uint32_t    decay_raw;
    uint32_t    sustain;
    uint32_t    sustain_prev;
    int16_t     release_prev_ms;
    uint32_t    release_raw;
    uint32_t    release_init_val;
    uint32_t    release_init_val_prev;
    uint32_t    velocity;
    int16_t     trigger_prev;
    uint8_t     env_phase;
    radspa_signal_t * output_sig;
    radspa_signal_t * input_sig;
    radspa_signal_t * trigger_sig;
    radspa_signal_t * attack_sig;
    radspa_signal_t * decay_sig;
    radspa_signal_t * sustain_sig;
    radspa_signal_t * release_sig;
    radspa_signal_t * gain_sig;
} env_adsr_data_t;

extern radspa_descriptor_t env_adsr_desc;
radspa_t * env_adsr_create(uint32_t init_var);
void env_adsr_run(radspa_t * osc, uint16_t num_samples, uint32_t render_pass_id);

