#include "bl00mbox_line_in.h"

radspa_descriptor_t bl00mbox_line_in_desc = {
    .name = "bl00mbox_line_in",
    .id = 4001,
    .description = "connects to the line input of bl00mbox",
    .create_plugin_instance = bl00mbox_line_in_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

void bl00mbox_line_in_run(radspa_t * line_in, uint16_t num_samples, uint32_t render_pass_id){
    if(bl00mbox_line_in_interlaced == NULL) return;
    radspa_signal_t * left_sig = radspa_signal_get_by_index(line_in, 0);
    radspa_signal_t * right_sig = radspa_signal_get_by_index(line_in, 1);
    radspa_signal_t * mid_sig = radspa_signal_get_by_index(line_in, 2);
    radspa_signal_t * gain_sig = radspa_signal_get_by_index(line_in, 3);

    for(uint16_t i = 0; i < num_samples; i++){
        int32_t gain = gain_sig->get_value(gain_sig, i, num_samples, render_pass_id);
        if(left_sig->buffer != NULL){
            int16_t left = radspa_gain(bl00mbox_line_in_interlaced[2*i], gain);
            left_sig->set_value(left_sig, i, left, num_samples, render_pass_id);
        }

        if(right_sig->buffer != NULL){
                int16_t right = radspa_gain(bl00mbox_line_in_interlaced[2*i], gain);
                right_sig->set_value(right_sig, i, right, num_samples, render_pass_id);
        }

        if(mid_sig->buffer != NULL){
            int16_t mid = bl00mbox_line_in_interlaced[2*i]>>1;
            mid += bl00mbox_line_in_interlaced[2*i]>>1;
            mid = radspa_gain(mid, gain);
            mid_sig->set_value(mid_sig, i, mid, num_samples, render_pass_id);
        }
    }
}

radspa_t * bl00mbox_line_in_create(uint32_t init_var){
    radspa_t * line_in = radspa_standard_plugin_create(&bl00mbox_line_in_desc, 4, 0, 0);
    if(line_in == NULL) return NULL;
    line_in->render = bl00mbox_line_in_run;
    radspa_signal_set(line_in, 0, "left", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(line_in, 1, "right", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(line_in, 2, "mid", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(line_in, 3, "gain", RADSPA_SIGNAL_HINT_GAIN, RADSPA_SIGNAL_VAL_UNITY_GAIN);
    return line_in;
}
