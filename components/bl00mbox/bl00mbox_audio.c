//SPDX-License-Identifier: CC0-1.0
#include "bl00mbox_audio.h"

static bool is_initialized = false;
static bool bl00mbox_audio_run = true;
void bl00mbox_audio_enable(){ bl00mbox_audio_run = true; }
void bl00mbox_audio_disable(){ bl00mbox_audio_run = false; }
static uint16_t full_buffer_len;

static uint32_t render_pass_id;

int16_t * bl00mbox_line_in_interlaced = NULL;

// fixed-length list of channels
static bl00mbox_channel_t channels[BL00MBOX_CHANNELS];
static int8_t last_chan_event = 0;
// foregrounded channel is always rendered
// note: regardless of settings the system channel 0 is always rendered
static uint8_t bl00mbox_channel_foreground = 0;
// channels may request being active while not being in foreground
static bool bl00mbox_channel_background_mute_override[BL00MBOX_CHANNELS] = {false,};

bool bl00mbox_channel_set_background_mute_override(uint8_t channel_index, bool enable){
#ifdef BL00MBOX_BACKGROUND_MUTE_OVERRIDE_ENABLE
    if(channel_index >= BL00MBOX_CHANNELS) return false;
    bl00mbox_channel_background_mute_override[channel_index] = enable;
    return true;
#else
    return false;
#endif
}

bool bl00mbox_channel_get_background_mute_override(uint8_t channel_index){
    if(channel_index >= BL00MBOX_CHANNELS) return false;
    return bl00mbox_channel_background_mute_override[channel_index];
}

static void ** ptr_to_be_set_by_audio_task = NULL;
static void * ptr_to_be_set_by_audio_task_target = NULL;
// TODO: multicore thread safety on this boi
static volatile bool ptr_set_request_pending = false;
static uint64_t bl00mbox_audio_waitfor_timeout = 0ULL;

bool bl00mbox_audio_waitfor_pointer_change(void ** ptr, void * new_val){
    /// takes pointer to pointer that is to be set null
    if(!is_initialized) return false;
    ptr_to_be_set_by_audio_task = ptr;
    ptr_to_be_set_by_audio_task_target = new_val;
    ptr_set_request_pending = true;

    volatile uint64_t timeout = 0; // cute
    while(ptr_set_request_pending){
        timeout++;
        // TODO: nop
        if(bl00mbox_audio_waitfor_timeout && (timeout = bl00mbox_audio_waitfor_timeout)){
            return false;
        }
    }
    return true;
}

bool bl00mbox_audio_do_pointer_change(){
    if(ptr_set_request_pending){
        (* ptr_to_be_set_by_audio_task) = ptr_to_be_set_by_audio_task_target;
        ptr_to_be_set_by_audio_task_target = NULL;
        ptr_to_be_set_by_audio_task = NULL;
        ptr_set_request_pending = false;
        return true;
    }
    return false;
}

void bl00mbox_channel_event(uint8_t chan){
#ifdef BL00MBOX_AUTO_FOREGROUNDING
    last_chan_event = chan;
#endif
}

bl00mbox_channel_t * bl00mbox_get_channel(uint8_t channel_index){
    if(channel_index >= BL00MBOX_CHANNELS) return NULL;
    return &(channels[channel_index]);
}

uint8_t bl00mbox_channel_get_foreground_index(){
    return bl00mbox_channel_foreground;
}

void bl00mbox_channel_set_foreground_index(uint8_t channel_index){
    if(channel_index >= BL00MBOX_CHANNELS) return;
    last_chan_event = channel_index;
}

bool bl00mbox_channel_get_free(uint8_t channel_index){
    if(channel_index >= BL00MBOX_CHANNELS) return false;
    return bl00mbox_get_channel(channel_index)->is_free;
}

bool bl00mbox_channel_set_free(uint8_t channel_index, bool free){
    if(channel_index >= BL00MBOX_CHANNELS) return false;
    bl00mbox_get_channel(channel_index)->is_free = free;
    return true;
}

uint8_t bl00mbox_channel_get_free_index(){
    uint8_t ret = 1; // never return system channel
    for(; ret < BL00MBOX_CHANNELS; ret++){
        if(bl00mbox_get_channel(ret)->is_free){
            bl00mbox_get_channel(ret)->is_free = false;
            break;
        }
    }
    last_chan_event = ret;
    return ret;
}

char * bl00mbox_channel_get_name(uint8_t channel_index){
    if(channel_index >= BL00MBOX_CHANNELS) return NULL;
    return bl00mbox_get_channel(channel_index)->name;
}

void bl00mbox_channel_set_name(uint8_t channel_index, char * new_name){
    if(channel_index >= BL00MBOX_CHANNELS) return;
    bl00mbox_channel_t * chan =  bl00mbox_get_channel(channel_index);
    if(chan->name != NULL) free(chan->name);
    chan->name = strdup(new_name);
}

