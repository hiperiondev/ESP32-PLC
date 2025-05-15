/*
 * Copyright 2025 Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * * Project Site: https://github.com/hiperiondev/ladderlib *
 *
 * This is based on other projects:
 *    PLsi (https://github.com/ElPercha/PLsi)
 *
 *    please contact their authors for more information.
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

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "ladder.h"
#include "ladderlib_esp32_gpio.h"
#include "ladderlib_esp32_std.h"

static const char *TAG = "ladderlib_esp32_gpio";

const uint32_t inputs[] = {
    INPUT_00, //
    INPUT_01, //
    INPUT_02, //
    INPUT_03, //
    INPUT_04, //
    INPUT_05, //
    INPUT_06, //
    INPUT_07, //
};

const uint32_t outputs[] = {
    OUTPUT_00, //
    OUTPUT_01, //
    OUTPUT_02, //
    OUTPUT_03, //
    OUTPUT_04, //
    OUTPUT_05, //
};

static bool _esp32_gpio_read_init(void) {
    gpio_config_t io_conf = {};

    io_conf.pin_bit_mask = ((1ULL << INPUT_00) | (1ULL << INPUT_01) | (1ULL << INPUT_02) | (1ULL << INPUT_03) | (1ULL << INPUT_04) | (1ULL << INPUT_05)
#ifdef ADC_01
                            | 0
#else
                            | (1ULL << INPUT_06)
#endif
#ifdef ADC_02
                            | 0
#else
                            | (1ULL << INPUT_07)
#endif
    );
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    if (gpio_config(&io_conf) != ESP_OK) {
        ESP_LOGI(TAG, "ERROR gpio input config");
        return false;
    }

    return true;
}

static bool _esp32_gpio_write_init(void) {
    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = ((1ULL << OUTPUT_00) | (1ULL << OUTPUT_01) | (1ULL << OUTPUT_02) | (1ULL << OUTPUT_03)
#ifdef DAC_01
                            | 0
#else
                            | (1ULL << OUTPUT_04)
#endif
#ifdef DAC_02
                            | 0
#else
                            | (1ULL << OUTPUT_05)
#endif
    );
    io_conf.pull_down_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    if (gpio_config(&io_conf) != ESP_OK) {
        ESP_LOGI(TAG, "ERROR gpio output config");
        return false;
    }

    return true;
}

bool esp32_local_init_read(ladder_ctx_t *ladder_ctx, uint32_t id, bool init) {
    if (init) {
        if (!_esp32_gpio_read_init())
            return false;

        (*ladder_ctx).input[id].I = calloc(sizeof(inputs) / sizeof(uint32_t), sizeof(uint8_t));
        (*ladder_ctx).input[id].IW = calloc(2, sizeof(int32_t));
        (*ladder_ctx).input[id].Ih = calloc(2, sizeof(uint8_t));
        (*ladder_ctx).input[id].i_qty = sizeof(inputs) / sizeof(uint32_t);
        (*ladder_ctx).input[id].iw_qty = 2;

        for (uint32_t n = 0; n < sizeof(inputs) / sizeof(uint32_t); n++) {
            (*ladder_ctx).input[id].I[n] = 0;
            (*ladder_ctx).input[id].Ih[n] = 0;
        }
        for (uint32_t n = 0; n < 2; n++) {
            (*ladder_ctx).input[id].IW[n] = 0;
        }
    } else {
        free((*ladder_ctx).input[id].I);
        free((*ladder_ctx).input[id].IW);
        free((*ladder_ctx).input[id].Ih);
        (*ladder_ctx).input[id].i_qty = 0;
        (*ladder_ctx).input[id].iw_qty = 0;
    }

    return true;
}

bool esp32_local_init_write(ladder_ctx_t *ladder_ctx, uint32_t id, bool init) {
    if (init) {
        if (!_esp32_gpio_write_init())
            return false;

        (*ladder_ctx).output[id].Q = calloc(sizeof(outputs) / sizeof(uint32_t), sizeof(uint8_t));
        (*ladder_ctx).output[id].QW = calloc(2, sizeof(int32_t));
        (*ladder_ctx).output[id].Qh = calloc(sizeof(outputs) / sizeof(uint32_t), sizeof(uint8_t));
        (*ladder_ctx).output[id].q_qty = sizeof(outputs) / sizeof(uint32_t);
        (*ladder_ctx).output[id].qw_qty = 2;
    } else {
        free((*ladder_ctx).output[id].Q);
        free((*ladder_ctx).output[id].QW);
        free((*ladder_ctx).output[id].Qh);
        (*ladder_ctx).output[id].q_qty = 0;
        (*ladder_ctx).output[id].qw_qty = 0;
    }

    return true;
}

void esp32_local_read(ladder_ctx_t *ladder_ctx, uint32_t id) {
    for (uint32_t is = 0; is < sizeof(inputs) / sizeof(inputs[0]); is++) {
        (*ladder_ctx).input[id].Ih[is] = (*ladder_ctx).input[id].I[is];
#ifdef INVERT_INPUT
        (*ladder_ctx).input[id].I[is] = !(uint8_t)gpio_get_level(inputs[is]);
#else
        (*ladder_ctx).input[id].I[is] = (uint8_t)gpio_get_level(inputs[is]);
#endif
    }
}

void esp32_local_write(ladder_ctx_t *ladder_ctx, uint32_t id) {
    for (uint32_t p = 0; p < sizeof(outputs) / sizeof(outputs[0]); p++) {
        (*ladder_ctx).output[id].Qh[p] = (*ladder_ctx).output[id].Q[p];
#ifdef INVERT_OUTPUT
        gpio_set_level(outputs[p], !(*ladder_ctx).output[id].Q[p]));
#else
        gpio_set_level(outputs[p], (*ladder_ctx).output[id].Q[p]);
#endif
    }
}

void _esp32_port_test(bool input) {
    if (input) {
        uint64_t start = 0;
        bool port = false;
        bool ports[sizeof(inputs) / sizeof(inputs[0])] = {};

        ESP_LOGI(TAG, "(Stop after 10 sec of no input)");
        start = esp32_millis();
        while (1) {
            for (uint32_t is = 0; is < sizeof(inputs) / sizeof(inputs[0]); is++) {
                port = gpio_get_level(inputs[is]);
                if (port != ports[is]) {
                    ports[is] = port;
                    ESP_LOGI(TAG, "PORT %d %s", (int)is, (port == 0 ? "OFF" : "ON"));
                    start = esp32_millis();
                }
            }
            esp32_delay(1);

            if (esp32_millis() - start > 10000)
                break;
        }
    } else {
        for (uint8_t cnt = 1; cnt < 11; cnt++) {
            printf("cnt: %d\n", cnt);
            for (uint32_t p = 0; p < sizeof(outputs) / sizeof(outputs[0]); p++) {
                gpio_set_level(outputs[p], cnt % 2);
                vTaskDelay(500 / portTICK_PERIOD_MS);
            }
        }
    }
}

void _clear_io(ladder_ctx_t *ladder_ctx, uint32_t id) {
    for (uint32_t is = 0; is < sizeof(inputs) / sizeof(inputs[0]); is++) {
        (*ladder_ctx).input[id].Ih[is] = 0;
    }
    for (uint32_t p = 0; p < sizeof(outputs) / sizeof(outputs[0]); p++) {
        (*ladder_ctx).output[id].Qh[p] = (*ladder_ctx).output[id].Q[p] = 0;
#ifdef INVERT_OUTPUT
        gpio_set_level(outputs[p], !(*ladder_ctx).output[id].Q[p]));
#else
        gpio_set_level(outputs[p], (*ladder_ctx).output[id].Q[p]);
#endif
    }
}
