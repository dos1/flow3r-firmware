#include "fs.h"

#include "st3m_mode.h"
#include "st3m_sys_data.h"
#include "st3m_tar.h"
#include "st3m_fs.h"

#include "esp_system.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"

static const char *TAG = "st3m-fs";

static const char *sysflag = "/flash/sys/.sys-installed";

static void _extract_callback(const char *path) {
    char msg[256];
    snprintf(msg, 256, "Installing %s...", path);
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
    fprintf(f, "remove me to reinstall /sys on next startup");
    fclose(f);
}

void flow3r_fs_init(void) {
    st3m_fs_init();

    bool have_mpy = false;
    struct stat st;
    if (stat(sysflag, &st) == 0) {
        have_mpy = S_ISREG(st.st_mode);
    }

    if (!have_mpy) {
        st3m_mode_set(st3m_mode_kind_starting, "Installing /flash/sys...");
        ESP_LOGI(TAG, "No %s on flash, preparing sys directory...", sysflag);
        _extract_sys_data();
    }
}
