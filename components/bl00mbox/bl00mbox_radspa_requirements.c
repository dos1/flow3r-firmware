//SPDX-License-Identifier: CC0-1.0
#include "bl00mbox_radspa_requirements.h"

bool radspa_host_request_buffer_render(int16_t * buf, uint16_t num_samples){
    bl00mbox_bud_t * bud = ((bl00mbox_connection_t *) buf)->source_bud;
    bl00mbox_audio_bud_render(bud, num_samples);
    return 1;
}

// py: bigtable = [int(22/200*(2**(14-5+8+x*4096/2400/64))) for x in range(64)]
// for 48kHz main sample rate
static const uint16_t bigtable[64] = {
        14417, 14686, 14960, 15240, 15524, 15813, 16108, 16409, 
        16715, 17027, 17345, 17668, 17998, 18334, 18676, 19024, 
        19379, 19741, 20109, 20484, 20866, 21255, 21652, 22056, 
        22467, 22887, 23313, 23748, 24191, 24643, 25103, 25571, 
        26048, 26534, 27029, 27533, 28047, 28570, 29103, 29646, 
        30199, 30763, 31336, 31921, 32517, 33123, 33741, 34371, 
        35012, 35665, 36330, 37008, 37699, 38402, 39118, 39848, 
        40592, 41349, 42120, 42906, 43706, 44522, 45352, 46199
};

// py: smoltable = [int(22/240*(2**(15-5+9+x*4096/2400/64/64))) for x in range(64)]
// for 48kHz main sample rate
static const uint16_t smoltable[64] = {
        48059, 48073, 48087, 48101, 48115, 48129, 48143, 48156, 
        48170, 48184, 48198, 48212, 48226, 48240, 48254, 48268, 
        48282, 48296, 48310, 48324, 48338, 48352, 48366, 48380, 
        48394, 48407, 48421, 48435, 48449, 48463, 48477, 48491, 
        48505, 48519, 48533, 48548, 48562, 48576, 48590, 48604, 
        48618, 48632, 48646, 48660, 48674, 48688, 48702, 48716, 
        48730, 48744, 48758, 48772, 48786, 48801, 48815, 48829, 
        48843, 48857, 48871, 48885, 48899, 48913, 48928, 48942,
};

uint32_t radspa_sct_to_rel_freq(int16_t sct, int16_t undersample_pow){
    /// returns approx. proportional to 2**((sct/2400) + undersample_pow) so that
    /// a uint32_t accumulator overflows at 440Hz with sct = INT16_MAX - 6*2400
    /// when sampled at (48>>undersample_pow)kHz

    // compiler explorer says this is 33 instructions with O2. might be alright?
    uint32_t a = sct;
    a = sct + 28*2400 - 32767 - 330;
    // at O2 u get free division for each modulo. still slow, 10 instructions or so.
    int16_t octa = a / 2400;
    a = a % 2400;
    uint8_t bigindex = a / 64;
    uint8_t smolindex = a % 64;
    
    uint32_t ret = 2; //weird but trust us
    ret *= bigtable[bigindex];
    ret *= smoltable[smolindex];

    int16_t shift = 27 - octa - undersample_pow;
    if(shift > 0){
        ret = ret >> shift;
    }
    return ret;
}

int16_t radspa_clip(int32_t a){
    if(a > 32767){
         return 32767;
    } else if(a < -32767){
         return -32767;
    }
    return a;
}

int16_t radspa_add_sat(int32_t a, int32_t b){ return radspa_clip(a+b); }
int16_t radspa_mult_shift(int32_t a, int32_t b){ return radspa_clip((a*b)>>15); }

int16_t radspa_trigger_start(int16_t velocity, int16_t * hist){
    int16_t ret = ((* hist) > 0) ? -velocity : velocity;
    (* hist) = ret;
    return ret;
}

int16_t radspa_trigger_stop(int16_t * hist){
    (* hist) = 0;
    return 0;
}

int16_t radspa_trigger_get(int16_t trigger_signal, int16_t * hist){
    int16_t ret = 0;
    if((!trigger_signal) && (* hist)){ //stop
        ret = -1;
    } else if(trigger_signal > 0 ){
        if((* hist) <= 0) ret = trigger_signal;
    } else if(trigger_signal < 0 ){
        if((* hist) >= 0) ret = -trigger_signal;
    }
    (* hist) = trigger_signal;
    return ret;
}
