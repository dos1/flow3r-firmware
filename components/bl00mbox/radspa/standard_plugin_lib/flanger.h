#pragma once
#include <radspa.h>
#include <radspa_helpers.h>

typedef struct {
    uint32_t write_head_position;
    int32_t read_head_position;
    int32_t read_head_offset;
    int32_t manual_prev;
    int32_t decay_reso;
    int16_t abs_decay_prev;
} flanger_data_t;

extern radspa_descriptor_t flanger_desc;
radspa_t * flanger_create(uint32_t init_var);
void flanger_run(radspa_t * osc, uint16_t num_samples, uint32_t render_pass_id);

