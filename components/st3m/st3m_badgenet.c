#include "st3m_badgenet.h"

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_timer.h"

#include "lwip/err.h"
#include "lwip/esp_netif_net_stack.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "flow3r_bsp.h"
#include "st3m_badgenet_switch.h"

#include <string.h>

static const char *TAG = "st3m-badgenet";
static esp_netif_t *_netif = NULL;
static SemaphoreHandle_t _mu = NULL;

#define LOCK                                         \
    do {                                             \
        xSemaphoreTakeRecursive(_mu, portMAX_DELAY); \
    } while (0)
#define UNLOCK                        \
    do {                              \
        xSemaphoreGiveRecursive(_mu); \
    } while (0)

#define RXBUF_SIZE 1500

// Ethernet frame header. Used to parse out dest/source MAC addresses.
typedef struct {
    uint8_t destmac[6];
    uint8_t srcmac[6];
    uint16_t ethertype;
    uint32_t crc32;
} eth_frame_t;

// Returns true if a given MAC address is a unicast address.
static bool _mac_unicast(uint8_t *mac) { return (mac[0] & 1) == 0; }

// State of a jack for Badge Net purposes.
typedef struct {
    // UART peripheral number driving this jack.
    uart_port_t uart_num;

    // Receive buffer on heap, handed over to LwIP once ready.
    uint8_t *rxbuf;
    // Write index into rxbuf.
    size_t rxbuf_ix;

    // Whether the jack has just received a SLIP escape sequence.
    bool in_escape;
} st3m_badgenet_jack_t;

// Reset jack state to allow receiving packets.
static void _jack_reset(st3m_badgenet_jack_t *jack) {
    if (jack->rxbuf != NULL) {
        free(jack->rxbuf);
        jack->rxbuf = NULL;
    }
    jack->rxbuf_ix = 0;
    jack->in_escape = false;
}

// Badge Net state and esp_netif driver.
typedef struct {
    // Base structure for esp_netif purposes.
    esp_netif_driver_base_t base;

    // Left and right jacks, or NULL if not configured.
    st3m_badgenet_jack_t *left;
    st3m_badgenet_jack_t *right;

    // Switch/Bridge.
    st3m_badgenet_switch_t sw;

} st3m_badgenet_driver_t;

static esp_err_t _transmit_jack(st3m_badgenet_driver_t *driver,
                                st3m_badgenet_jack_t *jack, const void *data,
                                size_t len);

// Main packet forwarding functin. Called with packet and information about
// source port.
//
// Forwards packet either to jacks or to local networking stack.
static void _forward(st3m_badgenet_driver_t *driver,
                     st3m_badgenet_switch_port_t src, const void *data,
                     size_t len) {
    LOCK;

    // TODO(q3k): fast track local MAC instead of going through switch.

    eth_frame_t *hdr = (eth_frame_t *)data;
    // Mac 3 destination ports + invalid marker.
    st3m_badgenet_switch_port_t dst[4];
    int64_t now = esp_timer_get_time();

    bool dst_unicast = _mac_unicast(hdr->destmac);
    bool src_unicast = _mac_unicast(hdr->srcmac);

    if (src_unicast) {
        st3m_badgenet_switch_learn(&driver->sw, now, hdr->srcmac, src);
    }

    if (!dst_unicast) {
        // Fast path: don't look up non-unicast addresses, just flod.
        st3m_badgenet_switch_flood_ports_for_src(src, dst);
    } else {
        // Look up unicast addresses in switch.
        dst[0] = st3m_badgenet_switch_get(&driver->sw, hdr->destmac);
        dst[1] = st3m_badgenet_switch_port_invalid;
        // Unknown address -> flood anyway.
        if (dst[0] == st3m_badgenet_switch_port_flood) {
            st3m_badgenet_switch_flood_ports_for_src(src, dst);
        }
    }

    // Send to destination ports.
    for (int i = 0; i < 4; i++) {
        esp_err_t ret = ESP_OK;
        bool done = false;
        switch (dst[i]) {
            case st3m_badgenet_switch_port_left:
                ret = _transmit_jack(driver, driver->left, data, len);
                break;
            case st3m_badgenet_switch_port_right:
                ret = _transmit_jack(driver, driver->right, data, len);
                break;
            case st3m_badgenet_switch_port_local:
                ret = esp_netif_receive(driver->base.netif, (void *)data, len,
                                        NULL);
                break;
            case st3m_badgenet_switch_port_invalid:
                done = true;
                break;
            default:
                ret = ESP_ERR_INVALID_ARG;
        }

        if (done) {
            UNLOCK;
            return;
        }
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to transmit to %s: %s",
                     st3m_badgenet_switch_port_name(dst[i]),
                     esp_err_to_name(ret));
        }
    }

    UNLOCK;
}

