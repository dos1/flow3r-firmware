#include "lowpass.h"

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

static float apply_lowpass(lowpass_data_t * data, int32_t input){
    data->pos++;
    if(data->pos >= 3) data->pos = 0;

	float out = 0;
    float in_acc = input;

	for(int8_t i = 0; i<2; i++){
        int8_t pos = data->pos - i - 1;
        if(pos < 0) pos += 3;
        in_acc += (2-i)*data->in_history[pos];

        out -= (data->out_history[pos] * data->out_coeff[i]);
    }
    out += in_acc * data->in_coeff;

    data->in_history[data->pos] = input;
    data->out_history[data->pos] = out;

	return out;
}

static uint8_t coeff_shift(float coeff) {
    int32_t ret = 16;
    while(1){
        int32_t test_val = (1<<ret) * coeff;
        if(test_val < 0) test_val = -test_val;
        if(test_val > (1<<12)){
            ret -= 3;
        } else if (test_val < (1<<8)){
            ret += 3;
        } else {
            break;
        }
        if(ret > 28) break;
        if(ret < 4) break;
    }
    return ret;
}

static void set_lowpass_coeffs(lowpass_data_t * data, float freq, float q){
    //molasses, sorry
    if(freq == 0) return;
    if(q == 0) return;
    float K = 2*48000;
    float omega = freq * 6.28;
    float A[3];
    A[0] = K*K;
    A[1] = K*omega/q;
    A[2] = omega*omega;
    float B = omega*omega;

    float sum_a = A[0] + A[1] + A[2];

    float round;
    //int16_t shift;

    round = B / sum_a;
    //shift = coeff_shift(round);
    data->in_coeff = round;// * (1<<shift);
    //data->in_coeff_shift = shift;

    round = 2.*(A[2]-A[0]) / sum_a;
    //shift = coeff_shift(round);
    data->out_coeff[0] = round;// * (1<<shift);
    //data->out_coeff_shift[0] = shift;

    round = (A[0]-A[1]+A[2]) / sum_a;
    //shift = coeff_shift(round);
    data->out_coeff[1] = round;// * (1<<shift);
    //data->out_coeff_shift[1] = shift;
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
        int16_t gain = radspa_signal_get_value(gain_sig, i, render_pass_id);

        if((freq != data->prev_freq) | (q != data->prev_q)){
            set_lowpass_coeffs(data, freq, ((float) q + 1.)/1000.);
            data->prev_freq = freq;
            data->prev_q = q;
        }
    
        float out = apply_lowpass(data, input) + 0.5;
        int16_t ret = radspa_clip(radspa_gain((int32_t) out, gain));
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
