// probably doesn't need all of these idk
#include <stdio.h>
#include <string.h>

#include "extmod/virtpin.h"
#include "machine_rtc.h"
#include "modmachine.h"
#include "mphalport.h"
#include "py/builtin.h"
#include "py/mphal.h"
#include "py/runtime.h"

#include "flow3r_bsp.h"
#include "st3m_mode.h"

STATIC mp_obj_t mp_mode_set(mp_obj_t kind) {
    // this does not support message at the time
    st3m_mode_set(mp_obj_get_int(kind), NULL);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_mode_set_obj, mp_mode_set);

STATIC const mp_rom_map_elem_t mp_module_sys_mode_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_sys_mode) },

    { MP_ROM_QSTR(MP_QSTR_mode_set), MP_ROM_PTR(&mp_mode_set_obj) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_sys_mode_globals,
                            mp_module_sys_mode_globals_table);

const mp_obj_module_t mp_module_sys_mode = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_sys_mode_globals,
};

MP_REGISTER_MODULE(MP_QSTR_sys_mode, mp_module_sys_mode);
