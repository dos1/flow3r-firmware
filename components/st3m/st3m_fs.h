#pragma once

// Mount filesystem. An error will be loged if the mount failed.
//
// Filesystem layout:
//  / - FAT from flash ('vfat' partition, wear leveled)
//
// This function must not be called more than once.
void st3m_fs_init(void);