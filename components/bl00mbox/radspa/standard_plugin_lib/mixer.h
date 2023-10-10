#pragma once
#include <radspa.h>
#include <radspa_helpers.h>

typedef struct {
    uint8_t num_inputs;
    radspa_signal_t * output_sig;
    radspa_signal_t * gain_sig;
    radspa_signal_t * input_sigs[];
} mixer_data_t;

extern radspa_descriptor_t mixer_desc;
radspa_t * mixer_create(uint32_t init_var);
void mixer_run(radspa_t * osc, uint16_t num_samples, uint32_t render_pass_id);

