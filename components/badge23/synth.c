#include "badge23/synth.h"
#include "badge23/audio.h"
#include <math.h>

float ks_osc(ks_osc_t * ks, float input){
    //TODO: FIX THIS
    ks->real_feedback = ks->feedback;

    float delay_time = ((float) (SAMPLE_RATE))/ks->freq;
    if(delay_time >= (KS_BUFFER_SIZE)) delay_time = (KS_BUFFER_SIZE) - 1;


    //ks->tape[0] = input + real_feedback * ks->tape[delay_time];    
    return ks->tape[0];
}

float waveshaper(uint8_t shape, float in);
float nes_noise(uint16_t * reg, uint8_t mode, uint8_t run);

void run_trad_env(trad_osc_t * osc){
    switch(osc->env_phase){
        case 0:
            osc->env = 0; osc->counter = 0; osc->env_counter = 0;
            break;
        case 1:
            if(osc->attack_steps){
                if(osc->env == 0){
                    osc->env = (TRAD_OSC_MIN_ATTACK_ENV);
                } else {
                    osc->env_counter++;
                    if(osc->env_counter > osc->attack_steps){
                        osc->env *= (1. + (TRAD_OSC_ATTACK_STEP));
                        osc->env_counter = 0;
                    }
                }
            } else {
                osc->env += osc->vol/TRAD_OSC_ATTACK_POP_BLOCK;
            }
            if(osc->env > osc->vol){
                osc->env_phase = 2;
                osc->env = osc->vol;
            }
            break;
        case 2:
            osc->env = osc->vol;
            osc->env_counter = 0;
            if(osc->skip_hold) osc->env_phase = 3;
            break;
        case 3:
            if(osc->decay_steps){
                osc->env_counter++;
                if(osc->env_counter > osc->decay_steps){
                    osc->env *= (1. - (TRAD_OSC_DECAY_STEP));
                    osc->env_counter = 0;
                }
                if(osc->env < osc->gate){
                    osc->env_phase = 0; osc->env = 0; osc->counter = 0;
                }
            } else {
                osc->env_phase = 0; osc->env = 0; osc->counter = 0;
            }
            break;
    }
}

float run_trad_osc(trad_osc_t * osc){
    run_trad_env(osc);
    if(!osc->env_phase) return 0;
    float ret = 0;

    //run core sawtooth
    float freq = osc->freq * osc->bend;
    if(freq > 10000) freq = 10000;
    if(freq < -10000) freq = -10000;
    if(freq != freq) freq = 0;
    osc->counter += 2. * freq / ((float)(SAMPLE_RATE));
    if(osc->counter != osc->counter){
        printf("trad_osc counter is NaN");
        abort();
    }
    while(osc->counter > 1.){
        osc->counter -= 2.;
        osc->overflow_event = 1;
    }
    while(osc->counter < -1.){
        osc->counter += 2.;
        osc->overflow_event = -1;
    }

    if(osc->waveform >= 7){
        ret = nes_noise(&(osc->noise_reg), osc->waveform == 7, osc->overflow_event);
        osc->overflow_event = 0;
    } else {
        //apply waveshaper
        ret = waveshaper(osc->waveform, osc->counter);
    }

    //apply volume
    ret *= osc->env;
    return ret;
}

float nes_noise(uint16_t * reg, uint8_t mode, uint8_t run){
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
    return ((float) ((*reg) & 1)) * 2 - 1;
}

float waveshaper(uint8_t shape, float in){
    //expects sawtooth input in [-1..1] range
    switch(shape){
        case 0: // TODO: implement proper sine
            in = sin(in * 3.1415);
            break;
        case 1: //fast sine
            in = waveshaper(2, in);
            if(in > 0.){
                in = 1. - in;
                in *= in;
                in = 1. - in;
            } else {
                in = 1. + in;
                in *= in;
                in = in - 1.;
            }
            break;
        case 2: //triangle
            in += 0.5;
            if(in > 1.0) in -= 2;
            if(in > 0.) in = -in;
            in = (2. * in) + 1.;
            break;
        case 3: //sawtooth
            break;
        case 4: //square
            if(in > 0){
                in = 1.;
            } else {
                in = -1.;
            }
            break;
        case 5: //33% pulse
            if(in > 0.33){
                in = 1.;
            } else {
                in = -1.;
            }
            break;
        case 6: //25% pulse
            if(in > 0.5){
                in = 1.;
            } else {
                in = -1.;
            }
            break;
    }
    return in;
}

#define NAT_LOG_SEMITONE 0.05776226504666215

void trad_osc_set_freq_semitone(trad_osc_t * osc, float bend){
    osc->freq = 440. * exp(bend * NAT_LOG_SEMITONE);
}

void trad_osc_set_freq_Hz(trad_osc_t * osc, float freq){
    osc->freq = freq;
}

void trad_osc_set_waveform(trad_osc_t * osc, uint8_t waveform){
    osc->waveform = waveform;
}

void trad_osc_set_attack(trad_osc_t * osc, uint16_t attack){
    osc->attack_steps = attack;
}

void trad_osc_set_decay(trad_osc_t * osc, uint16_t decay){
    osc->decay_steps = decay;
}

void trad_env_stop(trad_osc_t * osc){
    if(osc->env_phase) osc->env_phase = 3; 
}

void trad_env_fullstop(trad_osc_t * osc){
    osc->env_phase = 0; //stop and skip decay phase
}

void trad_env_start(trad_osc_t * osc){
    osc->env_phase = 1; //put into attack phase;
}