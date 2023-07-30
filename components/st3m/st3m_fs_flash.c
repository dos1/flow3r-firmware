#include "st3m_fs_flash.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "diskio.h"
#include "diskio_impl.h"
#include "diskio_wl.h"
#include "vfs_fat_internal.h"

static const char *TAG = "st3m-fs-flash";

static SemaphoreHandle_t _mu = NULL;
static st3m_fs_flash_status_t _status = st3m_fs_flash_status_unmounted;

// Handle of the wear levelling library instance
static wl_handle_t _wl_handle = WL_INVALID_HANDLE;

void st3m_fs_flash_init(void) {
    assert(_mu == NULL);
    _mu = xSemaphoreCreateMutex();
    assert(_mu != NULL);

    const esp_partition_t *data_partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "vfs");
    if (data_partition == NULL) {
        ESP_LOGE(TAG, "Could not find data partition.");
        return;
    }
    esp_err_t res = wl_mount(data_partition, &_wl_handle);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Mounting wear leveling failed.");
        return;
    }
}

st3m_fs_flash_status_t st3m_fs_flash_get_status(void) {
    xSemaphoreTake(_mu, portMAX_DELAY);
    st3m_fs_flash_status_t st = _status;
    xSemaphoreGive(_mu);
    return st;
}

static esp_err_t _f_mount_rw(FATFS *fs, const char *drv,
                             const esp_vfs_fat_mount_config_t *mount_config) {
    FRESULT fresult = f_mount(fs, drv, 1);
    if (fresult != FR_OK) {
        ESP_LOGW(TAG, "f_mount failed (%d)", fresult);

        bool need_mount_again =
            (fresult == FR_NO_FILESYSTEM || fresult == FR_INT_ERR) &&
            mount_config->format_if_mount_failed;
        if (!need_mount_again) {
            return ESP_FAIL;
        }

        const size_t workbuf_size = 4096;
        void *workbuf = ff_memalloc(workbuf_size);
        if (workbuf == NULL) {
            return ESP_ERR_NO_MEM;
        }

        size_t alloc_unit_size = esp_vfs_fat_get_allocation_unit_size(
            CONFIG_WL_SECTOR_SIZE, mount_config->allocation_unit_size);
        ESP_LOGI(TAG, "Formatting FATFS partition, allocation unit size=%d",
                 alloc_unit_size);
        const MKFS_PARM opt = { (BYTE)(FM_ANY | FM_SFD), 0, 0, 0,
                                alloc_unit_size };
        fresult = f_mkfs(drv, &opt, workbuf, workbuf_size);
        free(workbuf);
        workbuf = NULL;
        if (fresult != FR_OK) {
            ESP_LOGE(TAG, "f_mkfs failed: %d", fresult);
            return ESP_FAIL;
        }

        ESP_LOGI(TAG, "Mounting again");
        fresult = f_mount(fs, drv, 0);
        if (fresult != FR_OK) {
            ESP_LOGE(TAG, "f_mount failed after format: %d", fresult);
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

static esp_err_t _st3m_fs_flash_mount_unlocked(void) {
    esp_err_t ret;

    if (_status == st3m_fs_flash_status_mounted) {
        return ESP_OK;
    }

    BYTE pdrv = 0xFF;
    if ((ret = ff_diskio_get_drive(&pdrv)) != ESP_OK) {
        ESP_LOGE(TAG, "Getting drive number failed: %s", esp_err_to_name(ret));
        return ret;
    }

    char drv[3] = { (char)('0' + pdrv), ':', 0 };
    if ((ret = ff_diskio_register_wl_partition(pdrv, _wl_handle)) != ESP_OK) {
        ESP_LOGE(TAG, "Registering WL partition failed: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    FATFS *fs;
    esp_vfs_fat_mount_config_t mount_config = { .max_files = 16,
                                                .format_if_mount_failed = true,
                                                .allocation_unit_size =
                                                    CONFIG_WL_SECTOR_SIZE };

    ret = esp_vfs_fat_register("/flash", drv, mount_config.max_files, &fs);
    if (ret == ESP_ERR_INVALID_STATE) {
        // it's okay, already registered with VFS
    } else if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Registering FAT/WL partition failed: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    if ((ret = _f_mount_rw(fs, drv, &mount_config)) != ESP_OK) {
        ESP_LOGE(TAG, "Registering FAT/WL partition failed: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Mounted Flash VFS Partition at /flash");
    _status = st3m_fs_flash_status_mounted;
    return ESP_OK;
}

esp_err_t st3m_fs_flash_mount(void) {
    xSemaphoreTake(_mu, portMAX_DELAY);
    esp_err_t ret = _st3m_fs_flash_mount_unlocked();
    xSemaphoreGive(_mu);
    return ret;
}

static esp_err_t _st3m_fs_flash_unmount_unlocked(void) {
    if (_status == st3m_fs_flash_status_unmounted) {
        return ESP_OK;
    }

    BYTE pdrv = ff_diskio_get_pdrv_wl(_wl_handle);
    if (pdrv == 0xFF) {
        ESP_LOGE(TAG, "Could not get drive number");
        return ESP_FAIL;
    }

    esp_err_t ret;
    char drv[3] = { (char)('0' + pdrv), ':', 0 };
    f_mount(0, drv, 0);
    ff_diskio_unregister(pdrv);
    ff_diskio_clear_pdrv_wl(_wl_handle);
    if ((ret = esp_vfs_fat_unregister_path("/flash")) != ESP_OK) {
        ESP_LOGE(TAG, "Could not unmount: %s", esp_err_to_name(ret));
        return ret;
    }

    _status = st3m_fs_flash_status_unmounted;
    return ESP_OK;
}

esp_err_t st3m_fs_flash_unmount(void) {
    xSemaphoreTake(_mu, portMAX_DELAY);
    esp_err_t ret = _st3m_fs_flash_unmount_unlocked();
    xSemaphoreGive(_mu);
    return ret;
}

int32_t st3m_fs_flash_read(uint8_t lun, uint32_t lba, uint32_t offset,
                           void *buffer, uint32_t bufsize) {
    size_t src_addr = lba * wl_sector_size(_wl_handle) + offset;
    esp_err_t ret = wl_read(_wl_handle, src_addr, buffer, bufsize);
    if (ret == ESP_OK) {
        return bufsize;
    } else {
        return 0;
    }
}

int32_t st3m_fs_flash_write(uint8_t lun, uint32_t lba, uint32_t offset,
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
    free(copy);
    if (ret != ESP_OK) {
        return 0;
    }
    return bufsize;
}

esp_err_t st3m_fs_flash_erase(void) {
    xSemaphoreTake(_mu, portMAX_DELAY);
    if (_status != st3m_fs_flash_status_unmounted) {
        xSemaphoreGive(_mu);
        ESP_LOGE(TAG, "Cannot erase flash while mounted");
        return ESP_ERR_INVALID_STATE;
    }
    // Only erase first 32 sectors as that's enough to trigger FAT reformat on
    // reboot.
    esp_err_t ret =
        wl_erase_range(_wl_handle, 0, wl_sector_size(_wl_handle) * 32);
    xSemaphoreGive(_mu);
    return ret;
}

size_t st3m_fs_flash_get_blocksize(void) { return wl_sector_size(_wl_handle); }
size_t st3m_fs_flash_get_blockcount(void) {
    return wl_size(_wl_handle) / wl_sector_size(_wl_handle);
}