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

#include "badge23/audio.h"
#include "badge23/leds.h"
#include "badge23/captouch.h"
#include "badge23/display.h"
#include "badge23/spio.h"
#include "badge23/espan.h"
#include "badge23_hwconfig.h"

mp_obj_t mp_ctx_from_ctx(Ctx *ctx);

STATIC mp_obj_t mp_init_done(void) {
    return mp_obj_new_int(hardware_is_initialized());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_init_done_obj, mp_init_done);

STATIC mp_obj_t mp_captouch_calibration_active(void) {
    return mp_obj_new_int(captouch_calibration_active());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_captouch_calibration_active_obj, mp_captouch_calibration_active);

STATIC mp_obj_t mp_display_update(void) {
    display_update();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_display_update_obj, mp_display_update);

STATIC mp_obj_t mp_get_captouch(size_t n_args, const mp_obj_t *args) {
    uint16_t captouch = read_captouch();
    uint16_t pad = mp_obj_get_int(args[0]);
    uint8_t output = (captouch >> pad) & 1;

    return mp_obj_new_int(output);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_get_captouch_obj, 1, 2, mp_get_captouch);

STATIC mp_obj_t mp_captouch_get_petal_pad_raw(mp_obj_t petal_in, mp_obj_t pad_in) {
    uint8_t petal = mp_obj_get_int(petal_in);
    uint8_t pad = mp_obj_get_int(pad_in);
    uint16_t output = captouch_get_petal_pad_raw(petal, pad);

    return mp_obj_new_int(output);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mp_captouch_get_petal_pad_raw_obj, mp_captouch_get_petal_pad_raw);

STATIC mp_obj_t mp_captouch_get_petal_pad(mp_obj_t petal_in, mp_obj_t pad_in) {
    uint8_t petal = mp_obj_get_int(petal_in);
    uint8_t pad = mp_obj_get_int(pad_in);
    return mp_obj_new_int(captouch_get_petal_pad(petal, pad));

}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mp_captouch_get_petal_pad_obj, mp_captouch_get_petal_pad);

STATIC mp_obj_t mp_captouch_get_petal_rad(mp_obj_t petal_in) {
    uint8_t petal = mp_obj_get_int(petal_in);
    int32_t ret = captouch_get_petal_rad(petal);

    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_captouch_get_petal_rad_obj, mp_captouch_get_petal_rad);

STATIC mp_obj_t mp_captouch_get_petal_phi(mp_obj_t petal_in) {
    uint8_t petal = mp_obj_get_int(petal_in);
    int32_t ret = captouch_get_petal_phi(petal);

    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_captouch_get_petal_phi_obj, mp_captouch_get_petal_phi);

