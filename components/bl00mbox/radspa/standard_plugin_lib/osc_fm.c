#include "osc_fm.h"

radspa_descriptor_t osc_fm_desc = {
    .name = "osc_fm",
    .id = 4202,
    .description = "[DEPRECATED, replacement: osc] simple audio band oscillator with classic waveforms and linear fm input",
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
    radspa_signal_set(osc_fm, OSC_FM_PITCH, "pitch", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_SCT, RADSPA_SIGNAL_VAL_SCT_A440);
    radspa_signal_set(osc_fm, OSC_FM_WAVEFORM, "waveform", RADSPA_SIGNAL_HINT_INPUT, -16000);
    radspa_signal_set(osc_fm, OSC_FM_LIN_FM, "lin_fm", RADSPA_SIGNAL_HINT_INPUT, 0);
    radspa_signal_set(osc_fm, OSC_FM_PITCH_THRU, "fm_pitch_thru", RADSPA_SIGNAL_HINT_OUTPUT | RADSPA_SIGNAL_HINT_SCT, RADSPA_SIGNAL_VAL_SCT_A440);
    radspa_signal_set(osc_fm, OSC_FM_PITCH_OFFSET, "fm_pitch_offset", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_SCT, RADSPA_SIGNAL_VAL_SCT_A440);

    osc_fm_data_t * data = osc_fm->plugin_data;
    data->output_sig = radspa_signal_get_by_index(osc_fm, OSC_FM_OUTPUT);
    data->pitch_sig = radspa_signal_get_by_index(osc_fm, OSC_FM_PITCH);
    data->waveform_sig = radspa_signal_get_by_index(osc_fm, OSC_FM_WAVEFORM);
    data->lin_fm_sig = radspa_signal_get_by_index(osc_fm, OSC_FM_LIN_FM);
    data->fm_pitch_thru_sig = radspa_signal_get_by_index(osc_fm, OSC_FM_PITCH_THRU);
    data->fm_pitch_offset_sig = radspa_signal_get_by_index(osc_fm, OSC_FM_PITCH_OFFSET);
    data->waveform_sig->unit = "{SINE:-32767} {TRI:-10922} {SQUARE:10922} {SAW:32767}";
    return osc_fm;
}

static inline int16_t triangle(int32_t saw){
    saw -= 16384;
    if(saw < -32767) saw += 65535;
    if(saw > 0) saw = -saw;
    return saw * 2 + 32767;
}

static inline int16_t waveshaper(int32_t saw, int16_t shape){
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
    osc_fm_data_t * data = osc_fm->plugin_data;

    int16_t pitch_const = radspa_signal_get_const_value(data->pitch_sig, render_pass_id);
    int16_t fm_pitch_offset_const = radspa_signal_get_const_value(data->fm_pitch_offset_sig, render_pass_id);
    int16_t wave_const = radspa_signal_get_const_value(data->waveform_sig, render_pass_id);
    int16_t lin_fm_const = radspa_signal_get_const_value(data->lin_fm_sig, render_pass_id);

    int16_t pitch = pitch_const;
    int32_t fm_pitch_offset = fm_pitch_offset_const;
    int16_t wave = wave_const;
    int32_t lin_fm = lin_fm_const;

    if(pitch_const != -32768){
        if(pitch != data->prev_pitch){
            data->incr = radspa_sct_to_rel_freq(pitch, 0);
            data->prev_pitch = pitch;
        }
    }

    bool fm_thru_const = (pitch_const != -32768) && (fm_pitch_offset_const != -32768);

    if(fm_thru_const) radspa_signal_set_const_value(data->fm_pitch_thru_sig, pitch_const + fm_pitch_offset_const - RADSPA_SIGNAL_VAL_SCT_A440);

    for(uint16_t i = 0; i < num_samples; i++){
        if(pitch_const == -32768){
            pitch = radspa_signal_get_value(data->pitch_sig, i, render_pass_id);
            if(pitch != data->prev_pitch){
                data->incr = radspa_sct_to_rel_freq(pitch, 0);
                data->prev_pitch = pitch;
            }
        }

        if(wave_const == -32768) wave = radspa_signal_get_value(data->waveform_sig, i, render_pass_id);
        if(lin_fm_const == -32768) lin_fm = radspa_signal_get_value(data->lin_fm_sig, i, render_pass_id);

        data->counter += data->incr;
        if(lin_fm){
            data->counter += lin_fm * (data->incr >> 15);
        }

        int32_t tmp = (data->counter) >> 16;
        tmp = tmp - 32767;
        tmp = waveshaper(tmp, wave);
        radspa_signal_set_value(data->output_sig, i, tmp);

        if(fm_pitch_offset_const == -32768) fm_pitch_offset = radspa_signal_get_value(data->fm_pitch_offset_sig, i, render_pass_id);
        if(!fm_thru_const) radspa_signal_set_value(data->fm_pitch_thru_sig, i, pitch + fm_pitch_offset - RADSPA_SIGNAL_VAL_SCT_A440);
    }
}
