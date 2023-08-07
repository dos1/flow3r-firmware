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
#define ENV_ADSR_PHASE 1
#define ENV_ADSR_INPUT 2
#define ENV_ADSR_TRIGGER 3
#define ENV_ADSR_ATTACK 4
#define ENV_ADSR_DECAY 5
#define ENV_ADSR_SUSTAIN 6
#define ENV_ADSR_RELEASE 7
#define ENV_ADSR_GATE 8

#define ENV_ADSR_PHASE_OFF 0
#define ENV_ADSR_PHASE_ATTACK 1
#define ENV_ADSR_PHASE_DECAY 2
#define ENV_ADSR_PHASE_SUSTAIN 3
#define ENV_ADSR_PHASE_RELEASE 4

radspa_t * env_adsr_create(uint32_t init_var){
    radspa_t * env_adsr = radspa_standard_plugin_create(&env_adsr_desc, ENV_ADSR_NUM_SIGNALS, sizeof(env_adsr_data_t),0);
    env_adsr->render = env_adsr_run;
    radspa_signal_set(env_adsr, ENV_ADSR_OUTPUT, "output", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(env_adsr, ENV_ADSR_PHASE, "phase", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(env_adsr, ENV_ADSR_INPUT, "input", RADSPA_SIGNAL_HINT_INPUT, 32767);
    radspa_signal_set(env_adsr, ENV_ADSR_TRIGGER, "trigger", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_TRIGGER, 0);
    radspa_signal_set(env_adsr, ENV_ADSR_ATTACK, "attack", RADSPA_SIGNAL_HINT_INPUT, 100);
    radspa_signal_set(env_adsr, ENV_ADSR_DECAY, "decay", RADSPA_SIGNAL_HINT_INPUT, 250);
    radspa_signal_set(env_adsr, ENV_ADSR_SUSTAIN, "sustain", RADSPA_SIGNAL_HINT_INPUT, 16000);
    radspa_signal_set(env_adsr, ENV_ADSR_RELEASE, "release", RADSPA_SIGNAL_HINT_INPUT, 50);
    radspa_signal_set(env_adsr, ENV_ADSR_GATE, "gate", RADSPA_SIGNAL_HINT_INPUT,0);
    radspa_signal_get_by_index(env_adsr, ENV_ADSR_ATTACK)->unit = "ms";
    radspa_signal_get_by_index(env_adsr, ENV_ADSR_DECAY)->unit = "ms";
    radspa_signal_get_by_index(env_adsr, ENV_ADSR_SUSTAIN)->unit = "ms";

    env_adsr_data_t * data = env_adsr->plugin_data;
    data->trigger_prev = 0;
    data->env_phase = ENV_ADSR_PHASE_OFF;

    return env_adsr;
}

static int16_t env_adsr_run_single(env_adsr_data_t * env){
    uint32_t tmp;
    switch(env->env_phase){
        case ENV_ADSR_PHASE_OFF:
            env->env_counter = 0;;
            break;
        case ENV_ADSR_PHASE_ATTACK:
            tmp = env->env_counter + env->attack;
            if(tmp < env->env_counter){ // overflow
                tmp = ~((uint32_t) 0); // max out
                env->env_phase = ENV_ADSR_PHASE_DECAY;
            }
            env->env_counter = tmp;
            break;
        case ENV_ADSR_PHASE_DECAY:
            tmp = env->env_counter - env->decay;
            if(tmp > env->env_counter){ // underflow
                tmp = 0; //bottom out
            }
            env->env_counter = tmp;

            if(env->env_counter <= env->sustain){
                env->env_counter = env->sustain;
                env->env_phase = ENV_ADSR_PHASE_SUSTAIN;
            } else if(env->env_counter < env->gate){
                env->env_counter = 0;
                env->env_phase = ENV_ADSR_PHASE_OFF;
            }
            break;
        case ENV_ADSR_PHASE_SUSTAIN:
            if(env->sustain == 0) env->env_phase = ENV_ADSR_PHASE_OFF;
            env->env_counter = env->sustain;
            break;
        case ENV_ADSR_PHASE_RELEASE:
            tmp = env->env_counter - env->release;
            if(tmp > env->env_counter){ // underflow
                tmp = 0; //bottom out
                env->env_phase = ENV_ADSR_PHASE_OFF;
            }
            env->env_counter = tmp;
            /*
            if(env->env_counter < env->gate){
                env->env_counter = 0;
                env->env_phase = ENV_ADSR_PHASE_OFF;
            }
            */
            break;
    }
    return env->env_counter >> 17;
}

#define SAMPLE_RATE_SORRY 48000
#define ENV_ADSR_UNDERSAMPLING 5


static inline uint32_t env_adsr_time_ms_to_val_rise(uint16_t time_ms, uint32_t val){
    if(!time_ms) return UINT32_MAX;
    uint32_t div = time_ms * ((SAMPLE_RATE_SORRY)/1000);
    return val/div;
}

static inline uint32_t uint32_sat_leftshift(uint32_t input, uint16_t left){
    if(!left) return input; // nothing to do
    if(input >> (32-left)) return UINT32_MAX; // sat
    return input << left;
}


void env_adsr_run(radspa_t * env_adsr, uint16_t num_samples, uint32_t render_pass_id){
    env_adsr_data_t * plugin_data = env_adsr->plugin_data;
    radspa_signal_t * output_sig = radspa_signal_get_by_index(env_adsr, ENV_ADSR_OUTPUT);
    radspa_signal_t * phase_sig = radspa_signal_get_by_index(env_adsr, ENV_ADSR_PHASE);
    if((output_sig->buffer == NULL) && (phase_sig->buffer == NULL)) return;
    radspa_signal_t * trigger_sig = radspa_signal_get_by_index(env_adsr, ENV_ADSR_TRIGGER);
    radspa_signal_t * input_sig = radspa_signal_get_by_index(env_adsr, ENV_ADSR_INPUT);
    radspa_signal_t * attack_sig = NULL;
    radspa_signal_t * decay_sig = NULL;
    radspa_signal_t * sustain_sig = NULL;
    radspa_signal_t * release_sig = NULL;
    radspa_signal_t * gate_sig = NULL;

    int16_t ret = output_sig->value;

    for(uint16_t i = 0; i < num_samples; i++){
        static int16_t env = 0;

        int16_t trigger = trigger_sig->get_value(trigger_sig, i, num_samples, render_pass_id);
        int16_t vel = radspa_trigger_get(trigger, &(plugin_data->trigger_prev));

        if(vel < 0){ // stop
            if(plugin_data->env_phase != ENV_ADSR_PHASE_OFF){
                plugin_data->env_phase = ENV_ADSR_PHASE_RELEASE;
                plugin_data->release_init_val = plugin_data->env_counter;
            }
        } else if(vel > 0 ){ // start
            plugin_data->env_phase = ENV_ADSR_PHASE_ATTACK;
            plugin_data->velocity = ((uint32_t) vel) << 17;
        }

        if(!(i%(1<<ENV_ADSR_UNDERSAMPLING))){
            uint16_t time_ms;
            uint32_t sus;
            switch(plugin_data->env_phase){
                case ENV_ADSR_PHASE_OFF:
                    break;
                case ENV_ADSR_PHASE_ATTACK:
                    if(attack_sig == NULL){
                        attack_sig = radspa_signal_get_by_index(env_adsr, ENV_ADSR_ATTACK);
                    }
                    time_ms = attack_sig->get_value(attack_sig, i, num_samples, render_pass_id);
                    if(time_ms != plugin_data->attack_prev_ms){
                        plugin_data->attack = uint32_sat_leftshift(env_adsr_time_ms_to_val_rise(time_ms, UINT32_MAX), ENV_ADSR_UNDERSAMPLING);
                        plugin_data->attack_prev_ms = time_ms;
                    }
                    break;
                case ENV_ADSR_PHASE_DECAY:
                    if(sustain_sig == NULL){
                        sustain_sig = radspa_signal_get_by_index(env_adsr, ENV_ADSR_SUSTAIN);
                    }
                    sus = sustain_sig->get_value(sustain_sig, i, num_samples, render_pass_id);
                    plugin_data->sustain = sus<<17;

                    if(gate_sig == NULL){
                        gate_sig = radspa_signal_get_by_index(env_adsr, ENV_ADSR_GATE);
                    }
                    sus = gate_sig->get_value(gate_sig, i, num_samples, render_pass_id);
                    plugin_data->gate = sus<<17;

                    if(decay_sig == NULL){
                        decay_sig = radspa_signal_get_by_index(env_adsr, ENV_ADSR_DECAY);
                    }
                    time_ms = decay_sig->get_value(decay_sig, i, num_samples, render_pass_id);
                    if(time_ms != plugin_data->decay_prev_ms){
                        plugin_data->decay = uint32_sat_leftshift(env_adsr_time_ms_to_val_rise(time_ms, UINT32_MAX-plugin_data->sustain), ENV_ADSR_UNDERSAMPLING);
                        plugin_data->decay_prev_ms = time_ms;
                    }
                    break;
                case ENV_ADSR_PHASE_SUSTAIN:
                    if(sustain_sig == NULL){
                        sustain_sig = radspa_signal_get_by_index(env_adsr, ENV_ADSR_SUSTAIN);
                    }
                    sus = sustain_sig->get_value(sustain_sig, i, num_samples, render_pass_id);
                    plugin_data->sustain = sus<<17;
                    break;
                case ENV_ADSR_PHASE_RELEASE:
                    if(gate_sig == NULL){
                        gate_sig = radspa_signal_get_by_index(env_adsr, ENV_ADSR_GATE);
                    }
                    sus = gate_sig->get_value(gate_sig, i, num_samples, render_pass_id);
                    plugin_data->gate = sus<<17;

                    if(release_sig == NULL){
                        release_sig = radspa_signal_get_by_index(env_adsr, ENV_ADSR_RELEASE);
                    }
                    time_ms = release_sig->get_value(release_sig, i, num_samples, render_pass_id);
                    if(time_ms != plugin_data->release_prev_ms){
                        plugin_data->release = uint32_sat_leftshift(env_adsr_time_ms_to_val_rise(time_ms, plugin_data->release_init_val), ENV_ADSR_UNDERSAMPLING);
                        plugin_data->release_prev_ms = time_ms;
                    }
                    break;
            }
            env = env_adsr_run_single(plugin_data);
        }
        if(env){
            int16_t input = input_sig->get_value(input_sig, i, num_samples, render_pass_id);
            ret = radspa_mult_shift(env, input);
        } else {
            ret = 0;
        }
        if(phase_sig->buffer != NULL) (phase_sig->buffer)[i] = plugin_data->env_phase;
        if(output_sig->buffer != NULL) (output_sig->buffer)[i] = ret;
    }
    phase_sig->value = plugin_data->env_phase;
    output_sig->value = ret;
}
