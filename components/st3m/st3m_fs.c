#include "st3m_fs.h"

#include "esp_system.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"

static const char *TAG = "st3m-fs";

// Handle of the wear levelling library instance
static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;

void st3m_fs_init(void) {
    const esp_vfs_fat_mount_config_t mount_config = {
        .max_files = 4,
        .format_if_mount_failed = true,
        .allocation_unit_size = CONFIG_WL_SECTOR_SIZE
    };

    esp_err_t err =
        esp_vfs_fat_spiflash_mount("", "vfs", &mount_config, &s_wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FAT FS: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "Mounted Flash VFS Partition at /");
}
