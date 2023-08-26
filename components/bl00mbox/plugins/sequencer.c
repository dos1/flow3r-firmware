#include "sequencer.h"

radspa_descriptor_t sequencer_desc = {
    .name = "_sequencer",
    .id = 56709,
    .description =  "sequencer that can output triggers or general control signals, best enjoyed through the "
                    "'sequencer' patch.\ninit_var: 1st byte (lsb): number of tracks, 2nd byte: number of steps"
                    "\ntable encoding (all int16_t): index 0: track type (-32767: trigger track, 32767: direct "
                    "track). next 'number of steps' indices: track data (repeat for number of tracks)",
    .create_plugin_instance = sequencer_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

#define SEQUENCER_NUM_SIGNALS 7
#define SEQUENCER_STEP 0
#define SEQUENCER_SYNC_OUT 1
#define SEQUENCER_SYNC_IN 2
#define SEQUENCER_START_STEP 3
#define SEQUENCER_END_STEP 4
#define SEQUENCER_BPM 5
#define SEQUENCER_BEAT_DIV 6

// mpx'd
#define SEQUENCER_OUTPUT 7

static uint64_t target(uint64_t step_len, uint64_t bpm, uint64_t beat_div){
        if(bpm == 0) return 0;
        return (48000ULL * 60 * 4) / (bpm * beat_div);
}

void sequencer_run(radspa_t * sequencer, uint16_t num_samples, uint32_t render_pass_id){
    bool output_request = false;
    sequencer_data_t * data = sequencer->plugin_data;
    radspa_signal_t * track_sigs[data->num_tracks];

    for(uint8_t j = 0; j < data->num_tracks; j++){
        track_sigs[j] = radspa_signal_get_by_index(sequencer, SEQUENCER_OUTPUT+j);
        if(track_sigs[j]->buffer != NULL) output_request = true;
    }

    radspa_signal_t * step_sig = radspa_signal_get_by_index(sequencer, SEQUENCER_STEP);
    if(step_sig->buffer != NULL) output_request = true;
    radspa_signal_t * sync_out_sig = radspa_signal_get_by_index(sequencer, SEQUENCER_SYNC_OUT);
    if(sync_out_sig->buffer != NULL) output_request = true;
    if(!output_request) return;

    radspa_signal_t * sync_in_sig = radspa_signal_get_by_index(sequencer, SEQUENCER_SYNC_IN);
    radspa_signal_t * start_step_sig = radspa_signal_get_by_index(sequencer, SEQUENCER_START_STEP);
    radspa_signal_t * end_step_sig = radspa_signal_get_by_index(sequencer, SEQUENCER_END_STEP);
    radspa_signal_t * bpm_sig = radspa_signal_get_by_index(sequencer, SEQUENCER_BPM);
    radspa_signal_t * beat_div_sig = radspa_signal_get_by_index(sequencer, SEQUENCER_BEAT_DIV);

    int16_t * table = sequencer->plugin_table;

    int16_t s1 = radspa_signal_get_value(end_step_sig, 0, num_samples, render_pass_id);
    int16_t s2 = data->track_step_len - 1;
    data->step_end = s1 > 0 ? (s1 > s2 ? s2 : s1) : 1;
    data->step_start = radspa_signal_get_value(start_step_sig, 0, num_samples, render_pass_id);

    int16_t bpm = bpm_sig->get_value(bpm_sig, 0, num_samples, render_pass_id);
    int16_t beat_div = beat_div_sig->get_value(beat_div_sig, 0, num_samples, render_pass_id);
    if((bpm != data->bpm_prev) || (beat_div != data->beat_div_prev)){
        data->counter_target = target(data->track_step_len, bpm, beat_div);
        data->bpm_prev = bpm;
        data->beat_div_prev = beat_div;
    }

    for(uint16_t i = 0; i < num_samples; i++){
        int16_t sync_in = radspa_trigger_get(sync_in_sig->get_value(sync_in_sig, i, num_samples, render_pass_id),
                &(data->sync_in_hist));
        if(sync_in > 0){
            data->counter = 0;
            data->step = data->step_start;
            data->sync_out_start = true;
        } else if(sync_in < 0){
            data->counter_target = 0; // stop signal
            data->sync_out_stop = true;
        }

        if(data->counter_target){
            data->counter++;

            if(data->counter >= data->counter_target){
                data->counter = 0;
                data->step++;
                if(data->step > data->step_end){
                    data->step = data->step_start;
                    data->sync_out_start = true;
                }
            }

            if(!data->counter){ //event just happened
                for(uint8_t j = 0; j < data->num_tracks; j++){
                    int16_t type = table[j * (data->track_step_len + 1)];
                    int16_t stage_val = table[data->step + 1 + (1 + data->track_step_len) * j];
                    if(type == 32767){
                        data->tracks[j].track_fill = stage_val;
                    } else if(type == -32767){
                        if(stage_val > 0) data->tracks[j].track_fill = radspa_trigger_start(stage_val, &(data->tracks[j].trigger_hist));
                        if(stage_val < 0) data->tracks[j].track_fill = radspa_trigger_stop(&(data->tracks[j].trigger_hist));
                    }
                }
            }
          
            for(uint8_t j = 0; j < data->num_tracks; j++){
                track_sigs[j]->set_value(track_sigs[j], i, data->tracks[j].track_fill, num_samples, render_pass_id);
            }
        }

        int16_t sync_out = 0;
        if(data->sync_out_start){
            sync_out = radspa_trigger_start(sync_in, &(data->sync_out_hist));
        } else if(data->sync_out_stop){
            sync_out = radspa_trigger_stop(&(data->sync_out_hist));
        }
        sync_out_sig->set_value(sync_out_sig, i, sync_out, num_samples, render_pass_id);
        step_sig->set_value(step_sig, i, data->step, num_samples, render_pass_id);
    }
}

radspa_t * sequencer_create(uint32_t init_var){
    uint32_t num_tracks = 4;
    uint32_t num_pixels = 16;
    if(init_var){
        num_tracks = init_var & 0xFF;
        num_pixels = (init_var>>8) & 0xFF;
    }
    if(!num_tracks) return NULL;
    if(!num_pixels) return NULL;

    uint32_t table_size = num_tracks * (num_pixels + 1);
    uint32_t num_signals = num_tracks + SEQUENCER_NUM_SIGNALS; //one for each channel output
    size_t data_size = sizeof(sequencer_data_t) + sizeof(sequencer_track_data_t) * (num_tracks - 1);
    radspa_t * sequencer = radspa_standard_plugin_create(&sequencer_desc, num_signals, data_size, table_size);
    if(sequencer == NULL) return NULL;

    sequencer->render = sequencer_run;

    sequencer_data_t * data = sequencer->plugin_data;
    data->track_step_len = num_pixels;
    data->num_tracks = num_tracks;
    data->bpm_prev = 120;
    data->beat_div_prev = 16;
    data->counter_target = target(data->track_step_len, data->bpm_prev, data->beat_div_prev);
    radspa_signal_set(sequencer, SEQUENCER_STEP, "step", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(sequencer, SEQUENCER_SYNC_OUT, "sync_out", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(sequencer, SEQUENCER_SYNC_IN, "sync_in", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_TRIGGER, 0);
    radspa_signal_set(sequencer, SEQUENCER_START_STEP, "step_start", RADSPA_SIGNAL_HINT_INPUT, 0);
    radspa_signal_set(sequencer, SEQUENCER_END_STEP, "step_end", RADSPA_SIGNAL_HINT_INPUT, num_pixels-1);
    radspa_signal_set(sequencer, SEQUENCER_BPM, "bpm", RADSPA_SIGNAL_HINT_INPUT, data->bpm_prev);
    radspa_signal_set(sequencer, SEQUENCER_BEAT_DIV, "beat_div", RADSPA_SIGNAL_HINT_INPUT, data->beat_div_prev);
    radspa_signal_set_group(sequencer, data->num_tracks, 1, SEQUENCER_OUTPUT, "track",
            RADSPA_SIGNAL_HINT_OUTPUT | RADSPA_SIGNAL_HINT_TRIGGER, 0);

    data->counter = 0;
    data->sync_in_hist = 0;
    data->sync_out_hist = 0;
    data->sync_out_start = false;
    data->sync_out_stop = false;

    return sequencer;
}
