#include "st3m_audio.h"
#include "st3m_scope.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include <stdio.h>
#include <math.h>
#include <string.h>

#include "bl00mbox_audio.h"
#include "bl00mbox_plugin_registry.h"

static uint32_t bl00mbox_bud_index = 0; //TODO: better system

#if 0
uint16_t bl00mbox_source_add(void * render_data, void * render_function){
    //construct audio source struct
    bl00mbox_channel_root_t * src = malloc(sizeof(bl00mbox_channel_root_t));
    if(src == NULL) return;
    src->render_data = render_data;
    src->render_function = render_function;
    src->next = NULL;
    src->index = 0;

    //handle empty list special case
    if(_audio_sources == NULL){
        _audio_sources = src;
        return 0; //only nonempty lists from here on out!
    }

    //searching for lowest unused index
    bl00mbox_channel_root_t * index_source = _audio_sources;
    while(1){
        if(src->index == (index_source->index)){
            src->index++; //someone else has index already, try next
            index_source = _audio_sources; //start whole list for new index
        } else {
            index_source = index_source->next;
        }
        if(index_source == NULL){ //traversed the entire list
            break;
        }
    }

    bl00mbox_channel_root_t * audio_source = _audio_sources;
    //append new source to linked list
    while(audio_source != NULL){
        if(audio_source->next == NULL){
            audio_source->next = src;
            break;
        } else {
        audio_source = audio_source->next;
        }
    }
    return src->index;
}

void bl00mbox_source_remove(uint16_t index){
    bl00mbox_channel_root_t * audio_source = _audio_sources;
    bl00mbox_channel_root_t * start_gap = NULL;

    while(audio_source != NULL){
        if(index == audio_source->index){
            if(start_gap == NULL){
                _audio_sources = audio_source->next;
            } else {
                start_gap->next = audio_source->next;
            }
            vTaskDelay(20 / portTICK_PERIOD_MS); //give other tasks time to stop using
            free(audio_source); //terrible hack tbh
            break;
        }
        start_gap = audio_source;
        audio_source = audio_source->next;
    }
}

#endif

#if 0
// attempts to create a new bud in a channel. returns pointer to bud upon success, NULL pointer on failure.
bl00mbox_bud_t * bl00mbox_user_channel_new_bud(uint8_t channel, uint32_t plugin_id, uint32_t init_var){
    uint32_t index = 0;
    for(; index < BL00MBOX_NUM_PLUGINS; index++){ // traverse plugin registry for id
        if(bl00mbox_plugin_registry[index].id == plugin_id) break;
        if(index+1 = BL00MBOX_NUM_PLUGINS) return NULL; // plugin id not found
    }
    bl00mbox_bud_t * bud = malloc(sizeof(bl00mbox_bud_t));
    if(bud == NULL) return NULL;
    radspa_t * plugin = bl00mbox_plugin_registry[index].new_plugin_instance(init_var);
    if(plugin == NULL){
        free(bud);
        return NULL;
    }
    plugin->descriptor = &(bloombox_plugin_registry[index]);
    bud->plugin = plugin;
    bud->channel = channel;

    //TODO: look for empty indices pls
    bud->index = bl00mbox_bud_index;
    bl00mbox_bud_index++;

    dest = bl00mbox_channel[channel]->buds;
    while(dest != NULL){
        dest = dest->next;
    }
    dest = bud;
    return bud;
}

bl00mbox_bud_t * bl00mbox_user_get_bud_by_index(channel, index){
    bud = bl00mbox_channel[channel]->buds;
    while(bud != NULL){
        if(bud->index == index) break;
        bud = dest->nest;
    }
    return bud;
}

char * bl00mbox_user_bud_get_name(uint8_t channel, uint32_t bud_index){
    bl00mbox_bud_t bud = bl00mbox_user_get_bud_by_index(channel, but_index);
    if(bud == NULL) return NULL;
    return &(bud->plugin->descriptor.name);
}

char * bl00mbox_user_bud_get_description(uint8_t channel, uint32_t bud_index){
    bl00mbox_bud_t bud = bl00mbox_user_get_bud_by_index(channel, but_index);
    if(bud == NULL) return NULL;
    return &(bud->plugin->descriptor.name);
}

