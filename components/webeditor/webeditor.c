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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <esp_http_server.h>
#include <esp_log.h>

#include "ladder_program_json.h"
#include "webeditor.h"

static const char *TAG = "WebSocket Server";
extern const char index_html[] asm("_binary_ladder_editor_html_start");
extern const char favicon_ico[] asm("_binary_webeditor_favicon_ico_start");
extern ladder_ctx_t ladder_ctx;
bool websocket_open = false;
static httpd_handle_t server = NULL;
static char *response_data = NULL;

static char *ws_commands[] = {
    "get_flag",
    "load",
    "save",
};

enum WS_COMMAND {
    WS_GET_FLAG,
    WS_LOAD,
    WS_SAVE,
};

typedef struct async_resp_arg_s {
    httpd_handle_t hd;
    int fd;
} async_resp_arg_t;

static esp_err_t root_get_req_handler(httpd_req_t *req) {
    if (response_data != NULL)
        return httpd_resp_send(req, response_data, HTTPD_RESP_USE_STRLEN);
    else
        return httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t favicon_get_req_handler(httpd_req_t *req) {
    return httpd_resp_send(req, favicon_ico, HTTPD_RESP_USE_STRLEN);
}

static void ws_async_send(void *arg) {
    httpd_ws_frame_t ws_pkt;
    async_resp_arg_t *resp_arg = arg;
    httpd_handle_t hd = resp_arg->hd;

    if (response_data == NULL) {
        ESP_LOGI(TAG, "No response data");
        return;
    }

    //ESP_LOGI(TAG, "Start response data");

    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = (uint8_t *)response_data;
    ws_pkt.len = strlen(response_data);
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    static size_t max_clients = 2;
    size_t fds = max_clients;
    int client_fds[max_clients];

    esp_err_t ret = httpd_get_client_list(server, &fds, client_fds);
    //ESP_LOGI(TAG, "Client list err: %d", ret);
    //ESP_LOGI(TAG, "[HTTPD CLIENT TYPE]");
    //for (int i = 0; i < fds; i++) {
        //int client_info = httpd_ws_get_fd_info(server, client_fds[i]);
        //ESP_LOGI(TAG, "    [%d] = %s (%d)", i,
        //         client_info == HTTPD_WS_CLIENT_INVALID     ? "HTTPD_WS_CLIENT_INVALID"
        //         : client_info == HTTPD_WS_CLIENT_HTTP      ? "HTTPD_WS_CLIENT_HTTP"
        //         : client_info == HTTPD_WS_CLIENT_WEBSOCKET ? "HTTPD_WS_CLIENT_WEBSOCKET"
        //                                                    : "UNKNOWN",
        //         client_info);
    //}

    if (ret != ESP_OK) {
        ESP_LOGI(TAG, "No clients");
        free(response_data);
        response_data = NULL;
        return;
    }

    for (int i = 0; i < fds; i++) {
        int client_info = httpd_ws_get_fd_info(server, client_fds[i]);
        if (client_info == HTTPD_WS_CLIENT_WEBSOCKET) {
            //ESP_LOGI(TAG, "Response on client %d", i);
            httpd_ws_send_frame_async(hd, client_fds[i], &ws_pkt);
        }
    }
    if (resp_arg != NULL)
        free(resp_arg);
    if (response_data != NULL)
        free(response_data);
    response_data = NULL;
    //ESP_LOGI(TAG, "End response data");
}

static esp_err_t trigger_async_send(httpd_handle_t handle, httpd_req_t *req) {
    if (req == NULL)
        return ESP_OK;

    esp_err_t err = 0;
    async_resp_arg_t *resp_arg = malloc(sizeof(struct async_resp_arg_s));
    resp_arg->hd = req->handle;
    resp_arg->fd = httpd_req_to_sockfd(req);
    err = httpd_queue_work(handle, ws_async_send, resp_arg);
    return err;
}

static esp_err_t handle_ws_req(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "Handshake done, the new connection was opened");
        return ESP_OK;
    }

    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    uint8_t err = 0;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGI(TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
        return ret;
    }

    if (ws_pkt.len) {
        buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL) {
            ESP_LOGI(TAG, "Failed to calloc memory for buf");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK) {
            ESP_LOGI(TAG, "httpd_ws_recv_frame failed with %d", ret);
            free(buf);
            return ret;
        }
        //ESP_LOGI(TAG, "Got packet with message: %s", ws_pkt.payload);

        if (ws_pkt.type == HTTPD_WS_TYPE_TEXT) {
            char type[64], cmd[64];
            char *str_start = NULL, *str_end = NULL;

            str_start = strchr((char *)ws_pkt.payload, '"');
            if (str_start == NULL)
                return ESP_OK;

            str_end = strchr(str_start + 1, '"');
            if (str_end == NULL)
                return ESP_OK;

            memset(type, 0, 64);
            memcpy(type, str_start + 1, (str_end - str_start - 1) < 64 ? (str_end - str_start - 1) : 63);

            str_start = strchr(str_end + 1, '"');
            if (str_start == NULL)
                return ESP_OK;

            str_end = strchr(str_start + 1, '"');
            if (str_end == NULL)
                return ESP_OK;

            memset(cmd, 0, 64);
            memcpy(cmd, str_start + 1, (str_end - str_start - 1) < 64 ? (str_end - str_start - 1) : 63);

            ESP_LOGI(TAG, "type: %s, command: %s", type, cmd);

            int8_t _cmd = -1;
            for (uint8_t n = 0; n < sizeof(ws_commands) / sizeof(ws_commands[0]); n++) {
                if (strcmp(type, "action") == 0 && strcmp(cmd, ws_commands[n]) == 0) {
                    _cmd = n;
                    break;
                }
            }

            if (response_data != NULL) {
                free(response_data);
                response_data = NULL;
            }

            switch (_cmd) {
                case WS_GET_FLAG:
                    ESP_LOGI(TAG, "Requested: get_flag");
                    response_data = strdup("{\"flag\":\"sameDimensions\",\"value\":false}");
                    websocket_open = true;
                    break;
                case WS_LOAD:
                    ESP_LOGI(TAG, "Requested: load");
                    char *json = NULL;
                    if ((err = ladder_program_to_json("", &json, &ladder_ctx, true)) != JSON_ERROR_OK) {
                        ESP_LOGI(TAG, ">> ERROR: Program to websocket (%d)\n", err);
                        return 1;
                    }

                    if (json != NULL)
                        ESP_LOGI(TAG, "JSON: ok");
                    else
                        ESP_LOGI(TAG, "JSON: empty");

                    response_data = malloc(strlen(json) + 35);
                    sprintf(response_data, "{\"action\":\"load_response\",\"data\": %s}", json);
                    free(json);
                    break;
                case WS_SAVE:
                    ESP_LOGI(TAG, "Requested: save");
                    char *prg = NULL;
                    prg = strchr((char *)ws_pkt.payload, '[');
                    if (prg == NULL) {
                        ESP_LOGI(TAG, ">> ERROR: Program from websocket");
                        return ESP_OK;
                    }

                    prg[strlen(prg) - 1] = ' ';

                    if ((err = ladder_json_to_program("", prg, &ladder_ctx, true)) != JSON_ERROR_OK) {
                        ESP_LOGI(TAG, ">> ERROR: Program from websocket (%d)\n", err);
                        return 1;
                    }
                    break;
                default:
                    break;
            }

            free(buf);
            return trigger_async_send(req->handle, req);
        }
    }

    ESP_LOGI(TAG, "frame len is %d", ws_pkt.len);

    return ESP_OK;
}

