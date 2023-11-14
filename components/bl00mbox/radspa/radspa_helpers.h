//SPDX-License-Identifier: CC0-1.0
#pragma once
#include "radspa.h"

#define RADSPA_SIGNAL_NONCONST (-32768)
#define RADSPA_EVENT_MASK (0b11111)
#define RADSPA_EVENT_POW (5)

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

inline int16_t radspa_clip(int32_t a){
    if(a > 32767){
         return 32767;
    } else if(a < -32767){
         return -32767;
    }
    return a;
}

inline int16_t radspa_add_sat(int32_t a, int32_t b){ return radspa_clip(a+b); }
inline int32_t radspa_mult_shift(int32_t a, int32_t b){ return radspa_clip((a*b)>>15); }
inline int32_t radspa_gain(int32_t a, int32_t b){ return radspa_clip((a*b)>>12); }

inline int16_t radspa_trigger_start(int16_t velocity, int16_t * hist){
    if(!velocity) velocity = 1;
    if(velocity == -32768) velocity = 1;
    if(velocity < 0) velocity = -velocity;
    (* hist) = ((* hist) > 0) ? -velocity : velocity;
    return * hist;
}

inline int16_t radspa_trigger_stop(int16_t * hist){
    (* hist) = 0;
    return * hist;
}

inline int16_t radspa_trigger_get(int16_t trigger_signal, int16_t * hist){
    if((* hist) == trigger_signal) return 0;
    (* hist) = trigger_signal;
    if(!trigger_signal){
        return  -1;
    } else if(trigger_signal < 0 ){
        return -trigger_signal;
    } else {
        return trigger_signal;
    }
}


inline radspa_signal_t * radspa_signal_get_by_index(radspa_t * plugin, uint16_t signal_index){
    return &(plugin->signals[signal_index]);
}

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

inline void radspa_signal_set_value_noclip(radspa_signal_t * sig, int16_t index, int16_t val){
    if(sig->buffer != NULL){
        sig->buffer[index] = val;
    } else if(!index){
        sig->value = val;
    }
}

inline void radspa_signal_set_value(radspa_signal_t * sig, int16_t index, int32_t val){
    if(sig->buffer != NULL){
        sig->buffer[index] = radspa_clip(val);
    } else if(!index){
        sig->value = radspa_clip(val);
    }
}

inline void radspa_signal_set_value_check_const(radspa_signal_t * sig, int16_t index, int32_t val){
    // disabled for now, causes issues somewhere, no time to track it down
    radspa_signal_set_value(sig, index, val);
    return;

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

inline int16_t radspa_signal_set_value_check_const_result(radspa_signal_t * sig){
    if(sig->buffer != NULL){
        if(sig->buffer[1] == -32768) return sig->buffer[0];
        return RADSPA_SIGNAL_NONCONST;
    }
    return sig->value;
}

inline void radspa_signal_set_const_value(radspa_signal_t * sig, int32_t val){
    if(sig->buffer == NULL){
        sig->value = radspa_clip(val);
    } else {
        sig->buffer[0] = radspa_clip(val);
        sig->buffer[1] = -32768;
    }
}

inline void radspa_signal_set_values(radspa_signal_t * sig, uint16_t start, uint16_t stop, int32_t val){
    if(sig->buffer == NULL){
        if((!start) && stop) sig->value = radspa_clip(val);
    } else {
        val = radspa_clip(val);
        for(uint16_t i = start; i < stop; i++){
            sig->buffer[i] = val;
        }
    }
}

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

inline int16_t radspa_trigger_get_const(radspa_signal_t * sig, int16_t * hist, uint16_t * index, uint16_t num_samples, uint32_t render_pass_id){
    (* index) = 0;
    int16_t ret_const = radspa_signal_get_const_value(sig, render_pass_id);
    if(ret_const != RADSPA_SIGNAL_NONCONST) return radspa_trigger_get(ret_const, hist);
    int16_t ret = 0;
    for(uint16_t i = 0; i< num_samples; i++){
        int16_t tmp = radspa_trigger_get(radspa_signal_get_value(sig, i, render_pass_id), hist);
        if(tmp){
            ret = tmp;
            (* index) = i;
        }
    }
    return ret;
}

