#pragma once
#include <radspa.h>
#include <radspa_helpers.h>

extern radspa_descriptor_t noise_desc;
radspa_t * noise_create(uint32_t init_var);
void noise_run(radspa_t * osc, uint16_t num_samples, uint32_t render_pass_id);

