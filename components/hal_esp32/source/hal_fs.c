/*
 * @file hal_fs.c
 * @brief FILESYSTEM port
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see
 * @note This is based on other projects. See license files
 */

// #include "esp_log_timestamp.h"
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
