#include "slew_rate_limiter.h"

static inline int16_t waveshaper(int16_t saw, int16_t shape);

radspa_descriptor_t slew_rate_limiter_desc = {
    .name = "slew_rate_limiter",
    .id = 23,
    .description = "very cheap nonlinear filter",
    .create_plugin_instance = slew_rate_limiter_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

#define SLEW_RATE_LIMITER_NUM_SIGNALS 3
#define SLEW_RATE_LIMITER_OUTPUT 0
#define SLEW_RATE_LIMITER_INPUT 1
#define SLEW_RATE_LIMITER_SLEW_RATE 2

radspa_t * slew_rate_limiter_create(uint32_t init_var){
    radspa_t * slew_rate_limiter = radspa_standard_plugin_create(&slew_rate_limiter_desc, SLEW_RATE_LIMITER_NUM_SIGNALS, sizeof(slew_rate_limiter_data_t), 0);
    slew_rate_limiter->render = slew_rate_limiter_run;
    radspa_signal_set(slew_rate_limiter, SLEW_RATE_LIMITER_OUTPUT, "output", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(slew_rate_limiter, SLEW_RATE_LIMITER_INPUT, "input", RADSPA_SIGNAL_HINT_INPUT, 0);
    radspa_signal_set(slew_rate_limiter, SLEW_RATE_LIMITER_SLEW_RATE, "slew_rate", RADSPA_SIGNAL_HINT_INPUT, 1000);
    return slew_rate_limiter;
}

void slew_rate_limiter_run(radspa_t * slew_rate_limiter, uint16_t num_samples, uint32_t render_pass_id){
    radspa_signal_t * output_sig = radspa_signal_get_by_index(slew_rate_limiter, SLEW_RATE_LIMITER_OUTPUT);
    if(output_sig->buffer == NULL) return;
    radspa_signal_t * input_sig = radspa_signal_get_by_index(slew_rate_limiter, SLEW_RATE_LIMITER_INPUT);
    radspa_signal_t * slew_rate_sig = radspa_signal_get_by_index(slew_rate_limiter, SLEW_RATE_LIMITER_SLEW_RATE);

    slew_rate_limiter_data_t * data = slew_rate_limiter->plugin_data;

    int32_t ret = 0;
    for(uint16_t i = 0; i < num_samples; i++){
        int32_t input = input_sig->get_value(input_sig, i, num_samples, render_pass_id);
        int32_t slew_rate = (uint16_t) slew_rate_sig->get_value(slew_rate_sig, i, num_samples, render_pass_id);
        ret = data->prev;
        if(input - ret > slew_rate){
            ret += slew_rate;
        } else if(ret - input > slew_rate){
            ret -= slew_rate;
        } else {
            ret = input;
        }
        (output_sig->buffer)[i] = ret;
    }
    output_sig->value = ret;
}

