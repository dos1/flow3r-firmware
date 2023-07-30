#include "st3m_fs_sd.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"

#include "diskio.h"
#include "diskio_impl.h"
#include "diskio_sdmmc.h"
#include "driver/sdmmc_host.h"
#include "hal/gpio_types.h"
#include "sdmmc_cmd.h"
#include "vfs_fat_internal.h"

static const char *TAG = "st3m-fs-sd";

static void _st3m_fs_sd_check_unlocked(void);

SemaphoreHandle_t _mu = NULL;
st3m_fs_sd_status_t _status = st3m_fs_sd_status_ejected;
sdmmc_card_t _card = {
    0,
};
sdmmc_host_t _host = SDMMC_HOST_DEFAULT();

esp_err_t st3m_fs_sd_init(void) {
    assert(_mu == NULL);
    _mu = xSemaphoreCreateMutex();
    assert(_mu != NULL);

    esp_err_t ret = sdmmc_host_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Could not initialize SDMMC host: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
    slot.clk = GPIO_NUM_47;
    slot.cmd = GPIO_NUM_48;
    slot.d0 = GPIO_NUM_21;
    slot.width = 1;
    slot.flags = SDMMC_SLOT_NO_CD | SDMMC_SLOT_NO_WP;
    if ((ret = sdmmc_host_init_slot(SDMMC_HOST_SLOT_1, &slot)) != ESP_OK) {
        ESP_LOGE(TAG, "Could not initialize SDMMC slot: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "SD Card slot initialized");
    return ESP_OK;
};

// Borrowed from esp-idf-v5.1/components/fatfs/vfs/vfs_fat_sdmmc.c
//
// SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
//
// SPDX-License-Identifier: Apache-2.0
static esp_err_t _partition_card(const esp_vfs_fat_mount_config_t *mount_config,
                                 const char *drv, sdmmc_card_t *card,
                                 BYTE pdrv) {
    FRESULT res = FR_OK;
    esp_err_t err;
    const size_t workbuf_size = 4096;
    void *workbuf = NULL;
    ESP_LOGW(TAG, "partitioning card");

    workbuf = ff_memalloc(workbuf_size);
    if (workbuf == NULL) {
        return ESP_ERR_NO_MEM;
    }

    LBA_t plist[] = { 100, 0, 0, 0 };
    res = f_fdisk(pdrv, plist, workbuf);
    if (res != FR_OK) {
        err = ESP_FAIL;
        ESP_LOGD(TAG, "f_fdisk failed (%d)", res);
        goto fail;
    }
    size_t alloc_unit_size = esp_vfs_fat_get_allocation_unit_size(
        card->csd.sector_size, mount_config->allocation_unit_size);
    ESP_LOGW(TAG, "formatting card, allocation unit size=%d", alloc_unit_size);
    const MKFS_PARM opt = { (BYTE)FM_ANY, 0, 0, 0, alloc_unit_size };
    res = f_mkfs(drv, &opt, workbuf, workbuf_size);
    if (res != FR_OK) {
        err = ESP_FAIL;
        ESP_LOGD(TAG, "f_mkfs failed (%d)", res);
        goto fail;
    }

    free(workbuf);
    return ESP_OK;
fail:
    free(workbuf);
    return err;
}

// Borrowed from esp-idf-v5.1/components/fatfs/vfs/vfs_fat_sdmmc.c
//
// SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
//
// SPDX-License-Identifier: Apache-2.0
static esp_err_t _f_mount(sdmmc_card_t *card, FATFS *fs, const char *drv,
                          uint8_t pdrv,
                          const esp_vfs_fat_mount_config_t *mount_config) {
    esp_err_t err = ESP_OK;
    FRESULT res = f_mount(fs, drv, 1);
    if (res != FR_OK) {
        err = ESP_FAIL;
        ESP_LOGW(TAG, "failed to mount card (%d)", res);

        bool need_mount_again =
            (res == FR_NO_FILESYSTEM || res == FR_INT_ERR) &&
            mount_config->format_if_mount_failed;
        if (!need_mount_again) {
            return err;
        }

        err = _partition_card(mount_config, drv, card, pdrv);
        if (err != ESP_OK) {
            return err;
        }

        ESP_LOGW(TAG, "mounting again");
        res = f_mount(fs, drv, 0);
        if (res != FR_OK) {
            err = ESP_FAIL;
            ESP_LOGD(TAG, "f_mount failed after formatting (%d)", res);
            return err;
        }
    }

    return ESP_OK;
}

static esp_err_t _st3m_fs_sd_mount_unlocked(void) {
    esp_err_t ret;

    if (_status == st3m_fs_sd_status_mounted) {
        return ESP_OK;
    }

    BYTE pdrv = 0xFF;
    if ((ret = ff_diskio_get_drive(&pdrv)) != ESP_OK) {
        ESP_LOGE(TAG, "Getting drive number failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ff_diskio_register_sdmmc(pdrv, &_card);

    esp_vfs_fat_mount_config_t mount_config = {
        // Maximum number of concurrently open files.
        .max_files = 16,
        // TODO(q3k): expose this is an argument to st3m_fs_sd_mount.
        .format_if_mount_failed = false,
        .allocation_unit_size = 16 * 1024,
    };

    char drv[3] = { (char)('0' + pdrv), ':', 0 };
    FATFS *fs;
    ret = esp_vfs_fat_register("/sd", drv, mount_config.max_files, &fs);
    if (ret == ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "/sd already registered");
        // it's okay, already registered with VFS
    } else if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Registering FAT SD partition failed: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    if ((ret = _f_mount(&_card, fs, drv, pdrv, &mount_config)) != ESP_OK) {
        ESP_LOGE(TAG, "Registering FAT SD partition failed: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Mounted SD at /sd");
    _status = st3m_fs_sd_status_mounted;
    return ESP_OK;
}

static esp_err_t _st3m_fs_sd_unmount_unlocked(void) {
    if (_status != st3m_fs_sd_status_mounted) {
        return ESP_OK;
    }

    BYTE pdrv = ff_diskio_get_pdrv_card(&_card);
    if (pdrv == 0xFF) {
        ESP_LOGE(TAG, "Could not get drive number");
        return ESP_FAIL;
    }

    char drv[3] = { (char)('0' + pdrv), ':', 0 };
    f_mount(0, drv, 0);
    ff_diskio_unregister(pdrv);
    // Seems like if we don't unregister it, re-registration in mount() fails.
    // But that's not the case for spiflash/wl?
    //
    // TODO(q3k): investigate deeper
    esp_vfs_fat_unregister_path("/sd");
    _status = st3m_fs_sd_status_probed;
    _st3m_fs_sd_check_unlocked();
    return ESP_OK;
}

static esp_err_t _st3m_fs_sd_eject_unlocked(void) {
    esp_err_t ret;

    switch (_status) {
        case st3m_fs_sd_status_ejected:
            // Nothing to do.
            return ESP_OK;
        case st3m_fs_sd_status_probed:
            // Effectively no-op, just mark that the user expected the card to
            // be ejected.
            _status = st3m_fs_sd_status_ejected;
            return ESP_OK;
        case st3m_fs_sd_status_mounted:
            // Unmount then mark as ejected.
            if ((ret = _st3m_fs_sd_unmount_unlocked()) != ESP_OK) {
                return ret;
            }
            _status = st3m_fs_sd_status_ejected;
            return ESP_OK;
    }
    // Unreachable.
    abort();
}

static void _st3m_fs_sd_check_unlocked(void) {
    if (_status == st3m_fs_sd_status_ejected) {
        return;
    }

    esp_err_t ret = sdmmc_get_status(&_card);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "SD card seems to have disappeared");
        _st3m_fs_sd_eject_unlocked();
        return;
    }
}

void st3m_fs_sd_check(void) {
    xSemaphoreTake(_mu, portMAX_DELAY);
    _st3m_fs_sd_check_unlocked();
    xSemaphoreGive(_mu);
}

static esp_err_t _st3m_fs_sd_probe_unlocked(st3m_fs_sd_props_t *props) {
    esp_err_t ret = ESP_OK;
    _st3m_fs_sd_check_unlocked();
    // Only do something if the card is currently ejected.
    if (_status != st3m_fs_sd_status_ejected) {
        goto done;
    }

    _host.max_freq_khz = SDMMC_FREQ_DEFAULT;
    if ((ret = sdmmc_card_init(&_host, &_card)) != ESP_OK) {
        ESP_LOGE(TAG, "SD Card initialization failed: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    _status = st3m_fs_sd_status_probed;

done:
    if (props != NULL) {
        props->sector_size = _card.csd.sector_size;
        props->sector_count = _card.csd.capacity;
    }
    return ret;
}

esp_err_t st3m_fs_sd_mount(void) {
    xSemaphoreTake(_mu, portMAX_DELAY);
    esp_err_t ret = _st3m_fs_sd_mount_unlocked();
    xSemaphoreGive(_mu);
    return ret;
}

esp_err_t st3m_fs_sd_unmount(void) {
    xSemaphoreTake(_mu, portMAX_DELAY);
    esp_err_t ret = _st3m_fs_sd_unmount_unlocked();
    xSemaphoreGive(_mu);
    return ret;
}

esp_err_t st3m_fs_sd_eject(void) {
    xSemaphoreTake(_mu, portMAX_DELAY);
    esp_err_t ret = _st3m_fs_sd_eject_unlocked();
    xSemaphoreGive(_mu);
    return ret;
}

st3m_fs_sd_status_t st3m_fs_sd_get_status(void) {
    xSemaphoreTake(_mu, portMAX_DELAY);
    st3m_fs_sd_status_t st = _status;
    xSemaphoreGive(_mu);
    return st;
}

esp_err_t st3m_fs_sd_probe(st3m_fs_sd_props_t *props) {
    xSemaphoreTake(_mu, portMAX_DELAY);
    esp_err_t ret = _st3m_fs_sd_probe_unlocked(props);
    xSemaphoreGive(_mu);
    return ret;
}

int32_t st3m_fs_sd_read10(uint8_t lun, uint32_t lba, uint32_t offset,
                          void *buffer, uint32_t bufsize) {
    if (offset != 0) {
        ESP_LOGE(TAG, "Unaligned read: %d", (unsigned int)offset);
        return 0;
    }
    size_t sector_count = bufsize / _card.csd.sector_size;
    if (sector_count == 0) {
        ESP_LOGE(TAG, "Read too small: %d bytes", (unsigned int)bufsize);
        return 0;
    }
    esp_err_t ret = sdmmc_read_sectors(&_card, buffer, lba, sector_count);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Read failed: %s", esp_err_to_name(ret));
        st3m_fs_sd_check();
        return 0;
    }
    return sector_count * _card.csd.sector_size;
}

int32_t st3m_fs_sd_write10(uint8_t lun, uint32_t lba, uint32_t offset,
                           const void *buffer, uint32_t bufsize) {
    if (offset != 0) {
        ESP_LOGW(TAG, "Unaligned write: %d", (unsigned int)offset);
        return 0;
    }
    size_t sector_count = bufsize / _card.csd.sector_size;
    if (sector_count == 0) {
        ESP_LOGE(TAG, "Write too small: %d bytes", (unsigned int)bufsize);
        return 0;
    }
    esp_err_t ret = sdmmc_write_sectors(&_card, buffer, lba, sector_count);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Write failed: %s", esp_err_to_name(ret));
        st3m_fs_sd_check();
        return 0;
    }
    return sector_count * _card.csd.sector_size;
}
