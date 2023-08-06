// SPDX-License-Identifier: CC0-1.0
#include "bl00mbox.h"
#include "bl00mbox_audio.h"
#include "bl00mbox_plugin_registry.h"
#include "st3m_audio.h"

void bl00mbox_init() {
    bl00mbox_plugin_registry_init();
    st3m_audio_set_player_function(bl00mbox_audio_render);
    bl00mbox_channels_init();
}
