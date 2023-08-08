//SPDX-License-Identifier: CC0-1.0
#include "radspa_helpers.h"

radspa_signal_t * radspa_signal_get_by_index(radspa_t * plugin, uint16_t signal_index){
    radspa_signal_t * ret = plugin->signals;
    for(uint16_t i = 0; i < signal_index; i++){
        ret = ret->next;
        if(ret == NULL) break;
    }
    return ret;
}

void radspa_signal_set(radspa_t * plugin, uint8_t signal_index, char * name, uint32_t hints, int16_t value){
    radspa_signal_t * sig = radspa_signal_get_by_index(plugin, signal_index);
    if(sig == NULL) return;
    sig->name = name;
    sig->hints = hints;
    sig->value = value;
}

int16_t radspa_signal_add(radspa_t * plugin, char * name, uint32_t hints, int16_t value){
    radspa_signal_t * sig = calloc(1,sizeof(radspa_signal_t));
    if(sig == NULL) return -1; // allocation failed
    sig->name = name;
    sig->hints = hints;
    sig->unit = "";
    sig->description = "";
    sig->buffer = NULL;
    sig->next = NULL;
    sig->value = value;
    sig->name_multiplex = -1;
    sig->get_value = radspa_signal_get_value;
    
    //find end of linked list
    uint16_t list_index = 0;
    if(plugin->signals == NULL){
        plugin->signals = sig;
    } else {
        radspa_signal_t * sigs = plugin->signals;
        list_index++;
        while(sigs->next != NULL){
            sigs = sigs->next;
            list_index++;
        }
        sigs->next = sig;
    }
    if(plugin->len_signals != list_index){ abort(); }  
    plugin->len_signals++;
    return list_index;
}

int16_t radspa_signal_get_value(radspa_signal_t * sig, int16_t index, uint16_t num_samples, uint32_t render_pass_id){
    if(sig->buffer != NULL){
        if(sig->render_pass_id != render_pass_id){
            radspa_host_request_buffer_render(sig->buffer, num_samples); //, render_pass_id);
            sig->render_pass_id = render_pass_id;
        }
        return sig->buffer[index];
    }
    return sig->value;
}

radspa_t * radspa_standard_plugin_create(radspa_descriptor_t * desc, uint8_t num_signals, size_t plugin_data_size, uint32_t plugin_table_size){
    radspa_t * ret = calloc(1, sizeof(radspa_t));
    if(ret == NULL) return NULL;
    if(plugin_data_size){
        ret->plugin_data = calloc(1,plugin_data_size);
        if(ret->plugin_data == NULL){
            free(ret);
            return NULL;
        }
    }
    ret->signals = NULL;
    ret->len_signals = 0;
    ret->render = NULL;
    ret->descriptor = desc;
    ret->plugin_table_len = plugin_table_size;

    bool init_failed = false;
    for(uint8_t i = 0; i < num_signals; i++){
        if(radspa_signal_add(ret,"UNINITIALIZED",0,0) == -1){
            init_failed = true;
            break;
        }
    }

    if(ret->plugin_table_len){
        ret->plugin_table = calloc(plugin_table_size, sizeof(int16_t));
        if(ret->plugin_table == NULL) init_failed = true;
    } else {
        ret->plugin_table = NULL;
    }

    if(init_failed){
        radspa_standard_plugin_destroy(ret);
        return NULL;
    }
    return ret;
}

void radspa_standard_plugin_destroy(radspa_t * plugin){
    radspa_signal_t * sig = plugin->signals;
    while(sig != NULL){
        radspa_signal_t * sig_next = sig->next;
        free(sig);
        sig = sig_next;
    }
    if(plugin->plugin_table != NULL) free(plugin->plugin_table);
    if(plugin->plugin_data != NULL) free(plugin->plugin_data);
    free(plugin);
}
