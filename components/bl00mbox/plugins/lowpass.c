#include "lowpass.h"

//#define LOWPASS_DEBUG_PRINT

#ifdef LOWPASS_DEBUG_PRINT
#include <stdio.h>
#endif

radspa_t * lowpass_create(uint32_t init_var);
radspa_descriptor_t lowpass_desc = {
    .name = "lowpass",
    .id = 69420,
    .description = "2nd order lowpass lowpass, runs rly sluggish rn, sowy",
    .create_plugin_instance = lowpass_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

#define LOWPASS_NUM_SIGNALS 5
#define LOWPASS_OUTPUT 0
#define LOWPASS_INPUT 1
#define LOWPASS_FREQ 2
#define LOWPASS_Q 3
#define LOWPASS_GAIN 4

#define LOWPASS_INTERNAL_SHIFT 14
#define LOWPASS_OUT_COEFF_SHIFT 11

static void coeff_shift(uint64_t coeff, int32_t *shift, int32_t * shifted_coeff) {
    int32_t ret = 22;
    uint64_t ret_coeff = coeff >> (32-ret);
    while(1){
        if(ret > 31){
            ret = 31;
            ret_coeff = coeff >> (32-ret);
            break;
        }
        if(ret < (LOWPASS_INTERNAL_SHIFT)){
            ret = (LOWPASS_INTERNAL_SHIFT);
            ret_coeff = coeff >> (32-ret);
            break;
        }
        if(ret_coeff > (1UL<<14)){
            ret -= 3;
            ret_coeff = coeff >> (32-ret);
        } else if (ret_coeff < (1UL<<11)){
            ret += 3;
            ret_coeff = coeff >> (32-ret);
        } else {
            break;
        }
    }
    
    * shift = ret;
    * shifted_coeff = (int32_t) ret_coeff;
}

static void set_lowpass_coeffs(lowpass_data_t * data, int32_t freq, int32_t mq){
    if(freq < 15.) freq = 15.;
    if(freq > 15000.) freq = 15000.;
    if(mq < 130) mq = 130;
    int32_t K = 15287; // 2*48000/6.28
    int32_t omega = freq;
    int32_t A[3];
    A[0] = K * K;
    A[1] = K * 1000;
    A[1] = (int64_t) A[1] * omega / (mq+1);
    A[2] = omega*omega;
    int32_t sum_a = A[0] + A[1] + A[2];
    int64_t round = A[2];

    round = (round << 32) / sum_a;
    int32_t shift;
    int32_t shifted_coeff;
    coeff_shift(round, &shift, &shifted_coeff);
    data->in_coeff_shift = shift - (LOWPASS_INTERNAL_SHIFT);
    data->in_coeff = shifted_coeff;

    round = 2.*(A[2]-A[0]);
    data->out_coeff[0] = round * (1LL<<(LOWPASS_OUT_COEFF_SHIFT)) / sum_a;

    round = (A[0]-A[1]+A[2]);
    data->out_coeff[1] = round * (1LL<<(LOWPASS_OUT_COEFF_SHIFT)) / sum_a;
}


void lowpass_run(radspa_t * lowpass, uint16_t num_samples, uint32_t render_pass_id){
    radspa_signal_t * output_sig = radspa_signal_get_by_index(lowpass, LOWPASS_OUTPUT);
    if(output_sig->buffer == NULL) return;
    lowpass_data_t * data = lowpass->plugin_data;
    radspa_signal_t * input_sig = radspa_signal_get_by_index(lowpass, LOWPASS_INPUT);
    radspa_signal_t * freq_sig = radspa_signal_get_by_index(lowpass, LOWPASS_FREQ);
    radspa_signal_t * q_sig = radspa_signal_get_by_index(lowpass, LOWPASS_Q);
    radspa_signal_t * gain_sig = radspa_signal_get_by_index(lowpass, LOWPASS_GAIN);

    for(uint16_t i = 0; i < num_samples; i++){
        int16_t input = radspa_signal_get_value(input_sig, i, render_pass_id);
        int32_t freq = radspa_signal_get_value(freq_sig, i, render_pass_id);
        int16_t q = radspa_signal_get_value(q_sig, i, render_pass_id);
        int32_t gain = radspa_signal_get_value(gain_sig, i, render_pass_id);

        if((freq != data->prev_freq) | (q != data->prev_q)){
            set_lowpass_coeffs(data, freq, q);
#ifdef LOWPASS_DEBUG_PRINT
            printf("\nfreq: %ld, q: %d\n", freq, q);
            printf("in_coeff: %ld >> %ld, ", data->in_coeff, data->in_coeff_shift);
            printf("out_coeffs: %ld, %ld\n", data->out_coeff[0], data->out_coeff[1]);
#endif
            data->prev_freq = freq;
            data->prev_q = q;
        }
    
        data->pos++;
        if(data->pos >= 3) data->pos = 0;

        data->in_history[data->pos] = input;

        int32_t in_acc = input;
        int32_t ret = 0;

        for(int8_t i = 0; i<2; i++){
            int8_t pos = data->pos - i - 1;
            if(pos < 0) pos += 3;
            in_acc += (2-i)*data->in_history[pos];

            ret -= (((int64_t) data->out_history[pos]) * data->out_coeff[i]) >> (LOWPASS_OUT_COEFF_SHIFT);
        }

        ret += (in_acc * data->in_coeff) >> data->in_coeff_shift;

        data->out_history[data->pos] = ret;

        ret = ret >> (LOWPASS_INTERNAL_SHIFT);
        ret = radspa_clip(radspa_gain(ret, gain));
        radspa_signal_set_value(output_sig, i, ret);
    }
}

radspa_t * lowpass_create(uint32_t real_init_var){
    radspa_t * lowpass = radspa_standard_plugin_create(&lowpass_desc, LOWPASS_NUM_SIGNALS, sizeof(lowpass_data_t), 0);
    if(lowpass == NULL) return NULL;
    lowpass->render = lowpass_run;
    radspa_signal_set(lowpass, LOWPASS_OUTPUT, "output", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(lowpass, LOWPASS_INPUT, "input", RADSPA_SIGNAL_HINT_INPUT, 0);
    radspa_signal_set(lowpass, LOWPASS_FREQ, "freq", RADSPA_SIGNAL_HINT_INPUT, 500);
    radspa_signal_set(lowpass, LOWPASS_Q, "reso", RADSPA_SIGNAL_HINT_INPUT, 1000);
    radspa_signal_set(lowpass, LOWPASS_GAIN, "gain", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_GAIN,
                                RADSPA_SIGNAL_VAL_UNITY_GAIN);
    lowpass_data_t * data = lowpass->plugin_data;
    data->pos = 0;
    data->prev_freq = 1<<24;
    for(uint8_t i = 0; i < 3;i++){
        data->in_history[i] = 0.;
        data->out_history[i] = 0.;
    }
    return lowpass;
}
