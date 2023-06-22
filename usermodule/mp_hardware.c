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
#include "badge23/captouch.h"
#include "badge23/spio.h"
#include "badge23/espan.h"

#include "flow3r_bsp.h"
#include "st3m_gfx.h"
#include "st3m_scope.h"

#include "ctx_config.h"
#include "ctx.h"

mp_obj_t mp_ctx_from_ctx(Ctx *ctx);

STATIC mp_obj_t mp_init_done(void) {
    return mp_obj_new_int(hardware_is_initialized());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_init_done_obj, mp_init_done);

STATIC mp_obj_t mp_captouch_calibration_active(void) {
    return mp_obj_new_int(captouch_calibration_active());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_captouch_calibration_active_obj, mp_captouch_calibration_active);

STATIC mp_obj_t mp_display_set_backlight(mp_obj_t percent_in) {
    uint8_t percent = mp_obj_get_int(percent_in);
    flow3r_bsp_display_set_backlight(percent);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_display_set_backlight_obj, mp_display_set_backlight);

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



STATIC mp_obj_t mp_version(void) {
    mp_obj_t str = mp_obj_new_str(flow3r_bsp_hw_name, strlen(flow3r_bsp_hw_name));
    return str;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_version_obj, mp_version);

static st3m_ctx_desc_t *gfx_last_desc = NULL;

STATIC mp_obj_t mp_get_ctx(void) {
    if (gfx_last_desc == NULL) {
        gfx_last_desc = st3m_gfx_drawctx_free_get(0);
        if (gfx_last_desc == NULL) {
            return mp_const_none;
        }
    }
    mp_obj_t mp_ctx = mp_ctx_from_ctx(gfx_last_desc->ctx);
    return mp_ctx;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_get_ctx_obj, mp_get_ctx);

STATIC mp_obj_t mp_freertos_sleep(mp_obj_t ms_in) {
    uint32_t ms = mp_obj_get_int(ms_in);
    MP_THREAD_GIL_EXIT();
    vTaskDelay(ms / portTICK_PERIOD_MS);
    MP_THREAD_GIL_ENTER();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_freertos_sleep_obj, mp_freertos_sleep);

STATIC mp_obj_t mp_display_update(mp_obj_t in_ctx) {
    // TODO(q3k): check in_ctx? Or just drop from API?

    if (gfx_last_desc != NULL) {
        st3m_gfx_drawctx_pipe_put(gfx_last_desc);
	    gfx_last_desc = NULL;
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_display_update_obj, mp_display_update);

STATIC mp_obj_t mp_display_pipe_full(void) {
    if (st3m_gfx_drawctx_pipe_full()) {
        return mp_const_true;
    }
    return mp_const_false;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_display_pipe_full_obj, mp_display_pipe_full);

STATIC mp_obj_t mp_display_pipe_flush(void) {
    st3m_gfx_flush();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_display_pipe_flush_obj, mp_display_pipe_flush);

STATIC mp_obj_t mp_scope_draw(mp_obj_t ctx_in) {
    // TODO(q3k): check in_ctx? Or just drop from API?

    if (gfx_last_desc != NULL) {
        st3m_scope_draw(gfx_last_desc->ctx);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_scope_draw_obj, mp_scope_draw);

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

    { MP_ROM_QSTR(MP_QSTR_display_update), MP_ROM_PTR(&mp_display_update_obj) },
    { MP_ROM_QSTR(MP_QSTR_freertos_sleep), MP_ROM_PTR(&mp_freertos_sleep_obj) },
    { MP_ROM_QSTR(MP_QSTR_display_pipe_full), MP_ROM_PTR(&mp_display_pipe_full_obj) },
    { MP_ROM_QSTR(MP_QSTR_display_pipe_flush), MP_ROM_PTR(&mp_display_pipe_flush_obj) },
    { MP_ROM_QSTR(MP_QSTR_display_set_backlight), MP_ROM_PTR(&mp_display_set_backlight_obj) },
    { MP_ROM_QSTR(MP_QSTR_version), MP_ROM_PTR(&mp_version_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_ctx), MP_ROM_PTR(&mp_get_ctx_obj) },

    { MP_ROM_QSTR(MP_QSTR_BUTTON_PRESSED_LEFT), MP_ROM_INT(BUTTON_PRESSED_LEFT) },
    { MP_ROM_QSTR(MP_QSTR_BUTTON_PRESSED_RIGHT), MP_ROM_INT(BUTTON_PRESSED_RIGHT) },
    { MP_ROM_QSTR(MP_QSTR_BUTTON_PRESSED_DOWN), MP_ROM_INT(BUTTON_PRESSED_DOWN) },
    { MP_ROM_QSTR(MP_QSTR_BUTTON_NOT_PRESSED), MP_ROM_INT(BUTTON_NOT_PRESSED) },

    { MP_ROM_QSTR(MP_QSTR_scope_draw), MP_ROM_PTR(&mp_scope_draw_obj) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_hardware_globals, mp_module_hardware_globals_table);

const mp_obj_module_t mp_module_hardware = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_hardware_globals,
};

MP_REGISTER_MODULE(MP_QSTR_hardware, mp_module_hardware);

