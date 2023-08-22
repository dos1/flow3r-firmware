#include <st3m_media.h>

#include "py/builtin.h"
#include "py/runtime.h"

typedef struct _mp_ctx_obj_t {
    mp_obj_base_t base;
    Ctx *ctx;
    mp_obj_t user_data;
} mp_ctx_obj_t;

STATIC mp_obj_t mp_load(mp_obj_t path) {
    return mp_obj_new_int(st3m_media_load(mp_obj_str_get_str(path)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_load_obj, mp_load);

STATIC mp_obj_t mp_draw(mp_obj_t uctx_mp) {
    mp_ctx_obj_t *uctx = MP_OBJ_TO_PTR(uctx_mp);
    st3m_media_draw(uctx->ctx);
    return 0;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_draw_obj, mp_draw);

STATIC mp_obj_t mp_think(mp_obj_t ms_in) {
    st3m_media_think(mp_obj_get_float(ms_in));
    return 0;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_think_obj, mp_think);

STATIC mp_obj_t mp_stop(void) {
    st3m_media_stop();
    return 0;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_stop_obj, mp_stop);

STATIC const mp_rom_map_elem_t globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_draw), MP_ROM_PTR(&mp_draw_obj) },
    { MP_ROM_QSTR(MP_QSTR_think), MP_ROM_PTR(&mp_think_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&mp_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_load), MP_ROM_PTR(&mp_load_obj) },
};

STATIC MP_DEFINE_CONST_DICT(globals, globals_table);

const mp_obj_module_t mp_module_media = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&globals,
};

MP_REGISTER_MODULE(MP_QSTR_media, mp_module_media);
