#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include <stdio.h>
#include <math.h>
#include <string.h>

#include "bl00mbox_audio.h"
#include "bl00mbox_plugin_registry.h"

static uint64_t bl00mbox_bud_index = 0;

uint16_t bl00mbox_channel_buds_num(uint8_t channel){
    bl00mbox_channel_t * chan = bl00mbox_get_channel(channel);
    uint16_t ret = 0;
    if(chan->buds != NULL){
        bl00mbox_bud_t * last = chan->buds;
        ret++;
        while(last->chan_next != NULL){
            last = last->chan_next;
            ret++;
        } 
    }
    return ret;
}

bl00mbox_bud_t * bl00mbox_channel_get_bud_by_index(uint8_t channel, uint32_t index){
    bl00mbox_channel_t * chan = bl00mbox_get_channel(channel);
    if(chan == NULL) return NULL;
    if(chan->buds == NULL) return NULL;
    bl00mbox_bud_t * bud = chan->buds;
    while(true){
        if(bud->index == index) break;
        bud = bud->chan_next;
        if(bud == NULL) break;
    }
    return bud;
}

bl00mbox_bud_t * bl00mbox_channel_new_bud(uint8_t channel, uint32_t id, uint32_t init_var){
    /// creates a new bud instance of the plugin with descriptor id "id" and the initialization variable
    /// "init_var" and appends it to the plugin list of the corresponding channel. returns pointer to
    /// the bud if successfull, else NULL.
    bl00mbox_channel_t * chan = bl00mbox_get_channel(channel);
    if(chan == NULL) return NULL;
    radspa_descriptor_t * desc = bl00mbox_plugin_registry_get_descriptor_from_id(id);
    if(desc == NULL) return NULL;
    bl00mbox_bud_t * bud = malloc(sizeof(bl00mbox_bud_t));
    if(bud == NULL) return NULL;
    radspa_t * plugin = desc->create_plugin_instance(init_var);
    if(plugin == NULL){ free(bud); return NULL; }

    bud->plugin = plugin;
    bud->channel = channel;
    //TODO: look for empty indices
    bud->index = bl00mbox_bud_index;
    bl00mbox_bud_index++;
    bud->chan_next = NULL;

    // append to channel bud list
    if(chan->buds == NULL){
        chan->buds = bud;
    } else {
        bl00mbox_bud_t * last = chan->buds;
        while(last->chan_next != NULL){ last = last->chan_next; } 
        last->chan_next = bud;
    }
    bl00mbox_channel_event(channel);
    return bud;
}

static void bl00mbox_add_connection_to_channel_list(uint8_t channel, bl00mbox_connection_t * conn){
    bl00mbox_channel_t * chan = bl00mbox_get_channel(channel);
    bl00mbox_connection_t * chan_conn = chan->connections;
    if(chan_conn == NULL){
        chan_conn = conn;
    } else {
        while(chan_conn->chan_next != NULL){ chan_conn = chan_conn->chan_next; }
        chan_conn->chan_next = conn;
    }
}

static bool bl00mbox_add_signal_to_connection_list(bl00mbox_connection_t * conn, bl00mbox_bud_t * bud, uint8_t signal_index){
    bl00mbox_connection_signal_t * con_sig = malloc(sizeof(bl00mbox_connection_signal_t));
    if(con_sig == NULL) return false;
    con_sig->bud = bud;
    con_sig->signal_index = signal_index;
    con_sig->next = NULL;

    bl00mbox_connection_signal_t * sub = conn->subscribers;
    if(sub == NULL){
        conn->subscribers = con_sig;
    } else {
        while(sub->next != NULL){ sub = sub->next; }
        sub->next = con_sig;
    }
    return true;
}

