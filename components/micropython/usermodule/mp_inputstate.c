#include "py/builtin.h"
#include "py/runtime.h"

#include "st3m_captouch.h"
#include "st3m_inputstate.h"

#include <string.h>

st3m_inputstate_t * ins = st3m_inputstate_get_ins();

typedef struct {
    mp_obj_base_t base;
    mp_obj_t petal;
} mp_captouch_petal_pads_state_t;

const mp_obj_type_t captouch_petal_pads_state_type;

typedef struct {
    mp_obj_base_t base;
    mp_obj_t captouch;
    mp_obj_t pads;
    size_t ix;
} mp_captouch_petal_state_t;

const mp_obj_type_t captouch_petal_state_type;

typedef struct {
    mp_obj_base_t base;
    mp_obj_t petals;
    st3m_captouch_state_t underlying;
} mp_captouch_state_t;

const mp_obj_type_t captouch_state_type;

typedef struct {
    mp_obj_base_t base;
    mp_obj_t inputstate;
} mp_inputstate_imu_t;

const mp_obj_type_t inputstate_imu_type;

typedef struct {
    mp_obj_base_t base;
    mp_obj_t imu;
    mp_obj_t captouch;
    mp_obj_t buttons;
} mp_inputstate_t;

const mp_obj_type_t inputstate_type;

STATIC void mp_captouch_petal_pads_state_attr(mp_obj_t self_in, qstr attr,
                                              mp_obj_t *dest) {
    mp_captouch_petal_pads_state_t *self = MP_OBJ_TO_PTR(self_in);
    if (dest[0] != MP_OBJ_NULL) {
        return;
    }

    mp_captouch_petal_state_t *petal = MP_OBJ_TO_PTR(self->petal);
    mp_captouch_state_t *captouch = MP_OBJ_TO_PTR(petal->captouch);
    st3m_petal_state_t *state = &captouch->underlying.petals[petal->ix];
    bool top = (petal->ix % 2) == 0;

    if (top) {
        switch (attr) {
            case MP_QSTR_base:
                dest[0] = mp_obj_new_bool(state->base.pressed);
                break;
            case MP_QSTR_cw:
                dest[0] = mp_obj_new_bool(state->cw.pressed);
                break;
            case MP_QSTR_ccw:
                dest[0] = mp_obj_new_bool(state->ccw.pressed);
                break;
        }
    } else {
        switch (attr) {
            case MP_QSTR_tip:
                dest[0] = mp_obj_new_bool(state->tip.pressed);
                break;
            case MP_QSTR_base:
                dest[0] = mp_obj_new_bool(state->base.pressed);
                break;
            case MP_QSTR__base_raw:
                dest[0] = mp_obj_new_int(state->base.pressure);
                break;
            case MP_QSTR__tip_raw:
                dest[0] = mp_obj_new_int(state->tip.pressure);
                break;
        }
    }
}

STATIC void mp_captouch_petal_state_attr(mp_obj_t self_in, qstr attr,
                                         mp_obj_t *dest) {
    mp_captouch_petal_state_t *self = MP_OBJ_TO_PTR(self_in);
    if (dest[0] != MP_OBJ_NULL) {
        return;
    }

    mp_captouch_state_t *captouch = MP_OBJ_TO_PTR(self->captouch);
    st3m_petal_state_t *state = &captouch->underlying.petals[self->ix];

    bool top = (self->ix % 2) == 0;
    uint8_t i = self->ix;

    switch (attr) {
        case MP_QSTR_is_top:
            dest[0] = mp_obj_new_bool(top);
            break;
        case MP_QSTR_is_pressed:
            dest[0] = mp_obj_new_bool(ins.captouch[i].sw.is_pressed);
            break;
        case MP_QSTR_press_event:
            dest[0] = mp_obj_new_bool(ins.captouch[i].sw.press_event);
            break;
        case MP_QSTR_release_event:
            dest[0] = mp_obj_new_bool(ins.captouch[i].sw.release_event);
            break;
        case MP_QSTR_pressed_since_ms:
            dest[0] = mp_obj_new_int(ins.captouch[i].sw.pressed_since_ms);
            break;
        case MP_QSTR_touch_area:
            st3m_inputstate_update_petal_touch(i);
            dest[0] = mp_obj_new_float(ins.captouch[i].touch_area);
            break;
        case MP_QSTR_touch_radius:
            st3m_inputstate_update_petal_touch(i);
            dest[0] = mp_obj_new_float(ins.captouch[i].touch_area);
            break;
        case MP_QSTR_touch_angle_cw:
            st3m_inputstate_update_petal_touch(i);
            dest[0] = mp_obj_new_float(ins.captouch[i].touch_area);
            break;

        case MP_QSTR_pads:
            dest[0] = self->pads;
            break;

        // TODO: DEPRECATE
        case MP_QSTR_top:
            dest[0] = mp_obj_new_bool(top);
            break;
        case MP_QSTR_bottom:
            dest[0] = mp_obj_new_bool(!top);
            break;
        case MP_QSTR_pressed:
            dest[0] = st3m_inputstate_get_petal_is_pressed(i);
            break;
        case MP_QSTR_pressure:
            dest[0] = mp_obj_new_int(state->pressure);
            break;
        case MP_QSTR_position: {
            mp_obj_t items[2] = {
                mp_obj_new_int(state->pos_distance),
                mp_obj_new_int(state->pos_angle),
            };
            dest[0] = mp_obj_new_tuple(2, items);
            break;
        }
    }
}

