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

#ifndef LADDERLIB_ESP32_STD_H_
#define LADDERLIB_ESP32_STD_H_

#include "ladder.h"

/**
 * @fn bool esp32_on_scan_end(ladder_ctx_t*)
 * @brief
 *
 * @param ladder_ctx
 * @return
 */
bool esp32_on_scan_end(ladder_ctx_t *ladder_ctx);

/**
 * @fn bool esp32_on_instruction(ladder_ctx_t*)
 * @brief
 *
 * @param ladder_ctx
 * @return
 */
bool esp32_on_instruction(ladder_ctx_t *ladder_ctx);

/**
 * @fn bool esp32_on_task_before(ladder_ctx_t*)
 * @brief
 *
 * @param ladder_ctx
 * @return
 */
bool esp32_on_task_before(ladder_ctx_t *ladder_ctx);

/**
 * @fn bool esp32_on_task_after(ladder_ctx_t*)
 * @brief
 *
 * @param ladder_ctx
 * @return
 */
bool esp32_on_task_after(ladder_ctx_t *ladder_ctx);

/**
 * @fn void esp32_on_panic(ladder_ctx_t*)
 * @brief
 *
 * @param ladder_ctx
 */
void esp32_on_panic(ladder_ctx_t *ladder_ctx);

/**
 * @fn void esp32_on_end_task(ladder_ctx_t*)
 * @brief
 *
 * @param ladder_ctx
 */
void esp32_on_end_task(ladder_ctx_t *ladder_ctx);

/**
 * @fn void esp32_delay(long)
 * @brief
 *
 * @param msec
 */
void esp32_delay(long msec);

/**
 * @fn uint64_t esp32_millis(void)
 * @brief
 *
 * @return
 */
uint64_t esp32_millis(void);

#endif /* LADDERLIB_ESP32_STD_H_ */
