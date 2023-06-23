#pragma once

#include "radspa.h"

typedef struct _bl00mbox_plugin_registry_t{
    radspa_descriptor_t * descriptor;
    struct _bl00mbox_plugin_registry_t * next;
} bl00mbox_plugin_registry_t;

radspa_descriptor_t * bl00mbox_plugin_registry_get_descriptor_from_id(uint32_t id);
radspa_descriptor_t * bl00mbox_plugin_registry_get_descriptor_from_index(uint32_t index);
void bl00mbox_plugin_registry_init(void);
uint16_t bl00mbox_plugin_registry_get_plugin_num(void);

