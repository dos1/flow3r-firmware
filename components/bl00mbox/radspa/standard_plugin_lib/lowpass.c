#include "lowpass.h"

radspa_t * lowpass_create(uint32_t init_var);
radspa_descriptor_t lowpass_desc = {
    .name = "lowpass",
    .id = 694202,
    .description = "[DEPRECATED, replacement: filter]",
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
#define LOWPASS_OUT_COEFF_SHIFT 30
#define LOWPASS_SUM_SHIFT 25

static void set_lowpass_coeffs(lowpass_data_t * data, int32_t freq, int32_t mq){
    if(freq < 15.) freq = 15.;
    if(freq > 15000.) freq = 15000.;
    if(mq < 140) mq = 140;
    int32_t K = 15287; // 2*48000/6.28
    int32_t omega = freq;
    int32_t mq_rec = (1UL<<31)/(mq+1);
    int32_t A[3];
    A[0] = K * K;
    A[1] = K * omega * 8;
    A[1] = ((int64_t) A[1] * mq_rec) >> 32;
    A[1] *= 250;
    A[2] = omega*omega;
    // slow, figure out smth smarther:
    int32_t sum_a_rec = (1ULL<<(32 + (LOWPASS_SUM_SHIFT))) / (A[0] + A[1] + A[2]);

    int32_t tmp;

    tmp = (int64_t) A[2] * sum_a_rec >> 32;
    data->in_coeff = tmp << (32-LOWPASS_SUM_SHIFT);

    tmp = 2.*(A[2]-A[0]);
    tmp = ((int64_t) tmp * sum_a_rec) >> 32;
    data->out_coeff[0] = tmp<<((LOWPASS_OUT_COEFF_SHIFT) - (LOWPASS_SUM_SHIFT));

    tmp = (A[0]-A[1]+A[2]);
    tmp = ((int64_t) tmp * sum_a_rec) >> 32;
    data->out_coeff[1] = tmp<<((LOWPASS_OUT_COEFF_SHIFT) - (LOWPASS_SUM_SHIFT));
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
            ret -= (((int64_t) data->out_history[pos]) * data->out_coeff[i]) >> 32;
        }

        ret = ret << (32 - LOWPASS_OUT_COEFF_SHIFT);
        in_acc = in_acc << (LOWPASS_INTERNAL_SHIFT);
        in_acc = ((int64_t) in_acc * data->in_coeff) >> 32;
        ret += in_acc;

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
