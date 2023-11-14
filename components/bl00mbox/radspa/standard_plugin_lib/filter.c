#include "filter.h"

radspa_t * filter_create(uint32_t init_var);
radspa_descriptor_t filter_desc = {
    .name = "filter",
    .id = 69420,
    .description = "biquad filter. use negative q for allpass variations.",
    .create_plugin_instance = filter_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

#define FILTER_NUM_SIGNALS 7
#define FILTER_OUTPUT 0
#define FILTER_INPUT 1
#define FILTER_PITCH 2
#define FILTER_RESO 3
#define FILTER_GAIN 4
#define FILTER_MIX 5
#define FILTER_MODE 6

#define FILTER_MODE_LOWPASS 0
#define FILTER_MODE_BANDPASS 1
#define FILTER_MODE_HIGHPASS 2

static inline int32_t approx_cos(uint32_t x){
    // x: full circle: 1<<32
    // return val: 1<<30 <=> 1
    uint32_t sq = x & (~(1UL<<31));
    if(sq > (1UL<<30)) sq = (1UL<<31) - sq;
    sq = ((uint64_t) sq * sq) >> 32;

    int32_t ret = (((1<<28) - (uint64_t) sq)<<32) / (((1UL<<30) + sq));
    if((x > (1UL<<30)) && (x < (3UL<<30))) ret = -ret;
    return ret;
}

static inline void get_mode_coeffs(uint8_t mode, filter_data_t * data, int32_t * coeffs){
    switch(mode){
        case FILTER_MODE_LOWPASS:
            coeffs[1] = ((1L<<21) - data->cos_omega) * data->inv_norm;
            coeffs[0] = coeffs[1]/2;
            coeffs[2] = coeffs[0];
            break;
        case FILTER_MODE_BANDPASS:
            coeffs[0] = data->alpha * data->inv_norm;
            coeffs[1] = 0;
            coeffs[2] = -coeffs[0];
            break;
        case FILTER_MODE_HIGHPASS:
            coeffs[1] = -((1L<<21) + data->cos_omega) * data->inv_norm;
            coeffs[0] = -coeffs[1]/2;
            coeffs[2] = coeffs[0];
            break;
    }
}

static inline void get_coeffs(filter_data_t * data, int16_t pitch, int16_t reso, int16_t mode){
    if((pitch != data->pitch_prev) || (reso != data->reso_prev)){
        int32_t mqi = reso>>2;
        if(mqi < 0) mqi = -mqi;
        if(mqi < 171) mqi = 171;

        uint32_t freq = radspa_sct_to_rel_freq(pitch, 0);
        if(freq > (3UL<<29)) freq = 3UL<<29;

        // unit: 1<<21 <=> 1, range: [0..1<<21]
        data->cos_omega = approx_cos(freq)>>9;
        // unit: 1<<21 <=> 1, range: [0..3<<21]
        data->alpha = approx_cos(freq + (3UL<<30))/mqi;
        // unit transform from 1<<21 to 1<<29 <=> 1
        data->inv_norm = (1L<<29)/((1L<<21) + data->alpha);

        data->pitch_prev = pitch;
        data->reso_prev = reso;
    }
    if((pitch != data->pitch_prev) || (reso != data->reso_prev) || (mode != data->mode_prev)){
        // unit for {in/out}_coeffs: 1<<29 <=> 1, range: full
        data->out_coeffs[1] = -2*data->cos_omega * data->inv_norm;
        data->out_coeffs[2] = ((1<<21) - data->alpha) * data->inv_norm;
        if(mode == -32767){
            get_mode_coeffs(FILTER_MODE_LOWPASS, data, data->in_coeffs);
            return;
        } else if(mode == 0){
            get_mode_coeffs(FILTER_MODE_BANDPASS, data, data->in_coeffs);
            return;
        } else if(mode == 32767){
            get_mode_coeffs(FILTER_MODE_HIGHPASS, data, data->in_coeffs);
            return;
        }
        int32_t a[3];
        int32_t b[3];
        int32_t blend = mode;
        if(mode < 0){
            get_mode_coeffs(FILTER_MODE_LOWPASS, data, a);
            get_mode_coeffs(FILTER_MODE_BANDPASS, data, b);
            blend += 32767;
        } else {
            get_mode_coeffs(FILTER_MODE_BANDPASS, data, a);
            get_mode_coeffs(FILTER_MODE_HIGHPASS, data, b);
        }
        blend = blend << 16;
        for(uint8_t i = 0; i<3; i++){
            data->in_coeffs[i] = ((int64_t) b[i] * blend) >> 32;
            data->in_coeffs[i] += ((int64_t) a[i] * ((1L<<31) - blend)) >> 32;
            data->in_coeffs[i] = data->in_coeffs[i] << 1;
        }
        data->const_output = RADSPA_SIGNAL_NONCONST;
    }
}

void filter_run(radspa_t * filter, uint16_t num_samples, uint32_t render_pass_id){
    filter_data_t * data = filter->plugin_data;

    radspa_signal_t * output_sig = radspa_signal_get_by_index(filter, FILTER_OUTPUT);
    radspa_signal_t * mode_sig = radspa_signal_get_by_index(filter, FILTER_MODE);
    radspa_signal_t * input_sig = radspa_signal_get_by_index(filter, FILTER_INPUT);
    radspa_signal_t * pitch_sig = radspa_signal_get_by_index(filter, FILTER_PITCH);
    radspa_signal_t * gain_sig = radspa_signal_get_by_index(filter, FILTER_GAIN);
    radspa_signal_t * reso_sig = radspa_signal_get_by_index(filter, FILTER_RESO);
    radspa_signal_t * mix_sig = radspa_signal_get_by_index(filter, FILTER_MIX);

    int16_t input_const = radspa_signal_get_const_value(input_sig, render_pass_id);
    int16_t pitch_const = radspa_signal_get_const_value(pitch_sig, render_pass_id);
    int16_t gain_const = radspa_signal_get_const_value(gain_sig, render_pass_id);
    int16_t reso_const = radspa_signal_get_const_value(reso_sig, render_pass_id);
    int16_t mode_const = radspa_signal_get_const_value(mode_sig, render_pass_id);
    int16_t mix_const = radspa_signal_get_const_value(mix_sig, render_pass_id);

    int16_t pitch = pitch_const;
    int16_t gain = gain_const;
    int16_t reso = reso_const;
    int16_t mode = mode_const;
    int16_t mix = mix_const;
    int16_t input = input_const;
    bool always_update_coeffs = true;

    if((pitch_const != RADSPA_SIGNAL_NONCONST) && (reso_const != RADSPA_SIGNAL_NONCONST) && (mode_const != RADSPA_SIGNAL_NONCONST)){
        always_update_coeffs = false;
        get_coeffs(data, pitch, reso, mode);
    }
    if((input_const != RADSPA_SIGNAL_NONCONST) && (mix_const != RADSPA_SIGNAL_NONCONST) && (gain_const != RADSPA_SIGNAL_NONCONST)
        && (data->const_output != RADSPA_SIGNAL_NONCONST)){
        radspa_signal_set_const_value(output_sig, data->const_output);
        return;
    }

    int16_t out[num_samples];

    for(uint16_t i = 0; i < num_samples; i++){
        if(!(i & (RADSPA_EVENT_MASK))){
            if(always_update_coeffs){
                if(pitch_const == RADSPA_SIGNAL_NONCONST) pitch = radspa_signal_get_value(pitch_sig, i, render_pass_id);
                if(reso_const == RADSPA_SIGNAL_NONCONST) reso = radspa_signal_get_value(reso_sig, i, render_pass_id);
                if(mode_const == RADSPA_SIGNAL_NONCONST) mode = radspa_signal_get_value(mode_sig, i, render_pass_id);
                get_coeffs(data, pitch, reso, mode);
            }
            if(gain_const == RADSPA_SIGNAL_NONCONST) gain = radspa_signal_get_value(gain_sig, i, render_pass_id);
            if(mix_const == RADSPA_SIGNAL_NONCONST) mix = radspa_signal_get_value(mix_sig, i, render_pass_id);
        }

        if(input_const == RADSPA_SIGNAL_NONCONST) input = radspa_signal_get_value(input_sig, i, render_pass_id);
        int32_t in = radspa_clip(radspa_gain(input, gain));

        data->pos++;
        if(data->pos >= 3) data->pos = 0;

        data->in_history[data->pos] = in << 12;

        int32_t ret = ((int64_t) data->in_coeffs[0] * data->in_history[data->pos]) >> 32;

        for(int8_t i = 1; i<3; i++){
            int8_t pos = data->pos - i;
            if(pos < 0) pos += 3;
            ret += ((int64_t) data->in_history[pos] * data->in_coeffs[i]) >> 32;
            ret -= ((int64_t) data->out_history[pos] * data->out_coeffs[i]) >> 32;
        }
        if(ret >= (1L<<28)){
            ret = (1L<<28) - 1;
        } else if(ret <= -(1L<<28)){
            ret = 1-(1L<<28);
        }
        ret = ret << 3;
        data->out_history[data->pos] = ret;

        ret = ret >> 12;

        if(reso < 0) ret = input - (ret<<1); //allpass mode
        if(mix == -32767){
            ret = -ret;
        } else if (mix == 0){
            ret = input;
        } else  if(mix != 32767){
            ret *= mix;
            int32_t dry_vol = mix > 0 ? 32767 - mix : 32767 + mix;
            ret += radspa_gain(dry_vol, gain) * input;
            ret = ret >> 15;
        }
        out[i] = radspa_clip(ret);
    }

    if(input_const == RADSPA_SIGNAL_NONCONST){
        for(uint16_t i = 0; i < num_samples; i++){
            radspa_signal_set_value(output_sig, i, out[i]);
        }
    } else if(data->const_output == RADSPA_SIGNAL_NONCONST){
        for(uint16_t i = 0; i < num_samples; i++){
            radspa_signal_set_value_check_const(output_sig, i, out[i]);
        }
        data->const_output = radspa_signal_set_value_check_const_result(output_sig);
    } else {
        radspa_signal_set_const_value(output_sig,  data->const_output);
    }
}

radspa_t * filter_create(uint32_t real_init_var){
    radspa_t * filter = radspa_standard_plugin_create(&filter_desc, FILTER_NUM_SIGNALS, sizeof(filter_data_t), 0);
    if(filter == NULL) return NULL;
    filter->render = filter_run;
    filter_data_t * data = filter->plugin_data;
    radspa_signal_set(filter, FILTER_OUTPUT, "output", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(filter, FILTER_INPUT, "input", RADSPA_SIGNAL_HINT_INPUT, 0);
    radspa_signal_set(filter, FILTER_PITCH, "cutoff", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_SCT,
                                RADSPA_SIGNAL_VAL_SCT_A440);
    radspa_signal_set(filter, FILTER_RESO, "reso", RADSPA_SIGNAL_HINT_INPUT, RADSPA_SIGNAL_VAL_UNITY_GAIN);
    radspa_signal_set(filter, FILTER_GAIN, "gain", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_GAIN,
                                RADSPA_SIGNAL_VAL_UNITY_GAIN);
    radspa_signal_set(filter, FILTER_MIX, "mix", RADSPA_SIGNAL_HINT_INPUT, 32767);
    radspa_signal_set(filter, FILTER_MODE, "mode", RADSPA_SIGNAL_HINT_INPUT, -32767);

    radspa_signal_t * sig;
    sig = radspa_signal_get_by_index(filter, FILTER_MODE);
    sig->unit = "{LOWPASS:-32767} {BANDPASS:0} {HIGHPASS:32767}";
    sig = radspa_signal_get_by_index(filter, FILTER_RESO);
    sig->unit = "4096*Q";

    data->pos = 0;
    data->pitch_prev = RADSPA_SIGNAL_NONCONST;
    data->mode_prev = RADSPA_SIGNAL_NONCONST;
    data->reso_prev = RADSPA_SIGNAL_NONCONST;

    data->const_output = RADSPA_SIGNAL_NONCONST;
    for(uint8_t i = 0; i < 3;i++){
        data->in_history[i] = 0;
        data->out_history[i] = 0;
    }
    get_coeffs(data, RADSPA_SIGNAL_VAL_SCT_A440, RADSPA_SIGNAL_VAL_UNITY_GAIN, -32767);
    return filter;
}
