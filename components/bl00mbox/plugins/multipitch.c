#include "multipitch.h"

radspa_descriptor_t multipitch_desc = {
    .name = "multipitch",
    .id = 37,
    .description = "takes a pitch input and provides a number of shifted outputs.\ninit_var: number of outputs, default 0",
    .create_plugin_instance = multipitch_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

void multipitch_run(radspa_t * multipitch, uint16_t num_samples, uint32_t render_pass_id){
    bool output_request = false;
    radspa_signal_t * thru_sig = radspa_signal_get_by_index(multipitch, 0);
    if(thru_sig->buffer != NULL) output_request = true;
    radspa_signal_t * input_sig = radspa_signal_get_by_index(multipitch, 1);

    uint8_t num_outputs = (multipitch->len_signals - 2)/2;
    radspa_signal_t * output_sigs[num_outputs];
    radspa_signal_t * pitch_sigs[num_outputs];
    for(uint8_t j = 0; j < num_outputs; j++){
        output_sigs[j] = radspa_signal_get_by_index(multipitch, 2 + j);
        pitch_sigs[j] = radspa_signal_get_by_index(multipitch, 3 + j);
        if(output_sigs[j]->buffer != NULL) output_request = true;
    }
    if(!output_request) return;

    int32_t ret = 0;
    int32_t rets[num_outputs];

    for(uint16_t i = 0; i < num_samples; i++){
        int32_t input = input_sig->get_value(input_sig, i, num_samples, render_pass_id);
        ret = input;

        if(thru_sig->buffer != NULL) (thru_sig->buffer)[i] = ret;

        int32_t pitch;
        for(uint8_t j = 0; j < num_outputs; j++){
            pitch = pitch_sigs[j]->get_value(pitch_sigs[j], i, num_samples, render_pass_id);
            rets[j] = pitch + input - RADSPA_SIGNAL_VAL_SCT_A440;
            if(output_sigs[j]->buffer != NULL) (output_sigs[j]->buffer)[i] = rets[j];
        }
    }
    
    // clang-tidy only, num_samples is always nonzero
    if(!num_samples){
        for(uint8_t j = 0; j < num_outputs; j++){
            rets[j] = 0;
        }
    }

    for(uint8_t j = 0; j < num_outputs; j++){
        output_sigs[j]->value = rets[j];
    }
    thru_sig->value = ret;
}

radspa_t * multipitch_create(uint32_t init_var){
    if(init_var > 127) init_var = 127;
    radspa_t * multipitch = radspa_standard_plugin_create(&multipitch_desc, 2 + 2*init_var, sizeof(int32_t), 0);
    if(multipitch == NULL) return NULL;
    multipitch->render = multipitch_run;

    radspa_signal_set(multipitch, 0, "thru", RADSPA_SIGNAL_HINT_OUTPUT | RADSPA_SIGNAL_HINT_SCT, RADSPA_SIGNAL_VAL_SCT_A440);
    radspa_signal_set(multipitch, 1, "input", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_SCT, RADSPA_SIGNAL_VAL_SCT_A440);

    radspa_signal_set_group(multipitch, init_var, 2, 2, "output", RADSPA_SIGNAL_HINT_OUTPUT | RADSPA_SIGNAL_HINT_SCT,
            RADSPA_SIGNAL_VAL_SCT_A440);
    radspa_signal_set_group(multipitch, init_var, 2, 3, "shift", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_SCT,
            RADSPA_SIGNAL_VAL_SCT_A440);

    return multipitch;
}
