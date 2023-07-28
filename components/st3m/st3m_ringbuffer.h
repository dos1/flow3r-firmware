#pragma once

// A simple, non-concurrent ringbuffer.
//
// TODO(q3k): make generic and use from scope code

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    size_t write_ix;
    bool wrapped;
    uint16_t buf[4];
} st3m_ringbuffer_t;

// Size of ringbuffer, in elements.
inline size_t ringbuffer_size(const st3m_ringbuffer_t *rb) {
    return sizeof(rb->buf) / sizeof(uint16_t);
}

// Write to ringbuffer.
void ringbuffer_write(st3m_ringbuffer_t *rb, uint16_t data);

// Get ringbuffer average (mean), or 0 if no values have yet been inserted.
uint16_t ringbuffer_avg(const st3m_ringbuffer_t *rb);

// Get last inserted value, or 0 if no value have yet been inserted.
uint16_t ringbuffer_last(const st3m_ringbuffer_t *rb);