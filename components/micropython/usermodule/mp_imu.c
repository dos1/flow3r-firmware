#include "st3m_imu.h"

#include "py/builtin.h"
#include "py/runtime.h"

STATIC mp_obj_t mp_imu_acc_read(void) {
    float x, y, z;

    esp_err_t ret = st3m_imu_read_acc_mps(&x, &y, &z);

    if (ret == ESP_OK) {
        mp_obj_t items[3] = { mp_obj_new_float(x), mp_obj_new_float(y),
                              mp_obj_new_float(z) };
        return mp_obj_new_tuple(3, items);
    } else if (ret == ESP_ERR_NOT_FOUND) {
        return mp_const_none;
    }

    // TODO: raise exception here
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_imu_acc_read_obj, mp_imu_acc_read);

STATIC const mp_rom_map_elem_t globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_acc_read), MP_ROM_PTR(&mp_imu_acc_read_obj) },
};

STATIC MP_DEFINE_CONST_DICT(globals, globals_table);

const mp_obj_module_t mp_module_imu_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&globals,
};

MP_REGISTER_MODULE(MP_QSTR_imu, mp_module_imu_user_cmodule);