bool bl00mbox_channel_connect_signal_to_output_mixer(uint8_t channel, uint32_t bud_index, uint32_t bud_signal_index){
    bl00mbox_channel_t * chan = bl00mbox_get_channel(channel);
    if(chan == NULL) return false;
    bl00mbox_bud_t * bud = bl00mbox_channel_get_bud_by_index(channel, bud_index);
    if(bud == NULL) return false;
    radspa_signal_t * tx = radspa_signal_get_by_index(bud->plugin, bud_signal_index);
    if(tx == NULL) return false;

    bl00mbox_channel_root_t * root = malloc(sizeof(bl00mbox_channel_root_t));
    if(root == NULL) return false;

    bl00mbox_connection_t * conn;
    if(tx->buffer == NULL){ // doesn't feed a buffer yet
        conn = malloc(sizeof(bl00mbox_connection_t));
        if(conn == NULL){
            free(root);
            return false;
        }
        // set up new connection
        conn->signal_index = bud_signal_index;
        conn->source_bud = bud;
        conn->output_subscribers = 0;
        conn->channel = channel;
        tx->buffer = conn->buffer;
        bl00mbox_add_connection_to_channel_list(channel, conn);
    } else {
        conn = (bl00mbox_connection_t *) tx->buffer; // buffer sits on top of struct
    }
    conn->output_subscribers++;

    root->con = conn;
    root->next = NULL;
    
    if(chan->root_list == NULL){
        chan->root_list = root;
    } else {
        bl00mbox_channel_root_t * last_root = chan->root_list;
        while(last_root->next != NULL){ last_root = last_root->next; }
        last_root->next = root;
    }
    bl00mbox_channel_event(channel);
    return true;
}

bool bl00mbox_channel_connect_signal(uint8_t channel, uint32_t bud_rx_index, uint32_t bud_rx_signal_index,
                                               uint32_t bud_tx_index, uint32_t bud_tx_signal_index){

    bl00mbox_bud_t * bud_rx = bl00mbox_channel_get_bud_by_index(channel, bud_rx_index);
    bl00mbox_bud_t * bud_tx = bl00mbox_channel_get_bud_by_index(channel, bud_tx_index);
    if(bud_tx == NULL || bud_rx == NULL) return false; // bud index doesn't exist

    radspa_signal_t * rx = radspa_signal_get_by_index(bud_rx->plugin, bud_rx_signal_index);
    radspa_signal_t * tx = radspa_signal_get_by_index(bud_tx->plugin, bud_tx_signal_index);
    if(tx == NULL || rx == NULL) return false; // signal index doesn't exist
    
    bl00mbox_connection_t * conn;
    if(tx->buffer == NULL){ // doesn't feed a buffer yet
        conn = malloc(sizeof(bl00mbox_connection_t));
        if(conn == NULL) return false; // no ram for connection
        // set up new connection
        conn->signal_index = bud_tx_signal_index;
        conn->source_bud = bud_tx;
        conn->output_subscribers = 0;
        conn->channel = channel;
        tx->buffer = conn->buffer;
        bl00mbox_add_connection_to_channel_list(channel, conn);
    } else {
        conn = (bl00mbox_connection_t *) tx->buffer; // buffer sits on top of struct
    }
    
    if(rx->buffer != tx->buffer){
        conn->output_subscribers++;
        //bool ret = bl00mbox_add_signal_to_connection_list(conn, bud_rx, bud_rx_signal_index);

        rx->buffer = tx->buffer;
    }
    bl00mbox_channel_event(channel);
    return true;
}

bool bl00mbox_channel_disconnect_signal_rx(uint8_t channel, uint32_t bud_rx_index, uint32_t bud_rx_signal_index){
    bl00mbox_bud_t * bud_rx = bl00mbox_channel_get_bud_by_index(channel, bud_rx_index);
    if(bud_rx == NULL) return false; // bud index doesn't exist

    radspa_signal_t * rx = radspa_signal_get_by_index(bud_rx->plugin, bud_rx_signal_index);
    if(rx == NULL) return false; // signal index doesn't exist
    
    bl00mbox_connection_t * conn = (bl00mbox_connection_t *) rx->buffer; // buffer sits on top of struct
    if(conn == NULL) return false; //not connected
    
    bl00mbox_bud_t * bud_tx = conn->source_bud;
    if(bud_tx == NULL) return false; // bud index doesn't exist
    radspa_signal_t * tx = radspa_signal_get_by_index(bud_tx->plugin, conn->signal_index);
    if(tx == NULL) return false; // signal index doesn't exist

    if(rx->buffer ==  NULL) return false;
    rx->buffer = NULL;
    conn->output_subscribers--;
    bl00mbox_channel_event(channel);
    return true;
}

