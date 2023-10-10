#include "env_adsr.h"

radspa_descriptor_t env_adsr_desc = {
    .name = "env_adsr",
    .id = 42,
    .description = "simple ADSR envelope",
    .create_plugin_instance = env_adsr_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

#define ENV_ADSR_NUM_SIGNALS 8
#define ENV_ADSR_OUTPUT 0
#define ENV_ADSR_INPUT 1
#define ENV_ADSR_TRIGGER 2
#define ENV_ADSR_ATTACK 3
#define ENV_ADSR_DECAY 4
#define ENV_ADSR_SUSTAIN 5
#define ENV_ADSR_RELEASE 6
#define ENV_ADSR_GAIN 7

#define ENV_ADSR_PHASE_OFF 0
#define ENV_ADSR_PHASE_ATTACK 1
#define ENV_ADSR_PHASE_DECAY 2
#define ENV_ADSR_PHASE_SUSTAIN 3
#define ENV_ADSR_PHASE_RELEASE 4

radspa_t * env_adsr_create(uint32_t init_var){
    radspa_t * env_adsr = radspa_standard_plugin_create(&env_adsr_desc, ENV_ADSR_NUM_SIGNALS, sizeof(env_adsr_data_t),0);
    env_adsr->render = env_adsr_run;
    radspa_signal_set(env_adsr, ENV_ADSR_OUTPUT, "output", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(env_adsr, ENV_ADSR_INPUT, "input", RADSPA_SIGNAL_HINT_INPUT, 32767);
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
    data->env_counter_prev = 0;
    data->env_pre_gain = 0;

    data->output_sig = radspa_signal_get_by_index(env_adsr, ENV_ADSR_OUTPUT);
    data->input_sig = radspa_signal_get_by_index(env_adsr, ENV_ADSR_INPUT);
    data->trigger_sig = radspa_signal_get_by_index(env_adsr, ENV_ADSR_TRIGGER);
    data->attack_sig = radspa_signal_get_by_index(env_adsr, ENV_ADSR_ATTACK);
    data->decay_sig = radspa_signal_get_by_index(env_adsr, ENV_ADSR_DECAY);
    data->sustain_sig = radspa_signal_get_by_index(env_adsr, ENV_ADSR_SUSTAIN);
    data->release_sig = radspa_signal_get_by_index(env_adsr, ENV_ADSR_RELEASE);
    data->gain_sig = radspa_signal_get_by_index(env_adsr, ENV_ADSR_GAIN);

    return env_adsr;
}

#define SAMPLE_RATE_SORRY 48000
#define ENV_ADSR_UNDERSAMPLING 5


static inline uint32_t env_adsr_time_ms_to_val_rise(int16_t time_ms, uint32_t val, uint16_t leftshift){
    if(!time_ms) return UINT32_MAX;
    if(time_ms < 0) time_ms = -time_ms;
    uint32_t div = time_ms * ((SAMPLE_RATE_SORRY)/1000);
    uint32_t input =  val/div;
    if(!leftshift) return input; // nothing to do
    if(input >> (32-leftshift)) return UINT32_MAX; // sat
    return input << leftshift;
}

void env_adsr_run(radspa_t * env_adsr, uint16_t num_samples, uint32_t render_pass_id){
    env_adsr_data_t * data = env_adsr->plugin_data;
    int16_t trigger_const = radspa_signal_get_const_value(data->trigger_sig, render_pass_id);

    bool idle = data->env_phase == ENV_ADSR_PHASE_OFF;
    if((trigger_const != RADSPA_SIGNAL_NONCONST) && idle){
        int16_t tmp = data->trigger_prev;
        if(!radspa_trigger_get(trigger_const, &tmp)){
            radspa_signal_set_const_value(data->output_sig, 0);
            return;
        }
    }

    int16_t trigger = trigger_const;

    int16_t attack = radspa_signal_get_value(data->attack_sig, 0, render_pass_id);
    int16_t decay = radspa_signal_get_value(data->decay_sig, 0, render_pass_id);
    int32_t sustain = radspa_signal_get_value(data->sustain_sig, 0, render_pass_id);
    int16_t release = radspa_signal_get_value(data->release_sig, 0, render_pass_id);
    int32_t gain = radspa_signal_get_value(data->gain_sig, 0, render_pass_id);

    if(data->attack_prev_ms != attack){
        data->attack_raw = env_adsr_time_ms_to_val_rise(attack, UINT32_MAX, ENV_ADSR_UNDERSAMPLING);
        data->attack_prev_ms = attack;
    }

    data->sustain = ((uint32_t) sustain) << 17UL;

    if((data->decay_prev_ms != decay) || (data->sustain_prev != data->sustain)){
        data->decay_raw = env_adsr_time_ms_to_val_rise(decay, UINT32_MAX - data->sustain, ENV_ADSR_UNDERSAMPLING);
        data->decay_prev_ms = decay;
        data->sustain_prev = data->sustain;
    }

    if(data->release_prev_ms != release){
        data->release_raw = env_adsr_time_ms_to_val_rise(release, data->release_init_val, ENV_ADSR_UNDERSAMPLING);
        data->release_prev_ms = release;
    }

    int32_t env = 0;

    for(uint16_t i = 0; i < num_samples; i++){
        int16_t ret = 0;

        if((trigger_const == RADSPA_SIGNAL_NONCONST) || (!i)){
            if(trigger_const == RADSPA_SIGNAL_NONCONST) trigger = radspa_signal_get_value(data->trigger_sig, i, render_pass_id);

            int16_t vel = radspa_trigger_get(trigger, &(data->trigger_prev));

            if(vel < 0){ // stop
                if(data->env_phase != ENV_ADSR_PHASE_OFF){
                    data->env_phase = ENV_ADSR_PHASE_RELEASE;
                    data->release_init_val = data->env_counter;
                    data->release_raw = env_adsr_time_ms_to_val_rise(release, data->release_init_val, ENV_ADSR_UNDERSAMPLING);
                }
            } else if(vel > 0 ){ // start
                data->env_phase = ENV_ADSR_PHASE_ATTACK;
                data->velocity = ((uint32_t) vel) << 17;
                if(idle){
                    for(uint16_t j = 0; j < i; j++){
                        radspa_signal_set_value(data->output_sig, j, 0);
                    }
                    idle = false;
                }
            }
        }

        if(idle) continue;

        if(!(i%(1<<ENV_ADSR_UNDERSAMPLING))){
            uint32_t tmp;
            switch(data->env_phase){
                case ENV_ADSR_PHASE_OFF:
                    data->env_counter = 0;;
                    break;
                case ENV_ADSR_PHASE_ATTACK:
                    tmp = data->env_counter + data->attack_raw;
                    if(tmp < data->env_counter){ // overflow
                        tmp = ~((uint32_t) 0); // max out
                        data->env_phase = ENV_ADSR_PHASE_DECAY;
                    }
                    data->env_counter = tmp;
                    break;
                case ENV_ADSR_PHASE_DECAY:
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
                    if(data->sustain == 0) data->env_phase = ENV_ADSR_PHASE_OFF;
                    data->env_counter = data->sustain;
                    break;
                case ENV_ADSR_PHASE_RELEASE:
                    tmp = data->env_counter - data->release_raw;
                    if(tmp > data->env_counter){ // underflow
                        tmp = 0; //bottom out
                        data->env_phase = ENV_ADSR_PHASE_OFF;
                    }
                    data->env_counter = tmp;
                    break;
            }
            if(data->env_counter != data->env_counter_prev){
                int32_t tmp;
                tmp = data->env_counter >> 17;
                tmp = (tmp * (tmp + 1)) >> 11;
                data->env_pre_gain = tmp;
                data->env_counter_prev = data->env_counter;
            }
            env = (data->env_pre_gain * gain) >> 16;
            env = ((uint64_t) env * data->velocity) >> 32;
        }
        if(env){
            int16_t input = radspa_signal_get_value(data->input_sig, i, render_pass_id);
            ret = radspa_mult_shift(env, input);
        }
        radspa_signal_set_value(data->output_sig, i, ret);
    }

    if(idle) radspa_signal_set_const_value(data->output_sig, 0);
}
