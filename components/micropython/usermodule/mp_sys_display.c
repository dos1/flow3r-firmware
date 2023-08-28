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

STATIC mp_obj_t mp_set_overlay_height(mp_obj_t height_in) {
    int height = mp_obj_get_int(height_in);
    st3m_gfx_set_overlay_height(height);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_set_overlay_height_obj,
                                 mp_set_overlay_height);

STATIC mp_obj_t mp_set_backlight(mp_obj_t percent_in) {
    uint8_t percent = mp_obj_get_int(percent_in);
    flow3r_bsp_display_set_backlight(percent);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_set_backlight_obj, mp_set_backlight);

static Ctx *global_ctx = NULL;
STATIC mp_obj_t mp_get_ctx(void) {
    if (!global_ctx) global_ctx = st3m_ctx(0);
    if (global_ctx == NULL) return mp_const_none;
    return mp_ctx_from_ctx(global_ctx);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_get_ctx_obj, mp_get_ctx);

STATIC mp_obj_t mp_get_overlay_ctx(void) {
    return mp_ctx_from_ctx(st3m_overlay_ctx());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_get_overlay_ctx_obj, mp_get_overlay_ctx);

STATIC mp_obj_t mp_update(mp_obj_t ctx_in) {
    mp_ctx_obj_t *self = MP_OBJ_TO_PTR(ctx_in);
    if (self->base.type != &mp_ctx_type) {
        mp_raise_ValueError("not a ctx");
        return mp_const_none;
    }
    if (global_ctx) {
        st3m_ctx_end_frame(self->ctx);
        global_ctx = NULL;
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_update_obj, mp_update);

STATIC mp_obj_t mp_pipe_full(void) {
    if (st3m_gfx_drawctx_pipe_full()) {
        return mp_const_true;
    }
    return mp_const_false;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_pipe_full_obj, mp_pipe_full);

STATIC mp_obj_t mp_pipe_flush(void) {
    st3m_gfx_flush();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_pipe_flush_obj, mp_pipe_flush);

STATIC const mp_rom_map_elem_t mp_module_sys_display_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_sys_display) },
    { MP_ROM_QSTR(MP_QSTR_pipe_full), MP_ROM_PTR(&mp_pipe_full_obj) },
    { MP_ROM_QSTR(MP_QSTR_pipe_flush), MP_ROM_PTR(&mp_pipe_flush_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_backlight), MP_ROM_PTR(&mp_set_backlight_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_overlay_height),
      MP_ROM_PTR(&mp_set_overlay_height_obj) },
    { MP_ROM_QSTR(MP_QSTR_update), MP_ROM_PTR(&mp_update_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_ctx), MP_ROM_PTR(&mp_get_ctx_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_overlay_ctx),
      MP_ROM_PTR(&mp_get_overlay_ctx_obj) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_sys_display_globals,
                            mp_module_sys_display_globals_table);

const mp_obj_module_t mp_module_sys_display = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_sys_display_globals,
};

MP_REGISTER_MODULE(MP_QSTR_sys_display, mp_module_sys_display);
