#pragma once
#include "radspa.h"
#include "radspa_helpers.h"

typedef struct {
    uint32_t counter;
    int16_t sync_in;
    int16_t sync_out;
    int16_t pitch_prev;
    uint32_t pitch_coeffs[1];
    int32_t waveform_prev;
    uint32_t waveform_coeffs[2];
    int32_t blep_coeffs[1];
    int32_t incr_prev;
    int32_t morph_prev;
    int32_t morph_coeffs[3];
    int16_t morph_gate_prev;
    bool morph_no_pwm_prev;
} osc_data_t;

extern radspa_descriptor_t osc_desc;
radspa_t * osc_create(uint32_t init_var);
void osc_run(radspa_t * osc, uint16_t num_samples, uint32_t render_pass_id);
