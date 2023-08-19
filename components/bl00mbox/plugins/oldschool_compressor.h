#pragma once
#include "radspa.h"
#include "radspa_helpers.h"

typedef struct {
    int32_t vca_gain;
    int32_t div_prev;
    int32_t env;
} oldschool_compressor_data_t;

extern radspa_descriptor_t oldschool_compressor_desc;
radspa_t * oldschool_compressor_create(uint32_t init_var);
void oldschool_compressor_run(radspa_t * osc, uint16_t num_samples, uint32_t render_pass_id);

