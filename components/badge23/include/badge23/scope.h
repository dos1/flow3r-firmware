#pragma once
#include <stdint.h>

typedef struct {
    int16_t * buffer;
    int16_t buffer_size;
    int16_t write_head_position; // last written value
    volatile uint8_t is_being_read;
} scope_t;

void init_scope(uint16_t size);
void write_to_scope(int16_t value);
void begin_scope_read();
void end_scope_read();
void read_line_from_scope(uint16_t * line, int16_t point);
