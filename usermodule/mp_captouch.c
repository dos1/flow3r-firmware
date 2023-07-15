#include "py/runtime.h"
#include "py/builtin.h"

#include "badge23/captouch.h"

#include <string.h>

STATIC mp_obj_t mp_captouch_calibration_active(void)
{
    return mp_obj_new_int(captouch_calibration_active());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_captouch_calibration_active_obj, mp_captouch_calibration_active);

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
	captouch_state_t underlying;
} mp_captouch_state_t;

const mp_obj_type_t captouch_state_type;

STATIC void mp_captouch_petal_pads_state_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
	mp_captouch_petal_pads_state_t *self = MP_OBJ_TO_PTR(self_in);
	if (dest[0] != MP_OBJ_NULL) {
		return;
	}

	mp_captouch_petal_state_t *petal = MP_OBJ_TO_PTR(self->petal);
	mp_captouch_state_t *captouch = MP_OBJ_TO_PTR(petal->captouch);
	captouch_petal_state_t *state = &captouch->underlying.petals[petal->ix];
	bool top = (petal->ix % 2) == 0;

	if (top) {
		switch (attr) {
		case MP_QSTR_base: dest[0] = mp_obj_new_bool(state->pads.base_pressed); break;
		case MP_QSTR_cw: dest[0] = mp_obj_new_bool(state->pads.cw_pressed); break;
		case MP_QSTR_ccw: dest[0] = mp_obj_new_bool(state->pads.ccw_pressed); break;
		}
	} else {
		switch (attr) {
		case MP_QSTR_tip: dest[0] = mp_obj_new_bool(state->pads.tip_pressed); break;
		case MP_QSTR_base: dest[0] = mp_obj_new_bool(state->pads.base_pressed); break;
		}
	}
}

STATIC void mp_captouch_petal_state_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
	mp_captouch_petal_state_t *self = MP_OBJ_TO_PTR(self_in);
	if (dest[0] != MP_OBJ_NULL) {
		return;
	}

	mp_captouch_state_t *captouch = MP_OBJ_TO_PTR(self->captouch);
	captouch_petal_state_t *state = &captouch->underlying.petals[self->ix];

	bool top = (self->ix % 2) == 0;

	switch (attr) {
	case MP_QSTR_top: dest[0] = mp_obj_new_bool(top); break;
	case MP_QSTR_bottom: dest[0] = mp_obj_new_bool(!top); break;
	case MP_QSTR_pressed: dest[0] = mp_obj_new_bool(state->pressed); break;
	case MP_QSTR_pads: dest[0] = self->pads; break;
	}
}

STATIC void mp_captouch_state_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
	mp_captouch_state_t *self = MP_OBJ_TO_PTR(self_in);
	if (dest[0] != MP_OBJ_NULL) {
		return;
	}

	switch (attr) {
	case MP_QSTR_petals: dest[0] = self->petals; break;
	}
}

MP_DEFINE_CONST_OBJ_TYPE(
	captouch_petal_pads_state_type,
	MP_QSTR_CaptouchPetalPadsState,
	MP_TYPE_FLAG_NONE,
	attr, mp_captouch_petal_pads_state_attr
);

MP_DEFINE_CONST_OBJ_TYPE(
	captouch_petal_state_type,
	MP_QSTR_CaptouchPetalState,
	MP_TYPE_FLAG_NONE,
	attr, mp_captouch_petal_state_attr
);

MP_DEFINE_CONST_OBJ_TYPE(
	captouch_state_type,
	MP_QSTR_CaptouchState,
	MP_TYPE_FLAG_NONE,
	attr, mp_captouch_state_attr
);

STATIC mp_obj_t mp_captouch_state_new(const captouch_state_t *underlying) {
	mp_captouch_state_t *captouch = m_new_obj(mp_captouch_state_t);
	captouch->base.type = &captouch_state_type;
	memcpy(&captouch->underlying, underlying, sizeof(captouch_state_t));

	captouch->petals = mp_obj_new_list(0, NULL);
	for (int i = 0; i < 10; i++) {
		mp_captouch_petal_state_t *petal = m_new_obj(mp_captouch_petal_state_t);
		petal->base.type = &captouch_petal_state_type;
		petal->captouch = MP_OBJ_FROM_PTR(captouch);
		petal->ix = i;

		mp_captouch_petal_pads_state_t *pads = m_new_obj(mp_captouch_petal_pads_state_t);
		pads->base.type = &captouch_petal_pads_state_type;
		pads->petal = MP_OBJ_FROM_PTR(petal);
		petal->pads = MP_OBJ_FROM_PTR(pads);

		mp_obj_list_append(captouch->petals, MP_OBJ_FROM_PTR(petal));
	}

	return MP_OBJ_FROM_PTR(captouch);
}

STATIC mp_obj_t mp_captouch_read(void) {
	mp_captouch_state_t st;
	read_captouch_ex(&st);
	return mp_captouch_state_new(&st);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_captouch_read_obj, mp_captouch_read);

STATIC const mp_rom_map_elem_t globals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_captouch_read_obj) },
	{ MP_ROM_QSTR(MP_QSTR_calibration_active), MP_ROM_PTR(&mp_captouch_calibration_active_obj) }
};

STATIC MP_DEFINE_CONST_DICT(globals, globals_table);


const mp_obj_module_t mp_module_captouch_user_cmodule = {
	.base = { &mp_type_module },
	.globals = (mp_obj_dict_t *)&globals,
};

MP_REGISTER_MODULE(MP_QSTR_captouch, mp_module_captouch_user_cmodule);