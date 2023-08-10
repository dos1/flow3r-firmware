#include "fs.h"

#include "st3m_fs.h"
#include "st3m_fs_flash.h"
#include "st3m_fs_sd.h"
#include "st3m_mode.h"
#include "st3m_sys_data.h"
#include "st3m_tar.h"
#include "st3m_version.h"

#include "esp_system.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"

static const char *TAG = "st3m-fs";
static bool _updating = false;
static const char *sysflag = "/flash/sys/.sys-installed";

static void _extract_callback(const char *path) {
    char msg[256];
    // Strip /flash/sys/ from displayed paths...
    const char *prefix = "/flash/sys/";
    if (strstr(path, prefix) == path) {
        path += strlen(prefix);
    }
    if (_updating) {
        snprintf(msg, 256, "Updating %s", path);
    } else {
        snprintf(msg, 256, "Installing %s", path);
    }

    size_t limit = 30;
    if (strlen(msg) > limit) {
        strlcpy(msg + limit, "...", 256 - limit);
    }
    st3m_mode_set(st3m_mode_kind_starting, msg);
}

// Extract data from baked-in sys tarball into /flash/sys.
static void _extract_sys_data(void) {
    st3m_tar_extractor_t extractor;
    st3m_tar_extractor_init(&extractor);
    extractor.root = "/flash/sys/";
    extractor.on_file = _extract_callback;

    bool res = st3m_tar_parser_run_zlib(&extractor.parser, st3m_sys_data,
                                        st3m_sys_data_length);
    if (!res) {
        ESP_LOGE(TAG, "Failed to extract sys fs");
        return;
    }

    FILE *f = fopen(sysflag, "w");
    assert(f != NULL);
    fprintf(f, "%s", st3m_version);
    fclose(f);
}

typedef enum {
    sys_state_absent = 0,
    sys_state_diff_version = 1,
    sys_state_correct = 2,
} sys_state_t;

void flow3r_fs_init(void) {
    st3m_fs_init();

    esp_err_t err;
    if ((err = st3m_fs_flash_mount()) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FAT FS: %s", esp_err_to_name(err));
        return;
    }

    sys_state_t state = sys_state_absent;
    char local_version[128];
    struct stat st;
    if (stat(sysflag, &st) == 0 && S_ISREG(st.st_mode)) {
        state = sys_state_diff_version;
        FILE *f = fopen(sysflag, "r");
        size_t len = fread(local_version, 1, sizeof(local_version) - 1, f);
        fclose(f);
        local_version[len] = 0;
        if (strcmp(st3m_version, local_version) == 0) {
            state = sys_state_correct;
        }
    }

    if (state == sys_state_absent) {
        st3m_mode_set(st3m_mode_kind_starting, "Installing /flash/sys...");
        _updating = false;
        ESP_LOGI(
            TAG,
            "Installing /sys/flash... (st3m version: %s, local version: none)",
            st3m_version);
        _extract_sys_data();
        st3m_mode_set(st3m_mode_kind_starting,
                      "Installed, continuing startup...");
    }
    if (state == sys_state_diff_version) {
        st3m_mode_set(st3m_mode_kind_starting, "Updating /flash/sys...");
        _updating = true;
        ESP_LOGI(TAG,
                 "Updating /sys/flash... (st3m version: %s, local version: %s)",
                 st3m_version, local_version);
        _extract_sys_data();
        st3m_mode_set(st3m_mode_kind_starting,
                      "Updated, continuing startup...");
    }

    esp_err_t ret = st3m_fs_sd_mount();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
    }
}
