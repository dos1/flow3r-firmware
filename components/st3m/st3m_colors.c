#include "st3m_colors.h"

#include <math.h>

st3m_rgb_t st3m_hsv_to_rgb(st3m_hsv_t hsv) {
    float r = 0, g = 0, b = 0;

    if (hsv.s == 0) {
        r = hsv.v;
        g = hsv.v;
        b = hsv.v;
    } else {
        int i;
        float f, p, q, t;

        if (hsv.h == 360)
            hsv.h = 0;
        else
            hsv.h = hsv.h / 60;

        i = (int)truncf(hsv.h);
        f = hsv.h - i;

        p = hsv.v * (1.0 - hsv.s);
        q = hsv.v * (1.0 - (hsv.s * f));
        t = hsv.v * (1.0 - (hsv.s * (1.0 - f)));

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
        .r = r * 255,
        .g = g * 255,
        .b = b * 255,
    };
    return rgb;
}
