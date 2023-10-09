#pragma once
#include <radspa.h>
#include <radspa_helpers.h>

extern radspa_descriptor_t distortion_desc;
radspa_t * distortion_create(uint32_t init_var);
void distortion_run(radspa_t * osc, uint16_t num_samples, uint32_t render_pass_id);