STATIC void mp_captouch_state_attr(mp_obj_t self_in, qstr attr,
                                   mp_obj_t *dest) {
    mp_captouch_state_t *self = MP_OBJ_TO_PTR(self_in);
    if (dest[0] != MP_OBJ_NULL) {
        return;
    }

    switch (attr) {
        case MP_QSTR_petals:
            dest[0] = self->petals;
            break;
    }
}

STATIC void mp_inputstate_imu_attr(mp_obj_t self_in, qstr attr,
                                   mp_obj_t *dest) {
    mp_inputstate_imu_t *self = MP_OBJ_TO_PTR(self_in);
    if (dest[0] != MP_OBJ_NULL) {
        return;
    }
    switch (attr) {
        case MP_QSTR_acc:
            dest[0] = self->acc;
            break;
        case MP_QSTR_gyro:
            dest[0] = self->gyro;
            break;
    }
}

STATIC void mp_inputstate_attr(mp_obj_t self_in, qstr attr,
                                   mp_obj_t *dest) {
    mp_inputstate_t *self = MP_OBJ_TO_PTR(self_in);
    if (dest[0] != MP_OBJ_NULL) {
        return;
    }
    switch (attr) {
        case MP_QSTR_temperature:
            dest[0] = mp_obj_new_float(st3m_inputstate_get_temperature());
            break;
        case MP_QSTR_pressure:
            dest[0] = mp_obj_new_float(st3m_inputstate_get_pressure());
            break;
        case MP_QSTR_imu:
            dest[0] = self->imu;
            break;
        case MP_QSTR_captouch:
            dest[0] = self->imu;
            break;
        case MP_QSTR_buttons:
            dest[0] = self->imu;
            break;
    }
}


MP_DEFINE_CONST_OBJ_TYPE(captouch_petal_pads_state_type,
                         MP_QSTR_CaptouchPetalPadsState, MP_TYPE_FLAG_NONE,
                         attr, mp_captouch_petal_pads_state_attr);

MP_DEFINE_CONST_OBJ_TYPE(captouch_petal_state_type, MP_QSTR_CaptouchPetalState,
                         MP_TYPE_FLAG_NONE, attr, mp_captouch_petal_state_attr);

MP_DEFINE_CONST_OBJ_TYPE(captouch_state_type, MP_QSTR_CaptouchState,
                         MP_TYPE_FLAG_NONE, attr, mp_captouch_state_attr);

STATIC mp_obj_t mp_captouch_state_new(const st3m_captouch_state_t *underlying) {
    mp_captouch_state_t *captouch = m_new_obj(mp_captouch_state_t);
    captouch->base.type = &captouch_state_type;
    memcpy(&captouch->underlying, underlying, sizeof(st3m_captouch_state_t));

    captouch->petals = mp_obj_new_list(0, NULL);
    for (int i = 0; i < 10; i++) {
        mp_captouch_petal_state_t *petal = m_new_obj(mp_captouch_petal_state_t);
        petal->base.type = &captouch_petal_state_type;
        petal->captouch = MP_OBJ_FROM_PTR(captouch);
        petal->ix = i;

        mp_captouch_petal_pads_state_t *pads =
            m_new_obj(mp_captouch_petal_pads_state_t);
        pads->base.type = &captouch_petal_pads_state_type;
        pads->petal = MP_OBJ_FROM_PTR(petal);
        petal->pads = MP_OBJ_FROM_PTR(pads);

        mp_obj_list_append(captouch->petals, MP_OBJ_FROM_PTR(petal));
    }

    return MP_OBJ_FROM_PTR(captouch);
}

STATIC mp_obj_t mp_captouch_read(void) {
    st3m_captouch_state_t st;
    st3m_captouch_get_all(&st);
    return mp_captouch_state_new(&st);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_captouch_read_obj, mp_captouch_read);

STATIC const mp_rom_map_elem_t globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_captouch_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_CaptouchState), MP_ROM_PTR(&captouch_state_type) },
    { MP_ROM_QSTR(MP_QSTR_CaptouchPetalState), MP_ROM_PTR(&captouch_petal_state_type) },
};

STATIC MP_DEFINE_CONST_DICT(globals, globals_table);

const mp_obj_module_t mp_module_sys_inputstate_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&globals,
};

MP_REGISTER_MODULE(MP_QSTR_sys_inputstate, mp_module_sys_inputstate_user_cmodule);
