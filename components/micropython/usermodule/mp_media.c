#include <st3m_media.h>

#include "py/builtin.h"
#include "py/runtime.h"

typedef struct _mp_ctx_obj_t {
    mp_obj_base_t base;
    Ctx *ctx;
    mp_obj_t user_data;
} mp_ctx_obj_t;

STATIC mp_obj_t mp_load(size_t n_args, const mp_obj_t *args) {
    bool paused = false;
    if (n_args > 1) paused = args[1] == mp_const_true;
    return mp_obj_new_int(st3m_media_load(mp_obj_str_get_str(args[0]), paused));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_load_obj, 1, 2, mp_load);

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

STATIC mp_obj_t mp_pause(void) {
    st3m_media_pause();
    return 0;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_pause_obj, mp_pause);

STATIC mp_obj_t mp_play(void) {
    st3m_media_play();
    return 0;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_play_obj, mp_play);

STATIC mp_obj_t mp_is_playing(void) {
    return mp_obj_new_bool(st3m_media_is_playing());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_is_playing_obj, mp_is_playing);

STATIC mp_obj_t mp_get_duration(void) {
    return mp_obj_new_float(st3m_media_get_duration());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_get_duration_obj, mp_get_duration);

STATIC mp_obj_t mp_get_position(void) {
    return mp_obj_new_float(st3m_media_get_position());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_get_position_obj, mp_get_position);

STATIC mp_obj_t mp_get_time(void) {
    return mp_obj_new_float(st3m_media_get_time());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_get_time_obj, mp_get_time);

STATIC mp_obj_t mp_seek(mp_obj_t position) {
    st3m_media_seek(mp_obj_get_float(position));
    return 0;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_seek_obj, mp_seek);

STATIC mp_obj_t mp_seek_relative(mp_obj_t time) {
    st3m_media_seek_relative(mp_obj_get_float(time));
    return 0;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_seek_relative_obj, mp_seek_relative);

STATIC mp_obj_t mp_set_volume(mp_obj_t volume) {
    st3m_media_set_volume(mp_obj_get_float(volume));
    return 0;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_set_volume_obj, mp_set_volume);

STATIC mp_obj_t mp_get_volume(void) {
    return mp_obj_new_float(st3m_media_get_volume());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_get_volume_obj, mp_get_volume);

STATIC mp_obj_t mp_set(mp_obj_t key, mp_obj_t value) {
    st3m_media_set(mp_obj_str_get_str(key), mp_obj_get_float(value));
    return 0;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mp_set_obj, mp_set);

STATIC mp_obj_t mp_get(mp_obj_t key) {
    return mp_obj_new_float(st3m_media_get(mp_obj_str_get_str(key)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_get_obj, mp_get);

STATIC mp_obj_t mp_get_string(mp_obj_t key) {
    char *str = st3m_media_get_string(mp_obj_str_get_str(key));
    if (!str) return 0;
    return mp_obj_new_str(str, strlen(str));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_get_string_obj, mp_get_string);

STATIC const mp_rom_map_elem_t globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_draw), MP_ROM_PTR(&mp_draw_obj) },
    { MP_ROM_QSTR(MP_QSTR_think), MP_ROM_PTR(&mp_think_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&mp_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_load), MP_ROM_PTR(&mp_load_obj) },
    { MP_ROM_QSTR(MP_QSTR_pause), MP_ROM_PTR(&mp_pause_obj) },
    { MP_ROM_QSTR(MP_QSTR_play), MP_ROM_PTR(&mp_play_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_playing), MP_ROM_PTR(&mp_is_playing_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_duration), MP_ROM_PTR(&mp_get_duration_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_position), MP_ROM_PTR(&mp_get_position_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_time), MP_ROM_PTR(&mp_get_time_obj) },
    { MP_ROM_QSTR(MP_QSTR_seek), MP_ROM_PTR(&mp_seek_obj) },
    { MP_ROM_QSTR(MP_QSTR_seek_relative), MP_ROM_PTR(&mp_seek_relative_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_volume), MP_ROM_PTR(&mp_set_volume_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_volume), MP_ROM_PTR(&mp_get_volume_obj) },
    { MP_ROM_QSTR(MP_QSTR_set), MP_ROM_PTR(&mp_set_obj) },
    { MP_ROM_QSTR(MP_QSTR_get), MP_ROM_PTR(&mp_get_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_string), MP_ROM_PTR(&mp_get_string_obj) },
};

STATIC MP_DEFINE_CONST_DICT(globals, globals_table);

const mp_obj_module_t mp_module_media = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&globals,
};

MP_REGISTER_MODULE(MP_QSTR_media, mp_module_media);
