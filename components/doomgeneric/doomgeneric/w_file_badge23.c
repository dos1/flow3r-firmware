//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
// Copyright(C) 2023 q3k
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	WAD I/O functions.
//

#include <stdio.h>
#include <string.h>

#include "m_misc.h"
#include "w_file.h"
#include "z_zone.h"

#include "esp_log.h"
#include "esp_partition.h"
#include "esp_err.h"

static const char *TAG = "doom-file";

typedef struct
{
    wad_file_t wad;
    const esp_partition_t* part;
} badge23_wad_file_t;

extern wad_file_class_t badge23_wad_file;

static wad_file_t *W_Badge23_OpenFile(char *path)
{
    ESP_LOGI(TAG, "OpenFile: %s", path);
    if (strcmp(path, "doom1.wad") != 0) {
        ESP_LOGE(TAG, "Unhandled WAD file %s", path);
        return NULL;
    }

    badge23_wad_file_t *result;
    const esp_partition_t *part = esp_partition_find_first(66, 6, NULL);
    if (part == NULL) {
        ESP_LOGE(TAG, "WAD partition 66,6 not found");
        return NULL;
    }

    // Create a new badge23_wad_file_t to hold the file handle.

    result = Z_Malloc(sizeof(badge23_wad_file_t), PU_STATIC, 0);
    result->wad.file_class = &badge23_wad_file;
    result->wad.mapped = NULL;
    result->wad.length = part->size;
    result->part = part;
    ESP_LOGI(TAG, "WAD file %s succesfully opened from flash", path);

    return &result->wad;
}

static void W_Badge23_CloseFile(wad_file_t *wad)
{
    badge23_wad_file_t *badge23_wad;

    badge23_wad = (badge23_wad_file_t *) wad;
    Z_Free(badge23_wad);
}

// Read data from the specified position in the file into the 
// provided buffer.  Returns the number of bytes read.

size_t W_Badge23_Read(wad_file_t *wad, unsigned int offset,
                   void *buffer, size_t buffer_len)
{
    badge23_wad_file_t *badge23_wad;

    badge23_wad = (badge23_wad_file_t *) wad;
    size_t nbytes = buffer_len;
    if ((nbytes + offset) > wad->length) {
        nbytes = wad->length - offset;
    }

    esp_err_t res;
    if (res = esp_partition_read(badge23_wad->part, offset, buffer, nbytes), res != ESP_OK) {
        ESP_LOGE(TAG, "esp_partition_read(_, %x, _, %x) failed: %s", offset, buffer_len, esp_err_to_name(res));
    }
    return nbytes;
}


wad_file_class_t badge23_wad_file = 
{
    W_Badge23_OpenFile,
    W_Badge23_CloseFile,
    W_Badge23_Read,
};


