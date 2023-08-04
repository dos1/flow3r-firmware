#include "delay.h"

radspa_t * delay_create(uint32_t init_var);
radspa_descriptor_t delay_desc = {
    .name = "delay",
    .id = 42069,
    .description = "simple delay with ms input and feedback",
    .create_plugin_instance = delay_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

#define DELAY_NUM_SIGNALS 7
#define DELAY_OUTPUT 0
#define DELAY_INPUT 1
#define DELAY_TIME 2
#define DELAY_FEEDBACK 3
#define DELAY_LEVEL 4
#define DELAY_DRY_VOL 5
#define DELAY_REC_VOL 6

void delay_run(radspa_t * delay, uint16_t num_samples, uint32_t render_pass_id){
    radspa_signal_t * output_sig = radspa_signal_get_by_index(delay, DELAY_OUTPUT);
    if(output_sig->buffer == NULL) return;
    delay_data_t * data = delay->plugin_data;
    int16_t * buf = delay->plugin_table;
    radspa_signal_t * input_sig = radspa_signal_get_by_index(delay, DELAY_INPUT);
    radspa_signal_t * time_sig = radspa_signal_get_by_index(delay, DELAY_TIME);
    radspa_signal_t * feedback_sig = radspa_signal_get_by_index(delay, DELAY_FEEDBACK);
    radspa_signal_t * level_sig = radspa_signal_get_by_index(delay, DELAY_LEVEL);
    radspa_signal_t * dry_vol_sig = radspa_signal_get_by_index(delay, DELAY_DRY_VOL);
    radspa_signal_t * rec_vol_sig = radspa_signal_get_by_index(delay, DELAY_REC_VOL);

    static int16_t ret = 0;
    
    uint32_t buffer_size = delay->plugin_table_len;
    
    for(uint16_t i = 0; i < num_samples; i++){
        uint32_t time = radspa_signal_get_value(time_sig, i, num_samples, render_pass_id);
        if(time > data->max_delay) time = data->max_delay;

        data->write_head_position++;
        while(data->write_head_position >= buffer_size) data->write_head_position -= buffer_size; // maybe faster than %
        if(time != data->time_prev){
            data->read_head_position = time * (48000/1000) + data->write_head_position;
            data->time_prev = time;
        } else {
            data->read_head_position++;
        }
        while(data->read_head_position >= buffer_size) data->read_head_position -= buffer_size;

        //int16_t * buf = &(data->buffer);
    
        int16_t dry = radspa_signal_get_value(input_sig, i, num_samples, render_pass_id);
        int16_t wet = buf[data->read_head_position];

        int16_t fb = radspa_signal_get_value(feedback_sig, i, num_samples, render_pass_id);
        int16_t level = radspa_signal_get_value(level_sig, i, num_samples, render_pass_id);

        int16_t dry_vol = radspa_signal_get_value(dry_vol_sig, i, num_samples, render_pass_id);
        int16_t rec_vol = radspa_signal_get_value(rec_vol_sig, i, num_samples, render_pass_id);

        if(rec_vol){
            buf[data->write_head_position] = i16_add_sat(i16_mult_shift(rec_vol, dry), i16_mult_shift(wet,fb));
        }
        
        ret = i16_add_sat(i16_mult_shift(dry_vol,dry), i16_mult_shift(wet,level));

        (output_sig->buffer)[i] = ret;
    }
    output_sig->value = ret;
}

radspa_t * delay_create(uint32_t init_var){
    if(init_var == 0) init_var = 500;
    uint32_t buffer_size = init_var*(48000/1000);
    radspa_t * delay = radspa_standard_plugin_create(&delay_desc, DELAY_NUM_SIGNALS, sizeof(delay_data_t), buffer_size);

    if(delay == NULL) return NULL;
    delay_data_t * plugin_data = delay->plugin_data;
    plugin_data->max_delay = init_var;
    delay->render = delay_run;
    radspa_signal_set(delay, DELAY_OUTPUT, "output", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(delay, DELAY_INPUT, "input", RADSPA_SIGNAL_HINT_INPUT, 0);
    radspa_signal_set(delay, DELAY_TIME, "time", RADSPA_SIGNAL_HINT_INPUT, 200);
    radspa_signal_set(delay, DELAY_FEEDBACK, "feedback", RADSPA_SIGNAL_HINT_INPUT, 16000);
    radspa_signal_set(delay, DELAY_LEVEL, "level", RADSPA_SIGNAL_HINT_INPUT, 16000);
    radspa_signal_set(delay, DELAY_DRY_VOL, "dry_vol", RADSPA_SIGNAL_HINT_INPUT, 32767);
    radspa_signal_set(delay, DELAY_REC_VOL, "rec_vol", RADSPA_SIGNAL_HINT_INPUT, 32767);
    return delay;
}
