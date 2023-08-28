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
#include "st3m_console.h"
#include "st3m_gfx.h"
#include "st3m_io.h"
#include "st3m_scope.h"

#include "mp_uctx.h"

STATIC mp_obj_t mp_get_app(void) {
    return mp_obj_new_int(st3m_io_app_button_get());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_get_app_obj, mp_get_app);

STATIC mp_obj_t mp_get_os(void) {
    return mp_obj_new_int(st3m_io_os_button_get());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_get_os_obj, mp_get_os);

STATIC mp_obj_t mp_configure(mp_obj_t left_in) {
    bool left = mp_obj_is_true(left_in);
    st3m_io_app_button_configure(left);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_configure_obj, mp_configure);

STATIC mp_obj_t mp_app_is_left(void) {
    return mp_obj_new_bool(st3m_io_app_button_is_left());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_app_is_left_obj, mp_app_is_left);

STATIC const mp_rom_map_elem_t mp_module_sys_buttons_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_sys_buttons) },

    { MP_ROM_QSTR(MP_QSTR_get_app), MP_ROM_PTR(&mp_get_app_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_os), MP_ROM_PTR(&mp_get_os_obj) },
    { MP_ROM_QSTR(MP_QSTR_configure), MP_ROM_PTR(&mp_configure_obj) },
    { MP_ROM_QSTR(MP_QSTR_app_is_left), MP_ROM_PTR(&mp_app_is_left_obj) },

    { MP_ROM_QSTR(MP_QSTR_PRESSED_LEFT), MP_ROM_INT(st3m_tripos_left) },
    { MP_ROM_QSTR(MP_QSTR_PRESSED_RIGHT), MP_ROM_INT(st3m_tripos_right) },
    { MP_ROM_QSTR(MP_QSTR_PRESSED_DOWN), MP_ROM_INT(st3m_tripos_mid) },
    { MP_ROM_QSTR(MP_QSTR_NOT_PRESSED), MP_ROM_INT(st3m_tripos_none) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_sys_buttons_globals,
                            mp_module_sys_buttons_globals_table);

const mp_obj_module_t mp_module_sys_buttons = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_sys_buttons_globals,
};

MP_REGISTER_MODULE(MP_QSTR_sys_buttons, mp_module_sys_buttons);
