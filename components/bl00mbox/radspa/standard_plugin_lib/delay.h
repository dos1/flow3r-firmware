#pragma once
#include <radspa.h>
#include <radspa_helpers.h>

typedef struct {
    int32_t read_head_position;
    int32_t write_head_position;
    int32_t max_delay;
    int32_t time_prev;
} delay_data_t;

extern radspa_descriptor_t delay_desc;
radspa_t * delay_create(uint32_t init_var);
void delay_run(radspa_t * osc, uint16_t num_samples, uint32_t render_pass_id);

