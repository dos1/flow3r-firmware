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

STATIC mp_obj_t mp_set_global_volume_dB(size_t n_args, const mp_obj_t *args) {
    mp_float_t x = mp_obj_get_float(args[0]);
    int8_t d = x;
    set_global_vol_dB(d);
    mp_float_t l = x;
    return mp_obj_new_float(l);
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
    return mp_obj_new_int(d);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_dump_all_sources_obj, 0, 2, mp_dump_all_sources);

STATIC const mp_rom_map_elem_t mp_module_badge_audio_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_badge_audio) },
    { MP_ROM_QSTR(MP_QSTR_set_global_volume_dB), MP_ROM_PTR(&mp_set_global_volume_dB_obj) },
    { MP_ROM_QSTR(MP_QSTR_count_sources), MP_ROM_PTR(&mp_count_sources_obj) },
    { MP_ROM_QSTR(MP_QSTR_dump_all_sources), MP_ROM_PTR(&mp_dump_all_sources_obj) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_badge_audio_globals, mp_module_badge_audio_globals_table);

const mp_obj_module_t mp_module_badge_audio = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_badge_audio_globals,
};

MP_REGISTER_MODULE(MP_QSTR_badge_audio, mp_module_badge_audio);

