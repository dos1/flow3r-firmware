// probably doesn't need all of these idk
#include <stdio.h>
#include <string.h>

#include "extmod/virtpin.h"
#include "machine_rtc.h"
#include "modmachine.h"
#include "mphalport.h"
#include "py/builtin.h"
#include "py/mphal.h"
#include "py/obj.h"
#include "py/runtime.h"

#include "flow3r_bsp.h"
#include "st3m_scope.h"

STATIC mp_obj_t mp_get_buffer_x(void) {
    int16_t *buf;
    size_t size = st3m_scope_get_buffer_x(&buf);
    if (size) {
        return mp_obj_new_memoryview('h', size, buf);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_get_buffer_x_obj, mp_get_buffer_x);

STATIC const mp_rom_map_elem_t mp_module_sys_scope_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_sys_scope) },

    { MP_ROM_QSTR(MP_QSTR_get_buffer_x), MP_ROM_PTR(&mp_get_buffer_x_obj) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_sys_scope_globals,
                            mp_module_sys_scope_globals_table);

const mp_obj_module_t mp_module_sys_scope = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_sys_scope_globals,
};

MP_REGISTER_MODULE(MP_QSTR_sys_scope, mp_module_sys_scope);
