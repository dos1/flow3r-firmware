#pragma once
#include <math.h>
#include "radspa.h"
#include "radspa_helpers.h"

typedef struct {
    int16_t track_fill;
    int16_t trigger_hist;
} sequencer_track_data_t;

typedef struct {
    uint8_t num_tracks;
    uint16_t track_step_len;
    uint8_t step_target;
    uint8_t step;
    uint64_t counter;
    uint64_t counter_target;
    int16_t sync_in_prev;
    int16_t sync_out;
    int16_t bpm_prev;
    int16_t beat_div_prev;
    sequencer_track_data_t tracks[];
} sequencer_data_t;


extern radspa_descriptor_t sequencer_desc;
radspa_t * sequencer_create(uint32_t init_var);
void sequencer_run(radspa_t * osc, uint16_t num_samples, uint32_t render_pass_id);
