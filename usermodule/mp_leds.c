// probably doesn't need all of these idk
#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/mphal.h"
#include "mphalport.h"
#include "modmachine.h"
#include "extmod/virtpin.h"
#include "machine_rtc.h"
#include "py/builtin.h"
#include "py/runtime.h"

#include "badge23/leds.h"
#include "badge23/espan.h"
#include "badge23_hwconfig.h"

STATIC mp_obj_t mp_leds_set_brightness(mp_obj_t b) {
    leds_set_brightness(mp_obj_get_int(b));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_leds_set_brightness_obj, mp_leds_set_brightness);

STATIC mp_obj_t mp_leds_get_brightness() {
    return mp_obj_new_int(leds_get_brightness());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_leds_get_brightness_obj, mp_leds_get_brightness);

STATIC mp_obj_t mp_leds_set_auto_update(mp_obj_t on) {
    leds_set_auto_update(mp_obj_get_int(on));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_leds_set_auto_update_obj, mp_leds_set_auto_update);

STATIC mp_obj_t mp_leds_get_auto_update() {
    return mp_obj_new_int(leds_get_auto_update());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_leds_get_auto_update_obj, mp_leds_get_auto_update);

STATIC mp_obj_t mp_leds_set_gamma(mp_obj_t r, mp_obj_t g, mp_obj_t b) {
    leds_set_gamma(mp_obj_get_float(r), mp_obj_get_float(g), mp_obj_get_float(b));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(mp_leds_set_gamma_obj, mp_leds_set_gamma);

STATIC mp_obj_t mp_leds_set_slew_rate(mp_obj_t b) {
    leds_set_slew_rate(mp_obj_get_int(b));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_leds_set_slew_rate_obj, mp_leds_set_slew_rate);

STATIC mp_obj_t mp_leds_get_slew_rate() {
    return mp_obj_new_int(leds_get_slew_rate());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_leds_get_slew_rate_obj, mp_leds_get_slew_rate);

STATIC mp_obj_t mp_led_set_rgb(size_t n_args, const mp_obj_t *args) {
    uint8_t index =  mp_obj_get_int(args[0]);
    uint8_t red =  mp_obj_get_int(args[1]);
    uint8_t green =  mp_obj_get_int(args[2]);
    uint8_t blue =  mp_obj_get_int(args[3]);
    leds_set_single_rgb(index, red, green, blue);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_led_set_rgb_obj, 4, 4, mp_led_set_rgb);

STATIC mp_obj_t mp_led_set_hsv(size_t n_args, const mp_obj_t *args) {
    uint8_t index =  mp_obj_get_int(args[0]);
    float hue =  mp_obj_get_float(args[1]);
    float sat =  mp_obj_get_float(args[2]);
    float val =  mp_obj_get_float(args[3]);
    leds_set_single_hsv(index, hue, sat, val);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_led_set_hsv_obj,4,4, mp_led_set_hsv);

STATIC mp_obj_t mp_led_set_all_rgb(mp_obj_t r, mp_obj_t g, mp_obj_t b) {
    uint8_t red =  mp_obj_get_int(r);
    uint8_t green =  mp_obj_get_int(g);
    uint8_t blue =  mp_obj_get_int(b);
    leds_set_all_rgb(red, green, blue);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(mp_led_set_all_rgb_obj, mp_led_set_all_rgb);

STATIC mp_obj_t mp_led_set_all_hsv(mp_obj_t h, mp_obj_t s, mp_obj_t v) {
    float hue =  mp_obj_get_float(h);
    float sat =  mp_obj_get_float(s);
    float val =  mp_obj_get_float(v);
    leds_set_all_hsv(hue, sat, val);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(mp_led_set_all_hsv_obj, mp_led_set_all_hsv);

STATIC mp_obj_t mp_leds_update() {
    leds_update();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_leds_update_obj, mp_leds_update);

STATIC const mp_rom_map_elem_t mp_module_leds_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_badge_audio) },
    { MP_ROM_QSTR(MP_QSTR_set_rgb), MP_ROM_PTR(&mp_led_set_rgb_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_hsv), MP_ROM_PTR(&mp_led_set_hsv_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_all_rgb), MP_ROM_PTR(&mp_led_set_all_rgb_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_all_hsv), MP_ROM_PTR(&mp_led_set_all_hsv_obj) },
    { MP_ROM_QSTR(MP_QSTR_update), MP_ROM_PTR(&mp_leds_update_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_brightness), MP_ROM_PTR(&mp_leds_get_brightness_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_brightness), MP_ROM_PTR(&mp_leds_set_brightness_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_auto_update), MP_ROM_PTR(&mp_leds_get_auto_update_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_auto_update), MP_ROM_PTR(&mp_leds_set_auto_update_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_gamma), MP_ROM_PTR(&mp_leds_set_gamma_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_slew_rate), MP_ROM_PTR(&mp_leds_get_slew_rate_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_slew_rate), MP_ROM_PTR(&mp_leds_set_slew_rate_obj) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_leds_globals, mp_module_leds_globals_table);

const mp_obj_module_t mp_module_leds = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_leds_globals,
};

MP_REGISTER_MODULE(MP_QSTR_leds, mp_module_leds);

