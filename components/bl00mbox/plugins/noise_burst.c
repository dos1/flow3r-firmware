#include "noise_burst.h"

radspa_descriptor_t noise_burst_desc = {
    .name = "noise_burst",
    .id = 7,
    .description = "outputs flat noise upon trigger input for an amount of milliseconds",
    .create_plugin_instance = noise_burst_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

#define NOISE_BURST_NUM_SIGNALS 3
#define NOISE_BURST_OUTPUT 0
#define NOISE_BURST_TRIGGER 1
#define NOISE_BURST_LENGTH_MS 2

radspa_t * noise_burst_create(uint32_t init_var){
    radspa_t * noise_burst = radspa_standard_plugin_create(&noise_burst_desc, NOISE_BURST_NUM_SIGNALS, sizeof(noise_burst_data_t),0);
    noise_burst->render = noise_burst_run;
    radspa_signal_set(noise_burst, NOISE_BURST_OUTPUT, "output", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(noise_burst, NOISE_BURST_TRIGGER, "trigger", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_TRIGGER, 0);
    radspa_signal_set(noise_burst, NOISE_BURST_LENGTH_MS, "length", RADSPA_SIGNAL_HINT_INPUT, 100);
    radspa_signal_get_by_index(noise_burst, NOISE_BURST_LENGTH_MS)->unit = "ms";

    noise_burst_data_t * data = noise_burst->plugin_data;
    data->counter = 15;
    data->limit = 15;

    return noise_burst;
}

#define SAMPLE_RATE_SORRY 48000


void noise_burst_run(radspa_t * noise_burst, uint16_t num_samples, uint32_t render_pass_id){
    noise_burst_data_t * plugin_data = noise_burst->plugin_data;
    radspa_signal_t * output_sig = radspa_signal_get_by_index(noise_burst, NOISE_BURST_OUTPUT);
    if(output_sig->buffer == NULL) return;
    radspa_signal_t * trigger_sig = radspa_signal_get_by_index(noise_burst, NOISE_BURST_TRIGGER);
    radspa_signal_t * length_ms_sig = radspa_signal_get_by_index(noise_burst, NOISE_BURST_LENGTH_MS);

    int16_t ret = output_sig->value;

    for(uint16_t i = 0; i < num_samples; i++){

        int16_t trigger = trigger_sig->get_value(trigger_sig, i, num_samples, render_pass_id);
        int16_t vel = radspa_trigger_get(trigger, &(plugin_data->trigger_prev));

        if(vel < 0){ // stop
            plugin_data->counter = plugin_data->limit;
        } else if(vel > 0 ){ // start
            plugin_data->counter = 0;
            int32_t length_ms = trigger_sig->get_value(length_ms_sig, i, num_samples, render_pass_id);
            plugin_data->limit = length_ms * 48;
        }
        if(plugin_data->counter < plugin_data->limit){
            ret = radspa_random();
            plugin_data->counter++;
        } else {
            ret = 0;
        }
        if(output_sig->buffer != NULL) (output_sig->buffer)[i] = ret;
    }
    output_sig->value = ret;
}
