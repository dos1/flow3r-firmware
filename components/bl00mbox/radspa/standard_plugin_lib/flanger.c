#include "flanger.h"

radspa_t * flanger_create(uint32_t init_var);
radspa_descriptor_t flanger_desc = {
    .name = "flanger",
    .id = 123,
    .description = "flanger with subsample interpolation and negative mix/resonance capability.",
    .create_plugin_instance = flanger_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

#define FLANGER_BUFFER_SIZE 4800
#define FIXED_POINT_DIGITS 4
#define VARIABLE_NAME ((FLANGER_BUFFER_SIZE)<<(FIXED_POINT_DIGITS))

#define FLANGER_NUM_SIGNALS 7
#define FLANGER_OUTPUT 0
#define FLANGER_INPUT 1
#define FLANGER_MANUAL 2
#define FLANGER_RESONANCE 3
#define FLANGER_DECAY 4
#define FLANGER_LEVEL 5
#define FLANGER_MIX 6

static inline int32_t fixed_point_list_access(int32_t * buf, int32_t fp_index, uint32_t buf_len){
    int32_t index = (fp_index) >> (FIXED_POINT_DIGITS);
    while(index >= buf_len) index -= buf_len;
    int32_t next_index = index + 1;
    while(next_index >= buf_len) next_index -= buf_len;

    int32_t subindex = (fp_index) & ((1<<(FIXED_POINT_DIGITS)) - 1);
    int32_t ret =  buf[index] * ((1<<(FIXED_POINT_DIGITS)) - subindex);
    ret += (buf[next_index]) * subindex;
    ret = ret >> (FIXED_POINT_DIGITS);
    return radspa_clip(ret);
}

/* delay_time = 1/freq
 * delay_samples = delay_time * SAMPLE_RATE
 * freq(sct) = pow(2, (sct + 2708)/2400))
 * freq = sct_to_rel_freq(sct) * SAMPLE_RATE / UINT32_MAX
 * delay_samples = UINT32_MAX / sct_to_rel_freq(sct)
 * delay_samples = sct_to_rel_freq(-7572-sct + 2400 * FIXED_POINT_DIGITS)
 *
 * decay_reso_float = 2**(-delay_time_ms/decay_ms_per_6dB)
 * rho = (delay_time_ms * 48) * (1<<FIXED_POINT_DIGITS)
 * delay_time_ms * 2400 = (rho >> FIXED_POINT_DIGITS) * 50
 * delay_reso_int = sct_to_rel_freq(offset - (delay_time_ms * 2400)/decay_ms_per_6dB)
 */

void flanger_run(radspa_t * flanger, uint16_t num_samples, uint32_t render_pass_id){
    radspa_signal_t * output_sig = radspa_signal_get_by_index(flanger, FLANGER_OUTPUT);
    if(output_sig->buffer == NULL) return;
    flanger_data_t * data = flanger->plugin_data;
    int32_t * buf = (int32_t *) flanger->plugin_table;
    radspa_signal_t * input_sig = radspa_signal_get_by_index(flanger, FLANGER_INPUT);
    radspa_signal_t * manual_sig = radspa_signal_get_by_index(flanger, FLANGER_MANUAL);
    radspa_signal_t * reso_sig = radspa_signal_get_by_index(flanger, FLANGER_RESONANCE);
    radspa_signal_t * decay_sig = radspa_signal_get_by_index(flanger, FLANGER_DECAY);
    radspa_signal_t * level_sig = radspa_signal_get_by_index(flanger, FLANGER_LEVEL);
    radspa_signal_t * mix_sig = radspa_signal_get_by_index(flanger, FLANGER_MIX);

    int32_t reso = radspa_signal_get_value(reso_sig, 0, render_pass_id);
    reso = reso << 14;
    int32_t level = radspa_signal_get_value(level_sig, 0, render_pass_id);
    int32_t mix = radspa_signal_get_value(mix_sig, 0, render_pass_id);
    int32_t decay = radspa_signal_get_value(decay_sig, 0, render_pass_id);
    int32_t dry_vol = (mix>0) ? (32767-mix) : (32767+mix); //always pos polarity

    int32_t manual = radspa_signal_get_value(manual_sig, 0, render_pass_id);
    if(manual != data->manual_prev){
        int32_t manual_invert = ((2400*(FIXED_POINT_DIGITS)) - 7572) - manual; // magic numbers
        uint32_t rho = radspa_sct_to_rel_freq(radspa_clip(manual_invert), 0);
        if(rho > VARIABLE_NAME) rho = VARIABLE_NAME;
        data->read_head_offset = rho;
    }
    if(decay){
        int32_t sgn_decay = decay > 0 ? 1 : -1;
        int32_t abs_decay = decay * sgn_decay;
        if((abs_decay != data->abs_decay_prev) || (manual != data->manual_prev)){
            int32_t decay_invert = - ((data->read_head_offset * 50) >> FIXED_POINT_DIGITS)/abs_decay;
            decay_invert += 34614 - 4800 - 2400; // magic number
            data->decay_reso = radspa_sct_to_rel_freq(radspa_clip(decay_invert), 0);
        }
        int32_t tmp = reso + sgn_decay * data->decay_reso;
        if((sgn_decay == 1) && (tmp < reso)){
                tmp = INT32_MAX;
        } else if((sgn_decay == -1) && (tmp > reso)){
                tmp = INT32_MIN;
        }
        reso = tmp;
        data->abs_decay_prev = abs_decay;
    }
    data->manual_prev = manual;

    for(uint16_t i = 0; i < num_samples; i++){
        int32_t dry = radspa_signal_get_value(input_sig, i, render_pass_id);

        data->write_head_position++;
        while(data->write_head_position >= FLANGER_BUFFER_SIZE) data->write_head_position -= FLANGER_BUFFER_SIZE;
        data->read_head_position = ((data->write_head_position)<<(FIXED_POINT_DIGITS));
        data->read_head_position -= data->read_head_offset;
        while(data->read_head_position < 0) data->read_head_position += VARIABLE_NAME; //underflow

        buf[data->write_head_position] = dry;
        int32_t wet = fixed_point_list_access(buf, data->read_head_position, FLANGER_BUFFER_SIZE) << 3;
        bool sgn_wet = wet > 0;
        bool sgn_reso = reso > 0;
        if(sgn_wet != sgn_reso){
            buf[data->write_head_position] -= ((int64_t) (-wet) * reso) >> 32;
        } else {
            buf[data->write_head_position] += ((int64_t) wet * reso) >> 32;
        }

        int32_t ret = radspa_add_sat(radspa_mult_shift(dry, dry_vol), radspa_mult_shift(radspa_clip(wet), mix));
        ret = radspa_clip(radspa_gain(ret, level));
        radspa_signal_set_value(output_sig, i, ret);
    }
}

radspa_t * flanger_create(uint32_t init_var){
    radspa_t * flanger = radspa_standard_plugin_create(&flanger_desc, FLANGER_NUM_SIGNALS, sizeof(flanger_data_t),
                                                            2 * FLANGER_BUFFER_SIZE);
    if(flanger == NULL) return NULL;
    flanger_data_t * plugin_data = flanger->plugin_data;
    plugin_data->manual_prev = 40000;
    flanger->render = flanger_run;
    radspa_signal_set(flanger, FLANGER_OUTPUT, "output", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(flanger, FLANGER_INPUT, "input", RADSPA_SIGNAL_HINT_INPUT, 0);
    radspa_signal_set(flanger, FLANGER_MANUAL, "manual", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_SCT, 18367);
    radspa_signal_set(flanger, FLANGER_RESONANCE, "resonance", RADSPA_SIGNAL_HINT_INPUT, RADSPA_SIGNAL_VAL_UNITY_GAIN/2);
    radspa_signal_set(flanger, FLANGER_DECAY, "decay", RADSPA_SIGNAL_HINT_INPUT, 0);
    radspa_signal_set(flanger, FLANGER_LEVEL, "level", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_GAIN,
                            RADSPA_SIGNAL_VAL_UNITY_GAIN);
    radspa_signal_set(flanger, FLANGER_MIX, "mix", RADSPA_SIGNAL_HINT_INPUT, 1<<14);
    flanger->signals[FLANGER_DECAY].unit = "ms/6dB";
    return flanger;
}
