#include "range_shifter.h"

radspa_t * range_shifter_create(uint32_t init_var);
radspa_descriptor_t range_shifter_desc = {
    .name = "range_shifter",
    .id = 69,
    .description = "saturating multiplication and addition",
    .create_plugin_instance = range_shifter_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

#define RANGE_SHIFTER_NUM_SIGNALS 7
#define RANGE_SHIFTER_OUTPUT 0
#define RANGE_SHIFTER_OUTPUT_A 1
#define RANGE_SHIFTER_OUTPUT_B 2
#define RANGE_SHIFTER_INPUT 3
#define RANGE_SHIFTER_INPUT_A 4
#define RANGE_SHIFTER_INPUT_B 5
#define RANGE_SHIFTER_SPEED 6


#define GET_GAIN { \
    output_a = radspa_signal_get_value(output_a_sig, k, render_pass_id); \
    output_b = radspa_signal_get_value(output_b_sig, k, render_pass_id); \
    input_a = radspa_signal_get_value(input_a_sig, k, render_pass_id); \
    input_b = radspa_signal_get_value(input_b_sig, k, render_pass_id); \
    output_span = output_b - output_a; \
    input_span = input_b - input_a; \
    gain = (output_span << 14) / input_span; \
}

#define APPLY_GAIN { \
    if(ret == input_a){ \
        ret = output_a; \
    } else if(ret == input_b){ \
        ret = output_b; \
    } else { \
        ret -= input_a; \
        ret = (ret * gain) >> 14; \
        ret += output_a; \
    } \
}

void range_shifter_run(radspa_t * range_shifter, uint16_t num_samples, uint32_t render_pass_id){
    radspa_signal_t * output_sig = radspa_signal_get_by_index(range_shifter, RANGE_SHIFTER_OUTPUT);
    radspa_signal_t * output_a_sig = radspa_signal_get_by_index(range_shifter, RANGE_SHIFTER_OUTPUT_A);
    radspa_signal_t * output_b_sig = radspa_signal_get_by_index(range_shifter, RANGE_SHIFTER_OUTPUT_B);
    radspa_signal_t * input_sig = radspa_signal_get_by_index(range_shifter, RANGE_SHIFTER_INPUT);
    radspa_signal_t * input_a_sig = radspa_signal_get_by_index(range_shifter, RANGE_SHIFTER_INPUT_A);
    radspa_signal_t * input_b_sig = radspa_signal_get_by_index(range_shifter, RANGE_SHIFTER_INPUT_B);
    radspa_signal_t * speed_sig = radspa_signal_get_by_index(range_shifter, RANGE_SHIFTER_SPEED);
    int16_t speed = radspa_signal_get_value(speed_sig, 0, render_pass_id);
    int32_t output_a = radspa_signal_get_const_value(output_a_sig, render_pass_id);
    int32_t output_b = radspa_signal_get_const_value(output_b_sig, render_pass_id);
    int32_t input_a = radspa_signal_get_const_value(input_a_sig, render_pass_id);
    int32_t input_b = radspa_signal_get_const_value(input_b_sig, render_pass_id);
    int32_t input = radspa_signal_get_const_value(input_sig, render_pass_id);

    bool range_const = true;
    if(speed >= 10922){
        range_const = range_const && (output_a != RADSPA_SIGNAL_NONCONST) && (output_b != RADSPA_SIGNAL_NONCONST);  
        range_const = range_const && (input_a != RADSPA_SIGNAL_NONCONST) && (input_b != RADSPA_SIGNAL_NONCONST);
    }
    bool input_const = true;
    if(speed > -10922){
        bool input_const = input != RADSPA_SIGNAL_NONCONST;
    }

    int32_t output_span;
    int32_t input_span;
    int32_t gain;
    if(range_const){
        uint16_t k = 0;
        GET_GAIN
        if(!output_span){
            radspa_signal_set_const_value(output_sig, output_a);
        } else if(!input_span){
            radspa_signal_set_const_value(output_sig, (output_b - output_a)>>1);
        } else if(input_const){
            int32_t ret = radspa_signal_get_value(input_sig, 0, render_pass_id);
            APPLY_GAIN
            radspa_signal_set_const_value(output_sig, ret);
        } else {
            for(uint16_t i = 0; i < num_samples; i++){
                int32_t ret = radspa_signal_get_value(input_sig, i, render_pass_id);
                APPLY_GAIN
                radspa_signal_set_value(output_sig, i, ret);
            }
        }
    } else {
        for(uint16_t i = 0; i < num_samples; i++){
            uint16_t k = i;
            GET_GAIN
            if(!output_span){
                radspa_signal_set_value(output_sig, i, output_a);
            } else if(!input_span){
                radspa_signal_set_value(output_sig, i, (output_b - output_a)>>1);
            } else {
                int32_t ret = radspa_signal_get_value(input_sig, i, render_pass_id);
                APPLY_GAIN
                radspa_signal_set_value(output_sig, i, ret);
            }
        }
    }
}

radspa_t * range_shifter_create(uint32_t init_var){
    radspa_t * range_shifter = radspa_standard_plugin_create(&range_shifter_desc, RANGE_SHIFTER_NUM_SIGNALS, sizeof(char), 0);
    if(range_shifter == NULL) return NULL;
    range_shifter->render = range_shifter_run;
    radspa_signal_set(range_shifter, RANGE_SHIFTER_OUTPUT, "output", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set_group(range_shifter, 2, 1, RANGE_SHIFTER_OUTPUT_A, "output_range", RADSPA_SIGNAL_HINT_INPUT, -32767);
    radspa_signal_set(range_shifter, RANGE_SHIFTER_INPUT, "input", RADSPA_SIGNAL_HINT_INPUT, 0);
    radspa_signal_set_group(range_shifter, 2, 1, RANGE_SHIFTER_INPUT_A, "input_range", RADSPA_SIGNAL_HINT_INPUT, -32767);
    radspa_signal_set(range_shifter, RANGE_SHIFTER_SPEED, "speed", RADSPA_SIGNAL_HINT_INPUT, 32767);
    radspa_signal_get_by_index(range_shifter, RANGE_SHIFTER_SPEED)->unit = "{SLOW:-32767} {SLOW_RANGE:0} {FAST:32767}";
    range_shifter->signals[RANGE_SHIFTER_OUTPUT_B].value = 32767;
    range_shifter->signals[RANGE_SHIFTER_INPUT_B].value = 32767;
    return range_shifter;
}
