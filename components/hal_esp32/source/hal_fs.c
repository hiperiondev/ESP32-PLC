/*
 * Copyright 2025 Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * * Project Site: https://github.com/hiperiondev/ESP32-PLC *
 *
 * This is based on other projects, please contact their authors for more information.
 *
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#define _XOPEN_SOURCE 700
#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "esp_err.h"
#include "esp_log.h"

#include "esp_littlefs.h"
#include "hal_fs.h"

static const char* TAG = "hal_fs";

static bool littlefs_initialized = false;

int littlefs_init(void) {
    ESP_LOGI(TAG, "Initializing LittleFS");

    esp_vfs_littlefs_conf_t conf = {
        .base_path = MOUNT_POINT,
        .partition_label = PARTITION_LABEL,
        .format_if_mount_failed = true,
    };

    esp_err_t ret = esp_vfs_littlefs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find LittleFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }

    size_t total = 0, used = 0;
    ret = esp_littlefs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get LittleFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
    littlefs_initialized = true;
    return ESP_OK;
}

void littlefs_deinit(void) {
    if (!littlefs_initialized)
        return;
    esp_vfs_littlefs_unregister("littlefs");
    ESP_LOGI(TAG, "LittleFS unmounted");
}

FILE* littlefs_fopen(const char* file, const char* mode) {
    if (!littlefs_initialized)
        return NULL;
    char route[512];
    sprintf(route, "%s/%s", MOUNT_POINT, file);
    return fopen(route, mode);
}

FILE* littlefs_freopen(const char* filename, const char* opentype, FILE* stream) {
    if (!littlefs_initialized)
        return NULL;
    char route[512];

    sprintf(route, "%s/%s", MOUNT_POINT, filename);
    fclose(stream);
    return littlefs_fopen((char*)filename, (char*)opentype);
}

int littlefs_test(char* file) {
    char route[512];
    struct stat st;
    sprintf(route, "%s/%s", MOUNT_POINT, file);
    return stat(route, &st);
}

int littlefs_remove(const char* file) {
    if (!littlefs_initialized)
        return -1;
    char route[512];
    struct stat st;
    sprintf(route, "%s/%s", MOUNT_POINT, file);
    if (stat(route, &st) == 0) {
        unlink(route);
        return 0;
    }

    return -1;
}

int littlefs_rename(const char* file, char* newname) {
    if (!littlefs_initialized)
        return -1;
    char route_old[512];
    char route_new[512];
    sprintf(route_old, "%s/%s", MOUNT_POINT, file);
    sprintf(route_new, "%s/%s", MOUNT_POINT, newname);
    return rename(route_old, route_new);
}

int littlefs_ls(void) {
    struct dirent* de;

    DIR* dr = opendir(MOUNT_POINT);

    if (dr == NULL) {
        printf("Could not open current directory");
        return 0;
    }

    while ((de = readdir(dr)) != NULL)
        printf("%s\n", de->d_name);

    closedir(dr);

    return 0;
}
