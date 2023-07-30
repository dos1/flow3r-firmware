#include "trad_synth.h"

// plugin descriptions in trad_synth.h

static inline int16_t waveshaper(int16_t saw, int16_t shape);

// plugin: trad_osc
radspa_descriptor_t trad_osc_desc = {
    .name = "trad osc",
    .id = 420,
    .description = "simple audio band oscillator with classic waveforms",
    .create_plugin_instance = trad_osc_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

#define TRAD_OSC_NUM_SIGNALS 4
#define TRAD_OSC_OUTPUT 0
#define TRAD_OSC_PITCH 1
#define TRAD_OSC_WAVEFORM 2
#define TRAD_OSC_LIN_FM 3

radspa_t * trad_osc_create(uint32_t init_var){
    radspa_t * trad_osc = radspa_standard_plugin_create(&trad_osc_desc, TRAD_OSC_NUM_SIGNALS, sizeof(trad_osc_data_t));
    trad_osc->render = trad_osc_run;
    radspa_signal_set(trad_osc, TRAD_OSC_OUTPUT, "output", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(trad_osc, TRAD_OSC_PITCH, "pitch", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_SCT, 18367);
    radspa_signal_set(trad_osc, TRAD_OSC_WAVEFORM, "waveform", RADSPA_SIGNAL_HINT_INPUT, -16000);
    radspa_signal_set(trad_osc, TRAD_OSC_LIN_FM, "lin_fm", RADSPA_SIGNAL_HINT_INPUT, 0);
    return trad_osc;
}

void trad_osc_run(radspa_t * trad_osc, uint16_t num_samples, uint32_t render_pass_id){
    trad_osc_data_t * plugin_data = trad_osc->plugin_data;
    radspa_signal_t * output_sig = radspa_signal_get_by_index(trad_osc, TRAD_OSC_OUTPUT);
    radspa_signal_t * pitch_sig = radspa_signal_get_by_index(trad_osc, TRAD_OSC_PITCH);
    radspa_signal_t * waveform_sig = radspa_signal_get_by_index(trad_osc, TRAD_OSC_WAVEFORM);
    radspa_signal_t * lin_fm_sig = radspa_signal_get_by_index(trad_osc, TRAD_OSC_LIN_FM);
    if(output_sig->buffer == NULL) return;

    int16_t ret = 0;
    for(uint16_t i = 0; i < num_samples; i++){
        int16_t pitch = radspa_signal_get_value(pitch_sig, i, num_samples, render_pass_id);
        int16_t wave = radspa_signal_get_value(waveform_sig, i, num_samples, render_pass_id);
        int32_t lin_fm = radspa_signal_get_value(lin_fm_sig, i, num_samples, render_pass_id);

        if(pitch != plugin_data->prev_pitch){
            plugin_data->incr = radspa_sct_to_rel_freq(pitch, 0);
            plugin_data->prev_pitch = pitch;
        }
        plugin_data->counter += plugin_data->incr;
        if(lin_fm){
            plugin_data->counter += lin_fm * (plugin_data->incr >> 15);
        }

        int32_t tmp = (plugin_data->counter) >> 17;
        tmp = (tmp*2) - 32767;
        ret = waveshaper(tmp, wave);
        (output_sig->buffer)[i] = ret;
    }
    output_sig->value = ret;
}

static inline int16_t triangle(int16_t saw){
    int32_t tmp = saw;
    tmp += 16384;
    if(tmp > 32767) tmp -= 65535;
    if(tmp > 0) tmp = -tmp;
    tmp = (2 * tmp) + 32767;
    return tmp;
}

inline int16_t waveshaper(int16_t saw, int16_t shape){
    int32_t tmp = saw;
    uint8_t sh = ((uint16_t) shape) >> 14;
    sh = (sh + 2)%4;
    switch(sh){
        case 0: //sine
            tmp = triangle(tmp);
            if(tmp > 0){
                tmp = 32767 - tmp;
                tmp = (tmp*tmp)>>15;
                tmp = 32767. - tmp;
            } else {
                tmp = 32767 + tmp;
                tmp = (tmp*tmp)>>15;
                tmp = tmp - 32767.;
            }
            break;
        case 1: //tri
            tmp = triangle(tmp);
            break;
        case 2: //square:
            if(tmp > 0){
                tmp = 32767;
            } else {
                tmp = -32767;
            }
            break;
        default: //saw
            break;
    }
    return tmp;
}

// plugin: trad_env
radspa_descriptor_t trad_env_desc = {
    .name = "trad envelope",
    .id = 42,
    .description = "traditional ADSR envelope",
    .create_plugin_instance = trad_env_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

#define TRAD_ENV_NUM_SIGNALS 9
#define TRAD_ENV_OUTPUT 0
#define TRAD_ENV_PHASE 1
#define TRAD_ENV_INPUT 2
#define TRAD_ENV_TRIGGER 3
#define TRAD_ENV_ATTACK 4
#define TRAD_ENV_DECAY 5
#define TRAD_ENV_SUSTAIN 6
#define TRAD_ENV_RELEASE 7
#define TRAD_ENV_GATE 8

#define TRAD_ENV_PHASE_OFF 0
#define TRAD_ENV_PHASE_ATTACK 1
#define TRAD_ENV_PHASE_DECAY 2
#define TRAD_ENV_PHASE_SUSTAIN 3
#define TRAD_ENV_PHASE_RELEASE 4

radspa_t * trad_env_create(uint32_t init_var){
    radspa_t * trad_env = radspa_standard_plugin_create(&trad_env_desc, TRAD_ENV_NUM_SIGNALS, sizeof(trad_env_data_t));
    trad_env->render = trad_env_run;
    radspa_signal_set(trad_env, TRAD_ENV_OUTPUT, "output", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(trad_env, TRAD_ENV_PHASE, "phase", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(trad_env, TRAD_ENV_INPUT, "input", RADSPA_SIGNAL_HINT_INPUT, 32767);
    radspa_signal_set(trad_env, TRAD_ENV_TRIGGER, "trigger", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_TRIGGER, 0);
    radspa_signal_set(trad_env, TRAD_ENV_ATTACK, "attack", RADSPA_SIGNAL_HINT_INPUT, 100);
    radspa_signal_set(trad_env, TRAD_ENV_DECAY, "decay", RADSPA_SIGNAL_HINT_INPUT, 250);
    radspa_signal_set(trad_env, TRAD_ENV_SUSTAIN, "sustain", RADSPA_SIGNAL_HINT_INPUT, 16000);
    radspa_signal_set(trad_env, TRAD_ENV_RELEASE, "release", RADSPA_SIGNAL_HINT_INPUT, 50);
    radspa_signal_set(trad_env, TRAD_ENV_GATE, "gate", RADSPA_SIGNAL_HINT_INPUT,0);
    radspa_signal_get_by_index(trad_env, TRAD_ENV_ATTACK)->unit = "ms";
    radspa_signal_get_by_index(trad_env, TRAD_ENV_DECAY)->unit = "ms";
    radspa_signal_get_by_index(trad_env, TRAD_ENV_SUSTAIN)->unit = "ms";

    trad_env_data_t * data = trad_env->plugin_data;
    data->trigger = 0;
    data->env_phase = TRAD_ENV_PHASE_OFF;

    return trad_env;
}

static int16_t trad_env_run_single(trad_env_data_t * env){
    uint32_t tmp;
    switch(env->env_phase){
        case TRAD_ENV_PHASE_OFF:
            env->env_counter = 0;;
            break;
        case TRAD_ENV_PHASE_ATTACK:
            tmp = env->env_counter + env->attack;
            if(tmp < env->env_counter){ // overflow
                tmp = ~((uint32_t) 0); // max out
                env->env_phase = TRAD_ENV_PHASE_DECAY;
            }
            env->env_counter = tmp;
            break;
        case TRAD_ENV_PHASE_DECAY:
            tmp = env->env_counter - env->decay;
            if(tmp > env->env_counter){ // underflow
                tmp = 0; //bottom out
            }
            env->env_counter = tmp;

            if(env->env_counter <= env->sustain){
                env->env_counter = env->sustain;
                env->env_phase = TRAD_ENV_PHASE_SUSTAIN;
            } else if(env->env_counter < env->gate){
                env->env_counter = 0;
                env->env_phase = TRAD_ENV_PHASE_OFF;
            }
            break;
        case TRAD_ENV_PHASE_SUSTAIN:
            if(env->sustain == 0) env->env_phase = TRAD_ENV_PHASE_OFF;
            env->env_counter = env->sustain;
            break;
        case TRAD_ENV_PHASE_RELEASE:
            tmp = env->env_counter - env->release;
            if(tmp > env->env_counter){ // underflow
                tmp = 0; //bottom out
                env->env_phase = TRAD_ENV_PHASE_OFF;
            }
            env->env_counter = tmp;
            /*
            if(env->env_counter < env->gate){
                env->env_counter = 0;
                env->env_phase = TRAD_ENV_PHASE_OFF;
            }
            */
            break;
    }
    return env->env_counter >> 17;
}

#define SAMPLE_RATE_SORRY 48000
#define TRAD_ENV_UNDERSAMPLING 5


static inline uint32_t trad_env_time_ms_to_val_rise(uint16_t time_ms, uint32_t val){
    if(!time_ms) return UINT32_MAX;
    uint32_t div = time_ms * ((SAMPLE_RATE_SORRY)/1000);
    return val/div;
}

static inline uint32_t uint32_sat_leftshift(uint32_t input, uint16_t left){
    if(!left) return input; // nothing to do
    if(input >> (32-left)) return UINT32_MAX; // sat
    return input << left;
}


void trad_env_run(radspa_t * trad_env, uint16_t num_samples, uint32_t render_pass_id){
    trad_env_data_t * plugin_data = trad_env->plugin_data;
    radspa_signal_t * output_sig = radspa_signal_get_by_index(trad_env, TRAD_ENV_OUTPUT);
    radspa_signal_t * phase_sig = radspa_signal_get_by_index(trad_env, TRAD_ENV_PHASE);
    if((output_sig->buffer == NULL) && (phase_sig->buffer == NULL)) return;
    radspa_signal_t * trigger_sig = radspa_signal_get_by_index(trad_env, TRAD_ENV_TRIGGER);
    radspa_signal_t * input_sig = radspa_signal_get_by_index(trad_env, TRAD_ENV_INPUT);
    radspa_signal_t * attack_sig = NULL;
    radspa_signal_t * decay_sig = NULL;
    radspa_signal_t * sustain_sig = NULL;
    radspa_signal_t * release_sig = NULL;
    radspa_signal_t * gate_sig = NULL;

    int16_t ret = output_sig->value;

    for(uint16_t i = 0; i < num_samples; i++){
        static int16_t env = 0;

        if(!(i%(1<<TRAD_ENV_UNDERSAMPLING))){
            int16_t trigger = radspa_signal_get_value(trigger_sig, i, num_samples, render_pass_id);
            if(!trigger){
                if(plugin_data->env_phase != TRAD_ENV_PHASE_OFF){
                    plugin_data->env_phase = TRAD_ENV_PHASE_RELEASE;
                    plugin_data->release_init_val = plugin_data->env_counter;
                }
            } else if(trigger > 0 ){
                if(plugin_data->trigger <= 0)
                plugin_data->env_phase = TRAD_ENV_PHASE_ATTACK;
                uint32_t vel = trigger;
                plugin_data->velocity = vel << 17;
            } else if(trigger < 0 ){
                if(plugin_data->trigger >= 0)
                plugin_data->env_phase = TRAD_ENV_PHASE_ATTACK;
                uint32_t vel = -trigger;
                plugin_data->velocity = vel << 17;
            }
            plugin_data->trigger = trigger;

            uint16_t time_ms;
            uint32_t sus;
            switch(plugin_data->env_phase){
                case TRAD_ENV_PHASE_OFF:
                    break;
                case TRAD_ENV_PHASE_ATTACK:
                    if(attack_sig == NULL){
                        attack_sig = radspa_signal_get_by_index(trad_env, TRAD_ENV_ATTACK);
                    }
                    time_ms = radspa_signal_get_value(attack_sig, i, num_samples, render_pass_id);
                    if(time_ms != plugin_data->attack_prev_ms){
                        plugin_data->attack = uint32_sat_leftshift(trad_env_time_ms_to_val_rise(time_ms, UINT32_MAX), TRAD_ENV_UNDERSAMPLING);
                        plugin_data->attack_prev_ms = time_ms;
                    }
                    break;
                case TRAD_ENV_PHASE_DECAY:
                    if(sustain_sig == NULL){
                        sustain_sig = radspa_signal_get_by_index(trad_env, TRAD_ENV_SUSTAIN);
                    }
                    sus = radspa_signal_get_value(sustain_sig, i, num_samples, render_pass_id);
                    plugin_data->sustain = sus<<17;

                    if(gate_sig == NULL){
                        gate_sig = radspa_signal_get_by_index(trad_env, TRAD_ENV_GATE);
                    }
                    sus = radspa_signal_get_value(gate_sig, i, num_samples, render_pass_id);
                    plugin_data->gate = sus<<17;

                    if(decay_sig == NULL){
                        decay_sig = radspa_signal_get_by_index(trad_env, TRAD_ENV_DECAY);
                    }
                    time_ms = radspa_signal_get_value(decay_sig, i, num_samples, render_pass_id);
                    if(time_ms != plugin_data->decay_prev_ms){
                        plugin_data->decay = uint32_sat_leftshift(trad_env_time_ms_to_val_rise(time_ms, UINT32_MAX-plugin_data->sustain), TRAD_ENV_UNDERSAMPLING);
                        plugin_data->decay_prev_ms = time_ms;
                    }
                    break;
                case TRAD_ENV_PHASE_SUSTAIN:
                    if(sustain_sig == NULL){
                        sustain_sig = radspa_signal_get_by_index(trad_env, TRAD_ENV_SUSTAIN);
                    }
                    sus = radspa_signal_get_value(sustain_sig, i, num_samples, render_pass_id);
                    plugin_data->sustain = sus<<17;
                    break;
                case TRAD_ENV_PHASE_RELEASE:
                    if(gate_sig == NULL){
                        gate_sig = radspa_signal_get_by_index(trad_env, TRAD_ENV_GATE);
                    }
                    sus = radspa_signal_get_value(gate_sig, i, num_samples, render_pass_id);
                    plugin_data->gate = sus<<17;

                    if(release_sig == NULL){
                        release_sig = radspa_signal_get_by_index(trad_env, TRAD_ENV_RELEASE);
                    }
                    time_ms = radspa_signal_get_value(release_sig, i, num_samples, render_pass_id);
                    if(time_ms != plugin_data->release_prev_ms){
                        plugin_data->release = uint32_sat_leftshift(trad_env_time_ms_to_val_rise(time_ms, plugin_data->release_init_val), TRAD_ENV_UNDERSAMPLING);
                        plugin_data->release_prev_ms = time_ms;
                    }
                    break;
            }
            env = trad_env_run_single(plugin_data);
        }
        if(env){
            int16_t input = radspa_signal_get_value(input_sig, i, num_samples, render_pass_id);
            ret = i16_mult_shift(env, input);
        } else {
            ret = 0;
        }
        if(phase_sig->buffer != NULL) (phase_sig->buffer)[i] = plugin_data->env_phase;
        if(output_sig->buffer != NULL) (output_sig->buffer)[i] = ret;
    }
    phase_sig->value = plugin_data->env_phase;
    output_sig->value = ret;
}
