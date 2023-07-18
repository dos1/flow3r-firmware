#include "diskio_wl.h"

#include "rec_fatal.h"

#include "st3m_usb.h"

#include <string.h>

static wl_handle_t _wl_handle = WL_INVALID_HANDLE;

static int32_t _flash_read(uint8_t lun, uint32_t lba, uint32_t offset,
                           void *buffer, uint32_t bufsize) {
    size_t src_addr = lba * wl_sector_size(_wl_handle) + offset;
    esp_err_t ret = wl_read(_wl_handle, src_addr, buffer, bufsize);
    if (ret == ESP_OK) {
        return bufsize;
    } else {
        return 0;
    }
}

static int32_t _flash_write(uint8_t lun, uint32_t lba, uint32_t offset,
                            const void *buffer, uint32_t bufsize) {
    size_t sector_size = wl_sector_size(_wl_handle);

    size_t start = lba * sector_size + offset;
    size_t size = bufsize;

    size_t start_align = (start % sector_size);
    if (start_align != 0) {
        start -= start_align;
        size += start_align;
    }

    size_t size_align = (size % sector_size);
    if (size_align != 0) {
        size_align = sector_size - size_align;
        size += size_align;
    }

    uint8_t *copy = malloc(size);
    assert(copy != 0);

    // Prefix, if any.
    if (start_align != 0) {
        esp_err_t ret = wl_read(_wl_handle, start, copy, sector_size);
        if (ret != ESP_OK) {
            free(copy);
            return 0;
        }
    }
    // Suffix, if any.
    // TODO(q3k): skip if this is the same as the previous read.
    if (size_align != 0) {
        esp_err_t ret = wl_read(_wl_handle, start + size - sector_size,
                                copy + size - sector_size, sector_size);
        if (ret != ESP_OK) {
            free(copy);
            return 0;
        }
    }
    // Main data.
    memcpy(copy + start_align, buffer, bufsize);

    // Erase and write.
    wl_erase_range(_wl_handle, start, size);
    esp_err_t ret = wl_write(_wl_handle, start, copy, size);
    if (ret != ESP_OK) {
        free(copy);
        return 0;
    }

    return bufsize;
}

void rec_vfs_wl_init(void) {
    const esp_partition_t *data_partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "vfs");
    if (data_partition == NULL) {
        rec_fatal("no data partition");
    }
    esp_err_t res = wl_mount(data_partition, &_wl_handle);
    if (res != ESP_OK) {
        rec_fatal("wear leveling failed");
    }
}

void rec_vfs_wl_msc_start(void) {
    st3m_usb_msc_conf_t msc = {
        .block_size = wl_sector_size(_wl_handle),
        .block_count = wl_size(_wl_handle) / wl_sector_size(_wl_handle),
        .product_id = { 'f', 'l', 'o', 'w', '3', 'r', 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        0 },
        .fn_read10 = _flash_read,
        .fn_write10 = _flash_write,
    };
    st3m_usb_mode_t usb_mode = {
        .kind = st3m_usb_mode_kind_disk,
        .disk = &msc,
    };
    st3m_usb_mode_switch(&usb_mode);
}

void rec_vfs_wl_erase(void) {
    // Only erase first 32 sectors as that's enough to trigger FAT reformat on
    // reboot.
    esp_err_t ret =
        wl_erase_range(_wl_handle, 0, wl_sector_size(_wl_handle) * 32);
    if (ret != ESP_OK) {
        rec_fatal("erase failed");
    }
}