void start_websocket_server(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 32768;
    config.core_id = 0;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t root = {
            .uri = "/",                      //
            .method = HTTP_GET,              //
            .handler = root_get_req_handler, //
            .user_ctx = NULL                 //
        };
        httpd_register_uri_handler(server, &root);

        httpd_uri_t favicon = {
            .uri = "/favicon.ico",              //
            .method = HTTP_GET,                 //
            .handler = favicon_get_req_handler, //
            .user_ctx = NULL                    //
        };
        httpd_register_uri_handler(server, &favicon);

        httpd_uri_t ws = {
            .uri = "/ws",             //
            .method = HTTP_GET,       //
            .handler = handle_ws_req, //
            .user_ctx = NULL,         //
            .is_websocket = true      //
        };
        httpd_register_uri_handler(server, &ws);

        ESP_LOGI(TAG, "Websocket server started");
    } else {
        ESP_LOGI(TAG, "Websocket server failed");
    }
}

esp_err_t ws_send_netstate(bool running) {
    esp_err_t err = 0;
    char *msg = malloc(37);
    char msg_status[128];
    async_resp_arg_t *arg = malloc(sizeof(struct async_resp_arg_s));
    arg->hd = server;

    if (running) {
        msg = malloc(37);
        strcpy(msg, "{\"status\":\"running\",\"cell_states\":[");
        for (unsigned int network = 0; network < ladder_ctx.ladder.quantity.networks; network++) {
            for (unsigned int column = 0; column < ladder_ctx.exec_network->cols; column++) {
                for (unsigned int row = 0; row < ladder_ctx.exec_network->rows; row++) {
                    memset(msg_status, 0, 128);
                    snprintf(msg_status, 127, "{\"networkId\":%u,\"row\":%u,\"col\":%u,\"state\":%u},", network, row, column,
                             (unsigned int)(ladder_ctx.network[network].cells[row][column].state == true ? 1 : 0));
                    msg = realloc(msg, strlen(msg) + strlen(msg_status) + 1);
                    if (msg == NULL) {
                        ESP_LOGI(TAG, "Can't allocate networks status");
                        return ESP_FAIL;
                    }
                    memcpy(msg + strlen(msg), msg_status, strlen(msg_status) + 1);
                }
            }
        }
        memset(msg_status, 0, 128);
        msg[strlen(msg) - 1] = ' ';
        strcpy(msg_status, "]}");
        msg = realloc(msg, strlen(msg) + strlen(msg_status) + 1);
        if (msg == NULL) {
            ESP_LOGI(TAG, "Can't allocate networks status");
            return ESP_FAIL;
        }
        memcpy(msg + strlen(msg), msg_status, strlen(msg_status) + 1);
    } else {
        msg = malloc(37);
        strcpy(msg, "{\"status\":\"not_running\"}");
    }

    response_data = msg;
    ws_async_send(arg);

    return err;
}
