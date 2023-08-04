#include "sequence_timer.h"

radspa_descriptor_t sequence_timer_desc = {
    .name = "sequence_timer",
    .id = 777,
    .description = "creates one or more or infinite (repeats = 0) trigger output signal some time after having received an trigger input signal. time is defined by beats/(bpm*beat_div/4)",
    .create_plugin_instance = sequence_timer_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

#define SEQUENCE_TIMER_NUM_SIGNALS 6
#define SEQUENCE_TIMER_OUTPUT 0
#define SEQUENCE_TIMER_INPUT 1
#define SEQUENCE_TIMER_BPM 2
#define SEQUENCE_TIMER_BEATS 3
#define SEQUENCE_TIMER_BEAT_DIV 4
#define SEQUENCE_TIMER_REPEATS 5

#define SEQUENCE_TIMER_UNDERSAMPLING_POW 5

radspa_t * sequence_timer_create(uint32_t init_var){
    radspa_t * sequence_timer = radspa_standard_plugin_create(&sequence_timer_desc, SEQUENCE_TIMER_NUM_SIGNALS, sizeof(sequence_timer_data_t), 0);
    sequence_timer->render = sequence_timer_run;

    int16_t bpm = 120;
    int16_t beats = 1;
    int16_t beat_div = 4;
    int16_t repeats = 0;
    radspa_signal_set(sequence_timer, SEQUENCE_TIMER_OUTPUT, "trigger_output", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(sequence_timer, SEQUENCE_TIMER_INPUT, "trigger_input", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_TRIGGER, 0);
    radspa_signal_set(sequence_timer, SEQUENCE_TIMER_BPM, "bpm", RADSPA_SIGNAL_HINT_INPUT, bpm);
    radspa_signal_set(sequence_timer, SEQUENCE_TIMER_BEATS, "beats", RADSPA_SIGNAL_HINT_INPUT, beats);
    radspa_signal_set(sequence_timer, SEQUENCE_TIMER_BEAT_DIV, "beat_div", RADSPA_SIGNAL_HINT_INPUT, beat_div);
    radspa_signal_set(sequence_timer, SEQUENCE_TIMER_REPEATS, "repeats", RADSPA_SIGNAL_HINT_INPUT, repeats);

    sequence_timer_data_t * plugin_data = sequence_timer->plugin_data;
    plugin_data->counter = 0;
    plugin_data->target = ((48000ULL>>SEQUENCE_TIMER_UNDERSAMPLING_POW) * 60ULL * beats) / (bpm * beat_div);
    plugin_data->repeats_counter = 0;
    plugin_data->repeats = repeats;
    plugin_data->volume = 32767;
    plugin_data->trigger_in_prev = 0;
    plugin_data->trigger_out_prev = 0;

    return sequence_timer;
}

void sequence_timer_run(radspa_t * sequence_timer, uint16_t num_samples, uint32_t render_pass_id){
    radspa_signal_t * output_sig = radspa_signal_get_by_index(sequence_timer, SEQUENCE_TIMER_OUTPUT);
    if(output_sig->buffer == NULL) return;
    radspa_signal_t * input_sig = radspa_signal_get_by_index(sequence_timer, SEQUENCE_TIMER_INPUT);
    radspa_signal_t * bpm_sig = radspa_signal_get_by_index(sequence_timer, SEQUENCE_TIMER_BPM);
    radspa_signal_t * beats_sig = radspa_signal_get_by_index(sequence_timer, SEQUENCE_TIMER_BEATS);
    radspa_signal_t * beat_div_sig = radspa_signal_get_by_index(sequence_timer, SEQUENCE_TIMER_BEAT_DIV);
    radspa_signal_t * repeats_sig = radspa_signal_get_by_index(sequence_timer, SEQUENCE_TIMER_REPEATS);
    sequence_timer_data_t * plugin_data = sequence_timer->plugin_data;

    int16_t ret = 0;
    int16_t trig = plugin_data->trigger_out_prev;
    for(uint16_t i = 0; i < num_samples; i++){
        int16_t trigger_in = radspa_signal_get_value(input_sig, i, num_samples, render_pass_id);
        int16_t event = 0;
        if(trigger_in){
            if(plugin_data->trigger_in_prev == 0){
                event = trigger_in > 0 ? trigger_in : -trigger_in;
            } else if(plugin_data->trigger_in_prev > 0){
                if(trigger_in < 0) event = -trigger_in;
            } else {
                if(trigger_in > 0) event = trigger_in;
            }
        }
        plugin_data->trigger_in_prev = trigger_in;

        if(event > 0){
            int16_t bpm = radspa_signal_get_value(bpm_sig, i, num_samples, render_pass_id);
            int16_t beats = radspa_signal_get_value(beats_sig, i, num_samples, render_pass_id);
            int16_t beat_div = radspa_signal_get_value(beat_div_sig, i, num_samples, render_pass_id);
            int16_t repeats = radspa_signal_get_value(repeats_sig, i, num_samples, render_pass_id);
            plugin_data->counter = 0;
            plugin_data->target = ((48000ULL>>SEQUENCE_TIMER_UNDERSAMPLING_POW) * 60ULL *4* beats) / (bpm * beat_div);
            plugin_data->repeats_counter = 0;
            plugin_data->repeats = repeats;
            //plugin_data->volume = event;
        }
        bool trig_out_start = false;
      
        if(!(i%(1<<SEQUENCE_TIMER_UNDERSAMPLING_POW))){ 
            if(plugin_data->counter >= plugin_data->target){
                if(!plugin_data->repeats){
                    plugin_data->counter = 0;
                    trig_out_start = true;
                } else if(plugin_data->repeats_counter < plugin_data->repeats){ // didn't do all repeats;
                    plugin_data->repeats_counter++;
                    if(plugin_data->repeats_counter != plugin_data->repeats) plugin_data->counter = 0;
                    trig_out_start = true;
                }
            } else {
                plugin_data->counter++;
            }
        }

        if(trig_out_start){
            if(plugin_data->trigger_out_prev > 0){
                trig = -plugin_data->volume;
            } else {
                trig = plugin_data->volume;
            }
            ret = trig;
            plugin_data->trigger_out_prev = trig;
        }
        (output_sig->buffer)[i] = trig;
    }
    output_sig->value = ret;
}
