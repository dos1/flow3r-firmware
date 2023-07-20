#pragma once

#include <stdint.h>

typedef struct {
    // From 0 to 255.

    uint8_t r;
    uint8_t g;
    uint8_t b;
} st3m_rgb_t;

typedef struct {
    // From 0.0 to 1.0.

    double h;
    double s;
    double v;
} st3m_hsv_t;

// Convert an HSV colour to an RGB colour.
st3m_rgb_t st3m_hsv_to_rgb(st3m_hsv_t hsv);
