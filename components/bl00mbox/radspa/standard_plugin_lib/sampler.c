#include "sampler.h"

radspa_t * sampler_create(uint32_t init_var);
radspa_descriptor_t sampler_desc = {
    .name = "sampler",
    .id = 696969,
    .description = "simple sampler that stores a copy of the sample in ram and has basic recording functionality."
                   "\ninit_var: length of pcm sample memory\ntable layout: [0:2] read head position (uint32_t), [2:4] write head position (uint32_t), "
                   "[4:6] sample start (uint32_t), [6:8] sample length (uint32_t), [8:10] sample rate (uint32_t), [10] sampler status "
                   "(int16_t bitmask), , [11:init_var+11] pcm sample data (int16_t)",
    .create_plugin_instance = sampler_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

#define SAMPLER_NUM_SIGNALS 5
#define SAMPLER_OUTPUT 0
#define SAMPLER_TRIGGER 1
#define SAMPLER_PITCH_SHIFT 2
#define SAMPLER_REC_TRIGGER 3
#define SAMPLER_REC_IN 4

#define READ_HEAD_POS 0
#define WRITE_HEAD_POS 2
#define SAMPLE_START 4
#define SAMPLE_LEN 6
#define SAMPLE_RATE 8
#define STATUS 10
#define STATUS_PLAYBACK_ACTIVE 0
#define STATUS_PLAYBACK_LOOP 1
#define STATUS_RECORD_ACTIVE 2
#define STATUS_RECORD_OVERFLOW 3
#define STATUS_RECORD_NEW_EVENT 4
#define BUFFER_OFFSET 11

void sampler_run(radspa_t * sampler, uint16_t num_samples, uint32_t render_pass_id){
    radspa_signal_t * output_sig = radspa_signal_get_by_index(sampler, SAMPLER_OUTPUT);
    radspa_signal_t * trigger_sig = radspa_signal_get_by_index(sampler, SAMPLER_TRIGGER);
    radspa_signal_t * rec_trigger_sig = radspa_signal_get_by_index(sampler, SAMPLER_REC_TRIGGER);
    radspa_signal_t * rec_in_sig = radspa_signal_get_by_index(sampler, SAMPLER_REC_IN);
    radspa_signal_t * pitch_shift_sig = radspa_signal_get_by_index(sampler, SAMPLER_PITCH_SHIFT);
    sampler_data_t * data = sampler->plugin_data;

    int16_t trigger = radspa_signal_get_const_value(trigger_sig, render_pass_id);
    bool trigger_const = trigger != RADSPA_SIGNAL_NONCONST;
    if(trigger_const) trigger = radspa_trigger_get(trigger, &(data->trigger_prev));
    int16_t rec_trigger = radspa_signal_get_const_value(rec_trigger_sig, render_pass_id);
    bool rec_trigger_const = rec_trigger != RADSPA_SIGNAL_NONCONST;
    if(rec_trigger_const) rec_trigger = radspa_trigger_get(rec_trigger, &(data->rec_trigger_prev));

    /*
    if((!data->playback_active) && (!data->rec_active) && (trigger_const) && (rec_trigger_const) && (!rec_trigger) && (!trigger)){
        radspa_signal_set_const_value(output_sig, 0);
        return;
    }
    */

    int16_t * buf = sampler->plugin_table;
    uint32_t * buf32 = (uint32_t *) buf;
    uint32_t sample_len = buf32[SAMPLE_LEN/2];
    uint32_t sample_start = buf32[SAMPLE_START/2];
    uint32_t sample_rate = buf32[SAMPLE_RATE/2];
    if(!sample_rate){
        sample_rate = 1;
        buf32[SAMPLE_RATE/2] = 1;
    }
    uint32_t buffer_size = sampler->plugin_table_len - BUFFER_OFFSET;
    uint64_t buffer_size_long = buffer_size * 48000;

    if(sample_len >= buffer_size) sample_len = buffer_size - 1;
    if(sample_start >= buffer_size) sample_start = buffer_size - 1;

    bool output_mute = !data->playback_active;
    bool output_const = sample_rate < 100;
    if(output_const){
        sample_rate *= num_samples;
        num_samples = 1;
    }

    int32_t ret = 0;
    for(uint16_t i = 0; i < num_samples; i++){
        if((!rec_trigger_const) || (!i)){
            if(!rec_trigger_const) rec_trigger = radspa_trigger_get(radspa_signal_get_value(rec_trigger_sig, i, render_pass_id), &(data->rec_trigger_prev));
            if(rec_trigger > 0){
                data->rec_active = true;
                data->write_head_pos_long = 0;
                data->write_steps = 0;
                data->write_head_pos_prev = -1;
                data->write_overflow = false;
                buf[STATUS] |= 1<<(STATUS_RECORD_NEW_EVENT);
            } else if((rec_trigger < 0) && data->rec_active){
                data->rec_active = false;
            }
        }
        if(data->rec_active){
            int16_t rec_in = radspa_signal_get_value(rec_in_sig, i, render_pass_id);
            uint32_t write_head_pos = (data->write_head_pos_long * 699) >> 25; // equiv to x/48000 (acc 0.008%)
            if(data->write_head_pos_prev == write_head_pos){
                if(data->write_steps){
                    data->rec_acc += rec_in;
                } else {
                    data->rec_acc = buf[write_head_pos + BUFFER_OFFSET];
                }
                data->write_steps++;
            } else {
                if(data->write_steps) buf[data->write_head_pos_prev + BUFFER_OFFSET] = data->rec_acc/data->write_steps;
                data->write_steps = 0;
                if(write_head_pos > data->write_head_pos_prev){
                    for(uint32_t j = data->write_head_pos_prev + 1; j <= write_head_pos; j++){
                        buf[j + BUFFER_OFFSET] = rec_in;
                    }
                } else {
                    uint32_t write_head_max = write_head_pos + buffer_size;
                    for(uint32_t j = data->write_head_pos_prev + 1; j <= write_head_max; j++){
                        uint32_t index = j;
                        if(index >= buffer_size) index -= buffer_size;
                        buf[index + BUFFER_OFFSET] = rec_in;
                    }
                }
                
            }

            if(!data->write_overflow) data->write_overflow = write_head_pos < data->write_head_pos_prev;
            data->write_head_pos_prev = write_head_pos;

            if(data->write_overflow & (!(buf[STATUS] & (1<<(STATUS_RECORD_OVERFLOW))))){
                data->rec_active = false;
            } else {
                if(data->write_overflow){
                    sample_start = (data->write_head_pos_long * 699) >> 25;
                    sample_len = buffer_size;
                } else {
                    sample_start = 0;
                    sample_len = (data->write_head_pos_long * 699) >> 25;
                }
                data->write_head_pos_long += sample_rate;
                while(data->write_head_pos_long >= buffer_size_long) data->write_head_pos_long -= buffer_size_long;
            }
        }

        if((!trigger_const) || (!i)){
            if(!trigger_const) trigger = radspa_trigger_get(radspa_signal_get_value(trigger_sig, i, render_pass_id), &(data->trigger_prev));
            if(trigger > 0){
                data->playback_active = true;
                data->read_head_pos_long = 0;
                data->volume = trigger;
                if(output_mute){
                    radspa_signal_set_values(output_sig, 0, i, 0);
                    output_mute = false;
                }
            } else if(trigger < 0){
                data->playback_active = false;
            }
        }

        int32_t read_head_pos;
        int8_t read_head_pos_subsample;
        if(data->playback_active){
            read_head_pos = (data->read_head_pos_long * 699) >> (25-6); // equiv to (x<<6)/48000 (acc 0.008%)
            read_head_pos_subsample = read_head_pos & 0b111111;
            read_head_pos = read_head_pos >> 6;
            if(read_head_pos >= sample_len){
                if(buf[STATUS] & (1<<(STATUS_PLAYBACK_LOOP))){
                    while(read_head_pos > sample_len){
                        data->read_head_pos_long -= (uint64_t) sample_len * 48000;
                        read_head_pos -= sample_len;
                    }
                } else {
                    data->playback_active = false;
                }
            }
        }
        if(data->playback_active){
            uint32_t sample_offset_pos = read_head_pos + sample_start;
            while(sample_offset_pos >= sample_len) sample_offset_pos -= sample_len;
            ret = buf[sample_offset_pos + BUFFER_OFFSET];
            if(read_head_pos_subsample){
                ret *= (64 - read_head_pos_subsample);
                sample_offset_pos++;
                if(sample_offset_pos >= sample_len) sample_offset_pos -= sample_len;
                ret += buf[sample_offset_pos + BUFFER_OFFSET] * read_head_pos_subsample;
                ret = ret >> 6;
            }
            ret = radspa_mult_shift(ret, data->volume);
            radspa_signal_set_value(output_sig, i, ret);

            int32_t pitch_shift = radspa_signal_get_value(pitch_shift_sig, i, render_pass_id);
            if(pitch_shift != data->pitch_shift_prev){
                data->pitch_shift_mult = radspa_sct_to_rel_freq(radspa_clip(pitch_shift - 18376 - 10986 - 4800), 0);
                if(data->pitch_shift_mult > (1<<13)) data->pitch_shift_mult = (1<<13);
                
                data->pitch_shift_prev = pitch_shift;
            }
            data->read_head_pos_long += (sample_rate * data->pitch_shift_mult) >> 11;
        } else {
            if(!output_mute) radspa_signal_set_value(output_sig, i, 0);
        }
    }
    if(output_mute || output_const) radspa_signal_set_const_value(output_sig, ret);
    buf32[SAMPLE_START/2] = sample_start;
    buf32[SAMPLE_LEN/2] = sample_len;
    if(data->playback_active){
        buf[STATUS] |= 1<<(STATUS_PLAYBACK_ACTIVE);
        buf32[READ_HEAD_POS/2] = (data->read_head_pos_long * 699) >> 25;;
    } else {
        buf[STATUS] &= ~(1<<(STATUS_PLAYBACK_ACTIVE));
        buf32[READ_HEAD_POS/2] = 0;
    }
    if(data->rec_active){
        buf[STATUS] |= 1<<(STATUS_RECORD_ACTIVE);
        buf32[WRITE_HEAD_POS/2] = (data->write_head_pos_long * 699) >> 25;;
    } else {
        buf[STATUS] &= ~(1<<(STATUS_RECORD_ACTIVE));
        buf32[WRITE_HEAD_POS/2] = 0;
    }
}

#define MAX_SAMPLE_LEN (48000UL*300)

radspa_t * sampler_create(uint32_t init_var){
    if(init_var == 0) return NULL; //doesn't make sense
    if(init_var > MAX_SAMPLE_LEN) init_var = MAX_SAMPLE_LEN;
    uint32_t buffer_size = init_var;
    radspa_t * sampler = radspa_standard_plugin_create(&sampler_desc, SAMPLER_NUM_SIGNALS, sizeof(sampler_data_t), buffer_size + BUFFER_OFFSET);
    if(sampler == NULL) return NULL;
    sampler->render = sampler_run;
    radspa_signal_set(sampler, SAMPLER_OUTPUT, "playback_output", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(sampler, SAMPLER_TRIGGER, "playback_trigger", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_TRIGGER, 0);
    radspa_signal_set(sampler, SAMPLER_PITCH_SHIFT, "playback_speed", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_SCT, 18367);
    radspa_signal_set(sampler, SAMPLER_REC_IN, "record_input", RADSPA_SIGNAL_HINT_INPUT, 0);
    radspa_signal_set(sampler, SAMPLER_REC_TRIGGER, "record_trigger", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_TRIGGER, 0);
    sampler_data_t * data = sampler->plugin_data;
    data->pitch_shift_mult = 1<<11;

    int16_t * buf = sampler->plugin_table;
    uint32_t * buf32 = (uint32_t *) buf;
    buf32[SAMPLE_RATE/2] = 48000;
    buf[STATUS] = 1<<(STATUS_RECORD_OVERFLOW);
    return sampler;
}
