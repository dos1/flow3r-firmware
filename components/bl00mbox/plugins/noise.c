#include "noise.h"

radspa_t * noise_create(uint32_t init_var);
radspa_descriptor_t noise_desc = {
    .name = "noise",
    .id = 0,
    .description = "random data",
    .create_plugin_instance = noise_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

#define NOISE_NUM_SIGNALS 1
#define NOISE_OUTPUT 0

void noise_run(radspa_t * noise, uint16_t num_samples, uint32_t render_pass_id){
    radspa_signal_t * output_sig = radspa_signal_get_by_index(noise, NOISE_OUTPUT);
    if(output_sig->buffer == NULL) return;

    for(uint16_t i = 0; i < num_samples; i++){
        int16_t ret = radspa_random();
        output_sig->set_value(output_sig, i, ret, num_samples, render_pass_id);
    }
}

radspa_t * noise_create(uint32_t init_var){
    radspa_t * noise = radspa_standard_plugin_create(&noise_desc, NOISE_NUM_SIGNALS, sizeof(char), 0);
    if(noise == NULL) return NULL;
    noise->render = noise_run;
    radspa_signal_set(noise, NOISE_OUTPUT, "output", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    return noise;
}
