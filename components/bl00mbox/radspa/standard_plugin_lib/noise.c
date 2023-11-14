#include "noise.h"

radspa_t * noise_create(uint32_t init_var);
radspa_descriptor_t noise_desc = {
    .name = "noise",
    .id = 0,
    .description = "random data",
    .create_plugin_instance = noise_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

#define NOISE_NUM_SIGNALS 2
#define NOISE_OUTPUT 0
#define NOISE_SPEED 1

void noise_run(radspa_t * noise, uint16_t num_samples, uint32_t render_pass_id){
    radspa_signal_t * output_sig = radspa_signal_get_by_index(noise, NOISE_OUTPUT);
    radspa_signal_t * speed_sig = radspa_signal_get_by_index(noise, NOISE_SPEED);

    if(radspa_signal_get_value(speed_sig, 0, render_pass_id) < 0){
        radspa_signal_set_const_value(output_sig, radspa_random());
    } else {
        for(uint16_t i = 0; i < num_samples; i++){
            radspa_signal_set_value(output_sig, i, radspa_random());
        }
    }
}

radspa_t * noise_create(uint32_t init_var){
    radspa_t * noise = radspa_standard_plugin_create(&noise_desc, NOISE_NUM_SIGNALS, sizeof(char), 0);
    if(noise == NULL) return NULL;
    noise->render = noise_run;
    radspa_signal_set(noise, NOISE_OUTPUT, "output", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(noise, NOISE_SPEED, "speed", RADSPA_SIGNAL_HINT_INPUT, 32767);
    radspa_signal_get_by_index(noise, NOISE_SPEED)->unit = "{LFO:-32767} {AUDIO:32767}";
    return noise;
}
