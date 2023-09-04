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

STATIC mp_obj_t mp_fps(void) { return mp_obj_new_float(st3m_gfx_fps()); }
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_fps_obj, mp_fps);

STATIC mp_obj_t mp_overlay_clip(size_t n_arge, const mp_obj_t *args) {
    st3m_gfx_overlay_clip(mp_obj_get_int(args[0]), mp_obj_get_int(args[1]),
                          mp_obj_get_int(args[2]), mp_obj_get_int(args[3]));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_overlay_clip_obj, 4, 4,
                                           mp_overlay_clip);

STATIC mp_obj_t mp_set_backlight(mp_obj_t percent_in) {
    uint8_t percent = mp_obj_get_int(percent_in);
    flow3r_bsp_display_set_backlight(percent);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_set_backlight_obj, mp_set_backlight);

STATIC mp_obj_t mp_set_mode(mp_obj_t mode) {
    st3m_gfx_set_mode(mp_obj_get_int(mode));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_set_mode_obj, mp_set_mode);

STATIC mp_obj_t mp_get_mode(void) {
    return mp_obj_new_int(st3m_gfx_get_mode());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_get_mode_obj, mp_get_mode);

STATIC mp_obj_t mp_set_palette(mp_obj_t pal_in) {
    size_t count = mp_obj_get_int(mp_obj_len(pal_in));
    uint8_t *pal = m_malloc(count);
    for (size_t i = 0; i < count; i++) {
        pal[i] = mp_obj_get_int(
            mp_obj_subscr(pal_in, mp_obj_new_int(i), MP_OBJ_SENTINEL));
    }
    st3m_gfx_set_palette(pal, count / 3);
#if MICROPY_MALLOC_USES_ALLOCATED_SIZE
    m_free(pal, count);
#else
    m_free(pal);
#endif
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_set_palette_obj, mp_set_palette);

STATIC mp_obj_t mp_ctx(mp_obj_t mode_in) {
    return mp_ctx_from_ctx(st3m_ctx(mp_obj_get_int(mode_in)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_ctx_obj, mp_ctx);

STATIC mp_obj_t mp_fb(mp_obj_t mode_in) {
    int mode = mp_obj_get_int(mode_in);
    int size = 240 * 240;
    switch (mode) {
        case st3m_gfx_default:
            size *= 2;
            mode = 16;
            break;
        case st3m_gfx_16bpp:
        case st3m_gfx_16bpp_osd:
            size *= 2;
            break;
        case st3m_gfx_24bpp:
            size *= 3;
            break;
        case st3m_gfx_osd:
        case st3m_gfx_32bpp:
        case st3m_gfx_32bpp_osd:
            size *= 4;
            break;
    }
    return mp_obj_new_bytearray_by_ref(size, st3m_gfx_fb(mode));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_fb_obj, mp_fb);

STATIC mp_obj_t mp_update(mp_obj_t ctx_in) {
    mp_ctx_obj_t *self = MP_OBJ_TO_PTR(ctx_in);
    if (self->base.type != &mp_ctx_type) {
        mp_raise_ValueError("not a ctx");
        return mp_const_none;
    }
    st3m_ctx_end_frame(self->ctx);
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
    st3m_gfx_flush(1000);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_pipe_flush_obj, mp_pipe_flush);

STATIC const mp_rom_map_elem_t mp_module_sys_display_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_sys_display) },
    { MP_ROM_QSTR(MP_QSTR_pipe_full), MP_ROM_PTR(&mp_pipe_full_obj) },
    { MP_ROM_QSTR(MP_QSTR_pipe_flush), MP_ROM_PTR(&mp_pipe_flush_obj) },
    { MP_ROM_QSTR(MP_QSTR_update), MP_ROM_PTR(&mp_update_obj) },
    { MP_ROM_QSTR(MP_QSTR_fb), MP_ROM_PTR(&mp_fb_obj) },
    { MP_ROM_QSTR(MP_QSTR_ctx), MP_ROM_PTR(&mp_ctx_obj) },
    { MP_ROM_QSTR(MP_QSTR_fps), MP_ROM_PTR(&mp_fps_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_mode), MP_ROM_PTR(&mp_set_mode_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_mode), MP_ROM_PTR(&mp_get_mode_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_palette), MP_ROM_PTR(&mp_set_palette_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_backlight), MP_ROM_PTR(&mp_set_backlight_obj) },
    { MP_ROM_QSTR(MP_QSTR_overlay_clip), MP_ROM_PTR(&mp_overlay_clip_obj) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_sys_display_globals,
                            mp_module_sys_display_globals_table);

const mp_obj_module_t mp_module_sys_display = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_sys_display_globals,
};

MP_REGISTER_MODULE(MP_QSTR_sys_display, mp_module_sys_display);
