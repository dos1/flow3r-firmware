#include <stdio.h>

#include "py/obj.h"
#include "py/runtime.h"

#include "bl00mbox.h"
#include "bl00mbox_plugin_registry.h"
#include "bl00mbox_user.h"

#if !MICROPY_ENABLE_FINALISER
#error "BADGE23_SYNTH requires MICROPY_ENABLE_FINALISER"
#endif

#if 0
typedef struct _synth_tinysynth_obj_t {
    mp_obj_base_t base;
    trad_osc_t osc;
    uint16_t source_index;
} synth_tinysynth_obj_t;

const mp_obj_type_t synth_tinysynth_type;

STATIC void tinysynth_print(const mp_print_t *print, mp_obj_t self_in,
                            mp_print_kind_t kind) {
    (void)kind;
    synth_tinysynth_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_print_str(print, "tinysynth(");
    mp_obj_print_helper(print, mp_obj_new_int(self->source_index), PRINT_REPR);
    mp_print_str(print, ")");
}

STATIC mp_obj_t tinysynth_make_new(const mp_obj_type_t *type, size_t n_args,
                                   size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 1, true);
    synth_tinysynth_obj_t *self =
        m_new_obj_with_finaliser(synth_tinysynth_obj_t);
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

STATIC mp_obj_t tinysynth_env_phase(mp_obj_t self_in) {
    synth_tinysynth_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(trad_env_phase(&(self->osc)));
}
MP_DEFINE_CONST_FUN_OBJ_1(tinysynth_env_phase_obj, tinysynth_env_phase);

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
    { MP_ROM_QSTR(MP_QSTR_volume), MP_ROM_PTR(&tinysynth_volume_obj) },
    { MP_ROM_QSTR(MP_QSTR_sustain), MP_ROM_PTR(&tinysynth_sustain_obj) },
    { MP_ROM_QSTR(MP_QSTR_release_ms), MP_ROM_PTR(&tinysynth_release_ms_obj) },
    { MP_ROM_QSTR(MP_QSTR_env_phase), MP_ROM_PTR(&tinysynth_env_phase_obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&tinysynth_deinit_obj) },
};

