#pragma once
#include "radspa.h"
#include "radspa_helpers.h"

typedef struct {
    uint32_t counter;
    int16_t prev_pitch;
    int32_t incr;
} oldschool_compressor_data_t;

extern radspa_descriptor_t oldschool_compressor_desc;
radspa_t * oldschool_compressor_create(uint32_t init_var);
void oldschool_compressor_run(radspa_t * osc, uint16_t num_samples, uint32_t render_pass_id);

