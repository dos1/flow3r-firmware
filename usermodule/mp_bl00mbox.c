#include <stdio.h>

#include "py/obj.h"
#include "py/runtime.h"

#include "bl00mbox.h"
#include "bl00mbox_plugin_registry.h"
#include "bl00mbox_user.h"


// ========================
//    PLUGIN OPERATIONS
// ========================
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


// ========================
//    CHANNEL OPERATIONS
// ========================

STATIC mp_obj_t mp_channel_clear(mp_obj_t index) {
    return mp_obj_new_bool(bl00mbox_channel_clear(mp_obj_get_int(index)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_channel_clear_obj, mp_channel_clear);

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

STATIC mp_obj_t mp_channel_get_bud_by_list_pos(mp_obj_t chan, mp_obj_t pos) {
    return mp_obj_new_int(bl00mbox_channel_get_bud_by_list_pos(mp_obj_get_int(chan), mp_obj_get_int(pos)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mp_channel_get_bud_by_list_pos_obj, mp_channel_get_bud_by_list_pos);

STATIC mp_obj_t mp_channel_conns_num(mp_obj_t chan) {
    return mp_obj_new_int(bl00mbox_channel_conns_num(mp_obj_get_int(chan)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_channel_conns_num_obj, mp_channel_conns_num);

STATIC mp_obj_t mp_channel_mixer_num(mp_obj_t chan) {
    return mp_obj_new_int(bl00mbox_channel_mixer_num(mp_obj_get_int(chan)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_channel_mixer_num_obj, mp_channel_mixer_num);

STATIC mp_obj_t mp_channel_get_bud_by_mixer_list_pos(mp_obj_t chan, mp_obj_t pos) {
    return mp_obj_new_int(bl00mbox_channel_get_bud_by_mixer_list_pos(mp_obj_get_int(chan), mp_obj_get_int(pos)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mp_channel_get_bud_by_mixer_list_pos_obj, mp_channel_get_bud_by_mixer_list_pos);

STATIC mp_obj_t mp_channel_get_signal_by_mixer_list_pos(mp_obj_t chan, mp_obj_t pos) {
    return mp_obj_new_int(bl00mbox_channel_get_signal_by_mixer_list_pos(mp_obj_get_int(chan), mp_obj_get_int(pos)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mp_channel_get_signal_by_mixer_list_pos_obj, mp_channel_get_signal_by_mixer_list_pos);


// ========================
//      BUD OPERATIONS
// ========================

STATIC mp_obj_t mp_channel_new_bud(mp_obj_t chan, mp_obj_t id, mp_obj_t init_var) {
    bl00mbox_bud_t * bud = bl00mbox_channel_new_bud(mp_obj_get_int(chan), mp_obj_get_int(id), mp_obj_get_int(init_var));
    if(bud == NULL) return mp_const_none;
    return mp_obj_new_int(bud->index);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(mp_channel_new_bud_obj, mp_channel_new_bud);

STATIC mp_obj_t mp_channel_delete_bud(mp_obj_t chan, mp_obj_t bud) {
    bool ret = bl00mbox_channel_delete_bud(mp_obj_get_int(chan), mp_obj_get_int(bud));
    return mp_obj_new_bool(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mp_channel_delete_bud_obj, mp_channel_delete_bud);

STATIC mp_obj_t mp_channel_bud_exists(mp_obj_t chan, mp_obj_t bud) {
    bool ret = bl00mbox_channel_bud_exists(mp_obj_get_int(chan), mp_obj_get_int(bud));
    return mp_obj_new_bool(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mp_channel_bud_exists_obj, mp_channel_bud_exists);

STATIC mp_obj_t mp_channel_bud_get_name(mp_obj_t chan, mp_obj_t bud) {
    char * name = bl00mbox_channel_bud_get_name(mp_obj_get_int(chan), mp_obj_get_int(bud));
    return mp_obj_new_str(name, strlen(name));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mp_channel_bud_get_name_obj, mp_channel_bud_get_name);

STATIC mp_obj_t mp_channel_bud_get_description(mp_obj_t chan, mp_obj_t bud) {
    char * description = bl00mbox_channel_bud_get_description(mp_obj_get_int(chan), mp_obj_get_int(bud));
    return mp_obj_new_str(description, strlen(description));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mp_channel_bud_get_description_obj, mp_channel_bud_get_description);

STATIC mp_obj_t mp_channel_bud_get_plugin_id(mp_obj_t chan, mp_obj_t bud) {
    uint32_t plugin_id = bl00mbox_channel_bud_get_plugin_id(mp_obj_get_int(chan), mp_obj_get_int(bud));
    return mp_obj_new_int(plugin_id);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mp_channel_bud_get_plugin_id_obj, mp_channel_bud_get_plugin_id);

STATIC mp_obj_t mp_channel_bud_get_num_signals(mp_obj_t chan, mp_obj_t bud) {
    uint16_t ret = bl00mbox_channel_bud_get_num_signals(mp_obj_get_int(chan), mp_obj_get_int(bud));
    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mp_channel_bud_get_num_signals_obj, mp_channel_bud_get_num_signals);


// ========================
//      SIGNAL OPERATIONS
// ========================

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

STATIC mp_obj_t mp_channel_bud_get_signal_hints(mp_obj_t chan, mp_obj_t bud, mp_obj_t signal) {
    uint32_t val = bl00mbox_channel_bud_get_signal_hints(
            mp_obj_get_int(chan),
            mp_obj_get_int(bud),
            mp_obj_get_int(signal));
    return mp_obj_new_int(val);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(mp_channel_bud_get_signal_hints_obj, mp_channel_bud_get_signal_hints);

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

STATIC mp_obj_t mp_channel_subscriber_num(mp_obj_t chan, mp_obj_t bud, mp_obj_t signal) {
    return mp_obj_new_int(bl00mbox_channel_subscriber_num(mp_obj_get_int(chan), mp_obj_get_int(bud), mp_obj_get_int(signal)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(mp_channel_subscriber_num_obj, mp_channel_subscriber_num);

STATIC mp_obj_t mp_channel_get_bud_by_subscriber_list_pos(size_t n_args, const mp_obj_t *args) {
    return mp_obj_new_int(bl00mbox_channel_get_bud_by_subscriber_list_pos(
            mp_obj_get_int(args[0]), //chan
            mp_obj_get_int(args[1]), //bud_index
            mp_obj_get_int(args[2]), //bud_signal_index
            mp_obj_get_int(args[3])) //pos
    );
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_channel_get_bud_by_subscriber_list_pos_obj, 4, 4, mp_channel_get_bud_by_subscriber_list_pos);

STATIC mp_obj_t mp_channel_get_signal_by_subscriber_list_pos(size_t n_args, const mp_obj_t *args) {
    return mp_obj_new_int(bl00mbox_channel_get_signal_by_subscriber_list_pos(
            mp_obj_get_int(args[0]), //chan
            mp_obj_get_int(args[1]), //bud_index
            mp_obj_get_int(args[2]), //bud_signal_index
            mp_obj_get_int(args[3])) //pos
    );
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_channel_get_signal_by_subscriber_list_pos_obj, 4, 4, mp_channel_get_signal_by_subscriber_list_pos);

STATIC mp_obj_t mp_channel_get_source_bud(mp_obj_t chan, mp_obj_t bud, mp_obj_t signal) {
    uint64_t val = bl00mbox_channel_get_source_bud(
            mp_obj_get_int(chan),
            mp_obj_get_int(bud),
            mp_obj_get_int(signal));
    return mp_obj_new_int(val);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(mp_channel_get_source_bud_obj, mp_channel_get_source_bud);

STATIC mp_obj_t mp_channel_get_source_signal(mp_obj_t chan, mp_obj_t bud, mp_obj_t signal) {
    uint64_t val = bl00mbox_channel_get_source_signal(
            mp_obj_get_int(chan),
            mp_obj_get_int(bud),
            mp_obj_get_int(signal));
    return mp_obj_new_int(val);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(mp_channel_get_source_signal_obj, mp_channel_get_source_signal);


// ========================
//      TABLE OPERATIONS
// ========================

STATIC mp_obj_t mp_channel_bud_set_table_value(size_t n_args, const mp_obj_t *args) {
    bool success = bl00mbox_channel_bud_set_table_value(
            mp_obj_get_int(args[0]), //chan
            mp_obj_get_int(args[1]), //bud_index
            mp_obj_get_int(args[2]), //table_index
            mp_obj_get_int(args[3])); //value
    return mp_obj_new_bool(success);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_channel_bud_set_table_value_obj, 4, 4, mp_channel_bud_set_table_value);

STATIC mp_obj_t mp_channel_bud_get_table_value(mp_obj_t chan, mp_obj_t bud, mp_obj_t table_index) {
    int16_t val = bl00mbox_channel_bud_get_table_value(
            mp_obj_get_int(chan),
            mp_obj_get_int(bud),
            mp_obj_get_int(table_index));
    return mp_obj_new_int(val);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(mp_channel_bud_get_table_value_obj, mp_channel_bud_get_table_value);

STATIC mp_obj_t mp_channel_bud_get_table_len(mp_obj_t chan, mp_obj_t bud) {
    uint32_t val = bl00mbox_channel_bud_get_table_len(mp_obj_get_int(chan), mp_obj_get_int(bud));
    return mp_obj_new_int(val);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mp_channel_bud_get_table_len_obj, mp_channel_bud_get_table_len);



// ========================
//  CONNECTION OPERATIONS
// ========================

STATIC mp_obj_t mp_channel_disconnect_signal_rx(mp_obj_t chan, mp_obj_t bud, mp_obj_t signal) {
    bool ret = bl00mbox_channel_disconnect_signal_rx(mp_obj_get_int(chan), mp_obj_get_int(bud), mp_obj_get_int(signal));
    return mp_obj_new_bool(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(mp_channel_disconnect_signal_rx_obj, mp_channel_disconnect_signal_rx);

STATIC mp_obj_t mp_channel_disconnect_signal_tx(mp_obj_t chan, mp_obj_t bud, mp_obj_t signal) {
    bool ret = bl00mbox_channel_disconnect_signal_tx(mp_obj_get_int(chan), mp_obj_get_int(bud), mp_obj_get_int(signal));
    return mp_obj_new_bool(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(mp_channel_disconnect_signal_tx_obj, mp_channel_disconnect_signal_tx);

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

STATIC mp_obj_t mp_channel_connect_signal_to_output_mixer(mp_obj_t chan, mp_obj_t bud_index, mp_obj_t bud_signal_index) {
    bool success = bl00mbox_channel_connect_signal_to_output_mixer(
            mp_obj_get_int(chan), mp_obj_get_int(bud_index), mp_obj_get_int(bud_signal_index));
    return mp_obj_new_bool(success);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(mp_channel_connect_signal_to_output_mixer_obj, mp_channel_connect_signal_to_output_mixer);

STATIC mp_obj_t mp_channel_disconnect_signal_from_output_mixer(mp_obj_t chan, mp_obj_t bud, mp_obj_t signal) {
    bool ret = bl00mbox_channel_disconnect_signal_from_output_mixer(mp_obj_get_int(chan), mp_obj_get_int(bud), mp_obj_get_int(signal));
    return mp_obj_new_bool(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(mp_channel_disconnect_signal_from_output_mixer_obj, mp_channel_disconnect_signal_from_output_mixer);



STATIC const mp_map_elem_t bl00mbox_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_sys_bl00mbox) },

    // PLUGIN OPERATIONS
    { MP_ROM_QSTR(MP_QSTR_plugin_registry_num_plugins), MP_ROM_PTR(&mp_plugin_registry_num_plugins_obj) },
    { MP_ROM_QSTR(MP_QSTR_plugin_index_get_id), MP_ROM_PTR(&mp_plugin_index_get_id_obj) },
    { MP_ROM_QSTR(MP_QSTR_plugin_index_get_name), MP_ROM_PTR(&mp_plugin_index_get_name_obj) },
    { MP_ROM_QSTR(MP_QSTR_plugin_index_get_description), MP_ROM_PTR(&mp_plugin_index_get_description_obj) },

    // CHANNEL OPERATIONS
    { MP_ROM_QSTR(MP_QSTR_channel_get_free), MP_ROM_PTR(&mp_channel_get_free_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_get_foreground), MP_ROM_PTR(&mp_channel_get_foreground_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_enable), MP_ROM_PTR(&mp_channel_enable_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_disable), MP_ROM_PTR(&mp_channel_disable_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_clear), MP_ROM_PTR(&mp_channel_clear_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_set_volume), MP_ROM_PTR(&mp_channel_set_volume_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_get_volume), MP_ROM_PTR(&mp_channel_get_volume_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_buds_num), MP_ROM_PTR(&mp_channel_buds_num_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_get_bud_by_list_pos), MP_ROM_PTR(&mp_channel_get_bud_by_list_pos_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_conns_num), MP_ROM_PTR(&mp_channel_conns_num_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_mixer_num), MP_ROM_PTR(&mp_channel_mixer_num_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_get_bud_by_mixer_list_pos), MP_ROM_PTR(&mp_channel_get_bud_by_mixer_list_pos_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_get_signal_by_mixer_list_pos), MP_ROM_PTR(&mp_channel_get_signal_by_mixer_list_pos_obj) },

    // BUD OPERATIONS
    { MP_ROM_QSTR(MP_QSTR_channel_new_bud), MP_ROM_PTR(&mp_channel_new_bud_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_delete_bud), MP_ROM_PTR(&mp_channel_delete_bud_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_bud_exists), MP_ROM_PTR(&mp_channel_bud_exists_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_bud_get_name), MP_ROM_PTR(&mp_channel_bud_get_name_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_bud_get_description), MP_ROM_PTR(&mp_channel_bud_get_description_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_bud_get_plugin_id), MP_ROM_PTR(&mp_channel_bud_get_plugin_id_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_bud_get_num_signals), MP_ROM_PTR(&mp_channel_bud_get_num_signals_obj) },

    // SIGNAL OPERATIONS
    { MP_ROM_QSTR(MP_QSTR_channel_bud_get_signal_name), MP_ROM_PTR(&mp_channel_bud_get_signal_name_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_bud_get_signal_description), MP_ROM_PTR(&mp_channel_bud_get_signal_description_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_bud_get_signal_unit), MP_ROM_PTR(&mp_channel_bud_get_signal_unit_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_bud_set_signal_value), MP_ROM_PTR(&mp_channel_bud_set_signal_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_bud_get_signal_value), MP_ROM_PTR(&mp_channel_bud_get_signal_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_bud_get_signal_hints), MP_ROM_PTR(&mp_channel_bud_get_signal_hints_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_subscriber_num), MP_ROM_PTR(&mp_channel_subscriber_num_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_get_bud_by_subscriber_list_pos), MP_ROM_PTR(&mp_channel_get_bud_by_subscriber_list_pos_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_get_signal_by_subscriber_list_pos), MP_ROM_PTR(&mp_channel_get_signal_by_subscriber_list_pos_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_get_source_bud), MP_ROM_PTR(&mp_channel_get_source_bud_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_get_source_signal), MP_ROM_PTR(&mp_channel_get_source_signal_obj) },

    // TABLE OPERATIONS
    { MP_ROM_QSTR(MP_QSTR_channel_bud_set_table_value), MP_ROM_PTR(&mp_channel_bud_set_table_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_bud_get_table_value), MP_ROM_PTR(&mp_channel_bud_get_table_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_bud_get_table_len), MP_ROM_PTR(&mp_channel_bud_get_table_len_obj) },

    // CONNECTION OPERATIONS
    { MP_ROM_QSTR(MP_QSTR_channel_connect_signal), MP_ROM_PTR(&mp_channel_connect_signal_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_disconnect_signal_rx), MP_ROM_PTR(&mp_channel_disconnect_signal_rx_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_disconnect_signal_tx), MP_ROM_PTR(&mp_channel_disconnect_signal_tx_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_connect_signal_to_output_mixer), MP_ROM_PTR(&mp_channel_connect_signal_to_output_mixer_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_disconnect_signal_from_output_mixer), MP_ROM_PTR(&mp_channel_disconnect_signal_from_output_mixer_obj) },

    // CONSTANTS
    {MP_ROM_QSTR(MP_QSTR_NUM_CHANNELS), MP_ROM_INT(BL00MBOX_CHANNELS)},
};

STATIC MP_DEFINE_CONST_DICT(mp_module_bl00mbox_globals, bl00mbox_globals_table);

const mp_obj_module_t bl00mbox_user_cmodule = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&mp_module_bl00mbox_globals,
};

MP_REGISTER_MODULE(MP_QSTR_sys_bl00mbox, bl00mbox_user_cmodule);
