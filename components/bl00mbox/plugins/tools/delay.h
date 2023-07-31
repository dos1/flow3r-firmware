#pragma once
#include <radspa.h>

typedef struct {
    uint32_t read_head_position;
    uint32_t write_head_position;
    uint32_t max_delay;
    uint32_t time_prev;
} delay_data_t;

extern radspa_descriptor_t delay_desc;
radspa_t * delay_create(uint32_t init_var);
void delay_run(radspa_t * osc, uint16_t num_samples, uint32_t render_pass_id);