// Called when a jack has received a complete SLIP packet.
static void _receive_jack_finish(st3m_badgenet_driver_t *driver,
                                 st3m_badgenet_jack_t *jack) {
    if (jack->rxbuf_ix < sizeof(eth_frame_t)) {
        ESP_LOGW(TAG, "Received packet was too short (%d bytes)",
                 jack->rxbuf_ix);
        jack->rxbuf_ix = 0;
        return;
    }
    st3m_badgenet_switch_port_t src =
        (jack == driver->left ? st3m_badgenet_switch_port_left
                              : st3m_badgenet_switch_port_right);
    _forward(driver, src, jack->rxbuf, jack->rxbuf_ix);
    jack->rxbuf = NULL;
    jack->rxbuf_ix = 0;
}

// Called when a jack UART receive buffer should be drained. When the packet is
// complete, _jack_receive_finish is called, which in turn calls _forward.
static void _receive_jack(st3m_badgenet_driver_t *driver,
                          st3m_badgenet_jack_t *jack) {
    while (true) {
        if (jack->rxbuf == NULL) {
            jack->rxbuf = malloc(RXBUF_SIZE);
            assert(jack->rxbuf != NULL);
            jack->rxbuf_ix = 0;
        }
        if (jack->rxbuf_ix >= RXBUF_SIZE - 1) {
            ESP_LOGW(TAG, "Received packet was higher than MTU (%d)",
                     RXBUF_SIZE);
            jack->rxbuf_ix = 0;
        }

        uint8_t byte;
        int read = uart_read_bytes(jack->uart_num, &byte, 1, 0);
        if (read <= 0) {
            return;
        }

        if (jack->in_escape) {
            if (byte == 0xDC) {
                jack->rxbuf[jack->rxbuf_ix++] = 0xc0;
            } else if (byte == 0xDD) {
                jack->rxbuf[jack->rxbuf_ix++] = 0xdb;
            } else {
                ESP_LOGW(TAG, "Invalid escape sequence received: 0xDB 0x%02X",
                         byte);
                jack->rxbuf_ix = 0;
            }
            jack->in_escape = false;
        } else {
            if (byte == 0xDB) {
                jack->in_escape = true;
            } else if (byte == 0xC0) {
                _receive_jack_finish(driver, jack);
            } else {
                jack->rxbuf[jack->rxbuf_ix++] = byte;
            }
        }
    }
}

// Called by LwIP when it's done with a buffer. We malloc() them, so here we
// just free them.
static void _free_rx_buffer(void *h, void *buffer) { free(buffer); }

// Encode an ethernet frame and send over SLIP over UART for a jack.
static esp_err_t _transmit_jack(st3m_badgenet_driver_t *driver,
                                st3m_badgenet_jack_t *jack, const void *data,
                                size_t len) {
    if (jack == NULL) {
        // Nothing to do if the jack isn't configured.
        return ESP_OK;
    }

    // Send chunks of data, but SLIP-encode (RFC 1055) them on the way there.
    const uint8_t *bytes = data;
    const uint8_t *chunk_start = bytes;
    size_t chunk_len = 0;
    while (len > 0) {
        uint8_t byte = *bytes;
        switch (byte) {
            case 0xc0:  // END
                if (chunk_len > 0) {
                    uart_write_bytes(jack->uart_num, chunk_start, chunk_len);
                    chunk_len = 0;
                }
                bytes++;
                len--;
                chunk_start = bytes;
                // Transmit ESC, ESC_END
                uart_write_bytes(jack->uart_num, "\xdb\xdc", 2);
                break;
            case 0xdb:  // ESC
                if (chunk_len > 0) {
                    uart_write_bytes(jack->uart_num, chunk_start, chunk_len);
                    chunk_len = 0;
                }
                bytes++;
                len--;
                chunk_start = bytes;
                // Transmit ESC, ESC_ESC
                uart_write_bytes(jack->uart_num, "\xdb\xdd", 2);
                break;
            default:
                chunk_len++;
                bytes++;
                len--;
        }
    }
    if (chunk_len > 0) {
        // Transmit leftover chunk.
        uart_write_bytes(jack->uart_num, chunk_start, chunk_len);
    }
    // Transmit END
    uart_write_bytes(jack->uart_num, "\xc0", 1);
    return ESP_OK;
}

// Called when local network stack wishes to transmit over badgelink. Thin
// wrapper around _forward setting the source port to local.
static esp_err_t _transmit(void *h, void *data, size_t len) {
    st3m_badgenet_driver_t *driver = h;
    if (len < sizeof(eth_frame_t)) {
        ESP_LOGE(TAG, "Cannot send short frame: %d bytes", len);
        return ESP_FAIL;
    }

    _forward(driver, st3m_badgenet_switch_port_local, data, len);
    return ESP_OK;
}

