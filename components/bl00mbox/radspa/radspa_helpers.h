//SPDX-License-Identifier: CC0-1.0
#pragma once
#include "radspa.h"

// adds signal to plugin instance struct. typically used to initiate a plugin instance.
int16_t radspa_signal_add(radspa_t * plugin, char * name, uint32_t hints, int16_t value);
// as above, but sets parameters of an already existing signal with at list position signal_index
radspa_signal_t * radspa_signal_set(radspa_t * plugin, uint8_t signal_index, char * name, uint32_t hints, int16_t value);
radspa_signal_t * radspa_signal_set_group(radspa_t * plugin, uint8_t group_len, uint8_t step, uint8_t signal_index, char * name,
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
        if(sig->buffer[1] == -32768) return sig->buffer[0];
        return sig->buffer[index];
    }
    return sig->value;
}

inline void radspa_signal_set_value(radspa_signal_t * sig, int16_t index, int32_t val){
    if(sig->buffer != NULL){
        sig->buffer[index] = radspa_clip(val);
    } else if(!index){
        sig->value = radspa_clip(val);
    }
}

inline void radspa_signal_set_value_check_const(radspa_signal_t * sig, int16_t index, int32_t val){
    if(sig->buffer == NULL){
        if(!index) sig->value = radspa_clip(val);
        return;
    }
    val = radspa_clip(val);
    if(index == 0){
        sig->buffer[0] = val;
    } else if(index == 1){
        if(val == sig->buffer[0]) sig->buffer[1] = -32768;
    } else {
        if((sig->buffer[1] == -32768) && (val != sig->buffer[0])) sig->buffer[1] = sig->buffer[0];
        sig->buffer[index] = val;
    }
}

inline void radspa_signal_set_const_value(radspa_signal_t * sig, int32_t val){
    if(sig->buffer == NULL){
        sig->value = radspa_clip(val);
    } else {
        sig->buffer[0] = radspa_clip(val);
        sig->buffer[1] = -32768;
    }
}

#define RADSPA_SIGNAL_NONCONST -32768

inline int16_t radspa_signal_get_const_value(radspa_signal_t * sig, uint32_t render_pass_id){
    if(sig->buffer != NULL){
        if(sig->render_pass_id != render_pass_id){
            radspa_host_request_buffer_render(sig->buffer);
            sig->render_pass_id = render_pass_id;
        }
        if(sig->buffer[1] == -32768) return sig->buffer[0];
        return RADSPA_SIGNAL_NONCONST;
    }
    return sig->value;
}

// get signal struct from a signal index
radspa_signal_t * radspa_signal_get_by_index(radspa_t * plugin, uint16_t signal_index);

