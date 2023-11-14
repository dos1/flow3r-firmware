//SPDX-License-Identifier: CC0-1.0
#include "radspa_helpers.h"

extern inline int16_t radspa_signal_get_value(radspa_signal_t * sig, int16_t index, uint32_t render_pass_id);
extern inline int16_t radspa_signal_get_const_value(radspa_signal_t * sig, uint32_t render_pass_id);
extern inline void radspa_signal_set_value(radspa_signal_t * sig, int16_t index, int32_t value);
extern inline void radspa_signal_set_value_check_const(radspa_signal_t * sig, int16_t index, int32_t value);
extern inline void radspa_signal_set_const_value(radspa_signal_t * sig, int32_t value);
extern inline int16_t radspa_clip(int32_t a);
extern inline int16_t radspa_add_sat(int32_t a, int32_t b);
extern inline int32_t radspa_mult_shift(int32_t a, int32_t b);
extern inline int32_t radspa_gain(int32_t a, int32_t b);
extern inline int16_t radspa_trigger_start(int16_t velocity, int16_t * hist);
extern inline int16_t radspa_trigger_stop(int16_t * hist);
extern inline int16_t radspa_trigger_get(int16_t trigger_signal, int16_t * hist);
extern inline radspa_signal_t * radspa_signal_get_by_index(radspa_t * plugin, uint16_t signal_index);

radspa_signal_t * radspa_signal_set(radspa_t * plugin, uint8_t signal_index, char * name, uint32_t hints, int16_t value){
    radspa_signal_t * sig = radspa_signal_get_by_index(plugin, signal_index);
    if(sig == NULL) return NULL;
    sig->name = name;
    sig->hints = hints;
    sig->value = value;
    return sig;
}

void radspa_signal_set_description(radspa_t * plugin, uint8_t signal_index, char * description){
    radspa_signal_t * sig = radspa_signal_get_by_index(plugin, signal_index);
    if(sig == NULL) return;
    sig->description = description;
}

radspa_signal_t * radspa_signal_set_group(radspa_t * plugin, uint8_t group_len, uint8_t step, uint8_t signal_index, char * name,
                                    uint32_t hints, int16_t value){
    for(uint8_t i = 0; i < group_len; i++){
        radspa_signal_t * sig = radspa_signal_get_by_index(plugin, signal_index + i * step);
        if(sig == NULL) return NULL;
        sig->name = name;
        sig->hints = hints;
        sig->value = value;
        sig->name_multiplex = i;
    }
    return radspa_signal_get_by_index(plugin, signal_index);
}

void radspa_signal_set_group_description(radspa_t * plugin, uint8_t group_len, uint8_t step, uint8_t signal_index,
                                    char * description){
    for(uint8_t i = 0; i < group_len; i++){
        radspa_signal_t * sig = radspa_signal_get_by_index(plugin, signal_index + i * step);
        if(sig == NULL) return;
        sig->description = description;
    }
}

static void radspa_signal_init(radspa_signal_t * sig){
    sig->name = "UNINITIALIZED";
    sig->hints = 0;
    sig->unit = "";
    sig->description = "";
    sig->value = 0;
    sig->name_multiplex = -1;
    sig->buffer = NULL;
}

radspa_t * radspa_standard_plugin_create(radspa_descriptor_t * desc, uint8_t num_signals, size_t plugin_data_size, uint32_t plugin_table_size){
    radspa_t * ret = calloc(1, sizeof(radspa_t) + num_signals * sizeof(radspa_signal_t));
    if(ret == NULL) return NULL;
    if(plugin_data_size){
        ret->plugin_data = calloc(1,plugin_data_size);
        if(ret->plugin_data == NULL){
            free(ret);
            return NULL;
        }
    }
    ret->len_signals = num_signals;
    ret->render = NULL;
    ret->descriptor = desc;
    ret->plugin_table_len = plugin_table_size;
    for(uint8_t i = 0; i < num_signals; i++){
        radspa_signal_init(&(ret->signals[i]));
    }

    bool init_failed = false;

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
    if(plugin->plugin_table != NULL) free(plugin->plugin_table);
    if(plugin->plugin_data != NULL) free(plugin->plugin_data);
    free(plugin);
}