uint32_t bl00mbox_user_bud_get_signal_num(uint8_t channel, uint32_t bud_index){
    bl00mbox_bud_t bud = bl00mbox_user_get_bud_by_index(channel, but_index);
    if(bud == NULL) return 0;
    return bud->plugin->signal_num;
}

char * bl00mbox_user_bud_get_signal_name(uint8_t channel, uint32_t bud_index, uint32_t signal_index){
    bl00mbox_bud_t bud = bl00mbox_user_get_bud_by_index(channel, but_index);
    if(bud == NULL) return NULL;
    radspa_signal_t sig = radspa_get_signal_by_index(bud->plugin, signal_index);
    if(sig == NULL) return NULL;
    return &(sig->name);
}

uint32_t bl00mbox_user_bud_get_signal_hints(uint8_t channel, uint32_t bud_index, uint32_t signal_index){
    bl00mbox_bud_t bud = bl00mbox_user_get_bud_by_index(channel, but_index);
    if(bud == NULL) return NULL;
    radspa_signal_t sig = radspa_get_signal_by_index(bud->plugin, signal_index);
    if(sig == NULL) return NULL;
    return sig->hints;
}

bool bl00mbox_user_connect_signal(uint8_t channel, uint32_t bud_rx_index, uint32_t bud_rx_signal_index,
                                               uint32_t bud_tx_index, uint32_t bud_tx_signal_index){

    bl00mbox_bud_t * bud_rx = bl00mbox_user_get_bud_by_index(channel, bud_rx_index);
    bl00mbox_bud_t * bud_tx = bl00mbox_user_get_bud_by_index(channel, bud_tx_index);
    if(bud_tx == NULL || bud_rx == NULL) return false; // bud index doesn't exist

    radspa_signal_t * rx = radspa_get_signal_by_index(bud_rx->plugin, bud_rx_signal_index);
    radspa_signal_t * tx = radspa_get_signal_by_index(bud_tx->plugin, bud_tx_signal_index);
    if(tx == NULL || rx == NULL) return false; // signal index doesn't exist
    
    bl00mbox_connection_source_t * conn;
    if(tx->buffer == NULL){ // doesn't feed a buffer yet
        bl00mbox_connection_source_t * conn = malloc(sizeof(bl00mbox_connection_source_t));
        if(conn == NULL) return false; // no ram for connection
        // set up new connection
        tx->buffer = conn->buffer;
        conn->signal_index = bud_tx_signal_index;
        conn->source_bud = bud_tx;
        conn->output_subscribers = 0;
        conn->channel = channel;
    } else {
        conn = (bl00mbox_connection_source_t *) tx->buffer; // buffer sits on top of struct
    }
    
    if(rx->buffer != tx->buffer){
        conn->output_subscribers++;
        rx->buffer = tx->buffer;
    }
    return true;
}

bool bl00mbox_user_disconnect_signal(uint8_t channel, uint32_t bud_rx_index, uint32_t bud_rx_signal_index, int16_t val){
    bl00mbox_bud_t * bud_rx = bl00mbox_user_get_bud_by_index(channel, bud_rx_index);
    if(bud_rx == NULL) return false; // bud index doesn't exist
    radspa_signal_t * rx = radspa_get_signal_by_index(bud_rx->plugin, bud_rx_signal_index);
    if(rx == NULL) return false; // signal index doesn't exist
    if(rx->buffer == NULL) return false; // signal not connected

    conn = (bl00mbox_connection_source_t *) rx->buffer // buffer sits on top of struct
    bl00mbox_bud_t * bud_tx = conn->source_bud;
    if(bud_tx == NULL) return false; // dangling source, should actually throw error
    radspa_signal_t * tx = radspa_get_signal_by_index(bud_tx->plugin, bud_tx_signal_index);
    if(tx == NULL) return false; // undefined source, should actually throw error

    if(tx->buffer == rx->buffer){
        rx->buffer = NULL;
        rx->value = val;
        conn->output_subscribers--;
    } else {
        return false; // wrongly indexed source, should actually throw error
    }   // should really look up how to throw errors huh

    if(!(conn->output_subscribers)){ // delete connection
        tx->buffer = NULL;
        free(conn); // pls don't call during render pls
    }
}
#endif
