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

// documentation: these are all super thin wrappers for the c api in components/badge23/include/badge23/audio.h

STATIC mp_obj_t mp_headset_is_connected() {
    return mp_obj_new_int(audio_headset_is_connected());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_headset_is_connected_obj, mp_headset_is_connected);

STATIC mp_obj_t mp_headphones_are_connected() {
    return mp_obj_new_int(audio_headphones_are_connected());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_headphones_are_connected_obj, mp_headphones_are_connected);

STATIC mp_obj_t mp_headphones_detection_override(mp_obj_t enable) {
    audio_headphones_detection_override(mp_obj_get_int(enable));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_headphones_detection_override_obj, mp_headphones_detection_override);



STATIC mp_obj_t mp_headphones_set_volume_dB(mp_obj_t vol_dB) {
    return mp_obj_new_float(audio_headphones_set_volume_dB(mp_obj_get_float(vol_dB)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_headphones_set_volume_dB_obj, mp_headphones_set_volume_dB);

STATIC mp_obj_t mp_speaker_set_volume_dB(mp_obj_t vol_dB) {
    return mp_obj_new_float(audio_speaker_set_volume_dB(mp_obj_get_float(vol_dB)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_speaker_set_volume_dB_obj, mp_speaker_set_volume_dB);

STATIC mp_obj_t mp_set_volume_dB(mp_obj_t vol_dB) {
    return mp_obj_new_float(audio_set_volume_dB(mp_obj_get_float(vol_dB)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_set_volume_dB_obj, mp_set_volume_dB);



STATIC mp_obj_t mp_headphones_adjust_volume_dB(mp_obj_t vol_dB) {
    return mp_obj_new_float(audio_headphones_adjust_volume_dB(mp_obj_get_float(vol_dB)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_headphones_adjust_volume_dB_obj, mp_headphones_adjust_volume_dB);

STATIC mp_obj_t mp_speaker_adjust_volume_dB(mp_obj_t vol_dB) {
    return mp_obj_new_float(audio_speaker_adjust_volume_dB(mp_obj_get_float(vol_dB)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_speaker_adjust_volume_dB_obj, mp_speaker_adjust_volume_dB);

STATIC mp_obj_t mp_adjust_volume_dB(mp_obj_t vol_dB) {
    return mp_obj_new_float(audio_adjust_volume_dB(mp_obj_get_float(vol_dB)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_adjust_volume_dB_obj, mp_adjust_volume_dB);



STATIC mp_obj_t mp_headphones_get_volume_dB() {
    return mp_obj_new_float(audio_headphones_get_volume_dB());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_headphones_get_volume_dB_obj, mp_headphones_get_volume_dB);

STATIC mp_obj_t mp_speaker_get_volume_dB() {
    return mp_obj_new_float(audio_speaker_get_volume_dB());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_speaker_get_volume_dB_obj, mp_speaker_get_volume_dB);

STATIC mp_obj_t mp_get_volume_dB() {
    return mp_obj_new_float(audio_get_volume_dB());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_get_volume_dB_obj, mp_get_volume_dB);



STATIC mp_obj_t mp_headphones_get_mute() {
    return mp_obj_new_int(audio_headphones_get_mute());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_headphones_get_mute_obj, mp_headphones_get_mute);

STATIC mp_obj_t mp_speaker_get_mute() {
    return mp_obj_new_int(audio_speaker_get_mute());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_speaker_get_mute_obj, mp_speaker_get_mute);

STATIC mp_obj_t mp_get_mute() {
    return mp_obj_new_int(audio_get_mute());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_get_mute_obj, mp_get_mute);



STATIC mp_obj_t mp_headphones_set_mute(mp_obj_t mute) {
    audio_headphones_set_mute(mp_obj_get_int(mute));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_headphones_set_mute_obj, mp_headphones_set_mute);

STATIC mp_obj_t mp_speaker_set_mute(mp_obj_t mute) {
    audio_speaker_set_mute(mp_obj_get_int(mute));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_speaker_set_mute_obj, mp_speaker_set_mute);

STATIC mp_obj_t mp_set_mute(mp_obj_t mute) {
    audio_set_mute(mp_obj_get_int(mute));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_set_mute_obj, mp_set_mute);



STATIC mp_obj_t mp_headphones_set_minimum_volume_dB(mp_obj_t vol_dB) {
    return mp_obj_new_float(audio_headphones_set_minimum_volume_dB(mp_obj_get_float(vol_dB)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_headphones_set_minimum_volume_dB_obj, mp_headphones_set_minimum_volume_dB);

STATIC mp_obj_t mp_speaker_set_minimum_volume_dB(mp_obj_t vol_dB) {
    return mp_obj_new_float(audio_speaker_set_minimum_volume_dB(mp_obj_get_float(vol_dB)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_speaker_set_minimum_volume_dB_obj, mp_speaker_set_minimum_volume_dB);

STATIC mp_obj_t mp_headphones_set_maximum_volume_dB(mp_obj_t vol_dB) {
    return mp_obj_new_float(audio_headphones_set_maximum_volume_dB(mp_obj_get_float(vol_dB)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_headphones_set_maximum_volume_dB_obj, mp_headphones_set_maximum_volume_dB);

STATIC mp_obj_t mp_speaker_set_maximum_volume_dB(mp_obj_t vol_dB) {
    return mp_obj_new_float(audio_speaker_set_maximum_volume_dB(mp_obj_get_float(vol_dB)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_speaker_set_maximum_volume_dB_obj, mp_speaker_set_maximum_volume_dB);



STATIC mp_obj_t mp_headphones_get_minimum_volume_dB() {
    return mp_obj_new_float(audio_headphones_get_minimum_volume_dB());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_headphones_get_minimum_volume_dB_obj, mp_headphones_get_minimum_volume_dB);

STATIC mp_obj_t mp_speaker_get_minimum_volume_dB() {
    return mp_obj_new_float(audio_speaker_get_minimum_volume_dB());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_speaker_get_minimum_volume_dB_obj, mp_speaker_get_minimum_volume_dB);

STATIC mp_obj_t mp_headphones_get_maximum_volume_dB() {
    return mp_obj_new_float(audio_headphones_get_maximum_volume_dB());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_headphones_get_maximum_volume_dB_obj, mp_headphones_get_maximum_volume_dB);

STATIC mp_obj_t mp_speaker_get_maximum_volume_dB() {
    return mp_obj_new_float(audio_speaker_get_maximum_volume_dB());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_speaker_get_maximum_volume_dB_obj, mp_speaker_get_maximum_volume_dB);



STATIC mp_obj_t mp_headphones_get_volume_relative() {
    return mp_obj_new_float(audio_headphones_get_volume_relative());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_headphones_get_volume_relative_obj, mp_headphones_get_volume_relative);

STATIC mp_obj_t mp_speaker_get_volume_relative() {
    return mp_obj_new_float(audio_speaker_get_volume_relative());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_speaker_get_volume_relative_obj, mp_speaker_get_volume_relative);

STATIC mp_obj_t mp_get_volume_relative() {
    return mp_obj_new_float(audio_get_volume_relative());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_get_volume_relative_obj, mp_get_volume_relative);



STATIC mp_obj_t mp_headphones_line_in_set_hardware_thru(mp_obj_t enable) {
    audio_headphones_line_in_set_hardware_thru(mp_obj_get_int(enable));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_headphones_line_in_set_hardware_thru_obj, mp_headphones_line_in_set_hardware_thru);

STATIC mp_obj_t mp_speaker_line_in_set_hardware_thru(mp_obj_t enable) {
    audio_speaker_line_in_set_hardware_thru(mp_obj_get_int(enable));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_speaker_line_in_set_hardware_thru_obj, mp_speaker_line_in_set_hardware_thru);

STATIC mp_obj_t mp_line_in_set_hardware_thru(mp_obj_t enable) {
    audio_line_in_set_hardware_thru(mp_obj_get_int(enable));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_line_in_set_hardware_thru_obj, mp_line_in_set_hardware_thru);



STATIC mp_obj_t mp_input_set_source(mp_obj_t enable) {
    audio_input_set_source(mp_obj_get_int(enable));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_input_set_source_obj, mp_input_set_source);

STATIC mp_obj_t mp_input_get_source(mp_obj_t enable) {
    return mp_obj_new_int(audio_input_get_source());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_input_get_source_obj, mp_input_get_source);



STATIC mp_obj_t mp_headset_set_gain_dB(mp_obj_t gain_dB) {
    audio_headset_set_gain_dB(mp_obj_get_int(gain_dB));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_headset_set_gain_dB_obj, mp_headset_set_gain_dB);

STATIC mp_obj_t mp_headset_get_gain_dB(mp_obj_t enable) {
    return mp_obj_new_int(audio_headset_get_gain_dB());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_headset_get_gain_dB_obj, mp_headset_get_gain_dB);



STATIC mp_obj_t mp_input_thru_set_volume_dB(mp_obj_t vol_dB) {
    return mp_obj_new_float(audio_input_thru_set_volume_dB(mp_obj_get_float(vol_dB)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_input_thru_set_volume_dB_obj, mp_input_thru_set_volume_dB);

STATIC mp_obj_t mp_input_thru_get_volume_dB() {
    return mp_obj_new_float(audio_input_thru_get_volume_dB());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_input_thru_get_volume_dB_obj, mp_input_thru_get_volume_dB);

STATIC mp_obj_t mp_input_thru_set_mute(mp_obj_t mute) {
    audio_input_thru_set_mute(mp_obj_get_int(mute));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_input_thru_set_mute_obj, mp_input_thru_set_mute);

STATIC mp_obj_t mp_input_thru_get_mute() {
    return mp_obj_new_int(audio_input_thru_get_mute());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_input_thru_get_mute_obj, mp_input_thru_get_mute);


STATIC mp_obj_t mp_codec_i2c_write(mp_obj_t reg_in, mp_obj_t data_in) {
    uint8_t reg = mp_obj_get_int(reg_in);
    uint8_t data = mp_obj_get_int(data_in);
    audio_codec_i2c_write(reg, data);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mp_codec_i2c_write_obj, mp_codec_i2c_write);



STATIC const mp_rom_map_elem_t mp_module_audio_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_audio) },
    { MP_ROM_QSTR(MP_QSTR_headset_is_connected), MP_ROM_PTR(&mp_headset_is_connected_obj) },
    { MP_ROM_QSTR(MP_QSTR_headphones_are_connected), MP_ROM_PTR(&mp_headphones_are_connected_obj) },
    { MP_ROM_QSTR(MP_QSTR_headphones_detection_override), MP_ROM_PTR(&mp_headphones_detection_override_obj) },

    { MP_ROM_QSTR(MP_QSTR_headphones_set_volume_dB), MP_ROM_PTR(&mp_headphones_set_volume_dB_obj) },
    { MP_ROM_QSTR(MP_QSTR_speaker_set_volume_dB), MP_ROM_PTR(&mp_speaker_set_volume_dB_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_volume_dB), MP_ROM_PTR(&mp_set_volume_dB_obj) },

    { MP_ROM_QSTR(MP_QSTR_headphones_adjust_volume_dB), MP_ROM_PTR(&mp_headphones_adjust_volume_dB_obj) },
    { MP_ROM_QSTR(MP_QSTR_speaker_adjust_volume_dB), MP_ROM_PTR(&mp_speaker_adjust_volume_dB_obj) },
    { MP_ROM_QSTR(MP_QSTR_adjust_volume_dB), MP_ROM_PTR(&mp_adjust_volume_dB_obj) },

    { MP_ROM_QSTR(MP_QSTR_headphones_get_volume_dB), MP_ROM_PTR(&mp_headphones_get_volume_dB_obj) },
    { MP_ROM_QSTR(MP_QSTR_speaker_get_volume_dB), MP_ROM_PTR(&mp_speaker_get_volume_dB_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_volume_dB), MP_ROM_PTR(&mp_get_volume_dB_obj) },

    { MP_ROM_QSTR(MP_QSTR_headphones_get_mute), MP_ROM_PTR(&mp_headphones_get_mute_obj) },
    { MP_ROM_QSTR(MP_QSTR_speaker_get_mute), MP_ROM_PTR(&mp_speaker_get_mute_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_mute), MP_ROM_PTR(&mp_get_mute_obj) },

    { MP_ROM_QSTR(MP_QSTR_headphones_set_mute), MP_ROM_PTR(&mp_headphones_set_mute_obj) },
    { MP_ROM_QSTR(MP_QSTR_speaker_set_mute), MP_ROM_PTR(&mp_speaker_set_mute_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_mute), MP_ROM_PTR(&mp_set_mute_obj) },

    { MP_ROM_QSTR(MP_QSTR_headphones_set_minimum_volume_dB), MP_ROM_PTR(&mp_headphones_set_minimum_volume_dB_obj) },
    { MP_ROM_QSTR(MP_QSTR_speaker_set_minimum_volume_dB), MP_ROM_PTR(&mp_speaker_set_minimum_volume_dB_obj) },
    { MP_ROM_QSTR(MP_QSTR_headphones_set_maximum_volume_dB), MP_ROM_PTR(&mp_headphones_set_maximum_volume_dB_obj) },
    { MP_ROM_QSTR(MP_QSTR_speaker_set_maximum_volume_dB), MP_ROM_PTR(&mp_speaker_set_maximum_volume_dB_obj) },

    { MP_ROM_QSTR(MP_QSTR_headphones_get_minimum_volume_dB), MP_ROM_PTR(&mp_headphones_get_minimum_volume_dB_obj) },
    { MP_ROM_QSTR(MP_QSTR_speaker_get_minimum_volume_dB), MP_ROM_PTR(&mp_speaker_get_minimum_volume_dB_obj) },
    { MP_ROM_QSTR(MP_QSTR_headphones_get_maximum_volume_dB), MP_ROM_PTR(&mp_headphones_get_maximum_volume_dB_obj) },
    { MP_ROM_QSTR(MP_QSTR_speaker_get_maximum_volume_dB), MP_ROM_PTR(&mp_speaker_get_maximum_volume_dB_obj) },

    { MP_ROM_QSTR(MP_QSTR_headphones_get_volume_relative), MP_ROM_PTR(&mp_headphones_get_volume_relative_obj) },
    { MP_ROM_QSTR(MP_QSTR_speaker_get_volume_relative), MP_ROM_PTR(&mp_speaker_get_volume_relative_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_volume_relative), MP_ROM_PTR(&mp_get_volume_relative_obj) },

    { MP_ROM_QSTR(MP_QSTR_headphones_line_in_set_hardware_thru), MP_ROM_PTR(&mp_headphones_line_in_set_hardware_thru_obj) },
    { MP_ROM_QSTR(MP_QSTR_speaker_line_in_set_hardware_thru), MP_ROM_PTR(&mp_speaker_line_in_set_hardware_thru_obj) },
    { MP_ROM_QSTR(MP_QSTR_line_in_set_hardware_thru), MP_ROM_PTR(&mp_line_in_set_hardware_thru_obj) },

    { MP_ROM_QSTR(MP_QSTR_input_set_source), MP_ROM_PTR(&mp_input_set_source_obj) },
    { MP_ROM_QSTR(MP_QSTR_input_get_source), MP_ROM_PTR(&mp_input_get_source_obj) },

    { MP_ROM_QSTR(MP_QSTR_headset_set_gain_dB), MP_ROM_PTR(&mp_headset_set_gain_dB_obj) },
    { MP_ROM_QSTR(MP_QSTR_headset_get_gain_dB), MP_ROM_PTR(&mp_headset_get_gain_dB_obj) },

    { MP_ROM_QSTR(MP_QSTR_input_thru_set_volume_dB), MP_ROM_PTR(&mp_input_thru_set_volume_dB_obj) },
    { MP_ROM_QSTR(MP_QSTR_input_thru_get_volume_dB), MP_ROM_PTR(&mp_input_thru_get_volume_dB_obj) },
    { MP_ROM_QSTR(MP_QSTR_input_thru_set_mute), MP_ROM_PTR(&mp_input_thru_set_mute_obj) },
    { MP_ROM_QSTR(MP_QSTR_input_thru_get_mute), MP_ROM_PTR(&mp_input_thru_get_mute_obj) },

    { MP_ROM_QSTR(MP_QSTR_codec_i2c_write), MP_ROM_PTR(&mp_codec_i2c_write_obj) },

    { MP_ROM_QSTR(MP_QSTR_INPUT_SOURCE_NONE), MP_ROM_INT(AUDIO_INPUT_SOURCE_NONE) },
    { MP_ROM_QSTR(MP_QSTR_INPUT_SOURCE_LINE_IN), MP_ROM_INT(AUDIO_INPUT_SOURCE_LINE_IN) },
    { MP_ROM_QSTR(MP_QSTR_INPUT_SOURCE_HEADSET_MIC), MP_ROM_INT(AUDIO_INPUT_SOURCE_HEADSET_MIC) },
    { MP_ROM_QSTR(MP_QSTR_INPUT_SOURCE_ONBOARD_MIC), MP_ROM_INT(AUDIO_INPUT_SOURCE_ONBOARD_MIC) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_audio_globals, mp_module_audio_globals_table);

const mp_obj_module_t mp_module_audio = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_audio_globals,
};

MP_REGISTER_MODULE(MP_QSTR_audio, mp_module_audio);

