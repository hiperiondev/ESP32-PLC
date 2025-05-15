/*
 * Copyright 2025 Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * * Project Site: https://github.com/hiperiondev/ladderlib *
 *
 * This is based on other projects:
 *    PLsi (https://github.com/ElPercha/PLsi)
 *
 *    please contact their authors for more information.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 *
 */

#include <string.h>

#include "esp_console.h"
#include "freertos/task.h"
#include "hal_fs.h"
#include "nvs_flash.h"
// #include <mdns.h>

#include "cmd_ladderlib.h"
#include "cmd_system.h"
#include "ladder.h"
#include "ladderlib_esp32_gpio.h"
#include "ladderlib_esp32_std.h"
#include "wifi-provisioning.h"

#define CONSOLE_MAX_COMMAND_LINE_LENGTH 1024
#define HISTORY_PATH                    "/littlefs/history.txt"

// registers quantity
#define QTY_M 8
#define QTY_C 8
#define QTY_T 8
#define QTY_D 8
#define QTY_R 8

ladder_ctx_t ladder_ctx;
TaskHandle_t laddertsk_handle;

//////////////////////////////////////////////////////////

void app_main(void) {
    nvs_flash_init();
    fs_init();

    wifi_provision_care("HiperionPLC");

    // ESP_LOGI(TAG, "Publish mDNS hostname %s.local.", TAG);
    // ESP_ERROR_CHECK(mdns_init());
    // ESP_ERROR_CHECK(mdns_hostname_set(TAG));

    ///////////////////////////////////////////////////////

    printf("Start console");

    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();

    repl_config.prompt = "ladderlib >";
    repl_config.max_cmdline_length = CONSOLE_MAX_COMMAND_LINE_LENGTH;

    repl_config.history_save_path = HISTORY_PATH;
    printf(" (Command history enabled)");

    //////// register commands ////////
    esp_console_register_help_command();

    register_system_common();
    register_ladder_status();
    register_fs_ls();
    register_fs_rm();
    register_ladder_save();
    register_ladder_load();
    register_ladder_start();
    register_ladder_stop();
    register_ftpserver();
    register_port_test();

    esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
    ///////////////////////////////////////////////////////

    printf("--[ laderlib version: %d.%d.%d ]--\n\n", LADDERLIB_VERSION_MAYOR, LADDERLIB_VERSION_MINOR, LADDERLIB_VERSION_PATCH);

    // initialize context
    if (!ladder_ctx_init(&ladder_ctx, 6, 7, 3, QTY_M, QTY_C, QTY_T, QTY_D, QTY_R, false)) {
        printf("ERROR Initializing\n");
    }

    // assign port functions
    if (!ladder_add_read_fn(&ladder_ctx, esp32_local_read, esp32_local_init_read)) {
        printf("ERROR Adding io read function\n");
        return;
    }

    if (!ladder_add_write_fn(&ladder_ctx, esp32_local_write, esp32_local_init_write)) {
        printf("ERROR Adding io write function\n");
        return;
    }

    ladder_ctx.on.scan_end = esp32_on_scan_end;
    ladder_ctx.on.instruction = esp32_on_instruction;
    ladder_ctx.on.task_before = esp32_on_task_before;
    ladder_ctx.on.task_after = esp32_on_task_after;
    ladder_ctx.on.panic = esp32_on_panic;
    ladder_ctx.on.end_task = esp32_on_end_task;
    ladder_ctx.hw.time.millis = esp32_millis;
    ladder_ctx.hw.time.delay = esp32_delay;

    ladder_ctx.ladder.state = LADDER_ST_STOPPED;
}
