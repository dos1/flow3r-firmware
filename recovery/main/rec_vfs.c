#include "diskio_wl.h"

#include "rec_fatal.h"

#include "st3m_fs_flash.h"
#include "st3m_fs_sd.h"
#include "st3m_usb.h"

#include <string.h>

void rec_vfs_wl_msc_start_flash(void) {
    st3m_usb_msc_conf_t msc = {
        .block_size = st3m_fs_flash_get_blocksize(),
        .block_count = st3m_fs_flash_get_blockcount(),
        .product_id = { 'f', 'l', 'o', 'w', '3', 'r', ' ', 'f', 'l', 'a', 's',
                        'h', 0, 0, 0, 0 },
        .fn_read10 = st3m_fs_flash_read,
        .fn_write10 = st3m_fs_flash_write,
    };
    st3m_usb_mode_t usb_mode = {
        .kind = st3m_usb_mode_kind_disk,
        .disk = &msc,
    };
    st3m_usb_mode_switch(&usb_mode);
}

void rec_vfs_wl_msc_start_sd(void) {
    st3m_fs_sd_status_t st = st3m_fs_sd_get_status();
    if (st != st3m_fs_sd_status_probed) {
        // TODO(q3k): don't panic
        rec_fatal("no SD card");
        return;
    }

    st3m_fs_sd_props_t props;
    esp_err_t ret = st3m_fs_sd_probe(&props);
    if (ret != ESP_OK) {
        // TODO(q3k): don't panic
        rec_fatal("SD card probe failed");
        return;
    }

    st3m_usb_msc_conf_t msc = {
        .block_size = props.sector_size,
        .block_count = props.sector_count,
        .product_id = { 'f', 'l', 'o', 'w', '3', 'r', ' ', 'S', 'D', 0, 0, 0, 0,
                        0, 0, 0 },
        .fn_read10 = st3m_fs_sd_read10,
        .fn_write10 = st3m_fs_sd_write10,
    };
    st3m_usb_mode_t usb_mode = {
        .kind = st3m_usb_mode_kind_disk,
        .disk = &msc,
    };
    st3m_usb_mode_switch(&usb_mode);
}