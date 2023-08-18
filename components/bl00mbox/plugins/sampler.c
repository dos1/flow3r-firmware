#include "sampler.h"

radspa_t * sampler_create(uint32_t init_var);
radspa_descriptor_t sampler_desc = {
    .name = "_sampler_ram",
    .id = 696969,
    .description = "simple sampler that stores a copy of the sample in ram and has basic recording functionality."
                   "\ninit_var: length of pcm sample memory\ntable layout: [0:2] read head position (uint32_t), [2:4] sample start (uint32_t), "
                   "[4:6] sample length (uint32_t), [6] new record event (bool), [7:init_var+7] pcm sample data (int16_t)",
    .create_plugin_instance = sampler_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

#define SAMPLER_NUM_SIGNALS 4
#define SAMPLER_OUTPUT 0
#define SAMPLER_TRIGGER 1
#define SAMPLER_REC_IN 2
#define SAMPLER_REC_TRIGGER 3
#define READ_HEAD_POS 0
#define SAMPLE_START 2
#define SAMPLE_LEN 4
#define BUFFER_OFFSET 7

static inline int16_t define_behavior(uint32_t in){
    int32_t ret = in & 0xFFFF;
    if(ret > 32767) return ret - 65535;
    return ret;
}

static inline void write_uint32_to_buffer_pos(int16_t * buf, uint32_t pos, uint32_t input){
    buf[pos] = define_behavior(input);
    buf[pos+1] = define_behavior(input>>16);
}

static inline uint32_t read_uint32_from_buffer_pos(int16_t * buf, uint32_t pos){
    int32_t lsb = buf[pos];
    if(lsb < 0) lsb += 65535;
    int32_t msb = buf[pos+1];
    if(msb < 0) msb += 65535;
    return (((uint32_t) msb) << 16) + ((uint32_t) lsb);
}

void sampler_run(radspa_t * sampler, uint16_t num_samples, uint32_t render_pass_id){
    radspa_signal_t * output_sig = radspa_signal_get_by_index(sampler, SAMPLER_OUTPUT);
    if(output_sig->buffer == NULL) return;
    sampler_data_t * data = sampler->plugin_data;
    int16_t * buf = sampler->plugin_table;
    radspa_signal_t * trigger_sig = radspa_signal_get_by_index(sampler, SAMPLER_TRIGGER);
    radspa_signal_t * rec_in_sig = radspa_signal_get_by_index(sampler, SAMPLER_REC_IN);
    radspa_signal_t * rec_trigger_sig = radspa_signal_get_by_index(sampler, SAMPLER_REC_TRIGGER);

    static int32_t ret = 0;
    
    uint32_t read_head_pos = read_uint32_from_buffer_pos(buf, READ_HEAD_POS);
    uint32_t sample_start = read_uint32_from_buffer_pos(buf, SAMPLE_START);
    uint32_t sample_len = read_uint32_from_buffer_pos(buf, SAMPLE_LEN);
    
    uint32_t buffer_size = sampler->plugin_table_len - 6;
    
    for(uint16_t i = 0; i < num_samples; i++){
        int16_t rec_trigger = radspa_trigger_get(rec_trigger_sig->get_value(rec_trigger_sig, i, num_samples, render_pass_id), &(data->rec_trigger_prev));
        int16_t trigger = radspa_trigger_get(trigger_sig->get_value(trigger_sig, i, num_samples, render_pass_id), &(data->trigger_prev));

        if(rec_trigger > 0){
            if(!(data->rec_active)){
                read_head_pos = sample_len;
                write_uint32_to_buffer_pos(buf, READ_HEAD_POS, read_head_pos);
                data->write_head_position = 0;
                sample_len = 0;
                write_uint32_to_buffer_pos(buf, SAMPLE_LEN, sample_len);
                data->rec_active = true;
            }
        } else if(rec_trigger < 0){
            if(data->rec_active){
                if(sample_len == buffer_size){
                    sample_start = data->write_head_position;
                    write_uint32_to_buffer_pos(buf, SAMPLE_START, sample_start);
                } else {
                    sample_start = 0;
                    write_uint32_to_buffer_pos(buf, SAMPLE_START, sample_start);
                }
                buf[6] = 1;
                data->rec_active = false;
            }
        }

        if(data->rec_active){
            int16_t rec_in = rec_in_sig->get_value(rec_in_sig, i, num_samples, render_pass_id);
            buf[data->write_head_position + BUFFER_OFFSET] = rec_in;
            data->write_head_position++;
            if(data->write_head_position >= buffer_size) data->write_head_position = 0;
            if(sample_len < buffer_size){
                sample_len++;
                write_uint32_to_buffer_pos(buf, SAMPLE_LEN, sample_len);
            }
        } else {
            if(trigger > 0){
                read_head_pos = 0;
                write_uint32_to_buffer_pos(buf, READ_HEAD_POS, read_head_pos);
                data->volume = trigger;
            } else if(trigger < 0){
                read_head_pos = sample_len;
                write_uint32_to_buffer_pos(buf, READ_HEAD_POS, read_head_pos);
            }

            if(read_head_pos < sample_len){
                uint32_t sample_offset_pos = read_head_pos + sample_start;
                if(sample_offset_pos >= sample_len) sample_offset_pos -= sample_len;
                ret = radspa_mult_shift(buf[sample_offset_pos + BUFFER_OFFSET], data->volume);
                read_head_pos++;
                write_uint32_to_buffer_pos(buf, READ_HEAD_POS, read_head_pos);
            } else {
                //ret = (ret * 255)>>8; // avoid dc clicks with bad samples
                ret = 0;
            }
            output_sig->set_value(output_sig, i, ret, num_samples, render_pass_id);
        }
    }
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
    sampler_data_t * data = sampler->plugin_data;
    data->trigger_prev = 0;
    data->rec_trigger_prev = 0;
    data->rec_active = false;
    //int16_t * buf = sampler->plugin_table;
    //write_uint32_to_buffer_pos(buf, SAMPLE_START, 0);
    //write_uint32_to_buffer_pos(buf, SAMPLE_LEN, sampler->plugin_table_len);
    return sampler;
}
