#include <stdio.h>

#include "py/runtime.h"
#include "py/obj.h"

#include "bl00mbox.h"
#include "tinysynth.h"

#if !MICROPY_ENABLE_FINALISER
#error "BADGE23_SYNTH requires MICROPY_ENABLE_FINALISER"
#endif

typedef struct _synth_tinysynth_obj_t {
    mp_obj_base_t base;
    trad_osc_t osc;
    uint16_t source_index;
} synth_tinysynth_obj_t;

const mp_obj_type_t synth_tinysynth_type;

STATIC void tinysynth_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    (void)kind;
    synth_tinysynth_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_print_str(print, "tinysynth(");
    mp_obj_print_helper(print, mp_obj_new_int(self->source_index), PRINT_REPR);
    mp_print_str(print, ")");
}

STATIC mp_obj_t tinysynth_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 1, true);
    synth_tinysynth_obj_t *self = m_new_obj_with_finaliser(synth_tinysynth_obj_t);
    self->base.type = &synth_tinysynth_type;

    self->osc.vol = 32767;
    self->osc.bend = 1;
    self->osc.noise_reg = 1;
    self->osc.waveform = TRAD_OSC_WAVE_TRI;

    trad_osc_set_freq_Hz(&(self->osc), mp_obj_get_float(args[0]));
    trad_osc_set_attack_ms(&(self->osc), 20);
    trad_osc_set_decay_ms(&(self->osc), 500);
    trad_osc_set_sustain(&(self->osc), 0);
    trad_osc_set_release_ms(&(self->osc), 500);
    self->source_index = bl00mbox_source_add(&(self->osc), run_trad_osc);

    return MP_OBJ_FROM_PTR(self);
}

// Class methods
STATIC mp_obj_t tinysynth_start(mp_obj_t self_in) {
    synth_tinysynth_obj_t *self = MP_OBJ_TO_PTR(self_in);
    trad_env_start(&(self->osc));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(tinysynth_start_obj, tinysynth_start);

STATIC mp_obj_t tinysynth_stop(mp_obj_t self_in) {
    synth_tinysynth_obj_t *self = MP_OBJ_TO_PTR(self_in);
    trad_env_stop(&(self->osc));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(tinysynth_stop_obj, tinysynth_stop);

STATIC mp_obj_t tinysynth_freq(mp_obj_t self_in, mp_obj_t freq) {
    synth_tinysynth_obj_t *self = MP_OBJ_TO_PTR(self_in);
    trad_osc_set_freq_Hz(&(self->osc), mp_obj_get_float(freq));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(tinysynth_freq_obj, tinysynth_freq);

STATIC mp_obj_t tinysynth_tone(mp_obj_t self_in, mp_obj_t tone) {
    synth_tinysynth_obj_t *self = MP_OBJ_TO_PTR(self_in);
    trad_osc_set_freq_semitone(&(self->osc), mp_obj_get_float(tone));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(tinysynth_tone_obj, tinysynth_tone);

STATIC mp_obj_t tinysynth_waveform(mp_obj_t self_in, mp_obj_t waveform) {
    synth_tinysynth_obj_t *self = MP_OBJ_TO_PTR(self_in);
    trad_osc_set_waveform(&(self->osc), mp_obj_get_int(waveform));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(tinysynth_waveform_obj, tinysynth_waveform);

STATIC mp_obj_t tinysynth_attack_ms(mp_obj_t self_in, mp_obj_t attack_ms) {
    synth_tinysynth_obj_t *self = MP_OBJ_TO_PTR(self_in);
    trad_osc_set_attack_ms(&(self->osc), mp_obj_get_float(attack_ms));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(tinysynth_attack_ms_obj, tinysynth_attack_ms);

STATIC mp_obj_t tinysynth_decay_ms(mp_obj_t self_in, mp_obj_t decay_ms) {
    synth_tinysynth_obj_t *self = MP_OBJ_TO_PTR(self_in);
    trad_osc_set_decay_ms(&(self->osc), mp_obj_get_int(decay_ms));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(tinysynth_decay_ms_obj, tinysynth_decay_ms);

STATIC mp_obj_t tinysynth_release_ms(mp_obj_t self_in, mp_obj_t release_ms) {
    synth_tinysynth_obj_t *self = MP_OBJ_TO_PTR(self_in);
    trad_osc_set_release_ms(&(self->osc), mp_obj_get_int(release_ms));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(tinysynth_release_ms_obj, tinysynth_release_ms);

STATIC mp_obj_t tinysynth_sustain(mp_obj_t self_in, mp_obj_t sus) {
    synth_tinysynth_obj_t *self = MP_OBJ_TO_PTR(self_in);
    trad_osc_set_sustain(&(self->osc), mp_obj_get_float(sus));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(tinysynth_sustain_obj, tinysynth_sustain);

STATIC mp_obj_t tinysynth_volume(mp_obj_t self_in, mp_obj_t vol) {
    synth_tinysynth_obj_t *self = MP_OBJ_TO_PTR(self_in);
    trad_osc_set_vol(&(self->osc), mp_obj_get_float(vol));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(tinysynth_volume_obj, tinysynth_volume);

STATIC mp_obj_t tinysynth_deinit(mp_obj_t self_in) {
    synth_tinysynth_obj_t *self = MP_OBJ_TO_PTR(self_in);
    bl00mbox_source_remove(self->source_index);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(tinysynth_deinit_obj, tinysynth_deinit);

STATIC const mp_rom_map_elem_t tinysynth_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&tinysynth_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&tinysynth_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_freq), MP_ROM_PTR(&tinysynth_freq_obj) },
    { MP_ROM_QSTR(MP_QSTR_tone), MP_ROM_PTR(&tinysynth_tone_obj) },
    { MP_ROM_QSTR(MP_QSTR_waveform), MP_ROM_PTR(&tinysynth_waveform_obj) },
    { MP_ROM_QSTR(MP_QSTR_attack_ms), MP_ROM_PTR(&tinysynth_attack_ms_obj) },
    { MP_ROM_QSTR(MP_QSTR_decay_ms), MP_ROM_PTR(&tinysynth_decay_ms_obj) },
    { MP_ROM_QSTR(MP_QSTR_volume), MP_ROM_PTR(&tinysynth_sustain_obj) },
    { MP_ROM_QSTR(MP_QSTR_sustain), MP_ROM_PTR(&tinysynth_sustain_obj) },
    { MP_ROM_QSTR(MP_QSTR_release_ms), MP_ROM_PTR(&tinysynth_release_ms_obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&tinysynth_deinit_obj) },
};

STATIC MP_DEFINE_CONST_DICT(tinysynth_locals_dict, tinysynth_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    synth_tinysynth_type,
    MP_QSTR_bl00mbox,
    MP_TYPE_FLAG_NONE,
    make_new, tinysynth_make_new,
    print, tinysynth_print,
    locals_dict, &tinysynth_locals_dict
    );    

STATIC const mp_map_elem_t bl00mbox_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_bl00mbox) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_tinysynth), (mp_obj_t)&synth_tinysynth_type },
};

STATIC MP_DEFINE_CONST_DICT (
    mp_module_bl00mbox_globals,
    bl00mbox_globals_table
);

const mp_obj_module_t bl00mbox_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_bl00mbox_globals,
};

MP_REGISTER_MODULE(MP_QSTR_bl00mbox, bl00mbox_user_cmodule);
