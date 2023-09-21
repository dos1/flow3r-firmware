#include "esp_vfs.h"
#include "esp_vfs_fat.h"

void get_statvfs(char* drive, uint32_t *out) {
    FATFS *fs;

    uint32_t fre_clust;
    if (f_getfree(drive, &fre_clust, &fs) != FR_OK) {
        return;
    }
    uint32_t fre_sect = (fre_clust * fs->csize);
    uint32_t tot_sect = (fs->n_fatent - 2) * fs->csize;
    uint32_t sector_size = fs->ssize;

    out[1] = fs->ssize;  // frsize, mapped to sector size
    out[2] = tot_sect;  // blocks, mapped to total sectors
    out[3] = fre_sect;  // bfree, mapped to free sectors
    out[4] = fre_sect;  // bavail, mapped to free sectors
}
