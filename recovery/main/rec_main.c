#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "flow3r_bsp.h"
#include "st3m_fs.h"
#include "st3m_fs_flash.h"
#include "st3m_fs_sd.h"
#include "st3m_gfx.h"
#include "st3m_io.h"
#include "st3m_usb.h"

#include "rec_fatal.h"
#include "rec_gui.h"
#include "rec_vfs.h"

#include "esp_app_desc.h"
#include "esp_app_format.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>

static const char *TAG = "flow3r-recovery";

static menu_t *_cur_menu = NULL;
static menu_t _main_menu;
static menu_t _list_menu;
static menu_t _diskmode_menu;
static menu_t _erasedone_menu;

static menu_entry_t _list_menu_entries[];

static bool _usb_initialized = false;

static void _main_reboot(void) { esp_restart(); }

static void _main_disk_mode_flash(void) {
    _cur_menu = &_diskmode_menu;
    _cur_menu->selected = 0;

    if (!_usb_initialized) {
        _usb_initialized = true;
        st3m_usb_init();
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    rec_vfs_wl_msc_start_flash();
}

static void _main_disk_mode_sd(void) {
    _cur_menu = &_diskmode_menu;
    _cur_menu->selected = 0;

    if (!_usb_initialized) {
        _usb_initialized = true;
        st3m_usb_init();
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    rec_vfs_wl_msc_start_sd();
}

static void _main_erase_fat32(void) {
    rec_erasing_draw();
    esp_err_t ret = st3m_fs_flash_erase();
    if (ret != ESP_OK) {
        rec_fatal("Erase failed");
    }
    _cur_menu = &_erasedone_menu;
    _cur_menu->selected = 0;
}

static void _diskmode_exit(void) {
    _cur_menu = &_main_menu;
    _cur_menu->selected = 0;

    st3m_usb_mode_t usb_mode = {
        .kind = st3m_usb_mode_kind_disabled,
    };
    st3m_usb_mode_switch(&usb_mode);
}

static void _list_exit(void) {
    _cur_menu = &_main_menu;
    _cur_menu->selected = 0;
}

static void _list_select(void) {
    // TODO: copy selected image over to app partition
    _cur_menu = &_main_menu;
    _cur_menu->selected = 0;
}

typedef struct {
    esp_image_header_t image;
    esp_image_segment_header_t segment;
    esp_app_desc_t app;
} esp32_standard_header_t;

static void _main_list(void) {
    _cur_menu = &_list_menu;
    _cur_menu->selected = 0;

    int entries = 1;

    struct dirent *pDirent;
    DIR *pDir;

    // TODO: also consider the SD card
    pDir = opendir("/flash");
    if (pDir != NULL) {
        while (((pDirent = readdir(pDir)) != NULL) && (entries < 64)) {
            ESP_LOGI(TAG, "Checking header of %s", pDirent->d_name);

            char *s = pDirent->d_name;
            int l = strlen(pDirent->d_name);

            if (l > 4 && strcmp(".bin", &s[l - 4]) == 0) {
                esp32_standard_header_t hdr;

                char path[128];
                snprintf(path, sizeof(path) / sizeof(char), "/flash/%s",
                         pDirent->d_name);
                FILE *f = fopen(path, "rb");
                if (f == NULL) {
                    ESP_LOGW(TAG, "fopen failed: %s", strerror(errno));
                    continue;
                }

                size_t n = fread(&hdr, 1, sizeof(hdr), f);
                fclose(f);
                if (n != sizeof(hdr)) {
                    ESP_LOGW(TAG, "only read %d of %d bytes", n, sizeof(hdr));
                    continue;
                }

                if (hdr.image.magic != ESP_IMAGE_HEADER_MAGIC ||
                    hdr.image.chip_id != ESP_CHIP_ID_ESP32S3 ||
                    hdr.app.magic_word != ESP_APP_DESC_MAGIC_WORD) {
                    continue;
                }

                // TODO have this in a global list, so the select function
                // can get the actual path
                size_t n = strlen(pDirent->d_name) + 2 +
                           strlen(hdr.app.project_name) + 1;
                char *label = malloc(n);
                snprintf(label, n, "%s: %s", pDirent->d_name,
                         hdr.app.project_name);
                _list_menu_entries[entries].label = label;
                _list_menu_entries[entries].enter = _list_select;
                entries++;
            }
        }
        closedir(pDir);
    } else {
        ESP_LOGE(TAG, "Opening /flash failed");
    }
    _list_menu.entries_count = entries;
}

static menu_entry_t _main_menu_entries[] = {
    { .label = "List Images", .enter = _main_list },
    { .label = "Reboot", .enter = _main_reboot },
    { .label = "Disk Mode (Flash)", .enter = _main_disk_mode_flash },
    { .label = "Disk Mode (SD)", .enter = _main_disk_mode_sd },
    { .label = "Erase FAT32", .enter = _main_erase_fat32 },
};

static menu_t _main_menu = {
    .entries = _main_menu_entries,
    .entries_count = 5,
    .selected = 0,
};

static menu_entry_t _diskmode_menu_entries[] = {
    { .label = "Exit", .enter = _diskmode_exit },
};

static menu_t _diskmode_menu = {
    .help =
        "Connect the badge to a PC to\naccess the internal flash\nFAT32 "
        "partition.",
    .entries = _diskmode_menu_entries,
    .entries_count = 1,
    .selected = 0,
};

static menu_entry_t _list_menu_entries[64] = {
    { .label = "Exit", .enter = _list_exit },
};

static menu_t _list_menu = {
    .entries = _list_menu_entries,
    .entries_count = 1,
    .selected = 0,
};

static menu_entry_t _erasedone_menu_entries[] = {
    { .label = "Reboot", .enter = _main_reboot },
};

static menu_t _erasedone_menu = {
    .help = "Erase done.\nBadge will now restart.",
    .entries = _erasedone_menu_entries,
    .entries_count = 1,
    .selected = 0,
};

void app_main(void) {
    ESP_LOGI(TAG, "Starting...");
    _cur_menu = &_main_menu;

    st3m_gfx_init();
    for (int i = 0; i < 4; i++) {
        st3m_gfx_splash("");
    }
    flow3r_bsp_display_set_backlight(100);
    flow3r_bsp_i2c_init();
    st3m_io_init();
    st3m_fs_init();

    if (st3m_fs_sd_get_status() != st3m_fs_sd_status_probed) {
        _main_menu_entries[3].disabled = true;
    }

    TickType_t last_wake;
    last_wake = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&last_wake, 100 / portTICK_PERIOD_MS);

        rec_menu_draw(_cur_menu);
        rec_menu_process(_cur_menu);
    }
}
