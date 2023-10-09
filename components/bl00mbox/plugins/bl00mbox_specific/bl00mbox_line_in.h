#pragma once
#include "radspa.h"
#include "radspa_helpers.h"
// SPECIAL REQUIREMENTS
#include "bl00mbox_audio.h"

extern radspa_descriptor_t bl00mbox_line_in_desc;
radspa_t * bl00mbox_line_in_create(uint32_t init_var);
void bl00mbox_line_in_run(radspa_t * line_in, uint16_t num_samples, uint32_t render_pass_id);
