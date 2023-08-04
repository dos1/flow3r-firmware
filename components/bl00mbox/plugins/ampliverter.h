#pragma once
#include <radspa.h>
#include <radspa_helpers.h>

extern radspa_descriptor_t ampliverter_desc;
radspa_t * ampliverter_create(uint32_t init_var);
void ampliverter_run(radspa_t * osc, uint16_t num_samples, uint32_t render_pass_id);

