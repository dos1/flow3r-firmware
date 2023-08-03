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

static const char *TAG = "flow3r-recovery";

static menu_t *_cur_menu = NULL;
static menu_t _main_menu;
static menu_t _list_menu;
static menu_t _diskmode_menu;
static menu_t _erasedone_menu;

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

static void _main_list(void) {
    _cur_menu = &_list_menu;
    _cur_menu->selected = 0;
}

static void _list_exit(void) {
    _cur_menu = &_main_menu;
    _cur_menu->selected = 0;
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

    esp_err_t err;
    if ((err = st3m_fs_flash_mount()) != ESP_OK) {
        ESP_LOGW(TAG, "Failed to mount FAT FS: %s", esp_err_to_name(err));
        _main_menu_entries[0].disabled = true;
    }

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