STATIC mp_obj_t mp_captouch_set_petal_pad_threshold(mp_obj_t petal_in, mp_obj_t pad_in, mp_obj_t thres_in) {
    uint8_t petal = mp_obj_get_int(petal_in);
    uint8_t pad = mp_obj_get_int(pad_in);
    uint16_t thres = mp_obj_get_int(thres_in);
    captouch_set_petal_pad_threshold(petal, pad, thres);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(mp_captouch_set_petal_pad_threshold_obj, mp_captouch_set_petal_pad_threshold);

STATIC mp_obj_t mp_captouch_autocalib(void) {
    captouch_force_calibration();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_captouch_autocalib_obj, mp_captouch_autocalib);

STATIC mp_obj_t mp_captouch_set_calibration_afe_target(mp_obj_t target_in) {
    uint16_t target = mp_obj_get_int(target_in);
    captouch_set_calibration_afe_target(target);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_captouch_set_calibration_afe_target_obj, mp_captouch_set_calibration_afe_target);


STATIC mp_obj_t mp_menu_button_set_left(mp_obj_t left) {
    spio_menu_button_set_left(mp_obj_get_int(left));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_menu_button_set_left_obj, mp_menu_button_set_left);

STATIC mp_obj_t mp_menu_button_get_left() {
    return mp_obj_new_int(spio_menu_button_get_left());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_menu_button_get_left_obj, mp_menu_button_get_left);

STATIC mp_obj_t mp_menu_button_get() {
    return mp_obj_new_int(spio_menu_button_get());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_menu_button_get_obj, mp_menu_button_get);

STATIC mp_obj_t mp_application_button_get() {
    return mp_obj_new_int(spio_application_button_get());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_application_button_get_obj, mp_application_button_get);

STATIC mp_obj_t mp_left_button_get() {
    return mp_obj_new_int(spio_left_button_get());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_left_button_get_obj, mp_left_button_get);

STATIC mp_obj_t mp_right_button_get() {
    return mp_obj_new_int(spio_right_button_get());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_right_button_get_obj, mp_right_button_get);



STATIC mp_obj_t mp_set_global_volume_dB(size_t n_args, const mp_obj_t *args) {
    //TODO: DEPRECATE
    mp_float_t d = mp_obj_get_float(args[0]);
    audio_set_volume_dB(d);
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

STATIC mp_obj_t mp_version(void) {
    mp_obj_t str = mp_obj_new_str(badge23_hw_name, strlen(badge23_hw_name));
    return str;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_version_obj, mp_version);

STATIC mp_obj_t mp_get_ctx(size_t n_args, const mp_obj_t *args) {
    // This might be called before the ctx is ready.
    // HACK: this will go away with the new drawing API.
    while (the_ctx == NULL) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    mp_obj_t mp_ctx = mp_ctx_from_ctx(the_ctx);
    return mp_ctx;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_get_ctx_obj, 0, 0, mp_get_ctx);

STATIC const mp_rom_map_elem_t mp_module_hardware_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_badge_audio) },
    { MP_ROM_QSTR(MP_QSTR_init_done), MP_ROM_PTR(&mp_init_done_obj) },

    { MP_ROM_QSTR(MP_QSTR_captouch_calibration_active), MP_ROM_PTR(&mp_captouch_calibration_active_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_captouch), MP_ROM_PTR(&mp_get_captouch_obj) },
    { MP_ROM_QSTR(MP_QSTR_captouch_get_petal_pad_raw), MP_ROM_PTR(&mp_captouch_get_petal_pad_raw_obj) },
    { MP_ROM_QSTR(MP_QSTR_captouch_get_petal_pad), MP_ROM_PTR(&mp_captouch_get_petal_pad_obj) },
    { MP_ROM_QSTR(MP_QSTR_captouch_get_petal_rad), MP_ROM_PTR(&mp_captouch_get_petal_rad_obj) },
    { MP_ROM_QSTR(MP_QSTR_captouch_get_petal_phi), MP_ROM_PTR(&mp_captouch_get_petal_phi_obj) },
    { MP_ROM_QSTR(MP_QSTR_captouch_set_petal_pad_threshold), MP_ROM_PTR(&mp_captouch_set_petal_pad_threshold_obj) },
    { MP_ROM_QSTR(MP_QSTR_captouch_autocalib), MP_ROM_PTR(&mp_captouch_autocalib_obj) },
    { MP_ROM_QSTR(MP_QSTR_captouch_set_calibration_afe_target), MP_ROM_PTR(&mp_captouch_set_calibration_afe_target_obj) },

    { MP_ROM_QSTR(MP_QSTR_menu_button_get), MP_ROM_PTR(&mp_menu_button_get_obj) },
    { MP_ROM_QSTR(MP_QSTR_application_button_get), MP_ROM_PTR(&mp_application_button_get_obj) },
    { MP_ROM_QSTR(MP_QSTR_left_button_get), MP_ROM_PTR(&mp_left_button_get_obj) },
    { MP_ROM_QSTR(MP_QSTR_right_button_get), MP_ROM_PTR(&mp_right_button_get_obj) },
    { MP_ROM_QSTR(MP_QSTR_menu_button_set_left), MP_ROM_PTR(&mp_menu_button_set_left_obj) },
    { MP_ROM_QSTR(MP_QSTR_menu_button_get_left), MP_ROM_PTR(&mp_menu_button_get_left_obj) },

    { MP_ROM_QSTR(MP_QSTR_set_global_volume_dB), MP_ROM_PTR(&mp_set_global_volume_dB_obj) },
    { MP_ROM_QSTR(MP_QSTR_count_sources), MP_ROM_PTR(&mp_count_sources_obj) },
    { MP_ROM_QSTR(MP_QSTR_dump_all_sources), MP_ROM_PTR(&mp_dump_all_sources_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_led_rgb), MP_ROM_PTR(&mp_set_led_rgb_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_led_hsv), MP_ROM_PTR(&mp_set_led_hsv_obj) },
    { MP_ROM_QSTR(MP_QSTR_update_leds), MP_ROM_PTR(&mp_update_leds_obj) },
    { MP_ROM_QSTR(MP_QSTR_display_update), MP_ROM_PTR(&mp_display_update_obj) },
    { MP_ROM_QSTR(MP_QSTR_version), MP_ROM_PTR(&mp_version_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_ctx), MP_ROM_PTR(&mp_get_ctx_obj) },

    { MP_ROM_QSTR(MP_QSTR_BUTTON_PRESSED_LEFT), MP_ROM_INT(BUTTON_PRESSED_LEFT) },
    { MP_ROM_QSTR(MP_QSTR_BUTTON_PRESSED_RIGHT), MP_ROM_INT(BUTTON_PRESSED_RIGHT) },
    { MP_ROM_QSTR(MP_QSTR_BUTTON_PRESSED_DOWN), MP_ROM_INT(BUTTON_PRESSED_DOWN) },
    { MP_ROM_QSTR(MP_QSTR_BUTTON_NOT_PRESSED), MP_ROM_INT(BUTTON_NOT_PRESSED) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_hardware_globals, mp_module_hardware_globals_table);

const mp_obj_module_t mp_module_hardware = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_hardware_globals,
};

MP_REGISTER_MODULE(MP_QSTR_hardware, mp_module_hardware);

