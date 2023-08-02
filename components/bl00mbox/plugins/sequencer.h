#pragma once
#include <math.h>
#include "radspa.h"

typedef struct {
    uint64_t counter;
    uint64_t target;
    uint16_t repeats;
    uint16_t repeats_counter;
    int16_t trigger_in_prev;
    int16_t trigger_out_prev;
    int16_t volume;
} sequencer_data_t;

extern radspa_descriptor_t sequencer_desc;
radspa_t * sequencer_create(uint32_t init_var);
void sequencer_run(radspa_t * osc, uint16_t num_samples, uint32_t render_pass_id);
