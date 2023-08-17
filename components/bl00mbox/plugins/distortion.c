#include "distortion.h"

radspa_t * distortion_create(uint32_t init_var);
radspa_descriptor_t distortion_desc = {
    .name = "_distortion",
    .id = 9000,
    .description = "distortion with linear interpolation between int16 values in plugin_table[129]",
    .create_plugin_instance = distortion_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

#define DISTORTION_NUM_SIGNALS 2
#define DISTORTION_OUTPUT 0
#define DISTORTION_INPUT 1

void distortion_run(radspa_t * distortion, uint16_t num_samples, uint32_t render_pass_id){
    radspa_signal_t * output_sig = radspa_signal_get_by_index(distortion, DISTORTION_OUTPUT);
    if(output_sig->buffer == NULL) return;
    int16_t * dist = distortion->plugin_table;
    radspa_signal_t * input_sig = radspa_signal_get_by_index(distortion, DISTORTION_INPUT);

    static int32_t ret = 0;
    
    for(uint16_t i = 0; i < num_samples; i++){

        int32_t input = input_sig->get_value(input_sig, i, num_samples, render_pass_id);
        input += 32768;
        uint8_t index = input>>9;
        int32_t blend = input & ((1<<7)-1);
        ret = dist[index]*((1<<7)-blend) + dist[index+1]*blend;
        ret = ret >> 7;
        output_sig->set_value(output_sig, i, ret, num_samples, render_pass_id);
    }
}

radspa_t * distortion_create(uint32_t init_var){
    radspa_t * distortion = radspa_standard_plugin_create(&distortion_desc, DISTORTION_NUM_SIGNALS, 0, (1<<7) + 1);
    if(distortion == NULL) return NULL;
    distortion->render = distortion_run;
    radspa_signal_set(distortion, DISTORTION_OUTPUT, "output", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(distortion, DISTORTION_INPUT, "input", RADSPA_SIGNAL_HINT_INPUT, 0);

    // prefill table with mild saturation
    int16_t * dist = distortion->plugin_table;
    for(int32_t i = 0; i < ((1<<7)+1) ; i++){
        if(i < 64){
            dist[i] = ((i*i)*32767>>12) - 32767;
        } else {
            dist[i] = -((128-i)*(128-i)*32767>>12) + 32767;
        }
    }

    return distortion;
}