void bl00mbox_channels_init(){
    for(uint8_t i = 0; i < BL00MBOX_CHANNELS; i++){
        bl00mbox_channel_t * chan = bl00mbox_get_channel(i);
        chan->volume = BL00MBOX_DEFAULT_CHANNEL_VOLUME;
        chan->root_list = NULL;
        chan->buds = NULL;
        chan->connections = NULL;
        chan->is_active = true;
        chan->is_free = true;
        chan->name = NULL;
        chan->dc = 0;
    }
    is_initialized = true;
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

void bl00mbox_audio_bud_render(bl00mbox_bud_t * bud){
    if(bud->render_pass_id == render_pass_id) return;
#ifdef BL00MBOX_LOOPS_ENABLE
    if(bud->is_being_rendered) return;
#endif
    bud->is_being_rendered = true;
    bud->plugin->render(bud->plugin, full_buffer_len, render_pass_id);
    bud->render_pass_id = render_pass_id;
    bud->is_being_rendered = false;
}

static bool bl00mbox_audio_channel_render(bl00mbox_channel_t * chan, int16_t * out, bool adding){
    if(render_pass_id == chan->render_pass_id) return false;
    chan->render_pass_id = render_pass_id;

    bl00mbox_channel_root_t * root = chan->root_list;

    // early exit when no sources:
    if((root == NULL) || (!chan->is_active)){
        return false;
    }

    int32_t acc[full_buffer_len];
    bool acc_init = false;

    while(root != NULL){
        bl00mbox_audio_bud_render(root->con->source_bud);
        if(root->con->buffer[1] == -32768){
            if(!acc_init){
                for(uint16_t i = 0; i < full_buffer_len; i++){
                    acc[i] = root->con->buffer[0];
                }
                acc_init = true;
            } else if(root->con->buffer[0]){
                for(uint16_t i = 0; i < full_buffer_len; i++){
                    acc[i] += root->con->buffer[0];
                }
            }
        } else {
            if(!acc_init){
                for(uint16_t i = 0; i < full_buffer_len; i++){
                    acc[i] = root->con->buffer[i];
                }
                acc_init = true;
            } else {
                for(uint16_t i = 0; i < full_buffer_len; i++){
                    acc[i] += root->con->buffer[i];
                }
            }
        }
        root = root->next;
    }

    for(uint16_t i = 0; i < full_buffer_len; i++){
        // flip around for rounding towards zero/mulsh boost
        bool invert = chan->dc < 0;
        if(invert) chan->dc = -chan->dc;
        chan->dc = ((uint64_t) chan->dc * (((1<<12) - 1)<<20)) >> 32;
        if(invert) chan->dc = -chan->dc;

        chan->dc += acc[i];

        acc[i] -= (chan->dc >> 12);
    }

    if(adding){
        for(uint16_t i = 0; i < full_buffer_len; i++){
            out[i] = radspa_add_sat(radspa_mult_shift(acc[i], chan->volume), out[i]);
        }
    } else {
        for(uint16_t i = 0; i < full_buffer_len; i++){
            out[i] = radspa_mult_shift(acc[i], chan->volume);
        }
    }
    return true;
}

bool _bl00mbox_audio_render(int16_t * rx, int16_t * tx, uint16_t len){
    if(!is_initialized) return false;

    bl00mbox_audio_do_pointer_change();
    bl00mbox_channel_foreground = last_chan_event;

    if(!bl00mbox_audio_run) return false;

    render_pass_id++; // fresh pass, all relevant sources must be recomputed
    full_buffer_len = len/2;
    bl00mbox_line_in_interlaced = rx;
    int16_t acc[full_buffer_len];
    bool acc_init = false;
    // system channel always runs non-adding
    acc_init = bl00mbox_audio_channel_render(&(channels[0]), acc, acc_init) || acc_init;

    // re-rendering channels is ok, if render_pass_id didn't change it will just exit
    acc_init = bl00mbox_audio_channel_render(&(channels[bl00mbox_channel_foreground]), acc, acc_init) || acc_init;

    // TODO: scales poorly if there's many channels
#ifdef BL00MBOX_BACKGROUND_MUTE_OVERRIDE_ENABLE
    for(uint8_t i = 1; i < (BL00MBOX_CHANNELS); i++){
        if(bl00mbox_channel_background_mute_override[i]){
            acc_init = bl00mbox_audio_channel_render(&(channels[i]), acc, acc_init) || acc_init;
        }
    }
#endif

    if(!acc_init) return false;

    for(uint16_t i = 0; i < full_buffer_len; i++){
        tx[2*i] = acc[i];
        tx[2*i+1] = acc[i];
    }
    return true;
}

void bl00mbox_audio_render(int16_t * rx, int16_t * tx, uint16_t len){
    if(!_bl00mbox_audio_render(rx, tx, len)) memset(tx, 0, len*sizeof(int16_t));
}
