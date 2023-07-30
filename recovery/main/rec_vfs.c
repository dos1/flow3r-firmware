#include "diskio_wl.h"

#include "rec_fatal.h"

#include "st3m_fs_flash.h"
#include "st3m_usb.h"

#include <string.h>

void rec_vfs_wl_msc_start(void) {
    st3m_usb_msc_conf_t msc = {
        .block_size = st3m_fs_flash_get_blocksize(),
        .block_count = st3m_fs_flash_get_blockcount(),
        .product_id = { 'f', 'l', 'o', 'w', '3', 'r', 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        0 },
        .fn_read10 = st3m_fs_flash_read,
        .fn_write10 = st3m_fs_flash_write,
    };
    st3m_usb_mode_t usb_mode = {
        .kind = st3m_usb_mode_kind_disk,
        .disk = &msc,
    };
    st3m_usb_mode_switch(&usb_mode);
}