#include "osc_fm.h"

radspa_descriptor_t osc_fm_desc = {
    .name = "osc_fm",
    .id = 420,
    .description = "simple audio band oscillator with classic waveforms and linear fm input",
    .create_plugin_instance = osc_fm_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

#define OSC_FM_NUM_SIGNALS 6
#define OSC_FM_OUTPUT 0
#define OSC_FM_PITCH 1
#define OSC_FM_WAVEFORM 2
#define OSC_FM_LIN_FM 3
#define OSC_FM_PITCH_THRU 4
#define OSC_FM_PITCH_OFFSET 5

radspa_t * osc_fm_create(uint32_t init_var){
    radspa_t * osc_fm = radspa_standard_plugin_create(&osc_fm_desc, OSC_FM_NUM_SIGNALS, sizeof(osc_fm_data_t), 0);
    osc_fm->render = osc_fm_run;
    radspa_signal_set(osc_fm, OSC_FM_OUTPUT, "output", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(osc_fm, OSC_FM_PITCH, "pitch", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_SCT, 18367);
    radspa_signal_set(osc_fm, OSC_FM_WAVEFORM, "waveform", RADSPA_SIGNAL_HINT_INPUT, -16000);
    radspa_signal_set(osc_fm, OSC_FM_LIN_FM, "lin_fm", RADSPA_SIGNAL_HINT_INPUT, 0);
    radspa_signal_set(osc_fm, OSC_FM_PITCH_THRU, "fm_pitch_thru", RADSPA_SIGNAL_HINT_OUTPUT | RADSPA_SIGNAL_HINT_SCT, 18367);
    radspa_signal_set(osc_fm, OSC_FM_PITCH_OFFSET, "fm_pitch_offset", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_SCT, 18367);
    return osc_fm;
}

static inline int16_t triangle(int32_t saw){
    saw -= 16384;
    if(saw < -32767) saw += 65535;
    if(saw > 0) saw = -saw;
    return saw * 2 + 32767;
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
                tmp = 32767 - tmp;
            } else {
                tmp = 32767 + tmp;
                tmp = (tmp*tmp)>>15;
                tmp = tmp - 32767;
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

void osc_fm_run(radspa_t * osc_fm, uint16_t num_samples, uint32_t render_pass_id){
    osc_fm_data_t * plugin_data = osc_fm->plugin_data;
    radspa_signal_t * output_sig = radspa_signal_get_by_index(osc_fm, OSC_FM_OUTPUT);
    radspa_signal_t * pitch_sig = radspa_signal_get_by_index(osc_fm, OSC_FM_PITCH);
    radspa_signal_t * waveform_sig = radspa_signal_get_by_index(osc_fm, OSC_FM_WAVEFORM);
    radspa_signal_t * lin_fm_sig = radspa_signal_get_by_index(osc_fm, OSC_FM_LIN_FM);

    radspa_signal_t * fm_pitch_thru_sig = radspa_signal_get_by_index(osc_fm, OSC_FM_PITCH_THRU);
    if(fm_pitch_thru_sig->buffer != NULL){
        radspa_signal_t * fm_pitch_offset_sig = radspa_signal_get_by_index(osc_fm, OSC_FM_PITCH_OFFSET);
        int32_t thru_acc = 0;
        for(uint16_t i = 0; i < num_samples; i++){
            thru_acc = radspa_signal_get_value(pitch_sig, i, render_pass_id);
            thru_acc += radspa_signal_get_value(fm_pitch_offset_sig, i, render_pass_id);
            thru_acc = radspa_clip(thru_acc);
            radspa_signal_set_value(fm_pitch_thru_sig, i, thru_acc);
        }
    };

    if(output_sig->buffer == NULL) return;

    int16_t ret = 0;
    for(uint16_t i = 0; i < num_samples; i++){
        int16_t pitch = radspa_signal_get_value(pitch_sig, i, render_pass_id);
        int16_t wave = radspa_signal_get_value(waveform_sig, i, render_pass_id);
        int32_t lin_fm = radspa_signal_get_value(lin_fm_sig, i, render_pass_id);

        if(pitch != plugin_data->prev_pitch){
            plugin_data->incr = radspa_sct_to_rel_freq(pitch, 0);
            plugin_data->prev_pitch = pitch;
        }
        plugin_data->counter += plugin_data->incr;
        if(lin_fm){
            plugin_data->counter += lin_fm * (plugin_data->incr >> 15);
        }

        int32_t tmp = (plugin_data->counter) >> 16;
        tmp = tmp - 32767;
        ret = waveshaper(tmp, wave);
        radspa_signal_set_value(output_sig, i, ret);
    }
}
