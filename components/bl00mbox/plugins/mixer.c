#include "mixer.h"

radspa_descriptor_t mixer_desc = {
    .name = "mixer",
    .id = 21,
    .description = "sums input and applies output gain\n  init_var: number of inputs, 1-127, default 4",
    .create_plugin_instance = mixer_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

void mixer_run(radspa_t * mixer, uint16_t num_samples, uint32_t render_pass_id){
    radspa_signal_t * output_sig = radspa_signal_get_by_index(mixer, 0);
    if(output_sig->buffer == NULL) return;
    radspa_signal_t * gain_sig = radspa_signal_get_by_index(mixer, 1);

    uint8_t num_inputs = mixer->len_signals - 2;
    radspa_signal_t * input_sigs[num_inputs];
    for(uint8_t i = 0; i < num_inputs; i++){
        input_sigs[i] = radspa_signal_get_by_index(mixer, 2 + i);
    }

    int32_t * dc_acc = mixer->plugin_data;

    static int32_t ret = 0;
    for(uint16_t i = 0; i < num_samples; i++){
        int16_t gain = gain_sig->get_value(gain_sig, i, num_samples, render_pass_id);
        ret = 0;
        for(uint8_t j = 0; j < num_inputs; j++){
            ret += input_sigs[j]->get_value(input_sigs[j], i, num_samples, render_pass_id);
        }
        // remove dc
        (* dc_acc) = (ret + (* dc_acc)*1023) >> 10;
        ret -= (* dc_acc);
        (output_sig->buffer)[i] = radspa_clip(radspa_gain(ret, gain));
    }
    output_sig->value = ret;
}

radspa_t * mixer_create(uint32_t init_var){
    if(init_var == 0) init_var = 4;
    if(init_var > 127) init_var = 127;
    radspa_t * mixer = radspa_standard_plugin_create(&mixer_desc, 2 + init_var, sizeof(int32_t), 0);
    if(mixer == NULL) return NULL;
    mixer->render = mixer_run;
    radspa_signal_set(mixer, 0, "output", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    int16_t gain = RADSPA_SIGNAL_VAL_UNITY_GAIN/init_var;
    radspa_signal_set(mixer, 1, "gain", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_GAIN, gain);
    radspa_signal_set_group(mixer, init_var, 1, 2, "input", RADSPA_SIGNAL_HINT_INPUT, 0);
    int32_t * dc_acc = mixer->plugin_data;
    (* dc_acc) = 0;
    return mixer;
}
