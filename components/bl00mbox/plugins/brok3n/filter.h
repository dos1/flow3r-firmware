#pragma once
#include <radspa.h>

typedef struct {
    int64_t in_history[3];
    int64_t in_coeff;
    int16_t in_coeff_shift;
    int64_t out_history[3];
    int64_t out_coeff[2];
    int16_t out_coeff_shift[2];

    int32_t prev_freq;
    int16_t prev_q;
    int8_t pos;
} filter_data_t;

extern radspa_descriptor_t filter_desc;
radspa_t* filter_create(uint32_t init_var);
void filter_run(radspa_t* osc, uint16_t num_samples, uint32_t render_pass_id);
