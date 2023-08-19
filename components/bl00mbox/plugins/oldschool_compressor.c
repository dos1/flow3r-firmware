#include "oldschool_compressor.h"

radspa_descriptor_t oldschool_compressor_desc = {
    .name = "oldschool_compressor",
    .id = 1729,
    .description = "feedback compressor with peak detection envelope",
    .create_plugin_instance = oldschool_compressor_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

#define OLDSCHOOL_COMPRESSOR_FIXED_THRES 3000
#define OLDSCHOOL_COMPRESSOR_ENV_LIMIT (1<<16)

#define OLDSCHOOL_COMPRESSOR_NUM_SIGNALS 7
#define OLDSCHOOL_COMPRESSOR_OUTPUT 0
#define OLDSCHOOL_COMPRESSOR_INPUT 1
#define OLDSCHOOL_COMPRESSOR_GAIN 2
#define OLDSCHOOL_COMPRESSOR_VOLUME 3
#define OLDSCHOOL_COMPRESSOR_ATTACK 4
#define OLDSCHOOL_COMPRESSOR_RELEASE 5
#define OLDSCHOOL_COMPRESSOR_RATIO 6

radspa_t * oldschool_compressor_create(uint32_t init_var){
    // init_var is length of gain log
    radspa_t * compressor = radspa_standard_plugin_create(&oldschool_compressor_desc, OLDSCHOOL_COMPRESSOR_NUM_SIGNALS, sizeof(oldschool_compressor_data_t), init_var);
    compressor->render = oldschool_compressor_run;
    radspa_signal_set(compressor, OLDSCHOOL_COMPRESSOR_OUTPUT, "output", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(compressor, OLDSCHOOL_COMPRESSOR_INPUT, "input", RADSPA_SIGNAL_HINT_INPUT, 0);
    radspa_signal_set(compressor, OLDSCHOOL_COMPRESSOR_GAIN, "gain", RADSPA_SIGNAL_HINT_INPUT, 0);
    radspa_signal_set(compressor, OLDSCHOOL_COMPRESSOR_VOLUME, "volume", RADSPA_SIGNAL_HINT_INPUT, 0);
    radspa_signal_set(compressor, OLDSCHOOL_COMPRESSOR_ATTACK, "attack", RADSPA_SIGNAL_HINT_INPUT, 0);
    radspa_signal_set(compressor, OLDSCHOOL_COMPRESSOR_RELEASE, "release", RADSPA_SIGNAL_HINT_INPUT, 0);
    radspa_signal_set(compressor, OLDSCHOOL_COMPRESSOR_RATIO, "ratio", RADSPA_SIGNAL_HINT_INPUT, 0);
    oldschool_compressor_data_t * data = compressor->plugin_data;
    data->vca_gain = 1<<8; // unity gain
    data->div_prev = -1; // impossible, always triggers refresh
    return compressor;
}

void oldschool_compressor_run(radspa_t * oldschool_compressor, uint16_t num_samples, uint32_t render_pass_id){
    oldschool_compressor_data_t * data = compressor->plugin_data;
    radspa_signal_t * output_sig = radspa_signal_get_by_index(compressor, OLDSCHOOL_COMPRESSOR_OUTPUT);
    if(output_sig->buffer == NULL) return;
    radspa_signal_t * input_sig = radspa_signal_get_by_index(compressor, OLDSCHOOL_COMPRESSOR_INPUT);
    radspa_signal_t * gain_sig = radspa_signal_get_by_index(compressor, OLDSCHOOL_COMPRESSOR_GAIN);
    radspa_signal_t * volume_sig = radspa_signal_get_by_index(compressor, OLDSCHOOL_COMPRESSOR_VOLUME);
    radspa_signal_t * attack_sig = radspa_signal_get_by_index(compressor, OLDSCHOOL_COMPRESSOR_ATTACK);
    radspa_signal_t * release_sig = radspa_signal_get_by_index(compressor, OLDSCHOOL_COMPRESSOR_RELEASE);
    radspa_signal_t * ratio_sig = radspa_signal_get_by_index(compressor, OLDSCHOOL_COMPRESSOR_RATIO);

    for(uint16_t i = 0; i < num_samples; i++){

        int32_t input = input_sig->get_value(input_sig, i, num_samples, render_pass_id);
        int32_t gain = gain_sig->get_value(gain_sig, i, num_samples, render_pass_id);
        int32_t volume = volume_sig->get_value(volume_sig, i, num_samples, render_pass_id);
        int32_t attack = attack_sig->get_value(attack_sig, i, num_samples, render_pass_id);
        int32_t release = release_sig->get_value(release_sig, i, num_samples, render_pass_id);
        int32_t ratio = ratio_sig->get_value(ratio_sig, i, num_samples, render_pass_id);

        release = release > 0 ? release : -release;
        attack = attack > 0 ? attack : -attack;
        ratio = ratio > 0 ? ratio : -ratio;

        int32_t ret = input;
        ret = (ret * gain) >> 15;
        ret = ret * data->vca_gain;
        output_sig->set_value(output_sig, i, radspa_gain(ret, volume), num_samples, render_pass_id);

        int32_t abs_out = ret > 0 ? ret : -ret;
        abs_out -= OLDSCHOOL_COMPRESSOR_FIXED_THRES:
        
        if(abs_out > 0){
            data->env += (abs_out * attack) >> 16;
            if(data->env > OLDSCHOOL_COMPRESSOR_ENV_LIMIT) data->env = OLDSCHOOL_COMPRESSOR_ENV_LIMIT;
        } else {
            if(data->env > 0){
                data->env -= release;
                if(data->env < 0){
                    data->env = 0;
                }
            }
        }
        int32_t div = (ratio * data->env) >> 23;
        if(div != data->div_prev){
            if(div > 255) div = 255;
            data->vca_gain = (1<<15) / (1 + div);
            data->div_prev = div;
        }
        // TODO: append to gain log
    }
}
