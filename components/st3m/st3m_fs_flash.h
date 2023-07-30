#pragma once

// This subsystem manages the wear-leveled FAT32 partition on the built-in SPI
// flash.

#include "esp_err.h"

typedef enum {
    // Not mounted, can be accessed via _write, _read, _erase.
    st3m_fs_flash_status_unmounted = 0,
    // Mounted on /flash.
    st3m_fs_flash_status_mounted = 1,
    st3m_fs_flash_status_error = 2,
} st3m_fs_flash_status_t;

// Return the current state/status of the internal flash FAT32 partition.
st3m_fs_flash_status_t st3m_fs_flash_get_status(void);
// Mount the partition on /flash. No-op if already mounted.
esp_err_t st3m_fs_flash_mount(void);
// Unmount the partition. No-op if already unmounted.
esp_err_t st3m_fs_flash_unmount(void);

// Read function for st3m_usb_msc. Should only be called if the flash is
// unmounted.
int32_t st3m_fs_flash_read(uint8_t lun, uint32_t lba, uint32_t offset,
                           void *buffer, uint32_t bufsize);
// Write function for st3m_usb_msc. Should only be called if the flash is
// unmounted.
int32_t st3m_fs_flash_write(uint8_t lun, uint32_t lba, uint32_t offset,
                            const void *buffer, uint32_t bufsize);
// Erase the FAT32 partition. Next mount will format it. Should only be called
// if the flash is ummounted.
esp_err_t st3m_fs_flash_erase(void);
// Return block size (top-level, ie. as exposed by the wear leveling layer).
size_t st3m_fs_flash_get_blocksize(void);
// Return block count (top-level, ie. as exposed by the wear leveling layer).
size_t st3m_fs_flash_get_blockcount(void);

// Private.
void st3m_fs_flash_init(void);