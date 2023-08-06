// SPDX-License-Identifier: CC0-1.0
#pragma once
#include <stdbool.h>
#include <stdint.h>

#define SAMPLE_RATE 48000

uint16_t bl00mbox_sources_count();
uint16_t bl00mbox_source_add(void* render_data, void* render_function);
void bl00mbox_source_remove(uint16_t index);
void bl00mbox_audio_render(int16_t* rx, int16_t* tx, uint16_t len);

// TEMP
void bl00mbox_player_function(int16_t* rx, int16_t* tx, uint16_t len);

void bl00mbox_init(void);
