#include "sequencer.h"

radspa_descriptor_t sequencer_desc = {
    .name = "sequencer",
    .id = 56709,
    .description = "i.o.u.",
    .create_plugin_instance = sequencer_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

#define SEQUENCER_NUM_SIGNALS 6
#define SEQUENCER_OUTPUT 0
#define SEQUENCER_INPUT 1
#define SEQUENCER_BPM 2
#define SEQUENCER_BEATS 3
#define SEQUENCER_BEAT_DIV 4
#define SEQUENCER_REPEATS 5

#define SEQUENCER_UNDERSAMPLING_POW 5

radspa_t * sequencer_create(uint32_t init_var){
    //init_var:
    // lsbyte: number of channels
    // lsbyte+1: number of pixels in channel (>= bars*beats_div)
    uint32_t num_tracks = init_var & 0xFF;
    uint32_t num_pixels = (init_var>>8) & 0xFF;
    uint32_t table_size = num_tracks * (num_pixels + 1);
    uint32_t num_signals = num_tracks + SEQUENCER_NUM_SIGNALS; //one for each channel output
    radspa_t * sequencer = radspa_standard_plugin_create(&sequencer_desc, num_signals, sizeof(sequencer_data_t), table_size);
    sequencer->render = sequencer_run;

    int16_t bpm = 120;
    int16_t beats = 1;
    int16_t beat_div = 4;
    int16_t repeats = 0;
    radspa_signal_set(sequencer, SEQUENCER_OUTPUT, "trigger_output", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(sequencer, SEQUENCER_INPUT, "trigger_input", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_TRIGGER, 0);
    radspa_signal_set(sequencer, SEQUENCER_BPM, "bpm", RADSPA_SIGNAL_HINT_INPUT, bpm);
    radspa_signal_set(sequencer, SEQUENCER_BEATS, "beats", RADSPA_SIGNAL_HINT_INPUT, beats);
    radspa_signal_set(sequencer, SEQUENCER_BEAT_DIV, "beat_div", RADSPA_SIGNAL_HINT_INPUT, beat_div);
    radspa_signal_set(sequencer, SEQUENCER_REPEATS, "repeats", RADSPA_SIGNAL_HINT_INPUT, repeats);

    for(uint8_t i = 0; i < num_signals; i++){
        radspa_signal_set(sequencer, SEQUENCER_NUM_SIGNALS + i, "track", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    }

    sequencer_data_t * plugin_data = sequencer->plugin_data;
    plugin_data->counter = 0;
    plugin_data->target = ((48000ULL>>SEQUENCER_UNDERSAMPLING_POW) * 60ULL * beats) / (bpm * beat_div);
    plugin_data->repeats_counter = 0;
    plugin_data->repeats = repeats;
    plugin_data->volume = 32767;
    plugin_data->trigger_in_prev = 0;
    plugin_data->trigger_out_prev = 0;

    plugin_data->track_step_len = num_pixels;
    plugin_data->num_tracks = num_tracks;

    return sequencer;
}

/* ~table encoding~
 * first int16_t: track type:
 * -32767 : trigger track
 * 32767 : direct track
 * in between: slew rate 
 */

static int16_t start_trigger(int16_t volume, int16_t * trigger_out_prev)
    int16_t ret = ((*trigger_out_prev) > 0) ? -volume : volume;
    (*trigger_out_prev) = ret;
    return ret;
}

void sequencer_run(radspa_t * sequencer, uint16_t num_samples, uint32_t render_pass_id){
    radspa_signal_t * output_sig = radspa_signal_get_by_index(sequencer, SEQUENCER_OUTPUT);
    if(output_sig->buffer == NULL) return;
    radspa_signal_t * input_sig = radspa_signal_get_by_index(sequencer, SEQUENCER_INPUT);
    radspa_signal_t * bpm_sig = radspa_signal_get_by_index(sequencer, SEQUENCER_BPM);
    radspa_signal_t * beats_sig = radspa_signal_get_by_index(sequencer, SEQUENCER_BEATS);
    radspa_signal_t * beat_div_sig = radspa_signal_get_by_index(sequencer, SEQUENCER_BEAT_DIV);
    radspa_signal_t * repeats_sig = radspa_signal_get_by_index(sequencer, SEQUENCER_REPEATS);
    sequencer_data_t * plugin_data = sequencer->plugin_data;

    int16_t ret = 0;
    int16_t trig = plugin_data->trigger_out_prev;
    for(uint16_t i = 0; i < num_samples; i++){

        plugin_data->counter++;
        if(plugin_data->counter >= plugin_data->counter_target){
            plugin_data->counter = 0;
            plugin_data->step++;
            if(plugin_data->step >= plugin_data->steps_target) plugin_data->steps = 0;
        }
        if(!plugin_data->counter){ //event just happened
            for(uint8_t track = 0; track < plugin_data->num_tracks; track++){
                int16_t type = plugin_table[track*plugin_data->track_steps_len];
                int16_t stage = plugin_table[plugin_data->step + 1 + plugin_data->track_steps_len * track];
                if(type == 32767){
                    plugin_data->track_fill[track] = stage;;
                } else if(type == -32767){
                    if(stage > 0) 
                    plugin_data->track_fill[track] = start_trigger(stage, &(plugin_data->track_prev[track]));
                }
            }
        }
      
        (output_sig->buffer)[i] = trig;
    }
    output_sig->value = ret;
}
