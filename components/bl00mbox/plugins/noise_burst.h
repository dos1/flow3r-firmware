#pragma once
#include "radspa.h"
#include "radspa_helpers.h"

typedef struct {
    int32_t counter;
    int32_t limit;
    int16_t trigger_prev;    
} noise_burst_data_t;

extern radspa_descriptor_t noise_burst_desc;
radspa_t * noise_burst_create(uint32_t init_var);
void noise_burst_run(radspa_t * osc, uint16_t num_samples, uint32_t render_pass_id);

