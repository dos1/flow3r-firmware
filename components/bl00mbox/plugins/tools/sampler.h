#pragma once
#include <radspa.h>

typedef struct {
    uint32_t read_head_position;
    int16_t trigger_prev;
    int16_t volume;
} sampler_data_t;

extern radspa_descriptor_t sampler_desc;
radspa_t * sampler_create(uint32_t init_var);
void sampler_run(radspa_t * osc, uint16_t num_samples, uint32_t render_pass_id);

