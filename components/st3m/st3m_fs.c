#include "st3m_fs.h"

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
	{ .d_ino = 1, .d_name = ".", .d_type = DT_DIR, },
	{ .d_ino = 2, .d_name = "flash", .d_type = DT_DIR, },
	{ .d_ino = 3, .d_name = "sd", .d_type = DT_DIR, },
	{ .d_ino = 4, .d_name = "console", .d_type = DT_DIR, },
};

// Handle of the wear levelling library instance
static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;

static int _root_vfs_close(int fd) {
    return 0;
} 

static int _root_vfs_fcntl(int fd, int cmd, int arg) {
    return 0;
}       

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
	return (DIR*)dir;
}

static struct dirent *_root_vfs_readdir(DIR *pdir) {
	st3m_rootfs_dir_t *dir = pdir;
	if (dir->ix >= (sizeof(_root_dirents)/sizeof(struct dirent))) {
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

	if ((err = esp_vfs_register("", &_root_vfs, NULL)) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to mount root VFS: %s", esp_err_to_name(err));
		return;
	}

    const esp_vfs_fat_mount_config_t mount_config = {
        .max_files = 4,
        .format_if_mount_failed = true,
        .allocation_unit_size = CONFIG_WL_SECTOR_SIZE
    };

    err = esp_vfs_fat_spiflash_mount("/flash", "vfs", &mount_config,
                                               &s_wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FAT FS: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "Mounted Flash VFS Partition at /flash");
}
