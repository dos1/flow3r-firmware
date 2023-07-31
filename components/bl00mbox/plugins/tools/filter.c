#include "filter.h"

radspa_t * filter_create(uint32_t init_var);
radspa_descriptor_t filter_desc = {
    .name = "filter",
    .id = 69420,
    .description = "simple filter",
    .create_plugin_instance = filter_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

#define FILTER_NUM_SIGNALS 5
#define FILTER_OUTPUT 0
#define FILTER_INPUT 1
#define FILTER_FREQ 2
#define FILTER_Q 3
#define FILTER_GAIN 4


static int16_t apply_filter(filter_data_t * data, int32_t input, int32_t gain){
    data->pos++;
    if(data->pos >= 3) data->pos = 0;

	int64_t out = 0;
    int64_t in_acc = input;

	for(int8_t i=0; i<2; i++){
        int8_t pos = data->pos - i - 1;
        if(pos < 0) pos += 3;
        in_acc += (2-i)*data->in_history[pos];

        int16_t shift = data->out_coeff_shift[i] - data->in_coeff_shift;
        if(shift >= 0){
            out -= (data->out_history[pos] * data->out_coeff[i]) >> shift;
        } else {
            out -= (data->out_history[pos] * data->out_coeff[i]) << (-shift);
        }
    }
    out += in_acc * data->in_coeff;
    out = out >> data->in_coeff_shift;

    data->in_history[data->pos] = input;
    data->out_history[data->pos] = out;

	return i16_mult_shift(out, gain);
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

static void set_filter_coeffs(filter_data_t * data, float freq, float q){
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
    int16_t shift;

    round = B / sum_a;
    shift = coeff_shift(round);
    data->in_coeff = round * (1<<shift);
    data->in_coeff_shift = shift;

    round = 2.*(A[2]-A[0]) / sum_a;
    shift = coeff_shift(round);
    data->out_coeff[0] = round * (1<<shift);
    data->out_coeff_shift[0] = shift;

    round = (A[0]-A[1]+A[2]) / sum_a;
    shift = coeff_shift(round);
    data->out_coeff[1] = round * (1<<shift);
    data->out_coeff_shift[1] = shift;
}

void filter_run(radspa_t * filter, uint16_t num_samples, uint32_t render_pass_id){
    radspa_signal_t * output_sig = radspa_signal_get_by_index(filter, FILTER_OUTPUT);
    if(output_sig->buffer == NULL) return;
    filter_data_t * data = filter->plugin_data;
    radspa_signal_t * input_sig = radspa_signal_get_by_index(filter, FILTER_INPUT);
    radspa_signal_t * freq_sig = radspa_signal_get_by_index(filter, FILTER_FREQ);
    radspa_signal_t * q_sig = radspa_signal_get_by_index(filter, FILTER_Q);
    radspa_signal_t * gain_sig = radspa_signal_get_by_index(filter, FILTER_GAIN);

    static int16_t ret = 0;
    
    for(uint16_t i = 0; i < num_samples; i++){
        int16_t input = radspa_signal_get_value(input_sig, i, num_samples, render_pass_id);
        int32_t freq = radspa_signal_get_value(freq_sig, i, num_samples, render_pass_id);
        int16_t q = radspa_signal_get_value(q_sig, i, num_samples, render_pass_id);
        int16_t gain = radspa_signal_get_value(gain_sig, i, num_samples, render_pass_id);

        if((freq != data->prev_freq) | (q != data->prev_q)){
            set_filter_coeffs(data, freq, ((float) q + 1.)/1000.);
            data->prev_freq = freq;
            data->prev_q = q;
        }

        ret = apply_filter(data, input, gain);
        (output_sig->buffer)[i] = ret;
    }
    output_sig->value = ret;
}

radspa_t * filter_create(uint32_t real_init_var){
    radspa_t * filter = radspa_standard_plugin_create(&filter_desc, FILTER_NUM_SIGNALS, sizeof(filter_data_t));
    if(filter == NULL) return NULL;
    filter->render = filter_run;
    radspa_signal_set(filter, FILTER_OUTPUT, "output", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(filter, FILTER_INPUT, "input", RADSPA_SIGNAL_HINT_INPUT, 0);
    radspa_signal_set(filter, FILTER_FREQ, "freq", RADSPA_SIGNAL_HINT_INPUT, 500);
    radspa_signal_set(filter, FILTER_Q, "reso", RADSPA_SIGNAL_HINT_INPUT, 1000);
    radspa_signal_set(filter, FILTER_GAIN, "gain", RADSPA_SIGNAL_HINT_INPUT, 32760);
    filter_data_t * data = filter->plugin_data;
    data->pos = 0;
    data->prev_freq = 1<<24;
    return filter;
}
