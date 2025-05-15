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

#include <inttypes.h>
#include <string.h>

#include "esp_console.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "ftpserver.h"
#include "hal_fs.h"

#include "ladder.h"
#include "ladder_program_check.h"
#include "ladder_program_json.h"
#include "ladderlib_esp32_gpio.h"

// registers quantity
#define QTY_M 8
#define QTY_C 8
#define QTY_T 8
#define QTY_D 8
#define QTY_R 8

extern ladder_ctx_t ladder_ctx;
extern TaskHandle_t laddertsk_handle;

static const char *TAG = "cmd_ladderlib";

static const char *check_errors[] = {
    "OK",                //
    "I_INV_MODULE",      //
    "I_INV_PORT",        //
    "Q_INV_MODULE",      //
    "Q_INV_PORT",        //
    "IW_INV_MODULE",     //
    "IW_INV_PORT",       //
    "QW_INV_MODULE",     //
    "QW_INV_PORT",       //
    "NO_INPUT_MODULES",  //
    "NO_OUTPUT_MODULES", //
    "FAIL",              //
};

static int ladder_status(int argc, char **argv) {
    if (ladder_ctx.network == NULL)
        return 1;

    printf("[scan time: %llu ms]\n", ladder_ctx.scan_internals.actual_scan_time);
    printf("Toggle I: 0-7  (Q: exit)\n");
    printf("-----------------------\n");

    printf("I0.0-I0.1-I0.2-I0.3-I0.4-I0.5-I0.6-I0.7\n");
    printf("%04d-%04d-%04d-%04d-%04d-%04d-%04d-%04d\n", ladder_ctx.input[0].I[0], ladder_ctx.input[0].I[1], ladder_ctx.input[0].I[2], ladder_ctx.input[0].I[3],
           ladder_ctx.input[0].I[4], ladder_ctx.input[0].I[5], ladder_ctx.input[0].I[6], ladder_ctx.input[0].I[7]);
    printf("\n");

    printf("M0-M1-M2-M3-M4-M5-M6-M7\n");
    printf("%02d-%02d-%02d-%02d-%02d-%02d-%02d-%02d\n", ladder_ctx.memory.M[0], ladder_ctx.memory.M[1], ladder_ctx.memory.M[2], ladder_ctx.memory.M[3],
           ladder_ctx.memory.M[4], ladder_ctx.memory.M[5], ladder_ctx.memory.M[6], ladder_ctx.memory.M[7]);
    printf("\n");

    printf("Q0.0-Q0.1-Q0.2-Q0.3-Q0.4-Q0.5-Q0.6-Q0.7\n");
    printf("%04d-%04d-%04d-%04d-%04d-%04d-%04d-%04d\n", ladder_ctx.output[0].Q[0], ladder_ctx.output[0].Q[1], ladder_ctx.output[0].Q[2],
           ladder_ctx.output[0].Q[3], ladder_ctx.output[0].Q[4], ladder_ctx.output[0].Q[5], ladder_ctx.output[0].Q[6], ladder_ctx.output[0].Q[7]);

    printf("-----------------------\n");
    printf("        +----+----+-------+\n");
    printf("        | Td | Tr |  acc  |\n");
    printf("        +----+----+-------+\n");
    printf("Timer 0 | %d  | %d  | %05" PRIu32 " |\n", ladder_ctx.memory.Td[0], ladder_ctx.memory.Tr[0], ladder_ctx.timers[0].acc);
    printf("Timer 1 | %d  | %d  | %05" PRIu32 " |\n", ladder_ctx.memory.Td[1], ladder_ctx.memory.Tr[1], ladder_ctx.timers[1].acc);
    printf("Timer 2 | %d  | %d  | %05" PRIu32 " |\n", ladder_ctx.memory.Td[2], ladder_ctx.memory.Tr[2], ladder_ctx.timers[2].acc);
    printf("Timer 3 | %d  | %d  | %05" PRIu32 " |\n", ladder_ctx.memory.Td[3], ladder_ctx.memory.Tr[3], ladder_ctx.timers[3].acc);
    printf("Timer 4 | %d  | %d  | %05" PRIu32 " |\n", ladder_ctx.memory.Td[4], ladder_ctx.memory.Tr[4], ladder_ctx.timers[4].acc);
    printf("Timer 5 | %d  | %d  | %05" PRIu32 " |\n", ladder_ctx.memory.Td[5], ladder_ctx.memory.Tr[5], ladder_ctx.timers[5].acc);
    printf("Timer 6 | %d  | %d  | %05" PRIu32 " |\n", ladder_ctx.memory.Td[6], ladder_ctx.memory.Tr[6], ladder_ctx.timers[6].acc);
    printf("        +----+----+-------+\n");

    printf("          +----+----+-------+\n");
    printf("          | Cd | Cr |   C   |\n");
    printf("          +----+----+-------+\n");
    printf("Counter 0 | %d  | %d  | %05" PRIu32 " |\n", ladder_ctx.memory.Cd[0], ladder_ctx.memory.Cr[0], ladder_ctx.registers.C[0]);
    printf("Counter 1 | %d  | %d  | %05" PRIu32 " |\n", ladder_ctx.memory.Cd[1], ladder_ctx.memory.Cr[1], ladder_ctx.registers.C[1]);
    printf("Counter 2 | %d  | %d  | %05" PRIu32 " |\n", ladder_ctx.memory.Cd[2], ladder_ctx.memory.Cr[2], ladder_ctx.registers.C[2]);
    printf("Counter 3 | %d  | %d  | %05" PRIu32 " |\n", ladder_ctx.memory.Cd[3], ladder_ctx.memory.Cr[3], ladder_ctx.registers.C[3]);
    printf("Counter 4 | %d  | %d  | %05" PRIu32 " |\n", ladder_ctx.memory.Cd[4], ladder_ctx.memory.Cr[4], ladder_ctx.registers.C[4]);
    printf("Counter 5 | %d  | %d  | %05" PRIu32 " |\n", ladder_ctx.memory.Cd[5], ladder_ctx.memory.Cr[5], ladder_ctx.registers.C[5]);
    printf("Counter 6 | %d  | %d  | %05" PRIu32 " |\n", ladder_ctx.memory.Cd[6], ladder_ctx.memory.Cr[6], ladder_ctx.registers.C[6]);
    printf("          +----+----+-------+\n");

    return 0;
}

