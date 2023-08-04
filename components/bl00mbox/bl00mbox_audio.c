//SPDX-License-Identifier: CC0-1.0
#include "bl00mbox_audio.h"

static bool is_initialized = false;
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
    if(!is_initialized) return false;
    /// takes pointer to pointer that is to be set null
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
    last_chan_event = chan;
}

bl00mbox_channel_t * bl00mbox_get_channel(uint8_t index){
    if(index >= BL00MBOX_CHANNELS) return NULL;
    return &(channels[index]);
}

uint8_t bl00mbox_channel_get_foreground_index(){
    return bl00mbox_channel_foreground;
}

void bl00mbox_channel_set_foreground_index(uint8_t index){
    if(index >= BL00MBOX_CHANNELS) return;
    bl00mbox_channel_foreground = index;
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
        chan->root_list = NULL;
        chan->buds = NULL;
        chan->connections = NULL;
        chan->is_active = true;
        chan->is_free = true;
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

void bl00mbox_audio_bud_render(bl00mbox_bud_t * bud, uint16_t num_samples){
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
            out[i] = radspa_add_sat(radspa_mult_shift(acc[i], chan->volume), out[i]);
        } else {
            out[i] = radspa_mult_shift(acc[i], chan->volume);
        }
    }
}

void bl00mbox_audio_render(int16_t * rx, int16_t * tx, uint16_t len){

    if(!is_initialized){
        memset(tx, 0, len*sizeof(int16_t)); // mute
        return;
    }

    bl00mbox_audio_do_pointer_change();
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

    // TODO: scales poorly if there's many channels
#ifdef BL00MBOX_BACKGROUND_MUTE_OVERRIDE_ENABLE
    for(uint8_t i = 1; i < (BL00MBOX_CHANNELS); i++){
        if(bl00mbox_channel_background_mute_override[i]){
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

// TEMP
void bl00mbox_player_function(int16_t * rx, int16_t * tx, uint16_t len){
    bl00mbox_audio_render(rx, tx, len);
}
