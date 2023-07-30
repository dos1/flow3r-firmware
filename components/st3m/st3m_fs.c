#include "st3m_fs.h"

#include "st3m_fs_flash.h"
#include "st3m_fs_sd.h"
#include "st3m_mode.h"
#include "st3m_sys_data.h"
#include "st3m_tar.h"

#include "esp_system.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"

#include <errno.h>

static const char *TAG = "st3m-fs";

// Entries for root filesystem. Ideally this wouldn't be static but would
// consult the ESP-IDF VFS registry.
static struct dirent _root_dirents[4] = {
    {
        .d_ino = 1,
        .d_name = ".",
        .d_type = DT_DIR,
    },
    {
        .d_ino = 2,
        .d_name = "flash",
        .d_type = DT_DIR,
    },
    {
        .d_ino = 3,
        .d_name = "sd",
        .d_type = DT_DIR,
    },
    {
        .d_ino = 4,
        .d_name = "console",
        .d_type = DT_DIR,
    },
};

static int _root_vfs_close(int fd) { return 0; }

static int _root_vfs_fcntl(int fd, int cmd, int arg) { return 0; }

static int _root_vfs_fstat(int fd, struct stat *st) {
    st->st_mode = S_IFDIR;
    return 0;
}

static int _root_vfs_open(const char *path, int flags, int mode) {
    if (strcmp(path, "/") == 0) {
        if (flags == 0) {
            return 0;
        }
        errno = EISDIR;
    } else {
        errno = ENOENT;
    }
    return -1;
}

static ssize_t _root_vfs_read(int fd, void *data, size_t size) {
    errno = EISDIR;
    return -1;
}

static ssize_t _root_vfs_write(int fd, const void *data, size_t size) {
    errno = EISDIR;
    return -1;
}

typedef struct {
    DIR _inner;
    size_t ix;
} st3m_rootfs_dir_t;

static DIR *_root_vfs_opendir(const char *name) {
    if (strcmp(name, "/") != 0) {
        errno = ENOENT;
        return NULL;
    }

    st3m_rootfs_dir_t *dir = calloc(1, sizeof(st3m_rootfs_dir_t));
    assert(dir != NULL);
    return (DIR *)dir;
}

static struct dirent *_root_vfs_readdir(DIR *pdir) {
    st3m_rootfs_dir_t *dir = (st3m_rootfs_dir_t *)pdir;
    if (dir->ix >= (sizeof(_root_dirents) / sizeof(struct dirent))) {
        return NULL;
    }
    return &_root_dirents[dir->ix++];
}

static int _root_vfs_closedir(DIR *pdir) {
    free(pdir);
    return 0;
}

// Root filesystem implementation, because otherwise os.listdir("/") fails...
static esp_vfs_t _root_vfs = {
    .flags = ESP_VFS_FLAG_DEFAULT,
    .close = &_root_vfs_close,
    .fcntl = &_root_vfs_fcntl,
    .fstat = &_root_vfs_fstat,
    .open = &_root_vfs_open,
    .read = &_root_vfs_read,
    .write = &_root_vfs_write,
    .opendir = &_root_vfs_opendir,
    .readdir = &_root_vfs_readdir,
    .closedir = &_root_vfs_closedir,
};

void st3m_fs_init(void) {
    esp_err_t err;

    st3m_fs_flash_init();
    err = st3m_fs_sd_init();
    if (err == ESP_OK) {
        st3m_fs_sd_props_t props;
        err = st3m_fs_sd_probe(&props);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Failed to detect SD card: %s", esp_err_to_name(err));
        } else {
            uint64_t sector_size = props.sector_size;
            uint64_t sector_count = props.sector_count;
            uint64_t bytes = sector_size * sector_count;
            // Overflows 32 bits at 4 PiB.
            uint64_t mib = bytes >> 20;
            if (mib > (1ULL << 32)) {
                ESP_LOGW(TAG,
                         "SD card has bogus size. Or you are in the future.");
            }
            ESP_LOGI(TAG, "SD card probed: %d MiB (%d sectors of %d bytes)",
                     (unsigned int)mib, props.sector_count, props.sector_size);
        }
    }

    if ((err = esp_vfs_register("", &_root_vfs, NULL)) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount root VFS: %s", esp_err_to_name(err));
        return;
    }
}
