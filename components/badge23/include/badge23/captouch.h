#pragma once
#include <stdint.h>

void captouch_init(void);
void captouch_read_cycle(void);
void captouch_print_debug_info(void);
void gpio_event_handler(void * arg);
void manual_captouch_readout(uint8_t top);
void captouch_get_cross(int paddle, int * x, int * y);
uint16_t captouch_get_petal_pad_raw(uint8_t petal, uint8_t pad, uint8_t amb);
void captouch_force_calibration();
uint16_t read_captouch();
