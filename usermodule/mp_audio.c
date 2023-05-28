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
#include "badge23_hwconfig.h"

/*
uint8_t audio_speaker_is_on();
uint8_t audio_headset_is_connected();
uint8_t audio_headphones_are_connected();
*/

STATIC mp_obj_t mp_speaker_is_on(size_t n_args, const mp_obj_t *args) {
    return mp_obj_new_int(audio_speaker_is_on());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_speaker_is_on_obj, 0, 1, mp_speaker_is_on);

STATIC mp_obj_t mp_headset_is_connected(size_t n_args, const mp_obj_t *args) {
    return mp_obj_new_int(audio_headset_is_connected());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_headset_is_connected_obj, 0, 1, mp_headset_is_connected);

STATIC mp_obj_t mp_headphones_are_connected(size_t n_args, const mp_obj_t *args) {
    return mp_obj_new_int(audio_headphones_are_connected());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_headphones_are_connected_obj, 0, 1, mp_headphones_are_connected);

STATIC const mp_rom_map_elem_t mp_module_audio_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_audio) },
    { MP_ROM_QSTR(MP_QSTR_speaker_is_on), MP_ROM_PTR(&mp_speaker_is_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_headset_is_connected), MP_ROM_PTR(&mp_headset_is_connected_obj) },
    { MP_ROM_QSTR(MP_QSTR_headphones_are_connected), MP_ROM_PTR(&mp_headphones_are_connected_obj) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_audio_globals, mp_module_audio_globals_table);

const mp_obj_module_t mp_module_audio = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_audio_globals,
};

MP_REGISTER_MODULE(MP_QSTR_audio, mp_module_audio);

