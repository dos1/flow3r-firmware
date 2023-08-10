#include "osc_fm.h"

static inline int16_t waveshaper(int16_t saw, int16_t shape);

radspa_descriptor_t osc_fm_desc = {
    .name = "osc_fm",
    .id = 420,
    .description = "simple audio band oscillator with classic waveforms and linear fm input",
    .create_plugin_instance = osc_fm_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

#define OSC_FM_NUM_SIGNALS 4
#define OSC_FM_OUTPUT 0
#define OSC_FM_PITCH 1
#define OSC_FM_WAVEFORM 2
#define OSC_FM_LIN_FM 3

radspa_t * osc_fm_create(uint32_t init_var){
    radspa_t * osc_fm = radspa_standard_plugin_create(&osc_fm_desc, OSC_FM_NUM_SIGNALS, sizeof(osc_fm_data_t), 0);
    osc_fm->render = osc_fm_run;
    radspa_signal_set(osc_fm, OSC_FM_OUTPUT, "output", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(osc_fm, OSC_FM_PITCH, "pitch", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_SCT, 18367);
    radspa_signal_set(osc_fm, OSC_FM_WAVEFORM, "waveform", RADSPA_SIGNAL_HINT_INPUT, -16000);
    radspa_signal_set(osc_fm, OSC_FM_LIN_FM, "lin_fm", RADSPA_SIGNAL_HINT_INPUT, 0);
    return osc_fm;
}

void osc_fm_run(radspa_t * osc_fm, uint16_t num_samples, uint32_t render_pass_id){
    osc_fm_data_t * plugin_data = osc_fm->plugin_data;
    radspa_signal_t * output_sig = radspa_signal_get_by_index(osc_fm, OSC_FM_OUTPUT);
    radspa_signal_t * pitch_sig = radspa_signal_get_by_index(osc_fm, OSC_FM_PITCH);
    radspa_signal_t * waveform_sig = radspa_signal_get_by_index(osc_fm, OSC_FM_WAVEFORM);
    radspa_signal_t * lin_fm_sig = radspa_signal_get_by_index(osc_fm, OSC_FM_LIN_FM);
    if(output_sig->buffer == NULL) return;

    int16_t ret = 0;
    for(uint16_t i = 0; i < num_samples; i++){
        int16_t pitch = pitch_sig->get_value(pitch_sig, i, num_samples, render_pass_id);
        int16_t wave = waveform_sig->get_value(waveform_sig, i, num_samples, render_pass_id);
        int32_t lin_fm = lin_fm_sig->get_value(lin_fm_sig, i, num_samples, render_pass_id);

        if(pitch != plugin_data->prev_pitch){
            plugin_data->incr = radspa_sct_to_rel_freq(pitch, 0);
            plugin_data->prev_pitch = pitch;
        }
        plugin_data->counter += plugin_data->incr;
        if(lin_fm){
            plugin_data->counter += lin_fm * (plugin_data->incr >> 15);
        }

        int32_t tmp = (plugin_data->counter) >> 17;
        tmp = (tmp*2) - 32767;
        ret = waveshaper(tmp, wave);
        (output_sig->buffer)[i] = ret;
    }
    output_sig->value = ret;
}

static inline int16_t triangle(int16_t saw){
    int32_t tmp = saw;
    tmp += 16384;
    if(tmp > 32767) tmp -= 65535;
    if(tmp > 0) tmp = -tmp;
    tmp = (2 * tmp) + 32767;
    return tmp;
}

static inline int16_t waveshaper(int16_t saw, int16_t shape){
    int32_t tmp = saw;
    uint8_t sh = ((uint16_t) shape) >> 14;
    sh = (sh + 2)%4;
    switch(sh){
        case 0: //sine
            tmp = triangle(tmp);
            if(tmp > 0){
                tmp = 32767 - tmp;
                tmp = (tmp*tmp)>>15;
                tmp = 32767. - tmp;
            } else {
                tmp = 32767 + tmp;
                tmp = (tmp*tmp)>>15;
                tmp = tmp - 32767.;
            }
            break;
        case 1: //tri
            tmp = triangle(tmp);
            break;
        case 2: //square:
            if(tmp > 0){
                tmp = 32767;
            } else {
                tmp = -32767;
            }
            break;
        default: //saw
            break;
    }
    return tmp;
}
