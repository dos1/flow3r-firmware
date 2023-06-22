#pragma once
#include <stdint.h>
#include <stdbool.h>

#define SAMPLE_RATE 48000

uint16_t bl00mbox_sources_count();
uint16_t bl00mbox_source_add(void * render_data, void * render_function);
void bl00mbox_source_remove(uint16_t index);
void bl00mbox_player_function(int16_t * rx, int16_t * tx, uint16_t len);
