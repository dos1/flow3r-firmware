#pragma once

#include <stdint.h>

typedef struct {
    // From 0.0 to 1.0.

    float r;
    float g;
    float b;
} st3m_rgb_t;

typedef struct {
    // From 0.0 to 1.0.

    float h;
    float s;
    float v;
} st3m_hsv_t;

// Convert an HSV colour to an RGB colour.
st3m_rgb_t st3m_hsv_to_rgb(st3m_hsv_t hsv);

// Convert an RGB colour to an HSV colour.
st3m_hsv_t st3m_rgb_to_hsv(st3m_rgb_t rgb);
