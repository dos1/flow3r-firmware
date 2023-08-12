#include "st3m_badgenet_switch.h"

#include <string.h>

void st3m_badgenet_switch_gc(st3m_badgenet_switch_t *sw, int64_t now) {
    uint32_t active = 0;
    for (int i = 0; i < ARP_ENTRIES; i++) {
        st3m_badgenet_arp_entry_t *entry = &sw->arp_table[i];
        if (entry->destination == st3m_badgenet_switch_port_invalid) {
            continue;
        }
        if (sw->arp_table[i].expires < now) {
            entry->destination = st3m_badgenet_switch_port_invalid;
            continue;
        }
        active++;
    }
    sw->arp_table_overflow = active == ARP_ENTRIES;
}

void st3m_badgenet_switch_learn(st3m_badgenet_switch_t *sw, int64_t now,
                                const uint8_t *mac,
                                st3m_badgenet_switch_port_t dest) {
    st3m_badgenet_switch_gc(sw, now);
    if (sw->arp_table_overflow) {
        return;
    }
    int available = -1;
    // Try to update an existing entry. Also make note of any free entry.
    for (int i = 0; i < ARP_ENTRIES; i++) {
        st3m_badgenet_arp_entry_t *entry = &sw->arp_table[i];
        if (memcmp(entry->mac, mac, 6) == 0) {
            entry->destination = dest;
            entry->expires = now + ARP_ENTRY_AGE_US;
            return;
        }
        if (available == -1 &&
            entry->destination == st3m_badgenet_switch_port_invalid) {
            available = i;
        }
    }

    if (available == -1) {
        sw->arp_table_overflow = true;
        return;
    }
    st3m_badgenet_arp_entry_t *entry = &sw->arp_table[available];
    memcpy(entry->mac, mac, 6);
    entry->destination = dest;
    entry->expires = now + ARP_ENTRY_AGE_US;
}

st3m_badgenet_switch_port_t st3m_badgenet_switch_get(
    const st3m_badgenet_switch_t *sw, const uint8_t *mac) {
    for (int i = 0; i < ARP_ENTRIES; i++) {
        const st3m_badgenet_arp_entry_t *entry = &sw->arp_table[i];
        if (entry->destination == st3m_badgenet_switch_port_invalid) {
            continue;
        }
        if (memcmp(entry->mac, mac, 6) != 0) {
            continue;
        }

        return entry->destination;
    }
    return st3m_badgenet_switch_port_flood;
}

void st3m_badgenet_switch_flood_ports_for_src(
    st3m_badgenet_switch_port_t src, st3m_badgenet_switch_port_t *out) {
    int i = 0;
    if (src != st3m_badgenet_switch_port_left) {
        out[i++] = st3m_badgenet_switch_port_left;
    }
    if (src != st3m_badgenet_switch_port_right) {
        out[i++] = st3m_badgenet_switch_port_right;
    }
    if (src != st3m_badgenet_switch_port_local) {
        out[i++] = st3m_badgenet_switch_port_local;
    }
    out[i++] = st3m_badgenet_switch_port_invalid;
}

const char *st3m_badgenet_switch_port_name(st3m_badgenet_switch_port_t dst) {
    switch (dst) {
        case st3m_badgenet_switch_port_flood:
            return "flood";
        case st3m_badgenet_switch_port_invalid:
            return "invalid";
        case st3m_badgenet_switch_port_left:
            return "left jack";
        case st3m_badgenet_switch_port_right:
            return "right jack";
        case st3m_badgenet_switch_port_local:
            return "local";
    }
    assert(0);  // Unreachable
}