// Called when local network stack has finished initializing Badge Net
// interface.
static esp_err_t _post_attach(esp_netif_t *esp_netif, void *args) {
    st3m_badgenet_driver_t *driver = args;
    const esp_netif_driver_ifconfig_t driver_ifconfig = {
        .driver_free_rx_buffer = _free_rx_buffer,
        .transmit = _transmit,
        .handle = args,
    };
    driver->base.netif = esp_netif;
    ESP_ERROR_CHECK(esp_netif_set_driver_config(esp_netif, &driver_ifconfig));

    // Configure MAC address, stealing ethernet MAC address.
    uint8_t mac[6];
    memset(mac, 0, 6);
    esp_read_mac(mac, ESP_MAC_ETH);
    esp_netif_set_mac(esp_netif, mac);

    // Set interface up.
    esp_netif_action_start(esp_netif, NULL, 0, NULL);
    esp_netif_action_connected(esp_netif, NULL, 0, NULL);

    // Give interface a link-local address.
    esp_err_t ret;
    if ((ret = esp_netif_create_ip6_linklocal(esp_netif)) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to generate link-local IPv6 address: %s",
                 esp_err_to_name(ret));
    }
    char name[6] = { 0 };
    esp_netif_get_netif_impl_name(esp_netif, name);
    ESP_LOGI(TAG, "Badge Net interface is %s", name);
    return ESP_OK;
}

// Singleton objects for driver and jacks.

static st3m_badgenet_jack_t _right = {
    .uart_num = UART_NUM_1,
};

static st3m_badgenet_jack_t _left = {
    .uart_num = UART_NUM_2,
};

static st3m_badgenet_driver_t _driver = {
    .base = {
        .post_attach = _post_attach,
    },
    .sw = {},
};

// Main processing task. Polls jack UART receive queues.
static void _task(void *data) {
    (void)data;

    TickType_t last_wake = xTaskGetTickCount();
    while (1) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(10));  // 100 Hz
        LOCK;
        if (_driver.left != NULL) {
            _receive_jack(&_driver, _driver.left);
        }
        if (_driver.right != NULL) {
            _receive_jack(&_driver, _driver.right);
        }
        UNLOCK;
    }
}

// Wraps ethernetif_init from esp_netif but sets the name to 'bl'.
err_t _badgelink_if_init(struct netif *netif) {
    err_t ret = ethernetif_init(netif);
    if (ret != ERR_OK) {
        return ret;
    }

    netif->name[0] = 'b';
    netif->name[1] = 'l';
    return ERR_OK;
}

static esp_netif_netstack_config_t _stack_config;
static esp_netif_inherent_config_t _inherent_config;

esp_err_t st3m_badgenet_init(void) {
    esp_err_t ret;

    assert(_mu == NULL);
    _mu = xSemaphoreCreateRecursiveMutex();
    assert(_mu != NULL);

    if ((ret = esp_netif_init()) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize netif: %s", esp_err_to_name(ret));
        return ret;
    }

    // Use default ethernet configuration...
    esp_netif_config_t netif_cfg = ESP_NETIF_DEFAULT_ETH();

    // ... but override init function to set name to 'bl'...
    memcpy(&_stack_config, netif_cfg.stack, sizeof(*netif_cfg.stack));
    netif_cfg.stack = &_stack_config;
    _stack_config.lwip.init_fn = _badgelink_if_init;

    // ... and also disable DHCP. No legacy protocols allowed.
    memcpy(&_inherent_config, netif_cfg.base, sizeof(*netif_cfg.base));
    netif_cfg.base = &_inherent_config;
    _inherent_config.flags &= (~ESP_NETIF_DHCP_CLIENT);

    // Start/attach interface.
    esp_netif_t *eth_netif = esp_netif_new(&netif_cfg);
    if ((ret = esp_netif_attach(eth_netif, &_driver)) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to attach driver: %s", esp_err_to_name(ret));
        return ret;
    }

    xTaskCreate(&_task, "badgenet", 4096, NULL, configMAX_PRIORITIES - 2, NULL);
    _netif = eth_netif;

    return ESP_OK;
}

esp_netif_t *st3m_badgenet_netif(void) { return _netif; }

void st3m_badgenet_switch(st3m_badgenet_switch_t *out) {
    LOCK;
    memcpy(out, &_driver.sw, sizeof(st3m_badgenet_switch_t));
    UNLOCK;
}

void st3m_badgenet_interface_name(char *out) {
    esp_netif_get_netif_impl_name(_netif, out);
}

