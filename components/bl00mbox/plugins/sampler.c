#include "sampler.h"

radspa_t * sampler_create(uint32_t init_var);
radspa_descriptor_t sampler_desc = {
    .name = "_sampler_ram",
    .id = 696969,
    .description = "simple sampler that stores a copy of the sample in ram and has basic recording functionality."
                   "\ninit_var: length of pcm sample memory\ntable layout: [0:2] read head position (uint32_t), [2:4] sample start (uint32_t), "
                   "[4:6] sample length (uint32_t), [6:8] sample rate (uint32_t, [8] new record event (bool), [9:init_var+9] pcm sample data (int16_t)",
    .create_plugin_instance = sampler_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

#define SAMPLER_NUM_SIGNALS 5
#define SAMPLER_OUTPUT 0
#define SAMPLER_TRIGGER 1
#define SAMPLER_REC_IN 2
#define SAMPLER_REC_TRIGGER 3
#define SAMPLER_PITCH_SHIFT 4

#define READ_HEAD_POS 0
#define SAMPLE_START 2
#define SAMPLE_LEN 4
#define SAMPLE_RATE 6
#define BUFFER_OFFSET 9

void sampler_run(radspa_t * sampler, uint16_t num_samples, uint32_t render_pass_id){
    radspa_signal_t * output_sig = radspa_signal_get_by_index(sampler, SAMPLER_OUTPUT);
    if(output_sig->buffer == NULL) return;
    sampler_data_t * data = sampler->plugin_data;
    int16_t * buf = sampler->plugin_table;
    uint32_t * buf32 = (uint32_t *) buf;
    radspa_signal_t * trigger_sig = radspa_signal_get_by_index(sampler, SAMPLER_TRIGGER);
    radspa_signal_t * rec_in_sig = radspa_signal_get_by_index(sampler, SAMPLER_REC_IN);
    radspa_signal_t * rec_trigger_sig = radspa_signal_get_by_index(sampler, SAMPLER_REC_TRIGGER);
    radspa_signal_t * pitch_shift_sig = radspa_signal_get_by_index(sampler, SAMPLER_PITCH_SHIFT);

    static int32_t ret = 0;
    
    uint32_t buffer_size = sampler->plugin_table_len - BUFFER_OFFSET;

    data->read_head_pos = buf32[READ_HEAD_POS/2];
    uint32_t sample_start = buf32[SAMPLE_START/2];
    uint32_t sample_len = buf32[SAMPLE_LEN/2];
    uint32_t sample_rate = buf32[SAMPLE_RATE/2];

    if(sample_start >= buffer_size) sample_start = buffer_size - 1;
    if(sample_len >= buffer_size) sample_len = buffer_size - 1;
    if(data->read_head_pos >= buffer_size) data->read_head_pos = buffer_size - 1;
    
    for(uint16_t i = 0; i < num_samples; i++){
        int16_t trigger = radspa_trigger_get(radspa_signal_get_value(trigger_sig, i, render_pass_id), &(data->trigger_prev));
        int16_t rec_trigger = radspa_trigger_get(radspa_signal_get_value(rec_trigger_sig, i, render_pass_id), &(data->rec_trigger_prev));

        if(rec_trigger > 0){
            if(!(data->rec_active)){
                data->read_head_pos = sample_len;
                data->write_head_pos = 0;
                data->write_head_pos_long = 0;
                sample_len = 0;
                data->rec_active = true;
            }
        } else if(rec_trigger < 0){
            if(data->rec_active){
                if(sample_len == buffer_size){
                    sample_start = data->write_head_pos;
                } else {
                    sample_start = 0;
                }
                buf[8] = 1;
                data->rec_active = false;
            }
        }
        

        if(data->rec_active){
            int16_t rec_in = radspa_signal_get_value(rec_in_sig, i, render_pass_id);
            buf[data->write_head_pos + BUFFER_OFFSET] = rec_in;

            data->write_head_pos_long += sample_rate;
            if(sample_rate == 48000){
                data->write_head_pos++;
            } else {
                data->write_head_pos = (data->write_head_pos_long * 699) >> 25; // equiv to _/48000 (acc 0.008%)
            }
            if(data->write_head_pos >= buffer_size){
                data->write_head_pos = 0;
                data->write_head_pos_long = 0;
            }
            if(sample_len < buffer_size){
                sample_len++;
                buf32[SAMPLE_LEN] = sample_len;
            }
        } else {
            if(trigger > 0){
                data->read_head_pos_long = 0;
                data->read_head_pos = 0;
                data->volume = trigger;
            } else if(trigger < 0){
                data->read_head_pos = sample_len;
            }

            if(data->read_head_pos < sample_len){
                uint32_t sample_offset_pos = data->read_head_pos + sample_start;
                if(sample_offset_pos >= sample_len) sample_offset_pos -= sample_len;
                ret = radspa_mult_shift(buf[sample_offset_pos + BUFFER_OFFSET], data->volume);

                int32_t pitch_shift = radspa_signal_get_value(pitch_shift_sig, i, render_pass_id);
                if(pitch_shift != data->pitch_shift_prev){
                    data->pitch_shift_mult = radspa_sct_to_rel_freq(radspa_clip(pitch_shift - 18376 - 10986 - 4800), 0);
                    if(data->pitch_shift_mult > (1<<13)) data->pitch_shift_mult = (1<<13);
                    
                    data->pitch_shift_prev = pitch_shift;
                }

                if(sample_rate == 48000 && data->pitch_shift_mult == (1<<11)){
                    data->read_head_pos_long += sample_rate;
                    data->read_head_pos++;
                } else {
                    data->read_head_pos_long += (sample_rate * data->pitch_shift_mult) >> 11;
                    data->read_head_pos = (data->read_head_pos_long * 699) >> 25; // equiv to _/48000 (acc 0.008%)
                }
            } else {
                //ret = (ret * 255)>>8; // avoid dc clicks with bad samples
                ret = 0;
            }
            radspa_signal_set_value(output_sig, i, ret);
        }
    }
    buf32[READ_HEAD_POS/2] = data->read_head_pos;
    buf32[SAMPLE_START/2] = sample_start;
    buf32[SAMPLE_LEN/2] = sample_len;
}

radspa_t * sampler_create(uint32_t init_var){
    if(init_var == 0) return NULL; //doesn't make sense
    uint32_t buffer_size = init_var;
    radspa_t * sampler = radspa_standard_plugin_create(&sampler_desc, SAMPLER_NUM_SIGNALS, sizeof(sampler_data_t), buffer_size + BUFFER_OFFSET);
    if(sampler == NULL) return NULL;
    sampler->render = sampler_run;
    radspa_signal_set(sampler, SAMPLER_OUTPUT, "output", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(sampler, SAMPLER_TRIGGER, "trigger", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_TRIGGER, 0);
    radspa_signal_set(sampler, SAMPLER_REC_IN, "rec_in", RADSPA_SIGNAL_HINT_INPUT, 0);
    radspa_signal_set(sampler, SAMPLER_REC_TRIGGER, "rec_trigger", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_TRIGGER, 0);
    radspa_signal_set(sampler, SAMPLER_PITCH_SHIFT, "pitch_shift", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_SCT, 18367);
    sampler_data_t * data = sampler->plugin_data;
    data->trigger_prev = 0;
    data->rec_trigger_prev = 0;
    data->rec_active = false;
    data->pitch_shift_mult = 1<<11;
    uint32_t * buf32 = (uint32_t *) sampler->plugin_table;
    buf32[SAMPLE_RATE/2] = 48000;
    //int16_t * buf = sampler->plugin_table;
    //write_uint32_to_buffer_pos(buf, SAMPLE_START, 0);
    //write_uint32_to_buffer_pos(buf, SAMPLE_LEN, sampler->plugin_table_len);
    return sampler;
}
