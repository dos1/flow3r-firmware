#include "sampler.h"

radspa_t * sampler_create(uint32_t init_var);
radspa_descriptor_t sampler_desc = {
    .name = "_sampler_ram",
    .id = 696969,
    .description = "simple sampler that stores a copy of the sample in ram",
    .create_plugin_instance = sampler_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

#define SAMPLER_NUM_SIGNALS 2
#define SAMPLER_OUTPUT 0
#define SAMPLER_TRIGGER 1

void sampler_run(radspa_t * sampler, uint16_t num_samples, uint32_t render_pass_id){
    radspa_signal_t * output_sig = radspa_signal_get_by_index(sampler, SAMPLER_OUTPUT);
    if(output_sig->buffer == NULL) return;
    sampler_data_t * data = sampler->plugin_data;
    int16_t * buf = sampler->plugin_table;
    radspa_signal_t * trigger_sig = radspa_signal_get_by_index(sampler, SAMPLER_TRIGGER);

    static int32_t ret = 0;
    
    uint32_t buffer_size = sampler->plugin_table_len;
    
    for(uint16_t i = 0; i < num_samples; i++){

        int16_t trigger = trigger_sig->get_value(trigger_sig, i, num_samples, render_pass_id);
        
        int16_t vel = radspa_trigger_get(trigger, &(data->trigger_prev));

        if(vel > 0){
            data->read_head_position = 0;
            data->volume = vel;
        } else if(vel < 0){
            data->read_head_position = buffer_size;
        }

        if(data->read_head_position < buffer_size){
            ret = radspa_mult_shift(buf[data->read_head_position], data->volume);
            data->read_head_position++;
        } else {
            //ret = (ret * 255)>>8; // avoid dc clicks with bad samples
            ret = 0;
        }

        (output_sig->buffer)[i] = ret;
    }
    output_sig->value = ret;
}

radspa_t * sampler_create(uint32_t init_var){
    if(init_var == 0) return NULL; //doesn't make sense
    uint32_t buffer_size = init_var;
    radspa_t * sampler = radspa_standard_plugin_create(&sampler_desc, SAMPLER_NUM_SIGNALS, sizeof(sampler_data_t), buffer_size);
    if(sampler == NULL) return NULL;
    sampler->render = sampler_run;
    radspa_signal_set(sampler, SAMPLER_OUTPUT, "output", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(sampler, SAMPLER_TRIGGER, "trigger", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_TRIGGER, 0);
    return sampler;
}
