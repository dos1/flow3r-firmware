#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "st3m_usb.h"

#include "esp_log.h"
#include "tusb.h"

static st3m_usb_msc_conf_t _conf = { 0 };

void st3m_usb_msc_set_conf(st3m_usb_msc_conf_t *conf) {
    assert(conf->block_count != 0);
    assert(conf->block_size != 0);
    assert(conf->fn_read10 != NULL);
    memcpy(&_conf, conf, sizeof(_conf));
}

static st3m_usb_msc_conf_t *_get_conf(void) {
    assert(_conf.block_count != 0);
    assert(_conf.block_size != 0);
    assert(_conf.fn_read10 != NULL);
    return &_conf;
}

// TinyUSB callback.
void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8],
                        uint8_t product_id[16], uint8_t product_rev[4]) {
    (void)lun;

    st3m_usb_msc_conf_t *conf = _get_conf();

    const char vid[] = "flow3r";
    const char rev[] = "1.0";

    memcpy(vendor_id, vid, strlen(vid));
    memcpy(product_id, conf->product_id, 16);
    memcpy(product_rev, rev, strlen(rev));
}

// TinyUSB callback.
bool tud_msc_test_unit_ready_cb(uint8_t lun) {
    (void)lun;
    st3m_usb_msc_conf_t *conf = _get_conf();
    if (conf->fn_ready != NULL) {
        return conf->fn_ready(lun);
    }
    return true;
}

// TinyUSB callback.
void tud_msc_capacity_cb(uint8_t lun, uint32_t *block_count,
                         uint16_t *block_size) {
    (void)lun;
    st3m_usb_msc_conf_t *conf = _get_conf();
    *block_count = conf->block_count;
    *block_size = conf->block_size;
}

// TinyUSB callback.
bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start,
                           bool load_eject) {
    st3m_usb_msc_conf_t *conf = _get_conf();
    if (conf->fn_start_stop != NULL) {
        return conf->fn_start_stop(lun, power_condition, start, load_eject);
    }
    return true;
}

// TinyUSB callback.
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset,
                          void *buffer, uint32_t bufsize) {
    st3m_usb_msc_conf_t *conf = _get_conf();
    return conf->fn_read10(lun, lba, offset, buffer, bufsize);
}

// TinyUSB callback.
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset,
                           uint8_t *buffer, uint32_t bufsize) {
    st3m_usb_msc_conf_t *conf = _get_conf();
    if (conf->fn_write10 != NULL) {
        return conf->fn_write10(lun, lba, offset, buffer, bufsize);
    }

    if (lba >= conf->block_count) {
        return -1;
    }
    return (int32_t)bufsize;
}

// TinyUSB callback.
int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void *buffer,
                        uint16_t bufsize) {
    void const *response = NULL;
    int32_t resplen = 0;

    // most scsi handled is input
    bool in_xfer = true;

    switch (scsi_cmd[0]) {
        default:
            // Set Sense = Invalid Command Operation
            tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);

            // negative means error -> tinyusb could stall and/or response with
            // failed status
            resplen = -1;
            break;
    }

    // return resplen must not larger than bufsize
    if (resplen > bufsize) {
        resplen = bufsize;
    }

    if (response && (resplen > 0)) {
        if (in_xfer) {
            memcpy(buffer, response, (size_t)resplen);
        } else {
            // SCSI output
        }
    }

    return resplen;
}