#include <stdio.h>
#include <string.h>

#include "extmod/virtpin.h"
#include "machine_rtc.h"
#include "modmachine.h"
#include "modnetwork.h"
#include "mphalport.h"
#include "py/builtin.h"
#include "py/mphal.h"
#include "py/runtime.h"

#include "st3m_badgenet.h"
#include "st3m_badgenet_switch.h"

typedef struct {
    base_if_obj_t base;
} badgenet_iface_obj_t;

const mp_obj_type_t badgenet_iface_type;

STATIC badgenet_iface_obj_t iface = {
    .base = {
        .base = {
            .type = &badgenet_iface_type,
        },
        .if_id = ESP_IF_ETH,
        .netif = NULL,
    },
};

STATIC mp_obj_t _get_switch_table(void) {
    st3m_badgenet_switch_t sw;
    st3m_badgenet_switch(&sw);
    mp_obj_t res = mp_obj_new_list(0, NULL);
    int64_t now = esp_timer_get_time();
    for (int i = 0; i < ARP_ENTRIES; i++) {
        st3m_badgenet_arp_entry_t *entry = &sw.arp_table[i];
        if (entry->destination == st3m_badgenet_switch_port_invalid) {
            continue;
        }
        mp_obj_t destination = mp_const_none;
        switch (entry->destination) {
            case st3m_badgenet_switch_port_left:
                destination = MP_ROM_QSTR(MP_QSTR_left);
                break;
            case st3m_badgenet_switch_port_right:
                destination = MP_ROM_QSTR(MP_QSTR_right);
                break;
            case st3m_badgenet_switch_port_local:
                destination = MP_ROM_QSTR(MP_QSTR_local);
                break;
            default:
                break;
        }
        char mac_str[6 * 12 + 5 + 1];
        uint8_t *m = entry->mac;
        snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
                 m[0], m[1], m[2], m[3], m[4], m[5]);
        mp_obj_t mac = mp_obj_new_str(mac_str, strlen(mac_str));
        mp_obj_t us_left = mp_obj_new_int_from_ll(entry->expires - now);
        mp_obj_t tuple[3] = {
            mac,
            destination,
            us_left,
        };
        mp_obj_list_append(res, mp_obj_new_tuple(3, tuple));
    }
    return res;
}

MP_DEFINE_CONST_FUN_OBJ_0(get_switch_table_obj, _get_switch_table);

STATIC mp_obj_t _interface_name(mp_obj_t self_in) {
    badgenet_iface_obj_t *self = MP_OBJ_TO_PTR(self_in);
    char name[6] = { 0 };
    esp_netif_get_netif_impl_name(self->base.netif, name);
    return mp_obj_new_str(name, strlen(name));
}

MP_DEFINE_CONST_FUN_OBJ_1(interface_name_obj, _interface_name);

STATIC const mp_rom_map_elem_t badgenet_iface_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_ifconfig6), MP_ROM_PTR(&esp_network_ifconfig6_obj) },
    { MP_ROM_QSTR(MP_QSTR_name), MP_ROM_PTR(&interface_name_obj) },
};

STATIC MP_DEFINE_CONST_DICT(badgenet_iface_locals_dict,
                            badgenet_iface_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(badgenet_iface_type, MP_QSTR_BadgenetInterface,
                         MP_TYPE_FLAG_NONE, locals_dict,
                         &badgenet_iface_locals_dict);

STATIC mp_obj_t _get_interface(void) {
    badgenet_iface_obj_t *self = &iface;
    if (self->base.netif != NULL) {
        return MP_OBJ_FROM_PTR(self);
    }

    self->base.netif = st3m_badgenet_netif();
    if (self->base.netif == NULL) {
        return mp_const_none;
    }
    return MP_OBJ_FROM_PTR(self);
}

MP_DEFINE_CONST_FUN_OBJ_0(get_interface_obj, _get_interface);

STATIC mp_obj_t _configure_jack(mp_obj_t jack_side_in, mp_obj_t jack_mode_in) {
    bool left = mp_obj_equal(jack_side_in, MP_ROM_QSTR(MP_QSTR_LEFT));
    bool right = mp_obj_equal(jack_side_in, MP_ROM_QSTR(MP_QSTR_RIGHT));
    if (!left && !right) {
        mp_raise_ValueError("side must be one of: badgenet.SIDE_{LEFT,RIGHT}");
        return mp_const_none;
    }
    st3m_badgenet_jack_side_t side =
        left ? st3m_badgenet_jack_left : st3m_badgenet_jack_right;

    bool disable = mp_obj_equal(jack_mode_in, MP_ROM_QSTR(MP_QSTR_DISABLE));
    bool enable_auto = mp_obj_equal(jack_mode_in, MP_ROM_QSTR(MP_QSTR_AUTO));
    bool enable_tx_tip =
        mp_obj_equal(jack_mode_in, MP_ROM_QSTR(MP_QSTR_TX_TIP));
    bool enable_tx_ring =
        mp_obj_equal(jack_mode_in, MP_ROM_QSTR(MP_QSTR_TX_RING));
    if (!disable && !enable_auto && !enable_tx_tip && !enable_tx_ring) {
        mp_raise_ValueError(
            "mode must be one of: "
            "badgenet.MODE_{DISABLE,ENABLE_AUTO,ENABLE_TX_TIP,ENABLE_TX_RING}");
        return mp_const_none;
    }
    st3m_badgenet_jack_mode_t mode = st3m_badgenet_jack_mode_disabled;
    if (enable_auto) mode = st3m_badgenet_jack_mode_enabled_auto;
    if (enable_tx_tip) mode = st3m_badgenet_jack_mode_enabled_tx_tip;
    if (enable_tx_ring) mode = st3m_badgenet_jack_mode_enabled_tx_ring;

    esp_err_t ret = st3m_badgenet_jack_configure(side, mode);
    if (ret != ESP_OK) {
        mp_raise_msg_varg(&mp_type_Exception, "jack configuration failed: %s",
                          esp_err_to_name(ret));
    }
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_2(configure_jack_obj, _configure_jack);

STATIC const mp_rom_map_elem_t mp_module_badgenet_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_get_interface), MP_ROM_PTR(&get_interface_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_switch_table),
      MP_ROM_PTR(&get_switch_table_obj) },
    { MP_ROM_QSTR(MP_QSTR_configure_jack), MP_ROM_PTR(&configure_jack_obj) },

    { MP_ROM_QSTR(MP_QSTR_SIDE_LEFT), MP_ROM_QSTR(MP_QSTR_LEFT) },
    { MP_ROM_QSTR(MP_QSTR_SIDE_RIGHT), MP_ROM_QSTR(MP_QSTR_RIGHT) },

    { MP_ROM_QSTR(MP_QSTR_MODE_DISABLE), MP_ROM_QSTR(MP_QSTR_DISABLE) },
    { MP_ROM_QSTR(MP_QSTR_MODE_ENABLE_AUTO), MP_ROM_QSTR(MP_QSTR_AUTO) },
    { MP_ROM_QSTR(MP_QSTR_MODE_ENABLE_TX_TIP), MP_ROM_QSTR(MP_QSTR_TX_TIP) },
    { MP_ROM_QSTR(MP_QSTR_MODE_ENABLE_TX_RING), MP_ROM_QSTR(MP_QSTR_TX_RING) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_badgenet_globals,
                            mp_module_badgenet_globals_table);

const mp_obj_module_t mp_module_badgenet = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_badgenet_globals,
};

MP_REGISTER_MODULE(MP_QSTR_badgenet, mp_module_badgenet);
