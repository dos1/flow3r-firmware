#include "range_shifter.h"

radspa_t * range_shifter_create(uint32_t init_var);
radspa_descriptor_t range_shifter_desc = {
    .name = "range_shifter",
    .id = 68,
    .description = "saturating multiplication and addition",
    .create_plugin_instance = range_shifter_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

#define RANGE_SHIFTER_NUM_SIGNALS 6
#define RANGE_SHIFTER_OUTPUT 0
#define RANGE_SHIFTER_INPUT 1
#define RANGE_SHIFTER_OUTPUT_A 2
#define RANGE_SHIFTER_OUTPUT_B 3
#define RANGE_SHIFTER_INPUT_A 2
#define RANGE_SHIFTER_INPUT_B 3

void range_shifter_run(radspa_t * range_shifter, uint16_t num_samples, uint32_t render_pass_id){
    radspa_signal_t * output_sig = radspa_signal_get_by_index(range_shifter, RANGE_SHIFTER_OUTPUT);
    if(output_sig->buffer == NULL) return;
    radspa_signal_t * input_sig = radspa_signal_get_by_index(range_shifter, RANGE_SHIFTER_INPUT);
    int32_t output_a = radspa_signal_get_value(radspa_signal_get_by_index(range_shifter, RANGE_SHIFTER_OUTPUT_A), 0, render_pass_id);
    int32_t output_b = radspa_signal_get_value(radspa_signal_get_by_index(range_shifter, RANGE_SHIFTER_OUTPUT_B), 0, render_pass_id);
    int32_t input_a = radspa_signal_get_value(radspa_signal_get_by_index(range_shifter, RANGE_SHIFTER_INPUT_A), 0, render_pass_id);
    int32_t input_b = radspa_signal_get_value(radspa_signal_get_by_index(range_shifter, RANGE_SHIFTER_INPUT_B), 0, render_pass_id);
    int32_t output_span = output_b - output_a;
    if(!output_span){
        for(uint16_t i = 0; i < num_samples; i++){
            radspa_signal_set_value(output_sig, i, output_a);
        }
        return;
    }
    int32_t input_span = input_b - input_a;
    if(!input_span){
        int32_t avg = (output_b - output_a)/2;
        for(uint16_t i = 0; i < num_samples; i++){
            radspa_signal_set_value(output_sig, i, avg);
        }
        return;
    }
    int32_t gain = (output_span << 14) / input_span;

    for(uint16_t i = 0; i < num_samples; i++){
        int32_t ret = radspa_signal_get_value(input_sig, i, render_pass_id);
        if(ret == input_a){
            ret = output_a;
        } else if(ret == input_b){
            ret = output_b;
        } else {
            ret -= input_a;
            ret = (ret * gain) >> 14;
            ret += output_a;
            if(ret > 32767){
                ret = 32767;
            } else if(ret < -32767){
                ret = -32767;
            }
        }
        radspa_signal_set_value(output_sig, i, ret);
    }
}

radspa_t * range_shifter_create(uint32_t init_var){
    radspa_t * range_shifter = radspa_standard_plugin_create(&range_shifter_desc, RANGE_SHIFTER_NUM_SIGNALS, sizeof(char), 0);
    if(range_shifter == NULL) return NULL;
    range_shifter->render = range_shifter_run;
    radspa_signal_set(range_shifter, RANGE_SHIFTER_OUTPUT, "output", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(range_shifter, RANGE_SHIFTER_INPUT, "input", RADSPA_SIGNAL_HINT_INPUT, 0);
    radspa_signal_set(range_shifter, RANGE_SHIFTER_OUTPUT_A, "output_a", RADSPA_SIGNAL_HINT_INPUT, -32767);
    radspa_signal_set(range_shifter, RANGE_SHIFTER_OUTPUT_B, "output_b", RADSPA_SIGNAL_HINT_INPUT, 32767);
    radspa_signal_set(range_shifter, RANGE_SHIFTER_INPUT_A, "input_a", RADSPA_SIGNAL_HINT_INPUT, -32767);
    radspa_signal_set(range_shifter, RANGE_SHIFTER_INPUT_B, "input_b", RADSPA_SIGNAL_HINT_INPUT, 32767);
    return range_shifter;
}
