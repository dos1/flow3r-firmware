#pragma once

// Badge Net is a second-layer (OSI layer 1.5ish) protocol on top of Badge Link.
//
// It runs Ethernet over the 3.5mm jacks, which effectively allows users to
// send/receive plain IPv6 packets over Badge Link.
//
// This is currently implemented using SLIP over UART.
//
// See docs/badge/badgenet.rst for more information.

#include "esp_err.h"
#include "esp_netif.h"

// Initialize badgelink network interface and internal switch / bridge.
//
// This initialization is independent of jack/badgelink configuration.
//
// Must be called exactly once.
esp_err_t st3m_badgenet_init(void);

// Return the ESP-IDF esp_netif object for the initialized badgelink interface.
esp_netif_t *st3m_badgenet_netif(void);

// Full definition in st3m_badgenet_switch.h.
typedef struct _st3m_badgenet_switch_t st3m_badgenet_switch_t;

// Return copy of switch state. Useful for debugging MAC table.
void st3m_badgenet_switch(st3m_badgenet_switch_t *out);

// Get name of badgenet interface. `out` needs to be able to hold at least 6
// characters.
//
// Usually 'bl1'.
void st3m_badgenet_interface_name(char *out);

typedef enum {
    st3m_badgenet_jack_left = 0,
    st3m_badgenet_jack_right = 1,
} st3m_badgenet_jack_side_t;

typedef enum {
    // Disable Badge Net on this jack.
    st3m_badgenet_jack_mode_disabled = 0,
    // Enable Badge Net on this jack, use tip to transmit on left jack, and ring
    // to transmit on right jack. This allows left-to-right connectivity between
    // badges.
    st3m_badgenet_jack_mode_enabled_auto = 1,
    // Enable Badge Net on this jack, use tip to transmit and ring to receive.
    // The other side has to be configured with tx_ring.
    st3m_badgenet_jack_mode_enabled_tx_tip = 2,
    // Enable Badge Net on this jack, use ring to transmit and tip to receive.
    // The other side has to be configured with tx_tip.
    st3m_badgenet_jack_mode_enabled_tx_ring = 3,
} st3m_badgenet_jack_mode_t;

// Configure Badge Net on the given jack.
//
// Once configured, Badge Link still needs to be enabled on the corresponding
// jack (this does _not_ mux out the UART onto the physical jack)>
esp_err_t st3m_badgenet_jack_configure(st3m_badgenet_jack_side_t side,
                                       st3m_badgenet_jack_mode_t mode);