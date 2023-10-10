#pragma once
#include "radspa.h"
#include "radspa_helpers.h"

typedef struct {
    uint32_t counter;
    int16_t prev_pitch;
    int32_t incr;
    radspa_signal_t * output_sig;
    radspa_signal_t * pitch_sig;
    radspa_signal_t * waveform_sig;
    radspa_signal_t * lin_fm_sig;
    radspa_signal_t * fm_pitch_thru_sig;
    radspa_signal_t * fm_pitch_offset_sig;
} osc_fm_data_t;

extern radspa_descriptor_t osc_fm_desc;
radspa_t * osc_fm_create(uint32_t init_var);
void osc_fm_run(radspa_t * osc, uint16_t num_samples, uint32_t render_pass_id);

