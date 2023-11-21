#include "distortion.h"

radspa_t * distortion_create(uint32_t init_var);
radspa_descriptor_t distortion_desc = {
    .name = "distortion",
    .id = 9000,
    .description = "distortion with linear interpolation between int16 values in plugin_table[129]",
    .create_plugin_instance = distortion_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

#define DISTORTION_NUM_SIGNALS 2
#define DISTORTION_OUTPUT 0
#define DISTORTION_INPUT 1

static inline int32_t distort(int32_t input, int16_t * dist, uint8_t lerp_glitch){
    // lerp glitch is a legacy feature from a math error of the 0th gen
    // pass 9 normally, pass 7 for legacy glitch
    input += 32768;
    uint8_t index = input >> 9;
    int32_t blend = input & ((1<<lerp_glitch)-1);
    int32_t ret = dist[index]*((1<<lerp_glitch)-blend) + dist[index+1]*blend;
    return ret >> lerp_glitch;
}

void distortion_run(radspa_t * distortion, uint16_t num_samples, uint32_t render_pass_id){
    radspa_signal_t * output_sig = radspa_signal_get_by_index(distortion, DISTORTION_OUTPUT);
    if(output_sig->buffer == NULL) return;
    int16_t * dist = distortion->plugin_table;
    radspa_signal_t * input_sig = radspa_signal_get_by_index(distortion, DISTORTION_INPUT);
    int32_t input = radspa_signal_get_const_value(input_sig, render_pass_id);
    uint8_t lerp_glitch = 9 - ((dist[129]) & 7); // legacy glitch
    if(input != RADSPA_SIGNAL_NONCONST){
        radspa_signal_set_const_value(output_sig, distort(input, dist, lerp_glitch));
    } else {
        for(uint16_t i = 0; i < num_samples; i++){
            int32_t input = radspa_signal_get_value(input_sig, i, render_pass_id);
            radspa_signal_set_value(output_sig, i, distort(input, dist, lerp_glitch));
        }
    }

}

radspa_t * distortion_create(uint32_t init_var){
    radspa_t * distortion = radspa_standard_plugin_create(&distortion_desc, DISTORTION_NUM_SIGNALS, 0, (1<<7) + 2);
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
    dist[129] = 0;

    return distortion;
}
