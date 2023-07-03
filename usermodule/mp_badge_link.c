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

#include "st3m_io.h"
#include "flow3r_bsp.h"

// Badgelink API.
//
// See mypystubs/badge_link.pyi for more information.

typedef struct _badgelink_jack_t {
    mp_obj_base_t base;
    bool left;

    // Lazy initialized on first access.
    mp_obj_t pin_tip;
    mp_obj_t pin_ring;
} badgelink_jack_t;

const mp_obj_type_t badgelink_jack_type;

typedef struct _badgelink_jack_pin_t {
    mp_obj_base_t base;

    bool left;
    bool tip;

    mp_obj_t pin;
} badgelink_jack_pin_t;

const mp_obj_type_t badgelink_jack_pin_type;

STATIC badgelink_jack_t left = {
    .base = {&badgelink_jack_type},
    .left = true,

    .pin_tip = mp_const_none,
    .pin_ring = mp_const_none,
};
STATIC badgelink_jack_t right = {
    .base = {&badgelink_jack_type},
    .left = false,

    .pin_tip = mp_const_none,
    .pin_ring = mp_const_none,
};

STATIC void badgelink_jack_pin_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    badgelink_jack_pin_t *self = MP_OBJ_TO_PTR(self_in);

    if (self->left) {
        if (self->tip)
            mp_printf(print, "JackPin(left tip)");
        else
            mp_printf(print, "JackPin(left ring)");
    } else {
        if (self->tip)
            mp_printf(print, "JackPin(right tip)");
        else
            mp_printf(print, "JackPin(right ring)");
    }
}

STATIC void badgelink_jack_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    badgelink_jack_t *self = MP_OBJ_TO_PTR(self_in);

    if (self->left)
        mp_printf(print, "Jack(left)");
    else
        mp_printf(print, "Jack(right)");
}

// From machine_pin.c. Used to make a machine.Pin below.
mp_obj_t mp_pin_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);

STATIC mp_obj_t badgelink_jack_pin_make_new(bool left, bool tip) {
    badgelink_jack_pin_t *self = m_new_obj(badgelink_jack_pin_t);
    self->base.type = &badgelink_jack_pin_type;
    self->left = left;
    self->tip = tip;

    uint32_t unum = left
        ? (tip
            ? flow3r_bsp_spio_programmable_pins.badgelink_left_tip
            : flow3r_bsp_spio_programmable_pins.badgelink_left_ring)
        : (tip
            ? flow3r_bsp_spio_programmable_pins.badgelink_right_tip
            : flow3r_bsp_spio_programmable_pins.badgelink_right_ring);
    mp_obj_t num = mp_obj_new_int_from_uint(unum);
    self->pin = mp_pin_make_new(NULL, 1, 0, &num);

    return MP_OBJ_FROM_PTR(self);
}

STATIC void badgelink_jack_pin_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    badgelink_jack_pin_t *self = MP_OBJ_TO_PTR(self_in);
    if (dest[0] != MP_OBJ_NULL) {
        return;
    }
    switch (attr) {
    case MP_QSTR_pin: dest[0] = self->pin; break;
    default:
        dest[1] = MP_OBJ_SENTINEL;
    }
}

STATIC void badgelink_jack_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    badgelink_jack_t *self = MP_OBJ_TO_PTR(self_in);
    if (dest[0] != MP_OBJ_NULL) {
        return;
    }
    if (self->pin_tip == mp_const_none) {
        self->pin_tip = badgelink_jack_pin_make_new(self->left, true);
    }
    if (self->pin_ring == mp_const_none) {
        self->pin_ring = badgelink_jack_pin_make_new(self->left, false);
    }
    switch (attr) {
    case MP_QSTR_tip: dest[0] = self->pin_tip; break;
    case MP_QSTR_ring: dest[0] = self->pin_ring; break;
    default:
        dest[1] = MP_OBJ_SENTINEL;
    }
}

