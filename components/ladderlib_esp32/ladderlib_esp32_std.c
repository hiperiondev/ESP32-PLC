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

#include <stdbool.h>
#include <stdint.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ladder.h"
#include "ladderlib_esp32_std.h"

static const char *TAG = "ladderlib_esp32_std";

static const char *_ladder_status_str[] = {
    "STOPPED",  //
    "RUNNING",  //
    "ERROR",    //
    "EXIT_TSK", //
    "NULLFN",   //
    "INVALID",  //
};

static const char *_fn_str[] = {
    "NOP",     //
    "CONN",    //
    "NEG",     //
    "NO",      //
    "NC",      //
    "RE",      //
    "FE",      //
    "COIL",    //
    "COILL",   //
    "COILU",   //
    "TON",     //
    "TOF",     //
    "TP",      //
    "CTU",     //
    "CTD",     //
    "MOVE",    //
    "SUB",     //
    "ADD",     //
    "MUL",     //
    "DIV",     //
    "MOD",     //
    "SHL",     //
    "SHR",     //
    "ROL",     //
    "ROR",     //
    "AND",     //
    "OR",      //
    "XOR",     //
    "NOT",     //
    "EQ",      //
    "GT",      //
    "GE",      //
    "LT",      //
    "LE",      //
    "NE",      //
    "FOREIGN", //
    "TMOVE",   //
};

static const char *_fn_err_str[] = {
    "OK",         //
    "GETPREVVAL", //
    "GETDATAVAL", //
    "NOFOREIGN",  //
    "NOTABLE",    //
    "OUTOFRANGE", //
    // [...] //
    "FAIL", //
};

void esp32_delay(long msec) {
    vTaskDelay(msec / portTICK_PERIOD_MS);
}

uint64_t esp32_millis(void) {
    return esp_timer_get_time() / 1000;
}

bool esp32_on_scan_end(ladder_ctx_t *ladder_ctx) {
    return false;
}

bool esp32_on_instruction(ladder_ctx_t *ladder_ctx) {
    return false;
}

bool esp32_on_task_before(ladder_ctx_t *ladder_ctx) {
    return false;
}

bool esp32_on_task_after(ladder_ctx_t *ladder_ctx) {
    if ((*ladder_ctx).scan_internals.actual_scan_time < 1)
        esp32_delay(1);

    return false;
}

void esp32_on_panic(ladder_ctx_t *ladder_ctx) {
    printf("\n");
    printf("------------ PANIC ------------\n");
    printf("        STATE: %s (%d)\n", _ladder_status_str[(*ladder_ctx).ladder.state], (*ladder_ctx).ladder.state);
    printf("  INSTRUCTION: %s (%d)\n", _fn_str[(*ladder_ctx).ladder.last.instr], (*ladder_ctx).ladder.last.instr);
    printf("      NETWORK: %d\n", (int)(*ladder_ctx).ladder.last.network);
    printf("         CELL: %d, %d\n", (int)(*ladder_ctx).ladder.last.cell_row, (int)(*ladder_ctx).ladder.last.cell_column);
    printf("        ERROR: %s (%d)\n", _fn_err_str[(*ladder_ctx).ladder.last.err], (*ladder_ctx).ladder.last.err);
    printf("-------------------------------\n\n");
}

void esp32_on_end_task(ladder_ctx_t *ladder_ctx) {
    ESP_LOGI(TAG, "End Task Ladder");
    vTaskDelete(NULL);
}
