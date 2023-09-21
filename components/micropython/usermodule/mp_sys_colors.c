#include "st3m_colors.h"

#include "py/builtin.h"
#include "py/runtime.h"

STATIC mp_obj_t mp_hsv_to_rgb(mp_obj_t h, mp_obj_t s, mp_obj_t v) {
    float hsv_f[3] = { mp_obj_get_float(h), mp_obj_get_float(s),
                       mp_obj_get_float(v) };
    // hue gets clipped internally somewhat
    for (uint8_t i = 1; i < 3; i++) {
        if (hsv_f[i] > 1.0) {
            hsv_f[i] = 1.0;
        } else if (hsv_f[i] < 0.0) {
            hsv_f[i] = 0.0;
        }
    }
    st3m_hsv_t hsv = { hsv_f[0], hsv_f[1], hsv_f[2] };
    st3m_rgb_t rgb = st3m_hsv_to_rgb(hsv);

    mp_obj_t items[3] = { mp_obj_new_float(rgb.r), mp_obj_new_float(rgb.g),
                          mp_obj_new_float(rgb.b) };
    return mp_obj_new_tuple(3, items);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(mp_hsv_to_rgb_obj, mp_hsv_to_rgb);

STATIC mp_obj_t mp_rgb_to_hsv(mp_obj_t r, mp_obj_t g, mp_obj_t b) {
    float rgb_f[3] = { mp_obj_get_float(r), mp_obj_get_float(g),
                       mp_obj_get_float(b) };
    for (uint8_t i = 0; i < 3; i++) {
        if (rgb_f[i] > 1.0) {
            rgb_f[i] = 1.0;
        } else if (rgb_f[i] < 0.0) {
            rgb_f[i] = 0.0;
        }
    }
    st3m_rgb_t rgb = { rgb_f[0], rgb_f[1], rgb_f[2] };
    st3m_hsv_t hsv = st3m_rgb_to_hsv(rgb);

    mp_obj_t items[3] = { mp_obj_new_float(hsv.h), mp_obj_new_float(hsv.s),
                          mp_obj_new_float(hsv.v) };
    return mp_obj_new_tuple(3, items);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(mp_rgb_to_hsv_obj, mp_rgb_to_hsv);

STATIC const mp_rom_map_elem_t globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_hsv_to_rgb), MP_ROM_PTR(&mp_hsv_to_rgb_obj) },
    { MP_ROM_QSTR(MP_QSTR_rgb_to_hsv), MP_ROM_PTR(&mp_rgb_to_hsv_obj) },
};

STATIC MP_DEFINE_CONST_DICT(globals, globals_table);

const mp_obj_module_t mp_module_colors_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&globals,
};

MP_REGISTER_MODULE(MP_QSTR_sys_colors, mp_module_colors_user_cmodule);
