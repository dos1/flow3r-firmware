#include "st3m_colors.h"

#include <math.h>

#define TAU 6.283185307179586
#define NTAU 0.15915494309189535

st3m_rgb_t st3m_hsv_to_rgb(st3m_hsv_t hsv) {
    float r = 0, g = 0, b = 0;

    if (hsv.s == 0) {
        r = hsv.v;
        g = hsv.v;
        b = hsv.v;
    } else {
        int16_t i;
        float f, p, q, t;
        hsv.h *= NTAU * 6;

        i = (int16_t)truncf(hsv.h);
        if (hsv.h < 0) i--;
        f = hsv.h - i;

        p = hsv.v * (1.0 - hsv.s);
        q = hsv.v * (1.0 - (hsv.s * f));
        t = hsv.v * (1.0 - (hsv.s * (1.0 - f)));

        while (i < 0) {
            i += 6 * (1 << 13);
        }
        i = i % 6;

        switch (i) {
            case 0:
                r = hsv.v;
                g = t;
                b = p;
                break;

            case 1:
                r = q;
                g = hsv.v;
                b = p;
                break;

            case 2:
                r = p;
                g = hsv.v;
                b = t;
                break;

            case 3:
                r = p;
                g = q;
                b = hsv.v;
                break;

            case 4:
                r = t;
                g = p;
                b = hsv.v;
                break;

            default:
                r = hsv.v;
                g = p;
                b = q;
                break;
        }
    }

    st3m_rgb_t rgb = {
        .r = r,
        .g = g,
        .b = b,
    };
    return rgb;
}

st3m_hsv_t st3m_rgb_to_hsv(st3m_rgb_t rgb) {
    st3m_hsv_t hsv;
    float min;
    float max;

    min = rgb.r < rgb.g ? (rgb.r < rgb.b ? rgb.r : rgb.b)
                        : (rgb.g < rgb.b ? rgb.g : rgb.b);
    max = rgb.r > rgb.g ? (rgb.r > rgb.b ? rgb.r : rgb.b)
                        : (rgb.g > rgb.b ? rgb.g : rgb.b);

    hsv.v = max;
    if (hsv.v == 0.) {
        hsv.h = 0.;
        hsv.s = 0.;
        return hsv;
    }

    hsv.s = (max - min) / hsv.v;
    if (hsv.s == 0) {
        hsv.h = 0;
        return hsv;
    }

    if ((max == rgb.r) && ((max != rgb.b) || (max == rgb.g))) {
        hsv.h = (TAU / 6) * (rgb.g - rgb.b) / (max - min);
    } else if (max == rgb.g) {
        hsv.h = (TAU / 3) + (TAU / 6) * (rgb.b - rgb.r) / (max - min);
    } else {
        hsv.h = (TAU * 2 / 3) + (TAU / 6) * (rgb.r - rgb.g) / (max - min);
    }

    return hsv;
}
