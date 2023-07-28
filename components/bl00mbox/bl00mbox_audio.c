#include "st3m_audio.h"
#include "st3m_scope.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "bl00mbox_audio.h"

static bool is_initialized = false;

#define BL00MBOX_DEFAULT_CHANNEL_VOLUME 3000
#define BL00MBOX_CHANNELS 32
static bool bl00mbox_audio_run = true;
void bl00mbox_audio_enable(){ bl00mbox_audio_run = true; }
void bl00mbox_audio_disable(){ bl00mbox_audio_run = false; }

static uint32_t render_pass_id;

// fixed-length list of channels
static bl00mbox_channel_t channels[BL00MBOX_CHANNELS];
static int8_t last_chan_event = 0;
// foregrounded channel is always rendered
// note: regardless of settings the system channel 0 is always rendered
static uint8_t bl00mbox_channel_foreground = 0;
// channels may request being active while not being in foreground
static bool bl00mbox_channel_background_active[BL00MBOX_CHANNELS] = {0,};
void bl00mbox_audio_channel_background_enable(uint8_t chan){ bl00mbox_channel_background_active[chan] = true; }
void bl00mbox_audio_channel_background_disable(uint8_t chan){ bl00mbox_channel_background_active[chan] = false; }

void bl00mbox_channel_event(uint8_t chan){
    last_chan_event = chan;
}

bl00mbox_channel_t * bl00mbox_get_channel(uint8_t chan){
    if(chan >= BL00MBOX_CHANNELS) return NULL;
    return &(channels[chan]);
}

uint8_t bl00mbox_channel_get_foreground_index(){
    return bl00mbox_channel_foreground;
}

uint8_t bl00mbox_channel_get_free_index(){
    uint8_t ret = 1;
    for(; ret < BL00MBOX_CHANNELS; ret++){
        if(bl00mbox_get_channel(ret)->is_free){
            bl00mbox_get_channel(ret)->is_free = false;
            break;
        }
    }
    last_chan_event = ret;
    return ret;
}

void bl00mbox_channels_init(){
    for(uint8_t i = 0; i < BL00MBOX_CHANNELS; i++){
        bl00mbox_channel_t * chan = bl00mbox_get_channel(i);
        chan->volume = BL00MBOX_DEFAULT_CHANNEL_VOLUME;
        chan->is_active = true;
        chan->is_free = true;
    }
}

void bl00mbox_channel_enable(uint8_t chan){
    if(chan >= (BL00MBOX_CHANNELS)) return;
    bl00mbox_channel_t * ch = bl00mbox_get_channel(chan);
    ch->is_active = true;
}

void bl00mbox_channel_disable(uint8_t chan){
    if(chan >= (BL00MBOX_CHANNELS)) return;
    bl00mbox_channel_t * ch = bl00mbox_get_channel(chan);
    ch->is_active = false;
}

void bl00mbox_channel_set_volume(uint8_t chan, uint16_t volume){
    if(chan >= (BL00MBOX_CHANNELS)) return;
    bl00mbox_channel_t * ch = bl00mbox_get_channel(chan);
    ch->volume = volume < 32767 ? volume : 32767;
}

int16_t bl00mbox_channel_get_volume(uint8_t chan){
    if(chan >= (BL00MBOX_CHANNELS)) return 0;
    bl00mbox_channel_t * ch = bl00mbox_get_channel(chan);
    return ch->volume;
}

static void bl00mbox_audio_bud_render(bl00mbox_bud_t * bud, uint16_t num_samples){
    if(bud->render_pass_id == render_pass_id) return;
    bud->plugin->render(bud->plugin, num_samples, render_pass_id);
    bud->render_pass_id = render_pass_id;
}

static void bl00mbox_audio_channel_render(bl00mbox_channel_t * chan, int16_t * out, uint16_t len, bool adding){
    if(render_pass_id == chan->render_pass_id) return;
    chan->render_pass_id = render_pass_id;

    bl00mbox_channel_root_t * root = chan->root_list;

    // early exit when no sources:
    if((root == NULL) || (!chan->is_active)){
        if(adding) return; // nothing to do
        memset(out, 0, len*sizeof(int16_t)); // mute
        return;
    }

    int32_t acc[256];
    bool first = true;
    
    while(root != NULL){
        bl00mbox_audio_bud_render(root->con->source_bud, len);
        if(first){
            for(uint16_t i = 0; i < len; i++){
                acc[i] = root->con->buffer[i];
            }
        } else {
            for(uint16_t i = 0; i < len; i++){ // replace this with proper ladspa-style adding function someday
                acc[i] += root->con->buffer[i];
            }
        }
        first = false;
        root = root->next;
    }
    for(uint16_t i = 0; i < len; i++){
        if(adding){
            out[i] = i16_add_sat(i16_mult_shift(acc[i], chan->volume), out[i]);
        } else {
            out[i] = i16_mult_shift(acc[i], chan->volume);
        }
    }
}

void bl00mbox_audio_render(int16_t * rx, int16_t * tx, uint16_t len){
    //if(!is_initialized) return;
    bl00mbox_channel_foreground = last_chan_event;
    if(!bl00mbox_audio_run){
        memset(tx, 0, len*sizeof(int16_t)); // mute
        return;
    }
    render_pass_id++; // fresh pass, all relevant sources must be recomputed
    uint16_t mono_len = len/2;
    int16_t acc[mono_len];
    // system channel always runs non-adding
    bl00mbox_audio_channel_render(&(channels[0]), acc, mono_len, 0);
    
    // re-rendering channels is ok, if render_pass_id didn't change it will just exit
    bl00mbox_audio_channel_render(&(channels[bl00mbox_channel_foreground]), acc, mono_len, 1);

#ifdef BL00MBOX_BACKGROUND_CHANNELS_ENABLE
    for(uint8_t i = 1; i < (BL00MBOX_CHANNELS); i++){
        if(bl00mbox_channel_background_active[i]){
            bl00mbox_audio_channel_render(&(channels[i]), acc, mono_len, 1);
        }
    }
#endif

    for(uint16_t i = 0; i < mono_len; i++){
        st3m_scope_write((acc[i])>>4);

        tx[2*i] = acc[i];
        tx[2*i+1] = acc[i];
    }
}

// radspa requirement helper functions

bool radspa_host_request_buffer_render(int16_t * buf, uint16_t num_samples){
    bl00mbox_bud_t * bud = ((bl00mbox_connection_source_t *) buf)->source_bud;
    bl00mbox_audio_bud_render(bud, num_samples);
    return 1;
}

// py: bigtable = [int(22/200*(2**(14-5+8+x*4096/2400/64))) for x in range(64)]
// for 48kHz main sample rate
const uint16_t bigtable[64] = {
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
const uint16_t smoltable[64] = {
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

// TEMP
void bl00mbox_player_function(int16_t * rx, int16_t * tx, uint16_t len){
    bl00mbox_audio_render(rx, tx, len);
}
