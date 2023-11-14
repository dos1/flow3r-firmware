#include "mixer.h"

radspa_descriptor_t mixer_desc = {
    .name = "mixer",
    .id = 21,
    .description = "sums input and applies output gain\n  init_var: number of inputs, 1-127, default 4",
    .create_plugin_instance = mixer_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

void mixer_run(radspa_t * mixer, uint16_t num_samples, uint32_t render_pass_id){
    mixer_data_t * data = mixer->plugin_data;

    int32_t ret[num_samples];
    bool ret_init = false;

    radspa_signal_t * input_sigs[data->num_inputs];
    radspa_signal_t * input_gain_sigs[data->num_inputs];

    for(uint8_t j = 0; j < data->num_inputs; j++){
        input_sigs[j] = radspa_signal_get_by_index(mixer, 3+2*j);
        input_gain_sigs[j] = radspa_signal_get_by_index(mixer, 4+2*j);
    }

    for(uint8_t j = 0; j < data->num_inputs; j++){
        int32_t input_gain_const = radspa_signal_get_const_value(input_gain_sigs[j], render_pass_id);
        int32_t input_const = radspa_signal_get_const_value(input_sigs[j], render_pass_id);
        if(!(input_const && input_gain_const)) continue; // if either is zero there's nothing to do

        if(input_gain_const == RADSPA_SIGNAL_VAL_UNITY_GAIN){
            if(ret_init){
                for(uint16_t i = 0; i < num_samples; i++){
                    ret[i] += radspa_signal_get_value(input_sigs[j], i, render_pass_id);
                }
            } else {
                for(uint16_t i = 0; i < num_samples; i++){
                    ret[i] = radspa_signal_get_value(input_sigs[j], i, render_pass_id);
                }
                ret_init = true;
            }
        } else if(input_gain_const == -RADSPA_SIGNAL_VAL_UNITY_GAIN){
            if(ret_init){
                for(uint16_t i = 0; i < num_samples; i++){
                    ret[i] -= radspa_signal_get_value(input_sigs[j], i, render_pass_id);
                }
            } else {
                for(uint16_t i = 0; i < num_samples; i++){
                    ret[i] = -radspa_signal_get_value(input_sigs[j], i, render_pass_id);
                }
                ret_init = true;
            }
        } else if(input_gain_const == RADSPA_SIGNAL_NONCONST){
            if(ret_init){
                for(uint16_t i = 0; i < num_samples; i++){
                    int32_t input_gain = radspa_signal_get_value(input_gain_sigs[j], i, render_pass_id);
                    ret[i] += (input_gain * radspa_signal_get_value(input_sigs[j], i, render_pass_id)) >> 12;
                }
            } else {
                for(uint16_t i = 0; i < num_samples; i++){
                    int32_t input_gain = radspa_signal_get_value(input_gain_sigs[j], i, render_pass_id);
                    ret[i] = (input_gain * radspa_signal_get_value(input_sigs[j], i, render_pass_id)) >> 12;
                }
                ret_init = true;
            }
        } else {
            if(ret_init){
                for(uint16_t i = 0; i < num_samples; i++){
                    ret[i] += (input_gain_const * radspa_signal_get_value(input_sigs[j], i, render_pass_id)) >> 12;
                }
            } else {
                for(uint16_t i = 0; i < num_samples; i++){
                    ret[i] = (input_gain_const * radspa_signal_get_value(input_sigs[j], i, render_pass_id)) >> 12;
                }
                ret_init = true;
            }
        }
    }

    radspa_signal_t * output_sig = radspa_signal_get_by_index(mixer, 0);
    radspa_signal_t * gain_sig = radspa_signal_get_by_index(mixer, 1);
    radspa_signal_t * block_dc_sig = radspa_signal_get_by_index(mixer, 2);
    bool block_dc = radspa_signal_get_value(block_dc_sig, 0, render_pass_id) > 0;

    if(block_dc){
        if(ret_init){
            for(uint16_t i = 0; i < num_samples; i++){
                bool invert = data->dc < 0;
                if(invert) data->dc = -data->dc;
                data->dc = ((uint64_t) data->dc * (((1<<12) - 1)<<20)) >> 32;
                if(invert) data->dc = -data->dc;
                data->dc += ret[i];
                ret[i] -= (data->dc >> 12);
            }
        } else {
            if(data->dc){
                for(uint16_t i = 0; i < num_samples; i++){
                    bool invert = data->dc < 0;
                    if(invert) data->dc = -data->dc;
                    data->dc = ((uint64_t) data->dc * (((1<<12) - 1)<<20)) >> 32;
                    if(invert) data->dc = -data->dc;
                    ret[i] = -(data->dc >> 12);
                    ret_init = true;
                }
            }
        }
    }

    if(ret_init){
        int16_t gain_const = radspa_signal_get_const_value(gain_sig, render_pass_id);
        int32_t gain = gain_const;
        if(gain_const != RADSPA_SIGNAL_VAL_UNITY_GAIN){
            for(uint16_t i = 0; i < num_samples; i++){
                if(gain_const == RADSPA_SIGNAL_NONCONST) gain = radspa_signal_get_value(gain_sig, i, render_pass_id);
                ret[i] = (ret[i] * gain) >> 12;
            }
        }

        for(uint16_t i = 0; i < num_samples; i++){
            radspa_signal_set_value_check_const(output_sig, i, ret[i]);
        }
    } else {
        radspa_signal_set_const_value(output_sig, 0);
    }
}

radspa_t * mixer_create(uint32_t init_var){
    if(init_var == 0) init_var = 4;
    if(init_var > 127) init_var = 127;
    radspa_t * mixer = radspa_standard_plugin_create(&mixer_desc, 3 + 2* init_var, sizeof(mixer_data_t), 0);
    if(mixer == NULL) return NULL;
    mixer->render = mixer_run;
    mixer_data_t * data = mixer->plugin_data;
    data->num_inputs = init_var;

    radspa_signal_set(mixer, 0, "output", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(mixer, 1, "gain", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_GAIN, RADSPA_SIGNAL_VAL_UNITY_GAIN/init_var);
    radspa_signal_set(mixer, 2, "block_dc", RADSPA_SIGNAL_HINT_INPUT, -32767);
    radspa_signal_set_group(mixer, init_var, 2, 3, "input", RADSPA_SIGNAL_HINT_INPUT, 0);
    radspa_signal_set_group(mixer, init_var, 2, 4, "input_gain", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_GAIN,
                                RADSPA_SIGNAL_VAL_UNITY_GAIN);
    radspa_signal_get_by_index(mixer, 2)->unit = "{OFF:-32767} {ON:32767}";
    return mixer;
}
