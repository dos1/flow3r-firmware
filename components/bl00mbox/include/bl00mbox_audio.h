#pragma once

//TODO: move this to kconfig someday
#define BL00MBOX_MAX_BUFFER_LEN 256

#include "radspa.h"

struct _bl00mbox_bud_t;
struct _bl00mbox_connection_source_t;
struct _bl00mbox_channel_root_t;
struct _bl00mbox_channel_t;

typedef struct _bl00mbox_bud_t{
    radspa_t * plugin; // plugin
    uint32_t index; // unique index number for bud
    uint32_t render_pass_id; // may be used by host to determine whether recomputation is necessary
    uint8_t channel; // index of channel that owns the plugin
    struct _bl00mbox_bud_t * chan_next; //for linked list in bl00mbox_channel_t
} bl00mbox_bud_t;

typedef struct _bl00mbox_connection_source_t{
    int16_t buffer[BL00MBOX_MAX_BUFFER_LEN]; // MUST stay on top of struct bc type casting!
    struct _bl00mbox_bud_t * source_bud;
    uint32_t signal_index; // signal of source_bud that renders to buffer
    uint8_t output_subscribers;
    uint8_t channel;
} bl00mbox_connection_source_t;

typedef struct _bl00mbox_channel_root_t{
    struct _bl00mbox_connection_source_t * con;
    struct _bl00mbox_channel_root_t * next;
} bl00mbox_channel_root_t;

typedef struct{
    bool is_active; // rendering can be skipped if false
    char name[32];
    int32_t volume;
    struct _bl00mbox_channel_root_t * root_list; // list of all roots associated with channels
    uint32_t render_pass_id; // may be used by host to determine whether recomputation is necessary
    struct _bl00mbox_bud_t * buds; // linked list with all channel buds
} bl00mbox_channel_t;

bl00mbox_channel_t * bl00mbox_get_channel(uint8_t chan);
void bl00mbox_channel_enable(uint8_t chan);

void bl00mbox_channel_enable(uint8_t chan);
void bl00mbox_channel_disable(uint8_t chan);
void bl00mbox_channel_set_volume(uint8_t chan, uint16_t volume);
int16_t bl00mbox_channel_get_volume(uint8_t chan);
