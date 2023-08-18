#include "sampler.h"

radspa_t * sampler_create(uint32_t init_var);
radspa_descriptor_t sampler_desc = {
    .name = "_sampler_ram",
    .id = 696969,
    .description = "simple sampler that stores a copy of the sample in ram and has basic recording functionality",
    .create_plugin_instance = sampler_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

#define SAMPLER_NUM_SIGNALS 4
#define SAMPLER_OUTPUT 0
#define SAMPLER_TRIGGER 1
#define SAMPLER_REC_IN 2
#define SAMPLER_REC_TRIGGER 3

void sampler_run(radspa_t * sampler, uint16_t num_samples, uint32_t render_pass_id){
    radspa_signal_t * output_sig = radspa_signal_get_by_index(sampler, SAMPLER_OUTPUT);
    if(output_sig->buffer == NULL) return;
    sampler_data_t * data = sampler->plugin_data;
    int16_t * buf = sampler->plugin_table;
    radspa_signal_t * trigger_sig = radspa_signal_get_by_index(sampler, SAMPLER_TRIGGER);
    radspa_signal_t * rec_in_sig = radspa_signal_get_by_index(sampler, SAMPLER_REC_IN);
    radspa_signal_t * rec_trigger_sig = radspa_signal_get_by_index(sampler, SAMPLER_REC_TRIGGER);

    static int32_t ret = 0;
    
    uint32_t buffer_size = sampler->plugin_table_len;
    
    for(uint16_t i = 0; i < num_samples; i++){
        int16_t rec_trigger = radspa_trigger_get(rec_trigger_sig->get_value(rec_trigger_sig, i, num_samples, render_pass_id), &(data->rec_trigger_prev));
        int16_t trigger = radspa_trigger_get(trigger_sig->get_value(trigger_sig, i, num_samples, render_pass_id), &(data->trigger_prev));

        if(rec_trigger > 0){
            if(!(data->rec_active)){
                data->read_head_position = data->sample_len; // reset sample player into off
                data->write_head_position = 0;
                data->sample_len = 0;
                data->rec_active = true;
            }
        } else if(rec_trigger < 0){
            if(data->rec_active){
                if(data->sample_len == buffer_size){
                    data->sample_start = data->write_head_position;
                } else {
                    data->sample_start = 0;
                }
                data->rec_active = false;
            }
        }

        if(data->rec_active){
            int16_t rec_in = rec_in_sig->get_value(rec_in_sig, i, num_samples, render_pass_id);
            buf[data->write_head_position] = rec_in;
            data->write_head_position++;
            if(data->write_head_position >= buffer_size) data->write_head_position = 0;
            if(data->sample_len < buffer_size) data->sample_len++;
        } else {
            if(trigger > 0){
                data->read_head_position = 0;
                data->volume = trigger;
            } else if(trigger < 0){
                data->read_head_position = data->sample_len;
            }

            if(data->read_head_position < data->sample_len){
                uint32_t sample_offset_pos = data->read_head_position + data->sample_start;
                if(sample_offset_pos >= data->sample_len) sample_offset_pos -= data->sample_len;
                ret = radspa_mult_shift(buf[sample_offset_pos], data->volume);
                data->read_head_position++;
            } else {
                //ret = (ret * 255)>>8; // avoid dc clicks with bad samples
                ret = 0;
            }
            output_sig->set_value(output_sig, i, ret, num_samples, render_pass_id);
        }
    }
}

radspa_t * sampler_create(uint32_t init_var){
    if(init_var == 0) return NULL; //doesn't make sense
    uint32_t buffer_size = init_var;
    radspa_t * sampler = radspa_standard_plugin_create(&sampler_desc, SAMPLER_NUM_SIGNALS, sizeof(sampler_data_t), buffer_size);
    if(sampler == NULL) return NULL;
    sampler->render = sampler_run;
    radspa_signal_set(sampler, SAMPLER_OUTPUT, "output", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(sampler, SAMPLER_TRIGGER, "trigger", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_TRIGGER, 0);
    radspa_signal_set(sampler, SAMPLER_REC_IN, "rec_in", RADSPA_SIGNAL_HINT_INPUT, 0);
    radspa_signal_set(sampler, SAMPLER_REC_TRIGGER, "rec_trigger", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_TRIGGER, 0);
    sampler_data_t * data = sampler->plugin_data;
    data->trigger_prev = 0;
    data->rec_trigger_prev = 0;
    data->rec_active = false;
    data->sample_start = 0;
    data->sample_len = sampler->plugin_table_len;
    return sampler;
}
