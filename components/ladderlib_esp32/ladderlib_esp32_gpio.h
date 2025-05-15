/*
 * Copyright 2025 Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * * Project Site: https://github.com/hiperiondev/ESP32-PLC *.
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
