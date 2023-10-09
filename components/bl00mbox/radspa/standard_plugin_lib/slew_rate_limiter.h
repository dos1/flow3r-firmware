#pragma once
#include "radspa.h"
#include "radspa_helpers.h"

typedef struct {
    int32_t prev;
} slew_rate_limiter_data_t;

extern radspa_descriptor_t slew_rate_limiter_desc;
radspa_t * slew_rate_limiter_create(uint32_t init_var);
void slew_rate_limiter_run(radspa_t * osc, uint16_t num_samples, uint32_t render_pass_id);

