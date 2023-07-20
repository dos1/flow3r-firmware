#pragma once

// st3m_scope implements an oscilloscope-style music visualizer.
//
// The audio subsystem will continuously send the global mixing output result
// into the oscilloscope. User code can decide when to draw said scope.

#include <stdint.h>

#include "flow3r_bsp.h"
#include "st3m_gfx.h"

typedef struct {
    // Scope buffer size, in samples. Currently always 240 (same as screen
    // width).
    size_t buffer_size;

    // Triple-buffering for lockless exhange between free-running writer and
    // reader. The exchange buffer is swapped to/from by the reader/writer
    // whenever they're done with a whole sample buffer.
    int16_t *write_buffer;
    int16_t *exchange_buffer;
    int16_t *read_buffer;

    // Offset where the write handler should write the next sample.
    uint32_t write_head_position;
    // Previous sample that was attempted to be written. Used for
    // zero-detection.
    int16_t prev_write_attempt;
} st3m_scope_t;

// Initialize global scope. Must be performed before any other access to scope
// is attempted.
//
// If initialization failes (eg. due to lack of memory) an error will be
// printed.
void st3m_scope_init(void);

// Write a sound sample to the scope.
void st3m_scope_write(int16_t value);

// Draw the scope at bounding box -120/-120 +120/+120.
//
// The scope will be drawn as a closable line segment starting at x:-120
// y:sample[0], through x:120 y:sample[239], then going through x:130 y:130,
// x:-130 y:130.
//
// The user is responsible for setting a color and running a fill/stroke
// afterwards.
void st3m_scope_draw(Ctx *ctx);
