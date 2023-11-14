#pragma once
#include <radspa.h>
#include <radspa_helpers.h>

typedef struct {
    int32_t in_coeffs[3];
    int32_t out_coeffs[3]; // 0th entry is a dummy and never used, just for comfy indexing
    int32_t in_history[3];
    int32_t out_history[3];
    int8_t pos;

    int16_t pitch_prev;
    int16_t reso_prev;
    int16_t mode_prev;
    int16_t const_output;

    int32_t cos_omega;
    int32_t alpha;
    int32_t inv_norm;
} filter_data_t;

extern radspa_descriptor_t filter_desc;
radspa_t * filter_create(uint32_t init_var);
void filter_run(radspa_t * osc, uint16_t num_samples, uint32_t render_pass_id);
