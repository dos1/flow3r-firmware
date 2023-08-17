#include "flanger.h"

radspa_t * flanger_create(uint32_t init_var);
radspa_descriptor_t flanger_desc = {
    .name = "flanger",
    .id = 123,
    .description = "flanger with subsample interpolation and negative mix/resonance capability.\
                    does not come with lfo.",
    .create_plugin_instance = flanger_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

#define FLANGER_BUFFER_SIZE 4800
#define FIXED_POINT_DIGITS 4
#define VARIABLE_NAME ((FLANGER_BUFFER_SIZE)<<(FIXED_POINT_DIGITS))

#define FLANGER_NUM_SIGNALS 6
#define FLANGER_OUTPUT 0
#define FLANGER_INPUT 1
#define FLANGER_MANUAL 2
#define FLANGER_RESONANCE 3
#define FLANGER_LEVEL 4
#define FLANGER_MIX 5

static int16_t fixed_point_list_access(int32_t * buf, uint32_t fp_index, uint32_t buf_len){
    uint32_t index = (fp_index) >> (FIXED_POINT_DIGITS);
    while(index >= buf_len) index -= buf_len;
    uint32_t next_index = index + 1;
    while(next_index >= buf_len) next_index -= buf_len;

    uint32_t subindex = (fp_index) & ((1<<(FIXED_POINT_DIGITS)) - 1);
    int32_t ret =  buf[index] * ((1<<(FIXED_POINT_DIGITS)) - subindex);
    ret += (buf[next_index]) * subindex;
    ret = ret >> (FIXED_POINT_DIGITS);
    return radspa_clip(ret);
}

void flanger_run(radspa_t * flanger, uint16_t num_samples, uint32_t render_pass_id){
    radspa_signal_t * output_sig = radspa_signal_get_by_index(flanger, FLANGER_OUTPUT);
    if(output_sig->buffer == NULL) return;
    flanger_data_t * data = flanger->plugin_data;
    int32_t * buf = flanger->plugin_table;
    radspa_signal_t * input_sig = radspa_signal_get_by_index(flanger, FLANGER_INPUT);
    radspa_signal_t * manual_sig = radspa_signal_get_by_index(flanger, FLANGER_MANUAL);
    radspa_signal_t * reso_sig = radspa_signal_get_by_index(flanger, FLANGER_RESONANCE);
    radspa_signal_t * level_sig = radspa_signal_get_by_index(flanger, FLANGER_LEVEL);
    radspa_signal_t * mix_sig = radspa_signal_get_by_index(flanger, FLANGER_MIX);

    static int16_t ret = 0;

    for(uint16_t i = 0; i < num_samples; i++){
        int32_t manual = manual_sig->get_value(manual_sig, i, num_samples, render_pass_id);
        if(manual != data->manual_prev){
            // index propto 1/radspa_sct_to_rel_freq(manual) -> signflip faster
            int32_t invert = ((2400*(FIXED_POINT_DIGITS)) - 7572) - manual;
            uint32_t rho = radspa_sct_to_rel_freq(radspa_clip(invert), 0);
            if(rho > VARIABLE_NAME) rho = VARIABLE_NAME;
            data->read_head_offset = rho;
        }
        data->manual_prev = manual;

        data->write_head_position++;
        while(data->write_head_position >= FLANGER_BUFFER_SIZE) data->write_head_position -= FLANGER_BUFFER_SIZE;
        data->read_head_position = ((data->write_head_position)<<(FIXED_POINT_DIGITS));
        data->read_head_position -= data->read_head_offset;
        while(data->read_head_position < 0) data->read_head_position += VARIABLE_NAME; //underflow

        int32_t dry = input_sig->get_value(input_sig, i, num_samples, render_pass_id);
        int16_t reso = reso_sig->get_value(reso_sig, i, num_samples, render_pass_id);
        int16_t level = level_sig->get_value(level_sig, i, num_samples, render_pass_id);
        int16_t mix = mix_sig->get_value(mix_sig, i, num_samples, render_pass_id);

        buf[data->write_head_position] = dry;
        int32_t wet = fixed_point_list_access(buf, data->read_head_position, FLANGER_BUFFER_SIZE);
        buf[data->write_head_position] += radspa_mult_shift(wet, reso);
        
        int16_t dry_vol = (mix>0) ? (32767-mix) : (32767+mix); //always pos polarity

        ret = radspa_add_sat(radspa_mult_shift(dry, dry_vol), radspa_mult_shift(radspa_clip(wet), mix));
        ret = radspa_clip(radspa_gain(ret, level));
        output_sig->set_value(output_sig, i, ret, num_samples, render_pass_id);
    }
}

radspa_t * flanger_create(uint32_t init_var){
    radspa_t * flanger = radspa_standard_plugin_create(&flanger_desc, FLANGER_NUM_SIGNALS, sizeof(flanger_data_t),
                                                            2 * FLANGER_BUFFER_SIZE);
    if(flanger == NULL) return NULL;
    flanger_data_t * plugin_data = flanger->plugin_data;
    plugin_data->manual_prev = 40000;
    flanger->render = flanger_run;
    radspa_signal_set(flanger, FLANGER_OUTPUT, "output", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(flanger, FLANGER_INPUT, "input", RADSPA_SIGNAL_HINT_INPUT, 0);
    radspa_signal_set(flanger, FLANGER_MANUAL, "manual", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_SCT, 18367);
    radspa_signal_set(flanger, FLANGER_RESONANCE, "resonance", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_GAIN,
                            RADSPA_SIGNAL_VAL_UNITY_GAIN/2);
    radspa_signal_set(flanger, FLANGER_LEVEL, "level", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_GAIN,
                            RADSPA_SIGNAL_VAL_UNITY_GAIN);
    radspa_signal_set(flanger, FLANGER_MIX, "mix", RADSPA_SIGNAL_HINT_INPUT, 1<<14);
    return flanger;
}

#undef FLANGER_DLY_GAIN
#undef FLANGER_BUFFER_SIZE
#undef FLANGER_NUM_SIGNALS
#undef FLANGER_OUTPUT
#undef FLANGER_INPUT
#undef FLANGER_MANUAL
#undef FLANGER_RESONANCE
#undef FLANGER_LEVEL
#undef FLANGER_MIX
#undef FIXED_POINT_DIGITS

/* delay_time = 1/freq
 * delay_samples = delay_time * SAMPLE_RATE
 * freq(sct) = pow(2, (sct + 2708)/2400))
 * freq = sct_to_rel_freq(sct) * SAMPLE_RATE / UINT32_MAX
 * delay_samples = UINT32_MAX / sct_to_rel_freq(sct)
 * delay_samples = sct_to_rel_freq(-7572-sct + 2400 * FIXED_POINT_DIGITS)
 */
