//SPDX-License-Identifier: CC0-1.0
#include "bl00mbox.h"
#include "bl00mbox_plugin_registry.h"
#include "bl00mbox_audio.h"

void bl00mbox_init(){
    bl00mbox_plugin_registry_init();
    bl00mbox_channels_init();
}
