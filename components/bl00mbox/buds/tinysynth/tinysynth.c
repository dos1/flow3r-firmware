#include "tinysynth.h"
#include <math.h>

#define SYNTH_UNDERSAMPLING 1
#define SYNTH_SAMPLE_RATE ((SAMPLE_RATE)/(SYNTH_UNDERSAMPLING))

int16_t waveshaper(uint8_t shape, int16_t in);
int16_t nes_noise(uint16_t * reg, uint8_t mode, uint8_t run);

int16_t run_trad_env(trad_env_t * env){
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
            break;
    }
    return env->env_counter >> 17;
}

int16_t run_trad_osc(trad_osc_t * osc){
    osc->undersampling_counter = (osc->undersampling_counter+1) % SYNTH_UNDERSAMPLING;
    if(osc->undersampling_counter) return osc->prev_output;

    int32_t ret; //lil bit buffer for operations

    int32_t env = run_trad_env(&(osc->env));
    if(osc->env.env_phase == TRAD_ENV_PHASE_OFF){
        osc->counter = ((uint64_t) 1) << 63;
        return 0;
    }

    //run core sawtooth
    //uint64_t incr = osc->freq * osc->bend;
    uint64_t incr = osc->freq;
    osc->counter += incr;

    osc->overflow_event = osc->counter_prev > osc->counter; //no neg f linfm for now
    osc->counter_prev = osc->counter;

    if(osc->waveform >= 7){
        ret = nes_noise(&(osc->noise_reg), osc->waveform == 7, osc->overflow_event);
    } else {
        //apply waveshaper
        int32_t tmp = (osc->counter) >> (33+16);
        tmp *= 2;
        tmp -= 32767;
        ret = waveshaper(osc->waveform, tmp);
    }

    //apply volume
    ret = (ret * env)>>15;
    ret = (ret * osc->vol)>>15;
    osc->prev_output = ret;
    return ret;
}

int16_t nes_noise(uint16_t * reg, uint8_t mode, uint8_t run){
    if(run) {
        uint8_t fb = *reg;
        if(mode){
            fb = fb>>6;
        } else {
            fb = fb>>1;
        }
        fb = (fb ^ (*reg)) & 1;
        *reg = (*reg >> 1);
        *reg = (*reg) | (((uint16_t) fb) << 14);
    }
    return ((int16_t) (((*reg) & 1))*2 - 1) * 32767;
}

int16_t fake_square(int16_t triangle, int16_t pwm, int16_t gain){
    //max gain (1<<14)-1
    int32_t tmp = triangle;
    tmp += pwm;
    tmp *= gain;
    if(tmp > 32767) tmp = 32767;
    if(tmp < -32767) tmp = -32767;
    return tmp;
}

int16_t waveshaper(uint8_t shape, int16_t in){
    int32_t tmp = 0;
    switch(shape){
        case TRAD_OSC_WAVE_SINE: // TODO: implement proper sine
        case TRAD_OSC_WAVE_FAKE_SINE:
            tmp = waveshaper(TRAD_OSC_WAVE_TRI, in);
            if(tmp > 0.){
                tmp = 32767 - tmp;
                tmp = (tmp*tmp)>>15;
                tmp = 32767. - tmp;
            } else {
                tmp = 32767 + tmp;
                tmp = (tmp*tmp)>>15;
                tmp = tmp - 32767.;
            }
            break;
        case TRAD_OSC_WAVE_TRI:
            tmp = in;
            tmp += 16384;
            if(tmp > 32767) tmp -= 65535;
            if(tmp > 0) tmp = -tmp;
            tmp = (2 * tmp) + 32767;
            break;
        case TRAD_OSC_WAVE_SAW:
            tmp = in;
            break;
        case TRAD_OSC_WAVE_SQUARE:
            tmp = waveshaper(TRAD_OSC_WAVE_TRI, in);
            tmp = fake_square(tmp, 0, 100);
            break;
        case TRAD_OSC_WAVE_PULSE:
            tmp = waveshaper(TRAD_OSC_WAVE_TRI, in);
            tmp = fake_square(tmp, 12269, 100);
            break;
        case TRAD_OSC_WAVE_BLIP:
            tmp = waveshaper(TRAD_OSC_WAVE_TRI, in);
            tmp = fake_square(tmp, 20384, 100);
            break;
    }
    if(tmp > 32767) tmp = 32767;
    if(tmp < -32767) tmp = -32767;
    return tmp;
}

#define NAT_LOG_SEMITONE 0.05776226504666215

void trad_osc_set_freq_semitone(trad_osc_t * osc, float tone){
    trad_osc_set_freq_Hz(osc, 440. * exp(tone * NAT_LOG_SEMITONE));
}

void trad_osc_set_freq_Hz(trad_osc_t * osc, float freq){
    uint64_t max = ~((uint64_t)0);
    osc->freq = (freq/(SYNTH_SAMPLE_RATE)) * max;
}

void trad_osc_set_waveform(trad_osc_t * osc, uint8_t waveform){
    osc->waveform = waveform;
}

void trad_osc_set_attack_ms(trad_osc_t * osc, float ms){
    osc->env.attack = (1000./ms/(SYNTH_SAMPLE_RATE)) * (~((uint32_t) 0)) ;
}

void trad_osc_set_decay_ms(trad_osc_t * osc, float ms){
    osc->env.decay = (1000./ms/(SYNTH_SAMPLE_RATE)) * ((~((uint32_t) 0)) - osc->env.sustain);
}

void trad_osc_set_sustain(trad_osc_t * osc, float sus){
    uint32_t max = ~((uint32_t)0);
    osc->env.sustain = max * sus;
}

void trad_osc_set_release_ms(trad_osc_t * osc, float ms){
    osc->env.release = (1000./ms/(SYNTH_SAMPLE_RATE)) * osc->env.sustain;
}

void trad_env_stop(trad_osc_t * osc){
    if(osc->env.env_phase != TRAD_ENV_PHASE_OFF) osc->env.env_phase = TRAD_ENV_PHASE_RELEASE; 
}

void trad_env_fullstop(trad_osc_t * osc){
    osc->env.env_phase = TRAD_ENV_PHASE_OFF; //stop and skip decay phase
}

void trad_env_start(trad_osc_t * osc){
    osc->env.env_phase = TRAD_ENV_PHASE_ATTACK; //put into attack phase;
}

void trad_osc_set_vol(trad_osc_t * osc, float volume){
    osc->vol = 32767 * volume;
}

