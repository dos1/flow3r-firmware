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

    for(uint8_t j = 0; j < data->num_inputs; j++){
        if(radspa_signal_get_const_value(data->input_sigs[j], render_pass_id)){
            if(ret_init){
                for(uint16_t i = 0; i < num_samples; i++){
                    ret[i] += radspa_signal_get_value(data->input_sigs[j], i, render_pass_id);
                }
            } else {
                for(uint16_t i = 0; i < num_samples; i++){
                    ret[i] = radspa_signal_get_value(data->input_sigs[j], i, render_pass_id);
                }
                ret_init = true;
            }
        }
    }
    if(ret_init){
        int16_t gain_const = radspa_signal_get_const_value(data->gain_sig, render_pass_id);
        int16_t gain = gain_const;
        for(uint16_t i = 0; i < num_samples; i++){
            if(gain_const == RADSPA_SIGNAL_NONCONST) gain = radspa_signal_get_value(data->gain_sig, i, render_pass_id);
            ret[i] = radspa_clip(radspa_gain(ret[i], gain));
            radspa_signal_set_value(data->output_sig, i, ret[i]);
        }
    } else {
        radspa_signal_set_const_value(data->output_sig, 0);
    }
}

radspa_t * mixer_create(uint32_t init_var){
    if(init_var == 0) init_var = 4;
    if(init_var > 127) init_var = 127;
    radspa_t * mixer = radspa_standard_plugin_create(&mixer_desc, 2 + init_var, sizeof(mixer_data_t) + sizeof(size_t) * init_var, 0);
    if(mixer == NULL) return NULL;
    mixer->render = mixer_run;
    mixer_data_t * data = mixer->plugin_data;
    data->num_inputs = init_var;
    data->output_sig = radspa_signal_set(mixer, 0, "output", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    data->gain_sig = radspa_signal_set(mixer, 1, "gain", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_GAIN, RADSPA_SIGNAL_VAL_UNITY_GAIN/init_var);
    radspa_signal_set_group(mixer, init_var, 1, 2, "input", RADSPA_SIGNAL_HINT_INPUT, 0);
    for(uint8_t i = 0; i < init_var; i++){ data->input_sigs[i] = radspa_signal_get_by_index(mixer, 2+i); }
    return mixer;
}
