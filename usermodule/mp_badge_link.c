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
#include "badge23/leds.h"
#include "badge23/captouch.h"
#include "badge23/display.h"
#include "badge23/spio.h"
#include "badge23/espan.h"
#include "badge23_hwconfig.h"

mp_obj_t mp_ctx_from_ctx(Ctx *ctx);

STATIC mp_obj_t mp_get_active(mp_obj_t pin_mask) {
    return mp_obj_new_int(spio_badge_link_get_active(mp_obj_get_int(pin_mask)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_get_active_obj, mp_get_active);

STATIC mp_obj_t mp_enable(mp_obj_t pin_mask) {
    return mp_obj_new_int(spio_badge_link_enable(mp_obj_get_int(pin_mask)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_enable_obj, mp_enable);

STATIC mp_obj_t mp_disable(mp_obj_t pin_mask) {
    return mp_obj_new_int(spio_badge_link_disable(mp_obj_get_int(pin_mask)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_disable_obj, mp_disable);

STATIC const mp_rom_map_elem_t mp_module_badge_link_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_badge_link) },
    { MP_ROM_QSTR(MP_QSTR_get_active), MP_ROM_PTR(&mp_get_active_obj) },
    { MP_ROM_QSTR(MP_QSTR_enable), MP_ROM_PTR(&mp_enable_obj) },
    { MP_ROM_QSTR(MP_QSTR_disable), MP_ROM_PTR(&mp_disable_obj) },

    { MP_ROM_QSTR(MP_QSTR_PIN_MASK_LINE_IN_TIP), MP_ROM_INT(BADGE_LINK_PIN_MASK_LINE_IN_TIP) },
    { MP_ROM_QSTR(MP_QSTR_PIN_MASK_LINE_IN_RING), MP_ROM_INT(BADGE_LINK_PIN_MASK_LINE_IN_RING) },
    { MP_ROM_QSTR(MP_QSTR_PIN_MASK_LINE_OUT_TIP), MP_ROM_INT(BADGE_LINK_PIN_MASK_LINE_OUT_TIP) },
    { MP_ROM_QSTR(MP_QSTR_PIN_MASK_LINE_OUT_RING), MP_ROM_INT(BADGE_LINK_PIN_MASK_LINE_OUT_RING) },
    { MP_ROM_QSTR(MP_QSTR_PIN_MASK_LINE_IN), MP_ROM_INT(BADGE_LINK_PIN_MASK_LINE_IN) },
    { MP_ROM_QSTR(MP_QSTR_PIN_MASK_LINE_OUT), MP_ROM_INT(BADGE_LINK_PIN_MASK_LINE_OUT) },
    { MP_ROM_QSTR(MP_QSTR_PIN_MASK_ALL), MP_ROM_INT(BADGE_LINK_PIN_MASK_ALL) },
    { MP_ROM_QSTR(MP_QSTR_PIN_INDEX_LINE_IN_TIP), MP_ROM_INT(BADGE_LINK_PIN_INDEX_LINE_IN_TIP) },
    { MP_ROM_QSTR(MP_QSTR_PIN_INDEX_LINE_IN_RING), MP_ROM_INT(BADGE_LINK_PIN_INDEX_LINE_IN_RING) },
    { MP_ROM_QSTR(MP_QSTR_PIN_INDEX_LINE_OUT_TIP), MP_ROM_INT(BADGE_LINK_PIN_INDEX_LINE_OUT_TIP) },
    { MP_ROM_QSTR(MP_QSTR_PIN_INDEX_LINE_OUT_RING), MP_ROM_INT(BADGE_LINK_PIN_INDEX_LINE_OUT_RING) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_badge_link_globals, mp_module_badge_link_globals_table);

const mp_obj_module_t mp_module_badge_link = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_badge_link_globals,
};

MP_REGISTER_MODULE(MP_QSTR_badge_link, mp_module_badge_link);

