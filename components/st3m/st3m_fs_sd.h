#pragma once

#include "esp_err.h"

// An SD card (or lack thereof) can have the following states:
typedef enum {
    // Not detected by the system on startup, or has been deemed unavailable
    // since, or has been explicitly ejected by user.
    // Can be re-probed by calling st3m_fs_sd_probe or st3m_fs_sd_mount.
    st3m_fs_sd_status_ejected = 0,
    // Detected by the system, but not mounted onto the VFS.
    // st3m_fs_sd_{read10,write10} calls may be used to access the device.
    st3m_fs_sd_status_probed = 1,
    // Mounted on VFS at /sd.
    st3m_fs_sd_status_mounted = 2,
} st3m_fs_sd_status_t;

// Properties of the detected SD card.
typedef struct {
    // Sector size in bytes.
    size_t sector_size;
    // Total number of sectors.
    size_t sector_count;
} st3m_fs_sd_props_t;

// Get the current status of the card.
st3m_fs_sd_status_t st3m_fs_sd_get_status(void);

// Attempt to probe the card, or get already probed card data. props can be
// NULL, in which case card properties will not be returned to the caller.
esp_err_t st3m_fs_sd_probe(st3m_fs_sd_props_t *props);

// Attempt to mount the card to the VFS at /sd. No-op if already mounted. Card
// doesn't have to be probed beforehand.
esp_err_t st3m_fs_sd_mount(void);

// Ensure the card isn't mounted. It might still be probed after this function
// returns.
esp_err_t st3m_fs_sd_unmount(void);

// Attempt to unmount (if necessary) and then unprobe/eject (if necessary).
// No-op if card is already ejected.
esp_err_t st3m_fs_sd_eject(void);

// SCSI Read (10) and Write (10) compatible operations. Offset must be 0. If an
// error occurs during the read/write operation, the card will be checked for
// errors, and might get ejected if deemed uanvailable/broken.
//
// Must only be called when st3m_fs_sd_get_status() == st3m_fs_sd_status_probed.
// This is not checked for performance reasons.
int32_t st3m_fs_sd_read10(uint8_t lun, uint32_t lba, uint32_t offset,
                          void *buffer, uint32_t bufsize);
int32_t st3m_fs_sd_write10(uint8_t lun, uint32_t lba, uint32_t offset,
                           const void *buffer, uint32_t bufsize);

// Poke the card to make sure it's all nice and happy. If not, unmount and eject
// it.
void st3m_fs_sd_check(void);

// Private.
esp_err_t st3m_fs_sd_init(void);