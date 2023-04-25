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
#include "../badge23/audio.h"
#include "../badge23/leds.h"
#include "../badge23/captouch.h"

#include "../badge23/display.h"
STATIC mp_obj_t mp_display_update(size_t n_args, const mp_obj_t *args) {
    display_update();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_display_update_obj, 0, 1, mp_display_update);

STATIC mp_obj_t mp_display_draw_pixel(size_t n_args, const mp_obj_t *args) {
    uint16_t x = mp_obj_get_int(args[0]);
    uint16_t y = mp_obj_get_int(args[1]);
    uint16_t col = mp_obj_get_int(args[2]);
    display_draw_pixel(x, y, col);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_display_draw_pixel_obj, 3, 4, mp_display_draw_pixel);

STATIC mp_obj_t mp_display_get_pixel(size_t n_args, const mp_obj_t *args) {
    uint16_t x = mp_obj_get_int(args[0]);
    uint16_t y = mp_obj_get_int(args[1]);
    return mp_obj_new_int(display_get_pixel(x, y));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_display_get_pixel_obj, 2, 3, mp_display_get_pixel);

STATIC mp_obj_t mp_display_fill(size_t n_args, const mp_obj_t *args) {
    uint16_t col = mp_obj_get_int(args[0]);
    display_fill(col);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_display_fill_obj, 1, 2, mp_display_fill);

STATIC mp_obj_t mp_get_captouch(size_t n_args, const mp_obj_t *args) {
    uint16_t captouch = read_captouch();
    uint16_t pad = mp_obj_get_int(args[0]);
    uint8_t output = (captouch >> pad) & 1;

    return mp_obj_new_int(output);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_get_captouch_obj, 1, 2, mp_get_captouch);

STATIC mp_obj_t mp_captouch_autocalib(size_t n_args, const mp_obj_t *args) {
    captouch_force_calibration();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_captouch_autocalib_obj, 0, 2, mp_captouch_autocalib);

STATIC mp_obj_t mp_set_global_volume_dB(size_t n_args, const mp_obj_t *args) {
    mp_float_t x = mp_obj_get_float(args[0]);
    int8_t d = x;
    set_global_vol_dB(d);
    mp_float_t l = x;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_set_global_volume_dB_obj, 1, 2, mp_set_global_volume_dB);

STATIC mp_obj_t mp_count_sources(size_t n_args, const mp_obj_t *args) {
    uint16_t d = count_audio_sources();
    return mp_obj_new_int(d);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_count_sources_obj, 0, 2, mp_count_sources);

STATIC mp_obj_t mp_dump_all_sources(size_t n_args, const mp_obj_t *args) {
    uint16_t d = count_audio_sources();
    for(uint16_t i = 0; i < d; i++){
        remove_audio_source(i);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_dump_all_sources_obj, 0, 2, mp_dump_all_sources);


STATIC mp_obj_t mp_set_led_rgb(size_t n_args, const mp_obj_t *args) {
    uint8_t index =  mp_obj_get_int(args[0]);
    uint8_t red =  mp_obj_get_int(args[1]);
    uint8_t green =  mp_obj_get_int(args[2]);
    uint8_t blue =  mp_obj_get_int(args[3]);
    leds_set_single_rgb(index, red, green, blue);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_set_led_rgb_obj, 4, 5, mp_set_led_rgb);

STATIC mp_obj_t mp_set_led_hsv(size_t n_args, const mp_obj_t *args) {
    uint8_t index =  mp_obj_get_int(args[0]);
    float hue =  mp_obj_get_float(args[1]);
    float sat =  mp_obj_get_float(args[2]);
    float val =  mp_obj_get_float(args[3]);
    leds_set_single_hsv(index, hue, sat, val);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_set_led_hsv_obj, 4, 5, mp_set_led_hsv);

STATIC mp_obj_t mp_update_leds(size_t n_args, const mp_obj_t *args) {
    leds_update();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_update_leds_obj, 0, 2, mp_update_leds);

STATIC const mp_rom_map_elem_t mp_module_hardware_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_badge_audio) },
    { MP_ROM_QSTR(MP_QSTR_get_captouch), MP_ROM_PTR(&mp_get_captouch_obj) },
    { MP_ROM_QSTR(MP_QSTR_captouch_autocalib), MP_ROM_PTR(&mp_captouch_autocalib_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_global_volume_dB), MP_ROM_PTR(&mp_set_global_volume_dB_obj) },
    { MP_ROM_QSTR(MP_QSTR_count_sources), MP_ROM_PTR(&mp_count_sources_obj) },
    { MP_ROM_QSTR(MP_QSTR_dump_all_sources), MP_ROM_PTR(&mp_dump_all_sources_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_led_rgb), MP_ROM_PTR(&mp_set_led_rgb_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_led_hsv), MP_ROM_PTR(&mp_set_led_hsv_obj) },
    { MP_ROM_QSTR(MP_QSTR_update_leds), MP_ROM_PTR(&mp_update_leds_obj) },
    { MP_ROM_QSTR(MP_QSTR_display_update), MP_ROM_PTR(&mp_display_update_obj) },
    { MP_ROM_QSTR(MP_QSTR_display_draw_pixel), MP_ROM_PTR(&mp_display_draw_pixel_obj) },
    { MP_ROM_QSTR(MP_QSTR_display_get_pixel), MP_ROM_PTR(&mp_display_get_pixel_obj) },
    { MP_ROM_QSTR(MP_QSTR_display_fill), MP_ROM_PTR(&mp_display_fill_obj) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_hardware_globals, mp_module_hardware_globals_table);

const mp_obj_module_t mp_module_hardware = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_hardware_globals,
};

MP_REGISTER_MODULE(MP_QSTR_hardware, mp_module_hardware);

