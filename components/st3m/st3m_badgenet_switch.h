#pragma once

// Tiny little L2 Ethernet switch. Supports basic MAC learning. Please be
// gentle.
//
// Writing this was easier than dealing with esp_netif/LwIP.

#include <stdbool.h>
#include <stdint.h>

#define ARP_ENTRIES 16
// 10 seconds
#define ARP_ENTRY_AGE_US 10000000

// Destination/source port for a packet.
typedef enum {
    st3m_badgenet_switch_port_invalid = 0,
    // Local network stack.
    st3m_badgenet_switch_port_local = 1,
    // Left jack.
    st3m_badgenet_switch_port_left = 2,
    // Right jack.
    st3m_badgenet_switch_port_right = 3,
    // Psuedo-port: switching decision to flood.
    st3m_badgenet_switch_port_flood = 4,
} st3m_badgenet_switch_port_t;

// MAC table entry.
typedef struct {
    // Destination MAC address.
    uint8_t mac[6];
    // Where packets should be sent to for this MAC. _invalid means this entry
    // is inactive.
    st3m_badgenet_switch_port_t destination;
    // Timestamp in microseconds when this entry should be considered expired.
    int64_t expires;
} st3m_badgenet_arp_entry_t;

typedef struct _st3m_badgenet_switch_t {
    st3m_badgenet_arp_entry_t arp_table[ARP_ENTRIES];
    // True if the MAC table is saturated and switch should act like a hub (fast
    // path).
    bool arp_table_overflow;
} st3m_badgenet_switch_t;

// Find MAC table entries that have expired and mark them _invalid / inactive.
void st3m_badgenet_switch_gc(st3m_badgenet_switch_t *sw, int64_t now);
// Tell switch that a given port has just sent a packet with a source MAC.
void st3m_badgenet_switch_learn(st3m_badgenet_switch_t *sw, int64_t now,
                                const uint8_t *mac,
                                st3m_badgenet_switch_port_t dest);
// Get switching decision for given destination MAC.
st3m_badgenet_switch_port_t st3m_badgenet_switch_get(
    const st3m_badgenet_switch_t *sw, const uint8_t *mac);
// Get ports that should be used to flood when a given src port has sent a
// packet. out should handle 4 entries. The last entry will be an _invalid.
void st3m_badgenet_switch_flood_ports_for_src(st3m_badgenet_switch_port_t src,
                                              st3m_badgenet_switch_port_t *out);
// Get user-readable name for a given port.
const char *st3m_badgenet_switch_port_name(st3m_badgenet_switch_port_t dst);