STATIC uint8_t pin_mask_for_jack(mp_obj_t self_in) {
    const mp_obj_type_t *ty = mp_obj_get_type(self_in);

    uint8_t pin_mask = 0;
	bool left = false;
    bool tip = false;
    bool ring = false;
    if (ty == &badgelink_jack_pin_type) {
        badgelink_jack_pin_t *self_jack_pin = MP_OBJ_TO_PTR(self_in);
        left = self_jack_pin->left;
        tip = self_jack_pin->tip;
        ring = !tip;
    } else {
        badgelink_jack_t *self_jack = MP_OBJ_TO_PTR(self_in);
        left = self_jack->left;
        tip = true;
        ring = true;
    }

    if (left && tip) pin_mask |= BADGE_LINK_PIN_MASK_LINE_OUT_TIP;
    if (left && ring) pin_mask |= BADGE_LINK_PIN_MASK_LINE_OUT_RING;
    if (!left && tip) pin_mask |= BADGE_LINK_PIN_MASK_LINE_IN_TIP;
    if (!left && ring) pin_mask |= BADGE_LINK_PIN_MASK_LINE_IN_RING;
    return pin_mask;
}

// Shared between Jack and JackPin.
STATIC mp_obj_t badgelink_jack_active(mp_obj_t self_in) {
    uint8_t pin_mask = pin_mask_for_jack(self_in);
    uint8_t val = st3m_io_badge_link_get_active(pin_mask);
    return mp_obj_new_bool(val == pin_mask);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(badgelink_jack_active_obj, badgelink_jack_active);

// Shared between Jack and JackPin.
STATIC mp_obj_t badgelink_jack_enable(mp_obj_t self_in) {
    uint8_t pin_mask = pin_mask_for_jack(self_in);
    uint8_t val = st3m_io_badge_link_enable(pin_mask);
    return mp_obj_new_bool(val == pin_mask);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(badgelink_jack_enable_obj, badgelink_jack_enable);

// Shared between Jack and JackPin.
STATIC mp_obj_t badgelink_jack_disable(mp_obj_t self_in) {
    uint8_t pin_mask = pin_mask_for_jack(self_in);
    st3m_io_badge_link_disable(pin_mask);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(badgelink_jack_disable_obj, badgelink_jack_disable);

// Shared between Jack and JackPin.
STATIC const mp_rom_map_elem_t badgelink_jack_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_enable), MP_ROM_PTR(&badgelink_jack_enable_obj) },
    { MP_ROM_QSTR(MP_QSTR_disable), MP_ROM_PTR(&badgelink_jack_disable_obj) },
    { MP_ROM_QSTR(MP_QSTR_active), MP_ROM_PTR(&badgelink_jack_active_obj) },
};

STATIC MP_DEFINE_CONST_DICT(badgelink_jack_locals_dict, badgelink_jack_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    badgelink_jack_pin_type,
    MP_QSTR_JackPin,
    MP_TYPE_FLAG_NONE,
    print, badgelink_jack_pin_print,
    attr, badgelink_jack_pin_attr,
    locals_dict, &badgelink_jack_locals_dict
);

MP_DEFINE_CONST_OBJ_TYPE(
    badgelink_jack_type,
    MP_QSTR_Jack,
    MP_TYPE_FLAG_NONE,
    print, badgelink_jack_print,
    attr, badgelink_jack_attr,
    locals_dict, &badgelink_jack_locals_dict
);

STATIC const mp_rom_map_elem_t mp_module_badge_link_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_badge_link) },
    { MP_ROM_QSTR(MP_QSTR_left), MP_ROM_PTR(&left) },
    { MP_ROM_QSTR(MP_QSTR_right), MP_ROM_PTR(&right) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_badge_link_globals, mp_module_badge_link_globals_table);

const mp_obj_module_t mp_module_badge_link = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_badge_link_globals,
};

MP_REGISTER_MODULE(MP_QSTR_badge_link, mp_module_badge_link);

