#pragma once

// GENERAL INFORMATION
//
// Geometry:
//
// The badge has 10 petals, 5 top petals (on the top PCB) and 5 bottom petals
// (on the bottom PCB). Each petal is made up of pads. Top petals have 3 pads,
// bottom petals have 2 pads.
//
// Every pad on a petal has a kind. The kind infidicates the relative position
// of the pad within the petal.
//
//   tip: pad closest to the outside of the badge
//   base: pad closest to the inside of the badge
//   cw: pad going clockwise around the badge
//   ccw: pad going counter-clockwise around the badge
//
// Top petals have base, cw, ccw pads. Bottom petals have tip, base pads.
//
// NOTE: if you have a 'proto3' badge, it has a slightly different top petal
// layout (tip, cw, ccw). This API pretends base == tip in this case.
//
// Petals are numbered. 0 is the top petal above the USB-C jack, increases
// clockwise so that bottom petals are uneven and top petals even.
//
// Processing:
//
// Every time new capacitive touch data is available, a 'raw' value is extracted
// for each pad. This value is then used to calcualte the following information:
//
//  1. Per-pad touch: if the raw value exceeds some threshold, the pad is
//     considered to be touched.
//  2. Per-petal touch: if any of a pad's petals is considered to be touched,
//     the petal is also considered to be touched.
//  3. Per-petal position: petals allow for estimting a polar coordinate of
//     touch. Top petals have two degrees of freedom, bottom petals have a
//     single degree of freedom (distance from center).

// NOTE: keep the enum definitions below in-sync with flow3r_bsp_captouch.h, as
// they are converted by numerical value internally.

#include <stdbool.h>
#include <stdint.h>

// One of the four possible touch points (pads) on a petal. Top petals have
// base/cw/ccw. Bottom petals have base/tip.
typedef enum {
    // Pad away from centre of badge.
    st3m_petal_pad_tip = 0,
    // Pad going counter-clockwise around the badge.
    st3m_petal_pad_ccw = 1,
    // Pad going clockwise around the badge.
    st3m_petal_pad_cw = 2,
    // Pad going towards the centre of the badge.
    st3m_petal_pad_base = 3,
} st3m_petal_pad_kind_t;

// Each petal can be either top or bottom (that is, on the bottom PCB or top
// PCB).
typedef enum {
    // Petal on the top layer. Has base, cw, ccw pads.
    st3m_petal_top = 0,
    // petal on the bottom layer. Has base and tip fields.
    st3m_petal_bottom = 1,
} st3m_petal_kind_t;

// State of capacitive touch for a petal's pad.
typedef struct {
    // Raw data.
    uint16_t raw;
    // Whether the pad is currently being touched. Calculated from ringbuffer
    // data.
    bool pressed;

    bool pressed_prev;
    bool press_event;
    bool press_event_new;
    bool fresh;
    // How strongly the pad is currently being pressed, in arbitrary units.
    uint16_t pressure;
} st3m_petal_pad_state_t;

// State of capacitive touch for a petal.
typedef struct {
    bool press_event_new;
    bool fresh;
    // Is this a top or bottom petal?
    st3m_petal_kind_t kind;

    // Valid if top or bottom.
    st3m_petal_pad_state_t base;
    // Valid if bottom.
    st3m_petal_pad_state_t tip;
    // Valid if top.
    st3m_petal_pad_state_t cw;
    // Valid if top.
    st3m_petal_pad_state_t ccw;

    // Whether the petal is currently being touched. Calculated from individual
    // pad data.
    bool pressed;

    // Whether a press has been seen on the petal since the last clear_*_event
    bool press_event;

    // How strongly the petal is currently being pressed, in arbitrary units.
    uint16_t pressure;

    // Touch position on petal, calculated from pad data.
    //
    // Arbitrary units around (0, 0).
    // TODO(q3k): normalize and document.

    // note moon2: changed these back to int16, no idea what was the motivation
    // here, there wasn't even a division to warrant it.
    int32_t pos_distance;
    int32_t pos_angle;
} st3m_petal_state_t;

typedef struct {
    // Petal 0 is a top petal next to the USB socket. Then, all other petals
    // follow clockwise.
    st3m_petal_state_t petals[10];
} st3m_captouch_state_t;

void st3m_captouch_init(void);
bool st3m_captouch_calibrating(void);
void st3m_captouch_request_calibration(void);
void st3m_captouch_get_all(st3m_captouch_state_t *dest);
void st3m_captouch_get_petal(st3m_petal_state_t *dest, uint8_t petal_ix);
void st3m_captouch_refresh_petal_events(uint8_t petal_ix);
void st3m_captouch_refresh_all_events();
