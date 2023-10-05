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
    radspa_signal_t * output_sig = radspa_signal_get_by_index(env_adsr, ENV_ADSR_OUTPUT);
    if(output_sig->buffer == NULL) return;
    radspa_signal_t * input_sig = radspa_signal_get_by_index(env_adsr, ENV_ADSR_INPUT);
    radspa_signal_t * trigger_sig = radspa_signal_get_by_index(env_adsr, ENV_ADSR_TRIGGER);
    radspa_signal_t * attack_sig = radspa_signal_get_by_index(env_adsr, ENV_ADSR_ATTACK);
    radspa_signal_t * decay_sig = radspa_signal_get_by_index(env_adsr, ENV_ADSR_DECAY);
    radspa_signal_t * sustain_sig = radspa_signal_get_by_index(env_adsr, ENV_ADSR_SUSTAIN);
    radspa_signal_t * release_sig = radspa_signal_get_by_index(env_adsr, ENV_ADSR_RELEASE);
    radspa_signal_t * gain_sig = radspa_signal_get_by_index(env_adsr, ENV_ADSR_GAIN);

    int16_t env = 0;
    for(uint16_t i = 0; i < num_samples; i++){
        int16_t ret = 0;

        int16_t trigger = radspa_signal_get_value(trigger_sig, i, render_pass_id);
        int16_t vel = radspa_trigger_get(trigger, &(data->trigger_prev));

        if(vel < 0){ // stop
            if(data->env_phase != ENV_ADSR_PHASE_OFF){
                data->env_phase = ENV_ADSR_PHASE_RELEASE;
                data->release_init_val = data->env_counter;
            }
        } else if(vel > 0 ){ // start
            data->env_phase = ENV_ADSR_PHASE_ATTACK;
            data->velocity = vel;
        }

        if(!(i%(1<<ENV_ADSR_UNDERSAMPLING))){
            uint32_t tmp;
            int16_t time_ms;
            switch(data->env_phase){
                case ENV_ADSR_PHASE_OFF:
                    data->env_counter = 0;;
                    break;
                case ENV_ADSR_PHASE_ATTACK:
                    time_ms = radspa_signal_get_value(attack_sig, i, render_pass_id);
                    if(data->attack_prev_ms != time_ms){
                        data->attack_raw = env_adsr_time_ms_to_val_rise(time_ms, UINT32_MAX, ENV_ADSR_UNDERSAMPLING);
                        data->attack_prev_ms = time_ms;
                    }

                    tmp = data->env_counter + data->attack_raw;
                    if(tmp < data->env_counter){ // overflow
                        tmp = ~((uint32_t) 0); // max out
                        data->env_phase = ENV_ADSR_PHASE_DECAY;
                    }
                    data->env_counter = tmp;
                    break;
                case ENV_ADSR_PHASE_DECAY:
                    data->sustain = radspa_signal_get_value(sustain_sig, i, render_pass_id) << 17UL;
                    time_ms = radspa_signal_get_value(decay_sig, i, render_pass_id);
                    if((data->decay_prev_ms != time_ms) || (data->sustain_prev != data->sustain)){
                        data->decay_raw = env_adsr_time_ms_to_val_rise(time_ms, UINT32_MAX - data->sustain, ENV_ADSR_UNDERSAMPLING);
                        data->decay_prev_ms = time_ms;
                        data->sustain_prev = data->sustain;
                    }
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
                    data->sustain = radspa_signal_get_value(sustain_sig, i, render_pass_id) << 17UL;
                    if(data->sustain == 0) data->env_phase = ENV_ADSR_PHASE_OFF;
                    data->env_counter = data->sustain;
                    break;
                case ENV_ADSR_PHASE_RELEASE:
                    time_ms = radspa_signal_get_value(release_sig, i, render_pass_id);
                    if((data->release_prev_ms != time_ms) || (data->release_init_val_prev != data->release_init_val)){
                        data->release_raw = env_adsr_time_ms_to_val_rise(time_ms, data->release_init_val, ENV_ADSR_UNDERSAMPLING);
                        data->release_prev_ms = time_ms;
                        data->release_init_val_prev = data->release_init_val;;
                    }
                    tmp = data->env_counter - data->release_raw;
                    if(tmp > data->env_counter){ // underflow
                        tmp = 0; //bottom out
                        data->env_phase = ENV_ADSR_PHASE_OFF;
                    }
                    data->env_counter = tmp;
                    break;
            }
            env = data->env_counter >> 17;
            env = (env * (env + 1)) >> 15;

            int32_t gain = radspa_signal_get_value(gain_sig, i, render_pass_id);
            env  = (env * gain) >> 12;
            env  = (env * data->velocity) >> 15;
        }
        if(env){
            int16_t input = radspa_signal_get_value(input_sig, i, render_pass_id);
            ret = radspa_mult_shift(env, input);
        }
        radspa_signal_set_value(output_sig, i, ret);
    }
}