static void _jack_disable_unlocked(bool left) {
    st3m_badgenet_jack_t *jack = left ? _driver.left : _driver.right;
    if (jack == NULL) {
        return;
    }
    esp_err_t ret = uart_driver_delete(jack->uart_num);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to disable UART %d: %s", jack->uart_num,
                 esp_err_to_name(ret));
    }
    if (left) {
        _driver.left = NULL;
        ESP_LOGI(TAG, "BadgeNet disabled on left jack");
    } else {
        _driver.right = NULL;
        ESP_LOGI(TAG, "BadgeNet disabled on right jack");
    }
}

esp_err_t st3m_badgenet_jack_configure(st3m_badgenet_jack_side_t side,
                                       st3m_badgenet_jack_mode_t mode) {
    esp_err_t ret;
    bool left = side == st3m_badgenet_jack_left;
    LOCK;
    if (mode == st3m_badgenet_jack_mode_disabled) {
        _jack_disable_unlocked(left);
        UNLOCK;
        return ESP_OK;
    }

    // Disable already configured jack.
    st3m_badgenet_jack_t *jack = left ? _driver.left : _driver.right;
    if (jack != NULL) {
        _jack_disable_unlocked(left);
    }

    // Start configuring new jack. Don't assign to driver until the end.
    if (left) {
        jack = &_left;
    } else {
        jack = &_right;
    }
    _jack_reset(jack);

    // Configure UART parameters
    const int uart_buffer_size = (1024 * 2);
    QueueHandle_t uart_queue;
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    ret = uart_param_config(jack->uart_num, &uart_config);
    if (ret != ESP_OK) {
        UNLOCK;
        ESP_LOGE(TAG, "Failed to configure UART %d: %s", jack->uart_num,
                 esp_err_to_name(ret));
        return ret;
    }

    // Turn mode into tx_tip.
    bool tx_tip = false;
    switch (mode) {
        case st3m_badgenet_jack_mode_enabled_auto:
            if (left) {
                tx_tip = true;
            } else {
                tx_tip = false;
            }
            break;
        case st3m_badgenet_jack_mode_enabled_tx_ring:
            tx_tip = false;
            break;
        case st3m_badgenet_jack_mode_enabled_tx_tip:
            tx_tip = true;
            break;
        default:
            UNLOCK;
            return ESP_ERR_INVALID_ARG;
    }

    // Find actual pin numbers.
    int pin_tx = 0;
    int pin_rx = 0;
    if (left) {
        if (tx_tip) {
            pin_tx = flow3r_bsp_spio_programmable_pins.badgelink_left_tip;
            pin_rx = flow3r_bsp_spio_programmable_pins.badgelink_left_ring;
        } else {
            pin_tx = flow3r_bsp_spio_programmable_pins.badgelink_left_ring;
            pin_rx = flow3r_bsp_spio_programmable_pins.badgelink_left_tip;
        }
    } else {
        if (tx_tip) {
            pin_tx = flow3r_bsp_spio_programmable_pins.badgelink_right_tip;
            pin_rx = flow3r_bsp_spio_programmable_pins.badgelink_right_ring;
        } else {
            pin_tx = flow3r_bsp_spio_programmable_pins.badgelink_right_ring;
            pin_rx = flow3r_bsp_spio_programmable_pins.badgelink_right_tip;
        }
    }

    // Configure pins.
    gpio_set_direction(pin_tx, GPIO_MODE_OUTPUT);
    gpio_set_direction(pin_rx, GPIO_MODE_INPUT);
    ret = uart_set_pin(jack->uart_num, pin_tx, pin_rx, -1, -1);
    if (ret != ESP_OK) {
        UNLOCK;
        ESP_LOGE(TAG, "Failed to configure UART %d with pins TX %d, RX %d: %s",
                 jack->uart_num, pin_tx, pin_rx, esp_err_to_name(ret));
        return ret;
    }

    // Finally, install UART driver.
    ret = uart_driver_install(jack->uart_num, uart_buffer_size,
                              uart_buffer_size, 10, &uart_queue,
                              ESP_INTR_FLAG_LOWMED | ESP_INTR_FLAG_SHARED);
    if (ret != ESP_OK) {
        UNLOCK;
        ESP_LOGE(TAG, "Failed to install UART %d driver: %s", jack->uart_num,
                 esp_err_to_name(ret));
        return ret;
    }

    // And assign jack to badgenet driver.
    if (left) {
        _driver.left = jack;
        ESP_LOGI(TAG, "Configured left jack with UART %d, TX pin %d, RX pin %d",
                 jack->uart_num, pin_tx, pin_rx);
    } else {
        _driver.right = jack;
        ESP_LOGI(TAG,
                 "Configured right jack with UART %d, TX pin %d, RX pin %d",
                 jack->uart_num, pin_tx, pin_rx);
    }

    UNLOCK;
    return ESP_OK;
}