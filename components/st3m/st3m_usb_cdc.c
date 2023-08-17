#include "st3m_usb.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "tusb.h"

#include "esp_log.h"

static st3m_usb_app_conf_t _conf = { 0 };
static SemaphoreHandle_t _write_mu;

void st3m_usb_cdc_init(void) { _write_mu = xSemaphoreCreateMutex(); }

void st3m_usb_cdc_set_conf(st3m_usb_app_conf_t *conf) {
    assert(conf->fn_rx != NULL);
    assert(conf->fn_txpoll != NULL);
    memcpy(&_conf, conf, sizeof(_conf));
}

static st3m_usb_app_conf_t *_get_conf(void) {
    assert(_conf.fn_rx != NULL);
    assert(_conf.fn_txpoll != NULL);
    return &_conf;
}

#define CDC_RX_BUFSIZE 64
// TinyUSB callback: when host sent data over OUT endpoint.
void tud_cdc_rx_cb(uint8_t itf) {
    st3m_usb_app_conf_t *conf = _get_conf();

    uint8_t buf[CDC_RX_BUFSIZE];
    while (tud_cdc_n_available(itf)) {
        int read_res = tud_cdc_n_read(itf, buf, CDC_RX_BUFSIZE);
        conf->fn_rx(buf, read_res);
    }
}

#define CDC_TX_BUFSIZE 64
void st3m_usb_cdc_txpoll(void) {
    st3m_usb_app_conf_t *conf = _get_conf();
    if (conf->fn_txpoll == NULL) {
        return;
    }

    for (;;) {
        uint32_t space = tud_cdc_n_write_available(st3m_usb_interface_app_cdc);
        if (space == 0) {
            tud_cdc_n_write_flush(st3m_usb_interface_app_cdc);
            return;
        }

        uint8_t buf[CDC_TX_BUFSIZE];
        if (space > CDC_TX_BUFSIZE) {
            space = CDC_TX_BUFSIZE;
        }

        size_t to_write = conf->fn_txpoll(buf, space);
        if (to_write == 0) {
            return;
        }
        tud_cdc_n_write(st3m_usb_interface_app_cdc, buf, to_write);
        tud_cdc_n_write_flush(st3m_usb_interface_app_cdc);
    }
}

void st3m_usb_cdc_detached(void) {
    st3m_usb_app_conf_t *conf = _get_conf();
    if (conf->fn_detach == NULL) {
        return;
    }
    conf->fn_detach();
}
