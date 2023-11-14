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
    return noise_burst;
}

#define SAMPLE_RATE_SORRY 48000

void noise_burst_run(radspa_t * noise_burst, uint16_t num_samples, uint32_t render_pass_id){
    noise_burst_data_t * plugin_data = noise_burst->plugin_data;
    radspa_signal_t * output_sig = radspa_signal_get_by_index(noise_burst, NOISE_BURST_OUTPUT);
    radspa_signal_t * trigger_sig = radspa_signal_get_by_index(noise_burst, NOISE_BURST_TRIGGER);
    radspa_signal_t * length_ms_sig = radspa_signal_get_by_index(noise_burst, NOISE_BURST_LENGTH_MS);

    int16_t trigger = radspa_signal_get_const_value(trigger_sig, render_pass_id);
    bool trigger_const = trigger != RADSPA_SIGNAL_NONCONST;
    int16_t vel = radspa_trigger_get(trigger, &(plugin_data->trigger_prev));

    bool output_const = plugin_data->counter == plugin_data->limit;

    // using output_const as proxy if system is running (will no longer be true further down)
    if((trigger_const) && ((vel < 0) || ((!vel) && output_const))){
        if(!plugin_data->hold) plugin_data->last_out = 0;
        plugin_data->counter = plugin_data->limit;
        radspa_signal_set_const_value(output_sig, plugin_data->last_out);
        return;
    }

    int16_t out = plugin_data->last_out;

    for(uint16_t i = 0; i < num_samples; i++){
        if(!(i && trigger_const)){
            if(!trigger_const){
                trigger = radspa_signal_get_value(trigger_sig, i, render_pass_id);
                vel = radspa_trigger_get(trigger, &(plugin_data->trigger_prev));
            }

            if(vel < 0){ // stop
                plugin_data->counter = plugin_data->limit;
            } else if(vel > 0 ){ // start
                if(output_const){
                    output_const = false;
                    radspa_signal_set_values(output_sig, 0, i, out);
                }
                plugin_data->counter = 0;
                int32_t length_ms = radspa_signal_get_value(length_ms_sig, i, render_pass_id);
                if(length_ms > 0){
                    plugin_data->hold = false;
                    plugin_data->limit = length_ms * ((SAMPLE_RATE_SORRY) / 1000);
                } else {
                    plugin_data->hold = true;
                    if(length_ms){
                        plugin_data->limit = -length_ms * ((SAMPLE_RATE_SORRY) / 1000);
                    } else {
                        plugin_data->limit = 1;
                    }
                }
            }
        }
        if(plugin_data->counter < plugin_data->limit){
            out = radspa_random();
            plugin_data->counter++;
        } else if(!plugin_data->hold){
            out = 0;
        }
        if(!output_const) radspa_signal_set_value(output_sig, i, out);
    }
    if(output_const) radspa_signal_set_const_value(output_sig, out);
    plugin_data->last_out = out;
}
