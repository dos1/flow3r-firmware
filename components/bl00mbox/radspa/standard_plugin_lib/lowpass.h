#pragma once
#include <radspa.h>
#include <radspa_helpers.h>

typedef struct {
    int32_t in_history[3];
    int32_t in_coeff;
    int32_t out_history[3];
    int32_t out_coeff[2];

    int32_t prev_freq;
    int16_t prev_q;
    int8_t pos;
} lowpass_data_t;

extern radspa_descriptor_t lowpass_desc;
radspa_t * lowpass_create(uint32_t init_var);
void lowpass_run(radspa_t * osc, uint16_t num_samples, uint32_t render_pass_id);