static int fn_fs_ls(int argc, char **argv) {
    fs_ls();

    return 0;
}

static int fn_fs_rm(int argc, char **argv) {
    if (argc < 2) {
        printf(">> Error: No file name\n");
        return 1;
    }

    fs_remove(argv[1]);

    return 0;
}

static int ladder_save(int argc, char **argv) {
    if (argc < 2) {
        printf(">> Error: No file name\n");
        return 1;
    }

    uint8_t err = 0;

    printf("Save program: %s\n", argv[1]);
    if ((err = ladder_program_to_json(argv[1], &ladder_ctx)) != JSON_ERROR_OK) {
        printf(">> ERROR: Save demo program (%d)\n", err);
        return 1;
    }

    printf("Dump OK\n");
    return 0;
}

static int ladder_load(int argc, char **argv) {
    if (argc < 2) {
        ESP_LOGI(TAG, ">> Error: No file name\n");
        return 1;
    }

    uint8_t err = 0;
    ladder_prg_check_t err_prg_check;

    printf("Load from: %s\n", argv[1]);
    if ((err = ladder_json_to_program(argv[1], &ladder_ctx)) != JSON_ERROR_OK) {
        printf(">> ERROR: Load demo program (%d)\n", err);
        return 1;
    }

    err_prg_check = ladder_program_check(ladder_ctx);
    if (err_prg_check.error != LADDER_ERR_PRG_CHECK_OK) {
        printf(">> ERROR: Program not valid %s (%u) at network:%" PRIu32 " [%" PRIu32 ",%" PRIu32 "] code: %u\n", check_errors[err_prg_check.error],
               err_prg_check.error, err_prg_check.network, err_prg_check.row, err_prg_check.column, err_prg_check.code);
        return 1;
    }

    return 0;
}

static int ladder_start(int argc, char **argv) {
    if (ladder_ctx.network == NULL) {
        ESP_LOGI(TAG, ">> ERROR: No networks!");
        return 1;
    }

    ladder_ctx.ladder.state = LADDER_ST_RUNNING;

    ESP_LOGI(TAG, "Start Task Ladder");
    if (xTaskCreatePinnedToCore(ladder_task, "ladder", 15000, (void *)&ladder_ctx, 10, &laddertsk_handle, 1) != pdPASS)
        ESP_LOGI(TAG, "ERROR: start task ladder");

    return 0;
}

void register_ladder_start(void) {
    const esp_console_cmd_t cmd = {
        .command = "start",
        .help = "Start ladder logic",
        .hint = NULL,
        .func = &ladder_start,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static int ladder_stop(int argc, char **argv) {
    ladder_ctx.ladder.state = LADDER_ST_EXIT_TSK;

    return 0;
}

static int ladder_ftpserver(int argc, char **argv) {
    ESP_LOGI(TAG, "Start FTP server");
    ftpserver_start("test", "test", "/littlefs");

    return 0;
}

static int port_test(int argc, char **argv) {
    if (argc < 2) {
        ESP_LOGI(TAG, ">> Error: input or output\n");
        return 1;
    }

    if (strcmp(argv[1], "input") == 0) {
        _esp32_port_test(true);
    } else if (strcmp(argv[1], "output") == 0) {
        _esp32_port_test(false);
    } else {
        ESP_LOGI(TAG, ">> Error: input or output\n");
        return 1;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

void register_ladder_status(void) {
    const esp_console_cmd_t cmd = {
        .command = "status",
        .help = "Get the current ladder status",
        .hint = NULL,
        .func = &ladder_status,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

void register_fs_ls(void) {
    const esp_console_cmd_t cmd = {
        .command = "ls",
        .help = "List files",
        .hint = NULL,
        .func = &fn_fs_ls,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

void register_fs_rm(void) {
    const esp_console_cmd_t cmd = {
        .command = "rm",
        .help = "Remove file",
        .hint = NULL,
        .func = &fn_fs_rm,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

void register_ladder_save(void) {
    const esp_console_cmd_t cmd = {
        .command = "save",
        .help = "Save networks to file",
        .hint = NULL,
        .func = &ladder_save,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

void register_ladder_load(void) {
    const esp_console_cmd_t cmd = {
        .command = "load",
        .help = "Load networks from file",
        .hint = NULL,
        .func = &ladder_load,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

void register_ladder_stop(void) {
    const esp_console_cmd_t cmd = {
        .command = "stop",
        .help = "Stop ladder logic",
        .hint = NULL,
        .func = &ladder_stop,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

void register_ftpserver(void) {
    const esp_console_cmd_t cmd = {
        .command = "ftpserver",
        .help = "Start FTP server",
        .hint = NULL,
        .func = &ladder_ftpserver,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

void register_port_test(void) {
    const esp_console_cmd_t cmd = {
        .command = "port_test",
        .help = "Port test (input/output)",
        .hint = NULL,
        .func = &port_test,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
