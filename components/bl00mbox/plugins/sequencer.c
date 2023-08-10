#include "sequencer.h"

radspa_descriptor_t sequencer_desc = {
    .name = "sequencer",
    .id = 56709,
    .description = "i.o.u.",
    .create_plugin_instance = sequencer_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

#define SEQUENCER_NUM_SIGNALS 6
#define SEQUENCER_STEP 0
#define SEQUENCER_STEP_LEN 1
#define SEQUENCER_SYNC_OUT 2
#define SEQUENCER_SYNC_IN 3
#define SEQUENCER_BPM 4
#define SEQUENCER_BEAT_DIV 5
#define SEQUENCER_OUTPUT 6

static uint64_t target(uint64_t step_len, uint64_t bpm, uint64_t beat_div){
        if(bpm == 0) return 0;
        return (48000ULL * 60 * 4) / (bpm * beat_div);
}

radspa_t * sequencer_create(uint32_t init_var){
    //init_var:
    // lsbyte: number of channels
    // lsbyte+1: number of pixels in channel (>= bars*beats_div)
    uint32_t num_tracks = 1;
    uint32_t num_pixels = 16;
    if(init_var){
        num_tracks = init_var & 0xFF;
        num_pixels = (init_var>>8) & 0xFF;
    }
    uint32_t table_size = num_tracks * (num_pixels + 1);
    uint32_t num_signals = num_tracks + SEQUENCER_NUM_SIGNALS; //one for each channel output
    radspa_t * sequencer = radspa_standard_plugin_create(&sequencer_desc, num_signals, sizeof(sequencer_data_t), table_size);
    sequencer->render = sequencer_run;

    sequencer_data_t * data = sequencer->plugin_data;
    data->track_step_len = num_pixels;
    data->num_tracks = num_tracks;
    data->bpm_prev = 120;
    data->beat_div_prev = 16;
    data->counter_target = target(data->track_step_len, data->bpm_prev, data->beat_div_prev);
    radspa_signal_set(sequencer, SEQUENCER_STEP, "step", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(sequencer, SEQUENCER_STEP_LEN, "step_len", RADSPA_SIGNAL_HINT_INPUT, num_pixels);
    radspa_signal_set(sequencer, SEQUENCER_SYNC_OUT, "sync_out", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(sequencer, SEQUENCER_SYNC_IN, "sync_in", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_TRIGGER, 0);
    radspa_signal_set(sequencer, SEQUENCER_BPM, "bpm", RADSPA_SIGNAL_HINT_INPUT, data->bpm_prev);
    radspa_signal_set(sequencer, SEQUENCER_BEAT_DIV, "beat_div", RADSPA_SIGNAL_HINT_INPUT, data->beat_div_prev);
    radspa_signal_set(sequencer, SEQUENCER_OUTPUT, "output", RADSPA_SIGNAL_HINT_OUTPUT, 0);

    /*
    for(uint8_t i = 0; i < num_signals; i++){
        radspa_signal_set(sequencer, SEQUENCER_NUM_SIGNALS + i, "track", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    }
    */

    data->counter = 0;
    data->sync_in_prev = 0;
    data->sync_out = 32767;


    return sequencer;
}

/* ~table encoding~
 * first int16_t: track type:
 * -32767 : trigger track
 * 32767 : direct track
 * in between: slew rate 
 */

void sequencer_run(radspa_t * sequencer, uint16_t num_samples, uint32_t render_pass_id){
    radspa_signal_t * step_sig = radspa_signal_get_by_index(sequencer, SEQUENCER_STEP);
    radspa_signal_t * sync_out_sig = radspa_signal_get_by_index(sequencer, SEQUENCER_SYNC_OUT);
    radspa_signal_t * output_sig = radspa_signal_get_by_index(sequencer, SEQUENCER_OUTPUT);
    if((output_sig->buffer == NULL) && (sync_out_sig->buffer == NULL) && (step_sig->buffer == NULL)) return;

    radspa_signal_t * step_len_sig = radspa_signal_get_by_index(sequencer, SEQUENCER_STEP_LEN);
    radspa_signal_t * sync_in_sig = radspa_signal_get_by_index(sequencer, SEQUENCER_SYNC_IN);
    radspa_signal_t * bpm_sig = radspa_signal_get_by_index(sequencer, SEQUENCER_BPM);
    radspa_signal_t * beat_div_sig = radspa_signal_get_by_index(sequencer, SEQUENCER_BEAT_DIV);
    sequencer_data_t * data = sequencer->plugin_data;
    int16_t * table = sequencer->plugin_table;

    int16_t s1 = radspa_signal_get_value(step_len_sig, 0, num_samples, render_pass_id);
    int16_t s2 = data->track_step_len;
    data->step_target = s1 > 0 ? (s1 > s2 ? s2 : s1) : 1;

    int16_t bpm = bpm_sig->get_value(bpm_sig, 0, num_samples, render_pass_id);
    int16_t beat_div = beat_div_sig->get_value(beat_div_sig, 0, num_samples, render_pass_id);
    if((bpm != data->bpm_prev) || (beat_div != data->beat_div_prev)){
        data->counter_target = target(data->track_step_len, bpm, beat_div);
        data->bpm_prev = bpm;
        data->beat_div_prev = beat_div;
    }
    if(data->counter_target){
        for(uint16_t i = 0; i < num_samples; i++){
            data->counter++;
            if(data->counter >= data->counter_target){
                data->counter = 0;
                data->step++;
                if(data->step >= data->step_target){
                    data->step = 0;
                    data->sync_out = -data->sync_out;
                }
            }

            int16_t sync_in = sync_in_sig->get_value(sync_in_sig, i, num_samples, render_pass_id);
            if(((sync_in > 0) && (data->sync_in_prev <= 0)) || ((sync_in > 0) && (data->sync_in_prev <= 0))){
                data->counter = 0;
                data->step = 0;
                data->sync_out = -data->sync_out;
            }
            data->sync_in_prev = sync_in;
            
            if(!data->counter){ //event just happened
                for(uint8_t track = 0; track < data->num_tracks; track++){
                    int16_t type = table[track*data->track_step_len];
                    int16_t stage_val = table[data->step + 1 + data->track_step_len * track];
                    if(type == 32767){
                        data->track_fill[track] = stage_val;
                    } else if(type == -32767){
                        if(stage_val > 0) data->track_fill[track] = radspa_trigger_start(stage_val, &(data->trigger_hist[track]));
                        if(stage_val < 0) data->track_fill[track] = radspa_trigger_stop(&(data->trigger_hist[track]));
                    }
                }
            }
          
            if(output_sig->buffer != NULL) (output_sig->buffer)[i] = data->track_fill[0];
            if(sync_out_sig->buffer != NULL) (sync_out_sig->buffer)[i] = data->sync_out;
            if(step_sig->buffer != NULL) (step_sig->buffer)[i] = data->step;
        }
    }
    sync_out_sig->value = data->sync_out;
    output_sig->value = data->track_fill[0];
    step_sig->value = data->step;
}
