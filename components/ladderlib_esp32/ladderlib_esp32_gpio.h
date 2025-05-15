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

#ifndef LADDERLIB_ESP32_GPIO_H_
#define LADDERLIB_ESP32_GPIO_H_

#include <stdbool.h>
#include <stdint.h>

#include "ladder.h"

#define INVERT_INPUT
//#define INVERT_OUTPUT

#define INPUT_00 GPIO_NUM_34
#define INPUT_01 GPIO_NUM_35
#define INPUT_02 GPIO_NUM_32
#define INPUT_03 GPIO_NUM_13
#define INPUT_04 GPIO_NUM_15
#define INPUT_05 GPIO_NUM_0
#define INPUT_06 GPIO_NUM_39
#define INPUT_07 GPIO_NUM_36

#define OUTPUT_00 GPIO_NUM_4
#define OUTPUT_01 GPIO_NUM_16
#define OUTPUT_02 GPIO_NUM_2
#define OUTPUT_03 GPIO_NUM_17
#define OUTPUT_04 GPIO_NUM_26
#define OUTPUT_05 GPIO_NUM_25

#define ADC_01 INPUT_06
#define ADC_02 INPUT_07

//#define DAC_01 OUTPUT_04
//#define DAC_02 OUTPUT_05

extern const uint32_t inputs[];
extern const uint32_t outputs[]; 

/**
 * @fn bool esp32_local_init_read(ladder_ctx_t *ladder_ctx, uint32_t id, bool init)
 * @brief
 *
 * @param ladder_ctx
 * @param id
 * @param init
 * @return
 */
bool esp32_local_init_read(ladder_ctx_t *ladder_ctx, uint32_t id, bool init);

/**
 * @fn bool esp32_local_init_write(ladder_ctx_t *ladder_ctx, uint32_t id, bool init)
 * @brief
 *
 * @param ladder_ctx
 * @param id
 * @param init
 * @return
 */
bool esp32_local_init_write(ladder_ctx_t *ladder_ctx, uint32_t id, bool init);

/**
 * @fn void esp32_local_read(ladder_ctx_t *ladder_ctx, uint32_t id)
 * @brief
 *
 * @param ladder_ctx
 * @param id
 * @return
 */
void esp32_local_read(ladder_ctx_t *ladder_ctx, uint32_t id);

/**
 * @fn void esp32_local_write(ladder_ctx_t *ladder_ctx, uint32_t id)
 * @brief
 *
 * @param ladder_ctx
 * @param id
 * @return
 */
void esp32_local_write(ladder_ctx_t *ladder_ctx, uint32_t id);

/**
 * @fn void _esp32_port_test(bool input)
 * @brief
 *
 * @param input
 */
void _esp32_port_test(bool input);

/**
 * @fn void _esp32_port_test(bool input)
 * @brief
 *
 * @param input
 */
void _clear_io(ladder_ctx_t *ladder_ctx, uint32_t id);

#endif /* LADDERLIB_ESP32_GPIO_H_ */
