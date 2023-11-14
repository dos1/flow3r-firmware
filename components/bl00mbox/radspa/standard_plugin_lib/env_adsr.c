#include "env_adsr.h"

radspa_descriptor_t env_adsr_desc = {
    .name = "env_adsr",
    .id = 42,
    .description = "simple ADSR envelope",
    .create_plugin_instance = env_adsr_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

#define ENV_ADSR_NUM_SIGNALS 9
#define ENV_ADSR_OUTPUT 0
#define ENV_ADSR_INPUT 1
#define ENV_ADSR_ENV_OUTPUT 2
#define ENV_ADSR_TRIGGER 3
#define ENV_ADSR_ATTACK 4
#define ENV_ADSR_DECAY 5
#define ENV_ADSR_SUSTAIN 6
#define ENV_ADSR_RELEASE 7
#define ENV_ADSR_GAIN 8

#define ENV_ADSR_PHASE_OFF 0
#define ENV_ADSR_PHASE_ATTACK 1
#define ENV_ADSR_PHASE_DECAY 2
#define ENV_ADSR_PHASE_SUSTAIN 3
#define ENV_ADSR_PHASE_RELEASE 4

#define SAMPLE_RATE_SORRY 48000

static inline uint32_t env_adsr_time_ms_to_val_rise(int16_t time_ms, uint32_t val, uint16_t num_samples){
    if(!time_ms) return UINT32_MAX;
    if(time_ms < 0) time_ms = -time_ms;
    uint32_t div = time_ms * ((SAMPLE_RATE_SORRY)/1000);
    uint32_t input =  val/div;
    if(((uint64_t) input * num_samples) >> 32){
        return UINT32_MAX; // sat
    } else {
        return input * num_samples;
    }
}

static inline void update_attack_coeffs(radspa_t * env_adsr, uint16_t num_samples, uint32_t render_pass_id){
    env_adsr_data_t * data = env_adsr->plugin_data;
    int16_t attack = radspa_signal_get_value(&env_adsr->signals[ENV_ADSR_ATTACK], 0, render_pass_id);
    if((data->attack_prev_ms != attack) || (data->num_samples_prev != num_samples)){
        data->attack_raw = env_adsr_time_ms_to_val_rise(attack, UINT32_MAX, num_samples);
        data->attack_prev_ms = attack;
    }
}

static inline void update_release_coeffs(radspa_t * env_adsr, uint16_t num_samples, uint32_t render_pass_id){
    env_adsr_data_t * data = env_adsr->plugin_data;
    int16_t release = radspa_signal_get_value(&env_adsr->signals[ENV_ADSR_RELEASE], 0, render_pass_id);
    if((data->release_prev_ms != release) || (data->num_samples_prev != num_samples)){
        data->release_raw = env_adsr_time_ms_to_val_rise(release, data->release_init_val, num_samples);
        data->release_prev_ms = release;
    }
}

static inline void update_sustain_coeffs(radspa_t * env_adsr, uint16_t num_samples, uint32_t render_pass_id){
    env_adsr_data_t * data = env_adsr->plugin_data;
    int16_t sustain = radspa_signal_get_value(&env_adsr->signals[ENV_ADSR_SUSTAIN], 0, render_pass_id);
    sustain = sustain < 0 ? -sustain : sustain;
    data->sustain = ((uint32_t) sustain) << 17UL;
}

static inline void update_decay_coeffs(radspa_t * env_adsr, uint16_t num_samples, uint32_t render_pass_id){
    update_sustain_coeffs(env_adsr, num_samples, render_pass_id);
    env_adsr_data_t * data = env_adsr->plugin_data;
    int32_t decay = radspa_signal_get_value(&env_adsr->signals[ENV_ADSR_DECAY], 0, render_pass_id);
    if((data->decay_prev_ms != decay) || (data->sustain_prev != data->sustain) || (data->num_samples_prev != num_samples)){
        data->decay_raw = env_adsr_time_ms_to_val_rise(decay, UINT32_MAX - data->sustain, num_samples);
        data->decay_prev_ms = decay;
        data->sustain_prev = data->sustain;
    }
}


void env_adsr_run(radspa_t * env_adsr, uint16_t num_samples, uint32_t render_pass_id){
    env_adsr_data_t * data = env_adsr->plugin_data;

    uint16_t throwaway;
    int16_t vel = radspa_trigger_get_const(&env_adsr->signals[ENV_ADSR_TRIGGER], &data->trigger_prev, &throwaway, num_samples, render_pass_id);
    if(vel > 0 ){ // start
        data->env_phase = ENV_ADSR_PHASE_ATTACK;
        data->velocity = vel;
    } else if(vel < 0){ // stop
        if(data->env_phase != ENV_ADSR_PHASE_OFF){
            data->env_phase = ENV_ADSR_PHASE_RELEASE;
            data->release_init_val = data->env_counter;
        }
    }

    uint32_t tmp;
    switch(data->env_phase){
        case ENV_ADSR_PHASE_OFF:
            data->env_counter = 0;;
            break;
        case ENV_ADSR_PHASE_ATTACK:
            update_attack_coeffs(env_adsr, num_samples, render_pass_id);
            tmp = data->env_counter + data->attack_raw;
            if(tmp < data->env_counter){ // overflow
                tmp = ~((uint32_t) 0); // max out
                data->env_phase = ENV_ADSR_PHASE_DECAY;
            }
            data->env_counter = tmp;
            break;
        case ENV_ADSR_PHASE_DECAY:
            update_decay_coeffs(env_adsr, num_samples, render_pass_id);
            tmp = data->env_counter - data->decay_raw;
            if(tmp > data->env_counter){ // underflow
                tmp = 0; //bottom out
            }

            if(tmp <= data->sustain){
                tmp = data->sustain;
                data->env_phase = ENV_ADSR_PHASE_SUSTAIN;
            }

            data->env_counter = tmp;

            break;
        case ENV_ADSR_PHASE_SUSTAIN:
            update_sustain_coeffs(env_adsr, num_samples, render_pass_id);
            if(data->sustain == 0) data->env_phase = ENV_ADSR_PHASE_OFF;
            data->env_counter = data->sustain;
            break;
        case ENV_ADSR_PHASE_RELEASE:
            update_release_coeffs(env_adsr, num_samples, render_pass_id);
            tmp = data->env_counter - data->release_raw;
            if(tmp > data->env_counter){ // underflow
                tmp = 0; //bottom out
                data->env_phase = ENV_ADSR_PHASE_OFF;
            }
            data->env_counter = tmp;
            break;
    }
    data->num_samples_prev = num_samples;

    int32_t gain = radspa_signal_get_value(&env_adsr->signals[ENV_ADSR_GAIN], 0, render_pass_id);

    if((data->env_phase == ENV_ADSR_PHASE_OFF) || (!gain)){
        data->env_prev = 0;
        radspa_signal_set_const_value(&env_adsr->signals[ENV_ADSR_OUTPUT], 0);
        radspa_signal_set_const_value(&env_adsr->signals[ENV_ADSR_ENV_OUTPUT], 0);
        return;
    }

    int32_t env = data->env_counter >> 17;
    env = (env * data->velocity) >> 15;
    env = (env * gain) >> 15;
    radspa_signal_set_const_value(&env_adsr->signals[ENV_ADSR_ENV_OUTPUT], env);

    int16_t ret = radspa_signal_get_const_value(&env_adsr->signals[ENV_ADSR_INPUT], render_pass_id);
    if(ret == RADSPA_SIGNAL_NONCONST){
        int32_t env_slope = ((env - data->env_prev) << 15) / num_samples;
        for(uint16_t i = 0; i < num_samples; i++){
            ret = radspa_signal_get_value(&env_adsr->signals[ENV_ADSR_INPUT], i, render_pass_id);
            int32_t env_inter = data->env_prev + ((env_slope * i)>>15);
            ret = (ret * env_inter) >> 12;
            radspa_signal_set_value(&env_adsr->signals[ENV_ADSR_OUTPUT], i, ret);
        }
    } else {
        ret = (ret * env) >> 12;
        radspa_signal_set_const_value(&env_adsr->signals[ENV_ADSR_OUTPUT], ret);
    }
    data->env_prev = env;
}

radspa_t * env_adsr_create(uint32_t init_var){
    radspa_t * env_adsr = radspa_standard_plugin_create(&env_adsr_desc, ENV_ADSR_NUM_SIGNALS, sizeof(env_adsr_data_t), 0);
    env_adsr->render = env_adsr_run;
    radspa_signal_set(env_adsr, ENV_ADSR_OUTPUT, "output", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(env_adsr, ENV_ADSR_INPUT, "input", RADSPA_SIGNAL_HINT_INPUT, 32767);
    radspa_signal_set(env_adsr, ENV_ADSR_ENV_OUTPUT, "env_output", RADSPA_SIGNAL_HINT_OUTPUT | RADSPA_SIGNAL_HINT_GAIN, 0);
    radspa_signal_set(env_adsr, ENV_ADSR_TRIGGER, "trigger", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_TRIGGER, 0);
    radspa_signal_set(env_adsr, ENV_ADSR_ATTACK, "attack", RADSPA_SIGNAL_HINT_INPUT, 100);
    radspa_signal_set(env_adsr, ENV_ADSR_DECAY, "decay", RADSPA_SIGNAL_HINT_INPUT, 250);
    radspa_signal_set(env_adsr, ENV_ADSR_SUSTAIN, "sustain", RADSPA_SIGNAL_HINT_INPUT, 16000);
    radspa_signal_set(env_adsr, ENV_ADSR_RELEASE, "release", RADSPA_SIGNAL_HINT_INPUT, 50);
    radspa_signal_set(env_adsr, ENV_ADSR_GAIN, "gain", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_GAIN, RADSPA_SIGNAL_VAL_UNITY_GAIN);
    radspa_signal_get_by_index(env_adsr, ENV_ADSR_ATTACK)->unit = "ms";
    radspa_signal_get_by_index(env_adsr, ENV_ADSR_DECAY)->unit = "ms";
    radspa_signal_get_by_index(env_adsr, ENV_ADSR_SUSTAIN)->unit = "ms";

    env_adsr_data_t * data = env_adsr->plugin_data;
    data->trigger_prev = 0;
    data->env_phase = ENV_ADSR_PHASE_OFF;
    data->release_prev_ms = -1;
    data->release_init_val_prev = -1;
    data->attack_prev_ms = -1;
    data->sustain_prev = -1;
    data->decay_prev_ms = -1;
    data->env_prev = 0;

    return env_adsr;
}

