//SPDX-License-Identifier: CC0-1.0
#pragma once

//TODO: move this to kconfig someday?
#define BL00MBOX_MAX_BUFFER_LEN 128
#define BL00MBOX_DEFAULT_CHANNEL_VOLUME 8000
#define BL00MBOX_CHANNELS 32
#define BL00MBOX_BACKGROUND_MUTE_OVERRIDE_ENABLE

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "radspa.h"

struct _bl00mbox_bud_t;
struct _bl00mbox_connection_source_t;
struct _bl00mbox_channel_root_t;
struct _bl00mbox_channel_t;

extern int16_t * bl00mbox_line_in_interlaced;

typedef struct _bl00mbox_bud_t{
    radspa_t * plugin; // plugin
    char * name;
    uint64_t index; // unique index number for bud
    uint32_t render_pass_id; // may be used by host to determine whether recomputation is necessary
    uint8_t channel; // index of channel that owns the plugin
    struct _bl00mbox_bud_t * chan_next; //for linked list in bl00mbox_channel_t
} bl00mbox_bud_t;

typedef struct _bl00mbox_connection_subscriber_t{
    uint8_t type; // 0: standard signal input, 1: output mixer
    uint8_t channel;
    uint64_t bud_index;
    uint32_t signal_index;
    struct _bl00mbox_connection_subscriber_t * next;
} bl00mbox_connection_subscriber_t;

typedef struct _bl00mbox_connection_t{ //child of bl00mbox_ll_t
    int16_t buffer[BL00MBOX_MAX_BUFFER_LEN]; // MUST stay on top of struct bc type casting!
    struct _bl00mbox_bud_t * source_bud;
    uint32_t signal_index; // signal of source_bud that renders to buffer
    struct _bl00mbox_connection_subscriber_t * subs;
    uint8_t channel;
    struct _bl00mbox_connection_t * chan_next; //for linked list in bl00mbox_channel_t;
} bl00mbox_connection_t;

typedef struct _bl00mbox_channel_root_t{
    struct _bl00mbox_connection_t * con;
    struct _bl00mbox_channel_root_t * next;
} bl00mbox_channel_root_t;

typedef struct{
    bool is_active; // rendering can be skipped if false
    bool is_free;
    char * name;
    int32_t volume;
    struct _bl00mbox_channel_root_t * root_list; // list of all roots associated with channels
    uint32_t render_pass_id; // may be used by host to determine whether recomputation is necessary
    struct _bl00mbox_bud_t * buds; // linked list with all channel buds
    struct _bl00mbox_connection_t * connections; // linked list with all channel connections
} bl00mbox_channel_t;

bl00mbox_channel_t * bl00mbox_get_channel(uint8_t chan);
void bl00mbox_channel_enable(uint8_t chan);

void bl00mbox_channel_enable(uint8_t chan);
void bl00mbox_channel_disable(uint8_t chan);
void bl00mbox_channel_set_volume(uint8_t chan, uint16_t volume);
int16_t bl00mbox_channel_get_volume(uint8_t chan);
void bl00mbox_channel_event(uint8_t chan);
uint8_t bl00mbox_channel_get_free_index();
void bl00mbox_channels_init();
uint8_t bl00mbox_channel_get_foreground_index();
void bl00mbox_channel_set_foreground_index(uint8_t channel_index);
bool bl00mbox_channel_get_free(uint8_t channel_index);
bool bl00mbox_channel_set_free(uint8_t channel_index, bool free);

bool bl00mbox_channel_get_background_mute_override(uint8_t channel_index);
bool bl00mbox_channel_set_background_mute_override(uint8_t channel_index, bool enable);

char * bl00mbox_channel_get_name(uint8_t channel_index);
void bl00mbox_channel_set_name(uint8_t channel_index, char * new_name);

bool bl00mbox_audio_waitfor_pointer_change(void ** ptr, void * new_val);
void bl00mbox_audio_bud_render(bl00mbox_bud_t * bud);