uint16_t bl00mbox_channel_bud_get_num_signals(uint8_t channel, uint32_t bud_index){
    bl00mbox_channel_t * chan = bl00mbox_get_channel(channel);
    if(chan == NULL) return false;
    bl00mbox_bud_t * bud = bl00mbox_channel_get_bud_by_index(channel, bud_index);
    if(bud == NULL) return false;
    return bud->plugin->len_signals;
}

char * bl00mbox_channel_bud_get_name(uint8_t channel, uint32_t bud_index){
    bl00mbox_channel_t * chan = bl00mbox_get_channel(channel);
    if(chan == NULL) return false;
    bl00mbox_bud_t * bud = bl00mbox_channel_get_bud_by_index(channel, bud_index);
    if(bud == NULL) return false;
    return bud->plugin->descriptor->name;
}

char * bl00mbox_channel_bud_get_signal_name(uint8_t channel, uint32_t bud_index, uint32_t bud_signal_index){
    bl00mbox_channel_t * chan = bl00mbox_get_channel(channel);
    if(chan == NULL) return false;
    bl00mbox_bud_t * bud = bl00mbox_channel_get_bud_by_index(channel, bud_index);
    if(bud == NULL) return false;
    radspa_signal_t * sig = radspa_signal_get_by_index(bud->plugin, bud_signal_index);
    if(sig == NULL) return false;
    return sig->name;
}

char * bl00mbox_channel_bud_get_signal_description(uint8_t channel, uint32_t bud_index, uint32_t bud_signal_index){
    bl00mbox_channel_t * chan = bl00mbox_get_channel(channel);
    if(chan == NULL) return false;
    bl00mbox_bud_t * bud = bl00mbox_channel_get_bud_by_index(channel, bud_index);
    if(bud == NULL) return false;
    radspa_signal_t * sig = radspa_signal_get_by_index(bud->plugin, bud_signal_index);
    if(sig == NULL) return false;
    return sig->description;
}

char * bl00mbox_channel_bud_get_signal_unit(uint8_t channel, uint32_t bud_index, uint32_t bud_signal_index){
    bl00mbox_channel_t * chan = bl00mbox_get_channel(channel);
    if(chan == NULL) return false;
    bl00mbox_bud_t * bud = bl00mbox_channel_get_bud_by_index(channel, bud_index);
    if(bud == NULL) return false;
    radspa_signal_t * sig = radspa_signal_get_by_index(bud->plugin, bud_signal_index);
    if(sig == NULL) return false;
    return sig->unit;
}

bool bl00mbox_channel_bud_set_signal_value(uint8_t channel, uint32_t bud_index, uint32_t bud_signal_index, int16_t value){
    bl00mbox_channel_t * chan = bl00mbox_get_channel(channel);
    if(chan == NULL) return false;
    bl00mbox_bud_t * bud = bl00mbox_channel_get_bud_by_index(channel, bud_index);
    if(bud == NULL) return false;
    radspa_signal_t * sig = radspa_signal_get_by_index(bud->plugin, bud_signal_index);
    if(sig == NULL) return false;

    sig->value = value; 
    bl00mbox_channel_event(channel);
    return true;
}

int16_t bl00mbox_channel_bud_get_signal_value(uint8_t channel, uint32_t bud_index, uint32_t bud_signal_index){
    bl00mbox_channel_t * chan = bl00mbox_get_channel(channel);
    if(chan == NULL) return false;
    bl00mbox_bud_t * bud = bl00mbox_channel_get_bud_by_index(channel, bud_index);
    if(bud == NULL) return false;
    radspa_signal_t * sig = radspa_signal_get_by_index(bud->plugin, bud_signal_index);
    if(sig == NULL) return false;

    return sig->value;
}

uint32_t bl00mbox_channel_bud_get_signal_hints(uint8_t channel, uint32_t bud_index, uint32_t bud_signal_index){
    bl00mbox_channel_t * chan = bl00mbox_get_channel(channel);
    if(chan == NULL) return false;
    bl00mbox_bud_t * bud = bl00mbox_channel_get_bud_by_index(channel, bud_index);
    if(bud == NULL) return false;
    radspa_signal_t * sig = radspa_signal_get_by_index(bud->plugin, bud_signal_index);
    if(sig == NULL) return false;

    return sig->hints;
}
