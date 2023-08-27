//SPDX-License-Identifier: CC0-1.0
#pragma once
#include "radspa.h"

// adds signal to plugin instance struct. typically used to initiate a plugin instance.
int16_t radspa_signal_add(radspa_t * plugin, char * name, uint32_t hints, int16_t value);
// as above, but sets parameters of an already existing signal with at list position signal_index
void radspa_signal_set(radspa_t * plugin, uint8_t signal_index, char * name, uint32_t hints, int16_t value);
void radspa_signal_set_group(radspa_t * plugin, uint8_t group_len, uint8_t step, uint8_t signal_index, char * name,
                                    uint32_t hints, int16_t value);
void radspa_signal_set_description(radspa_t * plugin, uint8_t signal_index, char * description);
void radspa_signal_set_group_description(radspa_t * plugin, uint8_t group_len, uint8_t step, uint8_t signal_index,
                                    char * description);

radspa_t * radspa_standard_plugin_create(radspa_descriptor_t * desc, uint8_t num_signals, size_t plugin_data_size,
                                                uint32_t plugin_table_size);
void radspa_standard_plugin_destroy(radspa_t * plugin);

// frees all signal structs. typically used to destroy a plugin instance.
void radspa_signals_free(radspa_t * plugin);

/* returns the value that a signal has at a given moment in time. time is
 * represented as the buffer index. requests rendering from host and requires implementation
 * of radspa_host_request_buffer_render.
 */
inline int16_t radspa_signal_get_value(radspa_signal_t * sig, int16_t index, uint32_t render_pass_id){
    if(sig->buffer != NULL){
        if(sig->render_pass_id != render_pass_id){
            radspa_host_request_buffer_render(sig->buffer);
            sig->render_pass_id = render_pass_id;
        }
        return sig->buffer[index];
    } else {
        return sig->value;
    }
}

inline void radspa_signal_set_value(radspa_signal_t * sig, int16_t index, int16_t val){
    if(sig->buffer != NULL) sig->buffer[index] = val;
    if(!index) sig->value = val;
}

// #define RADSPA_SIGNAL_CACHING
// get signal struct from a signal index
inline radspa_signal_t * radspa_signal_get_by_index(radspa_t * plugin, uint16_t signal_index){
    radspa_signal_t * ret = NULL;
    if(plugin == NULL) return ret; // clang-tidy
#ifdef RADSPA_SIGNAL_CACHING
    static radspa_signal_t * cache_s = NULL;
    static radspa_t * cache_p = NULL;
    static uint16_t cache_i = 0;

    if((plugin == cache_p) && (signal_index == cache_i + 1) && (cache_s != NULL)){
        ret = cache_s->next;
    }
    if(ret == NULL){
#endif
        ret = plugin->signals;
        for(uint16_t i = 0; i < signal_index; i++){
            ret = ret->next;
            if(ret == NULL) break;
        }
#ifdef RADSPA_SIGNAL_CACHING
    }
    cache_s = ret;
    cache_p = plugin;
    cache_i = signal_index;
#endif
    return ret;
}

