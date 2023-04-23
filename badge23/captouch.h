#pragma once
#include <stdint.h>

void captouch_init(void);
void captouch_print_debug_info(void);
void gpio_event_handler(void * arg);
void manual_captouch_readout(uint8_t top);
void captouch_get_cross(int paddle, int * x, int * y);
