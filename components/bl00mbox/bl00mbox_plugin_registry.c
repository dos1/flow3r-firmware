//SPDX-License-Identifier: CC0-1.0
#include "bl00mbox_plugin_registry.h"

bl00mbox_plugin_registry_t * bl00mbox_plugin_registry = NULL;
uint16_t bl00mbox_plugin_registry_len = 0;
bool bl00mbox_plugin_registry_is_initialized = false;

static void plugin_add(radspa_descriptor_t * descriptor){
    if(bl00mbox_plugin_registry_len == 65535){
        printf("too many plugins registered");
        abort();
    }

    // create plugin registry entry
    bl00mbox_plugin_registry_t * p = malloc(sizeof(bl00mbox_plugin_registry_t));
    if(p == NULL){ printf("bl00mbox: no memory for plugin list"); abort(); }
    p->descriptor = descriptor;
    p->next = NULL;

    // go to end of list
    bl00mbox_plugin_registry_t * plast = bl00mbox_plugin_registry;
    if(plast == NULL){
        bl00mbox_plugin_registry = p;
    } else {
        while(plast->next != NULL){ plast = plast->next; }
        plast->next = p;
    }
    bl00mbox_plugin_registry_len++;
}

uint16_t bl00mbox_plugin_registry_get_plugin_num(void){
    return bl00mbox_plugin_registry_len;
}

radspa_descriptor_t * bl00mbox_plugin_registry_get_descriptor_from_id(uint32_t id){
    /// searches plugin registry for first descriptor with given id number
    /// and returns pointer to it. returns NULL if no match is found.
    bl00mbox_plugin_registry_t * p = bl00mbox_plugin_registry;
    while(p != NULL){
        if(p->descriptor->id == id) break;
        p = p->next;
    }
    if(p != NULL) return p->descriptor;
    return NULL;
}

radspa_descriptor_t * bl00mbox_plugin_registry_get_descriptor_from_index(uint32_t index){
    /// returns pointer to descriptor of registry entry at given index.
    /// returns NULL if out of range.
    if(index >= bl00mbox_plugin_registry_len) return NULL;
    bl00mbox_plugin_registry_t * p = bl00mbox_plugin_registry;
    for(uint16_t i = 0; i < index; i++){
        p = p->next;
        if(p == NULL){
            printf("bl00mbox: plugin list length error");
            abort();
        }
    }
    return p->descriptor;
}

radspa_descriptor_t * bl00mbox_plugin_registry_get_id_from_index(uint32_t index){
    /// returns pointer to descriptor of registry entry at given index.
    /// returns NULL if out of range.
    if(index >= bl00mbox_plugin_registry_len) return NULL;
    bl00mbox_plugin_registry_t * p = bl00mbox_plugin_registry;
    for(uint16_t i = 0; i < index; i++){
        p = p->next;
        if(p == NULL){
            printf("bl00mbox: plugin list length error");
            abort();
        }
    }
    return p->descriptor;
}

/* REGISTER PLUGINS HERE!
 * - include .c file to SRCS in bl00mbox/CMakeLists.txt
 * - include .h file directory to INCLUDE_DIRS in bl00mbox/CMakeLists.txt
 * - include .h file below
 * - use plugin_add in bl00mbox_plugin_registry_init as
 *   exemplified below
 *
 * NOTE: the plugin registry linked list is intended to be filled
 * once at boot time. dynamically adding plugins at runtime may or
 * may not work (but will call abort() if no memory is available),
 * removing plugins from the registry at runtime is not intended.
 */

#include "osc_fm.h"
#include "osc.h"
#include "env_adsr.h"
#include "ampliverter.h"
#include "delay.h"
#include "lowpass.h"
#include "filter.h"
#include "sequencer.h"
#include "sampler.h"
#include "flanger.h"
#include "noise.h"
#include "noise_burst.h"
#include "distortion.h"
#include "mixer.h"
#include "multipitch.h"
#include "slew_rate_limiter.h"
#include "range_shifter.h"
#include "poly_squeeze.h"
#include "bl00mbox_line_in.h"

void bl00mbox_plugin_registry_init(void){
    if(bl00mbox_plugin_registry_is_initialized) return;
    plugin_add(&osc_desc);
    plugin_add(&filter_desc);
    plugin_add(&sequencer_desc);
    plugin_add(&sampler_desc);
    plugin_add(&multipitch_desc);
    plugin_add(&bl00mbox_line_in_desc);
    plugin_add(&distortion_desc);
    plugin_add(&mixer_desc);
    plugin_add(&flanger_desc);
    plugin_add(&noise_desc);
    plugin_add(&noise_burst_desc);
    plugin_add(&env_adsr_desc);   
    plugin_add(&delay_desc);   

    plugin_add(&range_shifter_desc);
    plugin_add(&poly_squeeze_desc);
    plugin_add(&slew_rate_limiter_desc);
    plugin_add(&ampliverter_desc);

    plugin_add(&osc_fm_desc);
    plugin_add(&lowpass_desc);   
}
