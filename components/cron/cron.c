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

#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ccronexpr.h"
#include "ll.h"

#include "cron.h"

static const char *TAG = "cron";

#define CRON_TIMER_MS  500
#define CRON_QUEUE_LEN 5

typedef struct cron_s {
    uint32_t id;
    cron_expr_t expr;
    time_t next;
    bool enabled;
    void *fn_ptr;
    uint32_t fn_value;
} cron_t;

static int8_t cron_tz = 0;
static uint32_t next_id;
static cron_t *_list_cron;
static TimerHandle_t cron_timer_hndl;
static QueueHandle_t cron_queue;
static TaskHandle_t cron_task_hndl;

static void cron_execute_task(void *pvParameters) {
    void (*cron_fn)(uint32_t value);
    uint32_t value;
    while (1) {
        if (cron_queue) {
            if (xQueueReceive(cron_queue, &cron_fn, (TickType_t)portMAX_DELAY)) {
                if (xQueueReceive(cron_queue, &value, (TickType_t)portMAX_DELAY)) {
                    cron_fn(value);
                }
            }
        }
    }
}

static void cron_timer(TimerHandle_t xTimer) {
    time_t now = 0;

    time(&now);
    now += 3600 * cron_tz;

    ll_foreach(_list_cron, item) {
        if (item != NULL && item->enabled) {
            if (difftime(item->next, now) <= 0) {
                item->next = cron_next(&item->expr, now);
                xQueueSend(cron_queue, &(item->fn_ptr), (TickType_t)10);
                xQueueSend(cron_queue, &(item->fn_value), (TickType_t)10);
                break;
            }
        }
    }
}

esp_err_t cron_start(int8_t tz) {
    ESP_LOGI(TAG, "cron_start");
    next_id = 0;
    _list_cron = NULL;
    cron_tz = tz;
    cron_queue = xQueueCreate(CRON_QUEUE_LEN, sizeof(uint32_t));

    ESP_LOGI(TAG, "start cron timer");
    cron_timer_hndl = xTimerCreate("cron_timer", pdMS_TO_TICKS(CRON_TIMER_MS), pdTRUE, (void *)0, cron_timer);

    if (xTimerStart(cron_timer_hndl, 0) != pdPASS) {
        ESP_LOGI(TAG, "error starting timer");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "start cron task");
    xTaskCreate(cron_execute_task, "cron_execute_task", 4096, NULL, 12, &cron_task_hndl);

    return ESP_OK;
}

void cron_stop(void) {
    xTimerStop(cron_timer_hndl, 0);
    vTaskDelete(cron_task_hndl);
    vQueueDelete(cron_queue);
    ll_free(_list_cron);
}

int32_t cron_add(char *expr, void *cron_cb_ptr, uint32_t fn_value) {
    const char *err = NULL;
    cron_expr_t parsed;
    time_t now = 0;

    time(&now);
    now += 3600 * cron_tz;

    cron_parse_expr(expr, &parsed, &err);
    if (err != NULL)
        return ESP_FAIL;

    _list_cron = ll_new(_list_cron);
    memcpy(&(_list_cron->expr), &parsed, sizeof(cron_expr_t));
    _list_cron->enabled = true;
    _list_cron->id = next_id++;
    _list_cron->next = cron_next(&parsed, now);
    _list_cron->fn_ptr = cron_cb_ptr;
    _list_cron->fn_value = fn_value;
    return _list_cron->id;
}

esp_err_t cron_enable(uint32_t id, bool enable) {
    bool found = false;

    ll_foreach(_list_cron, item) {
        if (item != NULL && item->id == id) {
            item->enabled = enable;
            found = true;
            break;
        }
    }

    return found ? ESP_OK : ESP_FAIL;
}

esp_err_t cron_delete(uint32_t id) {
    bool found = false;

    ll_foreach(_list_cron, item) {
        if (item != NULL && item->id == id) {
            ESP_LOGI(TAG, "delete id=%u", (uint)id);
            ll_pop(item);
            found = true;
            break;
        }
    }

    return found ? ESP_OK : ESP_FAIL;
}

void cron_list(void) {
    ESP_LOGI(TAG, "cron list");

    ll_foreach(_list_cron, item) {
        if (item != NULL)
            ESP_LOGI(TAG, "id=%d enabled=%d value %u cb=%p", (int)item->id, (int)item->enabled, (unsigned)item->fn_value, item->fn_ptr);
    }
}