STATIC MP_DEFINE_CONST_DICT(tinysynth_locals_dict, tinysynth_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(synth_tinysynth_type, MP_QSTR_bl00mbox,
                         MP_TYPE_FLAG_NONE, make_new, tinysynth_make_new, print,
                         tinysynth_print, locals_dict, &tinysynth_locals_dict);

#endif

STATIC mp_obj_t mp_plugin_index_get_id(mp_obj_t index) {
    /// prints name
    radspa_descriptor_t * desc = bl00mbox_plugin_registry_get_descriptor_from_index(mp_obj_get_int(index));
    if(desc == NULL) return mp_const_none;
    return mp_obj_new_int(desc->id);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_plugin_index_get_id_obj, mp_plugin_index_get_id);

STATIC mp_obj_t mp_plugin_index_get_name(mp_obj_t index) {
    /// prints name
    radspa_descriptor_t * desc = bl00mbox_plugin_registry_get_descriptor_from_index(mp_obj_get_int(index));
    if(desc == NULL) return mp_const_none;
    return mp_obj_new_str(desc->name, strlen(desc->name));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_plugin_index_get_name_obj, mp_plugin_index_get_name);

STATIC mp_obj_t mp_plugin_index_get_description(mp_obj_t index) {
    /// prints name
    radspa_descriptor_t * desc = bl00mbox_plugin_registry_get_descriptor_from_index(mp_obj_get_int(index));
    if(desc == NULL) return mp_const_none;
    return mp_obj_new_str(desc->description, strlen(desc->description));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_plugin_index_get_description_obj, mp_plugin_index_get_description);

STATIC mp_obj_t mp_plugin_registry_num_plugins(void) {
    return mp_obj_new_int(bl00mbox_plugin_registry_get_plugin_num());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_plugin_registry_num_plugins_obj, mp_plugin_registry_num_plugins);


STATIC mp_obj_t mp_channel_get_free() {
    return mp_obj_new_int(bl00mbox_channel_get_free_index());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_channel_get_free_obj, mp_channel_get_free);

STATIC mp_obj_t mp_channel_get_foreground() {
    return mp_obj_new_int(bl00mbox_channel_get_foreground_index());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_channel_get_foreground_obj, mp_channel_get_foreground);

STATIC mp_obj_t mp_channel_enable(mp_obj_t chan) {
    bl00mbox_channel_enable(mp_obj_get_int(chan));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_channel_enable_obj, mp_channel_enable);

STATIC mp_obj_t mp_channel_disable(mp_obj_t chan) {
    bl00mbox_channel_disable(mp_obj_get_int(chan));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_channel_disable_obj, mp_channel_disable);

STATIC mp_obj_t mp_channel_set_volume(mp_obj_t chan, mp_obj_t vol) {
    bl00mbox_channel_set_volume(mp_obj_get_int(chan), mp_obj_get_int(vol));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mp_channel_set_volume_obj, mp_channel_set_volume);

STATIC mp_obj_t mp_channel_get_volume(mp_obj_t chan) {
    return mp_obj_new_int(bl00mbox_channel_get_volume(mp_obj_get_int(chan)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_channel_get_volume_obj, mp_channel_get_volume);


STATIC mp_obj_t mp_channel_buds_num(mp_obj_t chan) {
    return mp_obj_new_int(bl00mbox_channel_buds_num(mp_obj_get_int(chan)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_channel_buds_num_obj, mp_channel_buds_num);

STATIC mp_obj_t mp_channel_new_bud(mp_obj_t chan, mp_obj_t id) {
    bl00mbox_bud_t * bud = bl00mbox_channel_new_bud(mp_obj_get_int(chan), mp_obj_get_int(id), 0);
    if(bud == NULL) return mp_const_none;
    return mp_obj_new_int(bud->index);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mp_channel_new_bud_obj, mp_channel_new_bud);

STATIC mp_obj_t mp_channel_connect_signal_to_output_mixer(mp_obj_t chan, mp_obj_t bud_index, mp_obj_t bud_signal_index) {
    bool success = bl00mbox_channel_connect_signal_to_output_mixer(
            mp_obj_get_int(chan), mp_obj_get_int(bud_index), mp_obj_get_int(bud_signal_index));
    return mp_obj_new_bool(success);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(mp_channel_connect_signal_to_output_mixer_obj, mp_channel_connect_signal_to_output_mixer);


STATIC mp_obj_t mp_channel_bud_get_num_signals(mp_obj_t chan, mp_obj_t bud) {
    uint16_t ret = bl00mbox_channel_bud_get_num_signals(mp_obj_get_int(chan), mp_obj_get_int(bud));
    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mp_channel_bud_get_num_signals_obj, mp_channel_bud_get_num_signals);

STATIC mp_obj_t mp_channel_bud_get_name(mp_obj_t chan, mp_obj_t bud) {
    char *name = bl00mbox_channel_bud_get_name(mp_obj_get_int(chan), mp_obj_get_int(bud));
    return mp_obj_new_str(name, strlen(name));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mp_channel_bud_get_name_obj, mp_channel_bud_get_name);

STATIC mp_obj_t mp_channel_bud_get_signal_name(mp_obj_t chan, mp_obj_t bud, mp_obj_t signal) {
    char * name = bl00mbox_channel_bud_get_signal_name(mp_obj_get_int(chan), mp_obj_get_int(bud), mp_obj_get_int(signal));
    return mp_obj_new_str(name, strlen(name));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(mp_channel_bud_get_signal_name_obj, mp_channel_bud_get_signal_name);

STATIC mp_obj_t mp_channel_bud_get_signal_description(mp_obj_t chan, mp_obj_t bud, mp_obj_t signal) {
    char * description = bl00mbox_channel_bud_get_signal_description(mp_obj_get_int(chan), mp_obj_get_int(bud), mp_obj_get_int(signal));
    return mp_obj_new_str(description, strlen(description));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(mp_channel_bud_get_signal_description_obj, mp_channel_bud_get_signal_description);

STATIC mp_obj_t mp_channel_bud_get_signal_unit(mp_obj_t chan, mp_obj_t bud, mp_obj_t signal) {
    char * unit = bl00mbox_channel_bud_get_signal_unit(mp_obj_get_int(chan), mp_obj_get_int(bud), mp_obj_get_int(signal));
    return mp_obj_new_str(unit, strlen(unit));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(mp_channel_bud_get_signal_unit_obj, mp_channel_bud_get_signal_unit);

STATIC mp_obj_t mp_channel_bud_set_signal_value(size_t n_args, const mp_obj_t *args) {
    bool success = bl00mbox_channel_bud_set_signal_value(
            mp_obj_get_int(args[0]), //chan
            mp_obj_get_int(args[1]), //bud_index
            mp_obj_get_int(args[2]), //bud_signal_index
            mp_obj_get_int(args[3])); //value
    return mp_obj_new_bool(success);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_channel_bud_set_signal_value_obj, 4, 4, mp_channel_bud_set_signal_value);

STATIC mp_obj_t mp_channel_bud_get_signal_value(mp_obj_t chan, mp_obj_t bud, mp_obj_t signal) {
    int16_t val = bl00mbox_channel_bud_get_signal_value(
            mp_obj_get_int(chan),
            mp_obj_get_int(bud),
            mp_obj_get_int(signal));
    return mp_obj_new_int(val);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(mp_channel_bud_get_signal_value_obj, mp_channel_bud_get_signal_value);

STATIC mp_obj_t mp_channel_bud_get_signal_hints(mp_obj_t chan, mp_obj_t bud, mp_obj_t signal) {
    uint32_t val = bl00mbox_channel_bud_get_signal_hints(
            mp_obj_get_int(chan),
            mp_obj_get_int(bud),
            mp_obj_get_int(signal));
    return mp_obj_new_int(val);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(mp_channel_bud_get_signal_hints_obj, mp_channel_bud_get_signal_hints);

STATIC mp_obj_t mp_channel_disconnect_signal(mp_obj_t chan, mp_obj_t bud, mp_obj_t signal) {
    bool ret = bl00mbox_channel_disconnect_signal_rx(mp_obj_get_int(chan), mp_obj_get_int(bud), mp_obj_get_int(signal));
    return mp_obj_new_bool(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(mp_channel_disconnect_signal_obj, mp_channel_disconnect_signal);

STATIC mp_obj_t mp_channel_connect_signal(size_t n_args, const mp_obj_t *args) {
    bool success = bl00mbox_channel_connect_signal(
            mp_obj_get_int(args[0]), //chan
            mp_obj_get_int(args[1]), //bud_tx_index
            mp_obj_get_int(args[2]), //bud_tx_signal_index
            mp_obj_get_int(args[3]), //bud_tx_index
            mp_obj_get_int(args[4])); //bud_tx_signal_index
    return mp_obj_new_bool(success);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_channel_connect_signal_obj, 5, 5, mp_channel_connect_signal);

STATIC const mp_map_elem_t bl00mbox_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_sys_bl00mbox) },
    { MP_ROM_QSTR(MP_QSTR_plugin_registry_num_plugins), MP_ROM_PTR(&mp_plugin_registry_num_plugins_obj) },
    { MP_ROM_QSTR(MP_QSTR_plugin_index_get_id), MP_ROM_PTR(&mp_plugin_index_get_id_obj) },
    { MP_ROM_QSTR(MP_QSTR_plugin_index_get_name), MP_ROM_PTR(&mp_plugin_index_get_name_obj) },
    { MP_ROM_QSTR(MP_QSTR_plugin_index_get_description), MP_ROM_PTR(&mp_plugin_index_get_description_obj) },

    { MP_ROM_QSTR(MP_QSTR_channel_get_free), MP_ROM_PTR(&mp_channel_get_free_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_get_foreground), MP_ROM_PTR(&mp_channel_get_foreground_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_enable), MP_ROM_PTR(&mp_channel_enable_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_disable), MP_ROM_PTR(&mp_channel_disable_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_set_volume), MP_ROM_PTR(&mp_channel_set_volume_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_get_volume), MP_ROM_PTR(&mp_channel_get_volume_obj) },

    { MP_ROM_QSTR(MP_QSTR_channel_buds_num), MP_ROM_PTR(&mp_channel_buds_num_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_new_bud), MP_ROM_PTR(&mp_channel_new_bud_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_connect_signal_to_output_mixer), MP_ROM_PTR(&mp_channel_connect_signal_to_output_mixer_obj) },

    { MP_ROM_QSTR(MP_QSTR_channel_bud_get_num_signals), MP_ROM_PTR(&mp_channel_bud_get_num_signals_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_bud_get_name), MP_ROM_PTR(&mp_channel_bud_get_name_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_bud_get_signal_name), MP_ROM_PTR(&mp_channel_bud_get_signal_name_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_bud_get_signal_description), MP_ROM_PTR(&mp_channel_bud_get_signal_description_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_bud_get_signal_unit), MP_ROM_PTR(&mp_channel_bud_get_signal_unit_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_bud_set_signal_value), MP_ROM_PTR(&mp_channel_bud_set_signal_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_bud_get_signal_value), MP_ROM_PTR(&mp_channel_bud_get_signal_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_bud_get_signal_hints), MP_ROM_PTR(&mp_channel_bud_get_signal_hints_obj) },

    { MP_ROM_QSTR(MP_QSTR_channel_connect_signal), MP_ROM_PTR(&mp_channel_connect_signal_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_disconnect_signal), MP_ROM_PTR(&mp_channel_disconnect_signal_obj) },
    //{ MP_OBJ_NEW_QSTR(MP_QSTR_tinysynth), (mp_obj_t)&synth_tinysynth_type },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_bl00mbox_globals, bl00mbox_globals_table);

const mp_obj_module_t bl00mbox_user_cmodule = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&mp_module_bl00mbox_globals,
};

MP_REGISTER_MODULE(MP_QSTR_sys_bl00mbox, bl00mbox_user_cmodule);
