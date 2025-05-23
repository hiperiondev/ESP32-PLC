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

#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "dirent.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_types.h"
#include "lwip/sockets.h"
#include "ftpserver.h"

const char *FTP_TAG = "ftp";

#define CONFIG_FTP_USER     "test"
#define CONFIG_FTP_PASSWORD "test"

#define FTPSERVER_CMD_PORT                 21
#define FTPSERVER_ACTIVE_DATA_PORT         20
#define FTPSERVER_PASIVE_DATA_PORT         2024
#define FTPSERVER_CMD_SIZE_MAX             6
#define FTPSERVER_CMD_CLIENTS_MAX          1
#define FTPSERVER_DATA_CLIENTS_MAX         1
#define FTPSERVER_MAX_PARAM_SIZE           (FTPSERVER_ALLOC_PATH_MAX + 1)
#define FTPSERVER_UNIX_SECONDS_180_DAYS    15552000
#define FTPSERVER_DATA_TIMEOUT_MS          10000 // 10 seconds
#define FTPSERVER_SOCKETFIFO_ELEMENTS_MAX  4
#define FTPSERVER_USER_PASS_LEN_MAX        32
#define FTPSERVER_CMD_TIMEOUT_MS           (FTPSERVER_TIMEOUT*1000)
#define FTPSERVER_BUFFER_SIZE              1024
#define FTPSERVER_TIMEOUT                  300
#define FTPSERVER_ALLOC_PATH_MAX           (512)
#define FTPSEREVR_MAX_ACTIVE_INTERFACES    3

static char ftpserver_mount_point[128];

static int ftp_buff_size = FTPSERVER_BUFFER_SIZE;
static int ftp_timeout = FTPSERVER_CMD_TIMEOUT_MS;

static uint8_t ftp_stop = 0;
static char ftp_user[FTPSERVER_USER_PASS_LEN_MAX + 1];
static char ftp_pass[FTPSERVER_USER_PASS_LEN_MAX + 1];
static esp_netif_t *net_if[FTPSEREVR_MAX_ACTIVE_INTERFACES];

static ftp_data_t ftp_data = { 0 };
static char *ftp_path = NULL;
static char *ftp_scratch_buffer = NULL;
static char *ftp_cmd_buffer = NULL;
static uint8_t ftp_nlist = 0;
static const ftp_cmd_t ftp_cmd_table[] = {
        { "FEAT" }, { "SYST" }, { "CDUP" }, { "CWD" },  { "PWD"  },
        { "XPWD" }, { "SIZE" }, { "MDTM" }, { "TYPE" }, { "USER" },
        { "PASS" }, { "PASV" }, { "LIST" }, { "RETR" }, { "STOR" },
        { "DELE" }, { "RMD"  }, { "MKD"  }, { "RNFR" }, { "RNTO" },
        { "NOOP" }, { "QUIT" }, { "APPE" }, { "NLST" }, { "AUTH" }
};

uint64_t mp_hal_ticks_ms(void) {
    uint64_t time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    return time_ms;
}

int network_get_active_interfaces(void) {
    ESP_LOGI(FTP_TAG, "network_get_active_interfaces");
    int n_if = 2;

    net_if[0] = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    net_if[1] = esp_netif_get_handle_from_ifkey("WIFI_APDEF");

    return n_if;
}

static void stoupper(char *str) {
    while (str && *str != '\0') {
        *str = (char) toupper((int ) (*str));
        str++;
    }
}

static bool ftp_open_file(const char *path, const char *mode) {
    ESP_LOGI(FTP_TAG, "ftp_open_file: path=[%s]", path);
    char fullname[128];
    strcpy(fullname, ftpserver_mount_point);
    strcat(fullname, path);
    ESP_LOGI(FTP_TAG, "ftp_open_file: fullname=[%s]", fullname);
    ftp_data.fp = fopen(fullname, mode);
    if (ftp_data.fp == NULL) {
        ESP_LOGE(FTP_TAG, "ftp_open_file: open fail [%s]", fullname);
        return false;
    }
    ftp_data.e_open = E_FTP_FILE_OPEN;
    return true;
}

static void ftp_close_files_dir(void) {
    if (ftp_data.e_open == E_FTP_FILE_OPEN) {
        fclose(ftp_data.fp);
        ftp_data.fp = NULL;
    } else if (ftp_data.e_open == E_FTP_DIR_OPEN) {
        closedir(ftp_data.dp);
        ftp_data.dp = NULL;
    }
    ftp_data.e_open = E_FTP_NOTHING_OPEN;
}

static void ftp_close_filesystem_on_error(void) {
    ftp_close_files_dir();
    if (ftp_data.fp) {
        fclose(ftp_data.fp);
        ftp_data.fp = NULL;
    }
    if (ftp_data.dp) {
        closedir(ftp_data.dp);
        ftp_data.dp = NULL;
    }
}

static ftp_result_t ftp_read_file(char *filebuf, uint32_t desiredsize, uint32_t *actualsize) {
    ftp_result_t result = E_FTP_RESULT_CONTINUE;
    *actualsize = fread(filebuf, 1, desiredsize, ftp_data.fp);
    if (*actualsize == 0) {
        ftp_close_files_dir();
        result = E_FTP_RESULT_FAILED;
    } else if (*actualsize < desiredsize) {
        ftp_close_files_dir();
        result = E_FTP_RESULT_OK;
    }
    return result;
}

static ftp_result_t ftp_write_file(char *filebuf, uint32_t size) {
    ftp_result_t result = E_FTP_RESULT_FAILED;
    uint32_t actualsize = fwrite(filebuf, 1, size, ftp_data.fp);
    if (actualsize == size) {
        result = E_FTP_RESULT_OK;
    } else {
        ftp_close_files_dir();
    }
    return result;
}

static ftp_result_t ftp_open_dir_for_listing(const char *path) {
    if (ftp_data.dp) {
        closedir(ftp_data.dp);
        ftp_data.dp = NULL;
    }
    ESP_LOGI(FTP_TAG, "ftp_open_dir_for_listing path=[%s] MOUNT_POINT=[%s]", path, ftpserver_mount_point);
    char fullname[128];
    strcpy(fullname, ftpserver_mount_point);
    strcat(fullname, path);
    ESP_LOGI(FTP_TAG, "ftp_open_dir_for_listing: %s", fullname);
    ftp_data.dp = opendir(fullname);  // Open the directory
    if (ftp_data.dp == NULL) {
        return E_FTP_RESULT_FAILED;
    }
    ftp_data.e_open = E_FTP_DIR_OPEN;
    ftp_data.listroot = false;
    return E_FTP_RESULT_CONTINUE;
}

static int ftp_get_eplf_item(char *dest, uint32_t destsize, struct dirent *de) {
    char *type = (de->d_type & DT_DIR) ? "d" : "-";

    // Get full file path needed for stat function
    char fullname[128];
    strcpy(fullname, ftpserver_mount_point);
    strcat(fullname, ftp_path);
    if (fullname[strlen(fullname) - 1] != '/')
        strcat(fullname, "/");
    strcat(fullname, de->d_name);

    struct stat buf;
    int res = stat(fullname, &buf);
    ESP_LOGI(FTP_TAG, "ftp_get_eplf_item res=%d buf.st_size=%ld", res, buf.st_size);
    if (res < 0) {
        buf.st_size = 0;
        buf.st_mtime = 946684800; // Jan 1, 2000
    }

    char str_time[64];
    struct tm *tm_info;
    time_t now;
    if (time(&now) < 0)
        now = 946684800; // get the current time from the RTC
    tm_info = localtime(&buf.st_mtime); // get broken-down file time

    // if file is older than 180 days show dat,month,year else show month, day and time
    if ((buf.st_mtime + FTPSERVER_UNIX_SECONDS_180_DAYS) < now)
        strftime(str_time, 127, "%b %d %Y", tm_info);
    else
        strftime(str_time, 63, "%b %d %H:%M", tm_info);

    int addsize = destsize + 64;

    while (addsize >= destsize) {
        if (ftp_nlist)
            addsize = snprintf(dest, destsize, "%s\r\n", de->d_name);
        else
            addsize = snprintf(dest, destsize, "%srw-rw-rw-   1 root  root %9"PRIu32" %s %s\r\n", type, (uint32_t) buf.st_size, str_time, de->d_name);
        if (addsize >= destsize) {
            ESP_LOGW(FTP_TAG, "Buffer too small, reallocating [%d > %"PRIi32"]", ftp_buff_size, ftp_buff_size + (addsize - destsize) + 64);
            char *new_dest = realloc(dest, ftp_buff_size + (addsize - destsize) + 65);
            if (new_dest) {
                ftp_buff_size += (addsize - destsize) + 64;
                destsize += (addsize - destsize) + 64;
                dest = new_dest;
                addsize = destsize + 64;
            } else {
                ESP_LOGE(FTP_TAG, "Buffer reallocation ERROR");
                addsize = 0;
            }
        }
    }
    return addsize;
}

static ftp_result_t ftp_list_dir(char *list, uint32_t maxlistsize, uint32_t *listsize) {
    uint next = 0;
    uint listcount = 0;
    ftp_result_t result = E_FTP_RESULT_CONTINUE;
    struct dirent *de;

    // read up to 8 directory items
    while (((maxlistsize - next) > 64) && (listcount < 8)) {
        de = readdir(ftp_data.dp); // Read a directory item
        if (de == NULL) {
            result = E_FTP_RESULT_OK;
            break; // Break on error or end of dp
        }
        if (de->d_name[0] == '.' && de->d_name[1] == 0)
            continue; // Ignore . entry
        if (de->d_name[0] == '.' && de->d_name[1] == '.' && de->d_name[2] == 0)
            continue; // Ignore .. entry

        // add the entry to the list
        ESP_LOGI(FTP_TAG, "Add to dir list: %s", de->d_name);
        next += ftp_get_eplf_item((list + next), (maxlistsize - next), de);
        listcount++;
    }
    if (result == E_FTP_RESULT_OK) {
        ftp_close_files_dir();
    }
    *listsize = next;
    return result;
}

static void ftp_close_cmd_data(void) {
    closesocket(ftp_data.c_sd);
    closesocket(ftp_data.d_sd);
    ftp_data.c_sd = -1;
    ftp_data.d_sd = -1;
    ftp_close_filesystem_on_error();
}

static void _ftp_reset(void) {
    // close all connections and start all over again
    ESP_LOGW(FTP_TAG, "FTP RESET");
    closesocket(ftp_data.lc_sd);
    closesocket(ftp_data.ld_sd);
    ftp_data.lc_sd = -1;
    ftp_data.ld_sd = -1;
    ftp_close_cmd_data();

    ftp_data.e_open = E_FTP_NOTHING_OPEN;
    ftp_data.state = E_FTP_STE_START;
    ftp_data.substate = E_FTP_STE_SUB_DISCONNECTED;
}

static bool ftp_create_listening_socket(int32_t *sd, uint32_t port, uint8_t backlog) {
    struct sockaddr_in sServerAddress;
    int32_t _sd;
    int32_t result;

    // open a socket for ftp data listen
    *sd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    _sd = *sd;

    if (_sd > 0) {
        // enable non-blocking mode
        uint32_t option = fcntl(_sd, F_GETFL, 0);
        option |= O_NONBLOCK;
        fcntl(_sd, F_SETFL, option);

        // enable address reusing
        option = 1;
        result = setsockopt(_sd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

        // bind the socket to a port number
        sServerAddress.sin_family = AF_INET;
        sServerAddress.sin_addr.s_addr = INADDR_ANY;
        sServerAddress.sin_len = sizeof(sServerAddress);
        sServerAddress.sin_port = htons(port);

        result |= bind(_sd, (const struct sockaddr*) &sServerAddress, sizeof(sServerAddress));

        // start listening
        result |= listen(_sd, backlog);

        if (!result) {
            return true;
        }
        closesocket(*sd);
    }
    return false;
}

static ftp_result_t ftp_wait_for_connection(int32_t l_sd, int32_t *n_sd, uint32_t *ip_addr) {
    esp_err_t err;
    struct sockaddr_in sClientAddress;
    socklen_t in_addrSize;

    // accepts a connection from a TCP client, if there is any, otherwise returns EAGAIN
    *n_sd = accept(l_sd, (struct sockaddr*) &sClientAddress, (socklen_t*) &in_addrSize);
    int32_t _sd = *n_sd;
    if (_sd < 0) {
        if (errno == EAGAIN) {
            return E_FTP_RESULT_CONTINUE;
        }
        // error
        _ftp_reset();
        return E_FTP_RESULT_FAILED;
    }

    if (ip_addr) {
        // check on which network interface the client was connected and save the IP address
        esp_netif_ip_info_t ip_info;
        int n_if = network_get_active_interfaces();

        if (n_if > 0) {
            struct sockaddr_in clientAddr;
            in_addrSize = sizeof(struct sockaddr_in);
            getpeername(_sd, (struct sockaddr*) &clientAddr, (socklen_t*) &in_addrSize);
            ESP_LOGI(FTP_TAG, "Client IP: 0x%08"PRIx32, clientAddr.sin_addr.s_addr);
            *ip_addr = 0;
            for (int i = 0; i < n_if; i++) {
                err = esp_netif_get_ip_info(net_if[i], &ip_info);
                if (err == ESP_ERR_ESP_NETIF_INVALID_PARAMS)
                    continue;

                ESP_LOGI(FTP_TAG, "Adapter: 0x%08"PRIx32", 0x%08"PRIx32, ip_info.ip.addr, ip_info.netmask.addr);
                if ((ip_info.ip.addr & ip_info.netmask.addr) == (ip_info.netmask.addr & clientAddr.sin_addr.s_addr)) {
                    *ip_addr = ip_info.ip.addr;
                    char name[8];
                    esp_netif_get_netif_impl_name(net_if[i], name);
                    ESP_LOGI(FTP_TAG, "Client connected on interface %s", name);
                    break;
                }
            }
            if (*ip_addr == 0) {
                ESP_LOGE(FTP_TAG, "No IP address detected (?!)");
            }
        } else {
            ESP_LOGE(FTP_TAG, "No active interface (?!)");
        }
    }

    // enable non-blocking mode if not data channel connection
    uint32_t option = fcntl(_sd, F_GETFL, 0);
    if (l_sd != ftp_data.ld_sd)
        option |= O_NONBLOCK;
    fcntl(_sd, F_SETFL, option);

    // client connected, so go on
    return E_FTP_RESULT_OK;
}

static void ftp_send_reply(uint32_t status, char *message) {
    if (!message) {
        message = "";
    }
    snprintf((char*) ftp_cmd_buffer, 4, "%"PRIu32, status);
    strcat((char*) ftp_cmd_buffer, " ");
    strcat((char*) ftp_cmd_buffer, message);
    strcat((char*) ftp_cmd_buffer, "\r\n");

    int32_t timeout = 200;
    ftp_result_t result;
    size_t size = strlen((char*) ftp_cmd_buffer);

    ESP_LOGI(FTP_TAG, "Send reply: [%.*s]", size - 2, ftp_cmd_buffer);
    vTaskDelay(1);

    while (1) {
        result = send(ftp_data.c_sd, ftp_cmd_buffer, size, 0);
        if (result == size) {
            if (status == 221) {
                closesocket(ftp_data.d_sd);
                ftp_data.d_sd = -1;
                closesocket(ftp_data.ld_sd);
                ftp_data.ld_sd = -1;
                closesocket(ftp_data.c_sd);
                ftp_data.substate = E_FTP_STE_SUB_DISCONNECTED;
                ftp_close_filesystem_on_error();
            } else if (status == 426 || status == 451 || status == 550) {
                closesocket(ftp_data.d_sd);
                ftp_data.d_sd = -1;
                ftp_close_filesystem_on_error();
            }
            vTaskDelay(1);
            ESP_LOGI(FTP_TAG, "Send reply: OK (%u)", size);
            break;
        } else {
            vTaskDelay(1);
            if ((timeout <= 0) || (errno != EAGAIN)) {
                // error
                _ftp_reset();
                ESP_LOGW(FTP_TAG, "Error sending command reply.");
                break;
            }
        }
        timeout -= portTICK_PERIOD_MS;
    }
}

static void ftp_send_list(uint32_t datasize) {
    int32_t timeout = 200;
    ftp_result_t result;

    ESP_LOGI(FTP_TAG, "Send list data: (%"PRIu32")", datasize);
    vTaskDelay(1);

    while (1) {
        result = send(ftp_data.d_sd, ftp_data.dBuffer, datasize, 0);
        if (result == datasize) {
            vTaskDelay(1);
            ESP_LOGI(FTP_TAG, "Send OK");
            break;
        } else {
            vTaskDelay(1);
            if ((timeout <= 0) || (errno != EAGAIN)) {
                // error
                _ftp_reset();
                ESP_LOGW(FTP_TAG, "Error sending list data.");
                break;
            }
        }
        timeout -= portTICK_PERIOD_MS;
    }
}

static void ftp_send_file_data(uint32_t datasize) {
    ftp_result_t result;
    uint32_t timeout = 200;

    ESP_LOGI(FTP_TAG, "Send file data: (%"PRIu32")", datasize);
    vTaskDelay(1);

    while (1) {
        result = send(ftp_data.d_sd, ftp_data.dBuffer, datasize, 0);
        if (result == datasize) {
            vTaskDelay(1);
            ESP_LOGI(FTP_TAG, "Send OK");
            break;
        } else {
            vTaskDelay(1);
            if ((timeout <= 0) || (errno != EAGAIN)) {
                // error
                _ftp_reset();
                ESP_LOGW(FTP_TAG, "Error sending file data.");
                break;
            }
        }
        timeout -= portTICK_PERIOD_MS;
    }
}

static ftp_result_t ftp_recv_non_blocking(int32_t sd, void *buff, int32_t Maxlen, int32_t *rxLen) {
    if (sd < 0)
        return E_FTP_RESULT_FAILED;

    *rxLen = recv(sd, buff, Maxlen, 0);
    if (*rxLen > 0)
        return E_FTP_RESULT_OK;
    else if (errno != EAGAIN)
        return E_FTP_RESULT_FAILED;

    return E_FTP_RESULT_CONTINUE;
}

static void ftp_open_child(char *pwd, char *dir) {
    ESP_LOGI(FTP_TAG, "open_child: %s + %s", pwd, dir);
    if (strlen(dir) > 0) {
        if (dir[0] == '/') {
            // absolute path
            strcpy(pwd, dir);
        } else {
            // relative path
            // add trailing '/' if needed
            if ((strlen(pwd) > 1) && (pwd[strlen(pwd) - 1] != '/') && (dir[0] != '/'))
                strcat(pwd, "/");
            // append directory/file name
            strcat(pwd, dir);
        }
    }
    ESP_LOGI(FTP_TAG, "open_child, New pwd: %s", pwd);
}

static void ftp_close_child(char *pwd) {
    ESP_LOGI(FTP_TAG, "close_child: %s", pwd);
    uint len = strlen(pwd);
    if (pwd[len - 1] == '/') {
        pwd[len - 1] = '\0';
        len--;
        if ((len == 0) || (strcmp(pwd, ftpserver_mount_point) == 0) || (strcmp(pwd, ftpserver_mount_point) == 0)) {
            strcpy(pwd, "/");
        }
    } else {
        while (len) {
            if (pwd[len - 1] == '/') {
                pwd[len - 1] = '\0';
                len--;
                break;
            }
            len--;
        }

        if (len == 0) {
            strcpy(pwd, "/");
        } else if ((strcmp(pwd, ftpserver_mount_point) == 0) || (strcmp(pwd, ftpserver_mount_point) == 0)) {
            strcat(pwd, "/");
        }
    }
    ESP_LOGI(FTP_TAG, "close_child, New pwd: %s", pwd);
}

static void remove_fname_from_path(char *pwd, char *fname) {
    ESP_LOGI(FTP_TAG, "remove_fname_from_path: %s - %s", pwd, fname);
    if (strlen(fname) == 0)
        return;
    char *xpwd = strstr(pwd, fname);
    if (xpwd == NULL)
        return;

    xpwd[0] = '\0';

    ESP_LOGI(FTP_TAG, "remove_fname_from_path: New pwd: %s", pwd);
}

static void ftp_pop_param(char **str, char *param, bool stop_on_space, bool stop_on_newline) {
    char lastc = '\0';
    while (**str != '\0') {
        if (stop_on_space && (**str == ' '))
            break;
        if ((**str == '\r') || (**str == '\n')) {
            if (!stop_on_newline) {
                (*str)++;
                continue;
            } else
                break;
        }
        if ((**str == '/') && (lastc == '/')) {
            (*str)++;
            continue;
        }
        lastc = **str;
        *param++ = **str;
        (*str)++;
    }
    *param = '\0';
}

static ftp_cmd_index_t ftp_pop_command(char **str) {
    char _cmd[FTPSERVER_CMD_SIZE_MAX];
    ftp_pop_param(str, _cmd, true, true);
    stoupper(_cmd);
    for (ftp_cmd_index_t i = 0; i < E_FTP_NUM_FTP_CMDS; i++) {
        if (!strcmp(_cmd, ftp_cmd_table[i].cmd)) {
            // move one step further to skip the space
            (*str)++;
            return i;
        }
    }
    return E_FTP_CMD_NOT_SUPPORTED;
}

static void ftp_get_param_and_open_child(char **bufptr) {
    ftp_pop_param(bufptr, ftp_scratch_buffer, false, false);
    ftp_open_child(ftp_path, ftp_scratch_buffer);
    ftp_data.closechild = true;
}

static void ftp_process_cmd(void) {
    int32_t len;
    char *bufptr = (char*) ftp_cmd_buffer;
    ftp_result_t result;
    struct stat buf;
    int res;

    memset(bufptr, 0, FTPSERVER_MAX_PARAM_SIZE + FTPSERVER_CMD_SIZE_MAX);
    ftp_data.closechild = false;

    // use the reply buffer to receive new commands
    result = ftp_recv_non_blocking(ftp_data.c_sd, ftp_cmd_buffer, FTPSERVER_MAX_PARAM_SIZE + FTPSERVER_CMD_SIZE_MAX, &len);
    if (result == E_FTP_RESULT_OK) {
        ftp_cmd_buffer[len] = '\0';
        // bufptr is moved as commands are being popped
        ftp_cmd_index_t cmd = ftp_pop_command(&bufptr);
        if (!ftp_data.loggin.passvalid
                && ((cmd != E_FTP_CMD_USER) && (cmd != E_FTP_CMD_PASS) && (cmd != E_FTP_CMD_QUIT) && (cmd != E_FTP_CMD_FEAT) && (cmd != E_FTP_CMD_AUTH))) {
            ftp_send_reply(332, NULL);
            return;
        }
        if ((cmd >= 0) && (cmd < E_FTP_NUM_FTP_CMDS)) {
            ESP_LOGI(FTP_TAG, "CMD: %s", ftp_cmd_table[cmd].cmd);
        } else {
            ESP_LOGI(FTP_TAG, "CMD: %d", cmd);
        }
        char fullname[128];
        char fullname2[128];
        strcpy(fullname, ftpserver_mount_point);
        strcpy(fullname2, ftpserver_mount_point);

        switch (cmd) {
            case E_FTP_CMD_FEAT:
                ftp_send_reply(502, "no-features");
                break;
            case E_FTP_CMD_AUTH:
                ftp_send_reply(504, "not-supported");
                break;
            case E_FTP_CMD_SYST:
                ftp_send_reply(215, "UNIX Type: L8");
                break;
            case E_FTP_CMD_CDUP:
                ftp_close_child(ftp_path);
                ftp_send_reply(250, NULL);
                break;
            case E_FTP_CMD_CWD:
                ftp_pop_param(&bufptr, ftp_scratch_buffer, false, false);

                if (strlen(ftp_scratch_buffer) > 0) {
                    if ((ftp_scratch_buffer[0] == '.') && (ftp_scratch_buffer[1] == '\0')) {
                        ftp_data.dp = NULL;
                        ftp_send_reply(250, NULL);
                        break;
                    }
                    if ((ftp_scratch_buffer[0] == '.') && (ftp_scratch_buffer[1] == '.') && (ftp_scratch_buffer[2] == '\0')) {
                        ftp_close_child(ftp_path);
                        ftp_send_reply(250, NULL);
                        break;
                    } else
                        ftp_open_child(ftp_path, ftp_scratch_buffer);
                }

                if ((ftp_path[0] == '/') && (ftp_path[1] == '\0')) {
                    ftp_data.dp = NULL;
                    ftp_send_reply(250, NULL);
                } else {
                    strcat(fullname, ftp_path);
                    ESP_LOGI(FTP_TAG, "E_FTP_CMD_CWD fullname=[%s]", fullname);
                    ftp_data.dp = opendir(fullname);
                    if (ftp_data.dp != NULL) {
                        closedir(ftp_data.dp);
                        ftp_data.dp = NULL;
                        ftp_send_reply(250, NULL);
                    } else {
                        ftp_close_child(ftp_path);
                        ftp_send_reply(550, NULL);
                    }
                }
                break;
            case E_FTP_CMD_PWD:
            case E_FTP_CMD_XPWD: {
                char lpath[128];
                strcpy(lpath, ftp_path);

                ftp_send_reply(257, lpath);
            }
                break;
            case E_FTP_CMD_SIZE: {
                ftp_get_param_and_open_child(&bufptr);
                strcat(fullname, ftp_path);
                ESP_LOGI(FTP_TAG, "E_FTP_CMD_SIZE fullname=[%s]", fullname);
                int res = stat(fullname, &buf);
                if (res == 0) {
                    // send the file size
                    snprintf((char*) ftp_data.dBuffer, ftp_buff_size, "%"PRIu32, (uint32_t) buf.st_size);
                    ftp_send_reply(213, (char*) ftp_data.dBuffer);
                } else {
                    ftp_send_reply(550, NULL);
                }
            }
                break;
            case E_FTP_CMD_MDTM:
                ftp_get_param_and_open_child(&bufptr);
                strcat(fullname, ftp_path);
                ESP_LOGI(FTP_TAG, "E_FTP_CMD_MDTM fullname=[%s]", fullname);
                res = stat(fullname, &buf);
                if (res == 0) {
                    // send the file modification time
                    time_t time = buf.st_mtime;
                    struct tm *ptm = localtime(&time);
                    strftime((char*) ftp_data.dBuffer, ftp_buff_size, "%Y%m%d%H%M%S", ptm);
                    ESP_LOGI(FTP_TAG, "E_FTP_CMD_MDTM ftp_data.dBuffer=[%s]", ftp_data.dBuffer);
                    ftp_send_reply(213, (char*) ftp_data.dBuffer);
                } else {
                    ftp_send_reply(550, NULL);
                }
                break;
            case E_FTP_CMD_TYPE:
                ftp_send_reply(200, NULL);
                break;
            case E_FTP_CMD_USER:
                ftp_pop_param(&bufptr, ftp_scratch_buffer, true, true);
                if (!memcmp(ftp_scratch_buffer, ftp_user,
                        ((strlen(ftp_scratch_buffer)) > (strlen(ftp_user)) ? (strlen(ftp_scratch_buffer)) : (strlen(ftp_user))))) {
                    ftp_data.loggin.uservalid = true && (strlen(ftp_user) == strlen(ftp_scratch_buffer));
                }
                ftp_send_reply(331, NULL);
                break;
            case E_FTP_CMD_PASS:
                ftp_pop_param(&bufptr, ftp_scratch_buffer, true, true);
                if (!memcmp(ftp_scratch_buffer, ftp_pass,
                        ((strlen(ftp_scratch_buffer)) > (strlen(ftp_pass)) ? (strlen(ftp_scratch_buffer)) : (strlen(ftp_pass)))) && ftp_data.loggin.uservalid) {
                    ftp_data.loggin.passvalid = true && (strlen(ftp_pass) == strlen(ftp_scratch_buffer));
                    if (ftp_data.loggin.passvalid) {
                        ftp_send_reply(230, NULL);
                        break;
                    }
                }
                ftp_send_reply(530, NULL);
                break;
            case E_FTP_CMD_PASV: {
                // some servers (e.g. google chrome) send PASV several times very quickly
                closesocket(ftp_data.d_sd);
                ftp_data.d_sd = -1;
                ftp_data.substate = E_FTP_STE_SUB_DISCONNECTED;
                bool socketcreated = true;
                if (ftp_data.ld_sd < 0) {
                    socketcreated = ftp_create_listening_socket(&ftp_data.ld_sd, FTPSERVER_PASIVE_DATA_PORT, FTPSERVER_DATA_CLIENTS_MAX - 1);
                }
                if (socketcreated) {
                    uint8_t *pip = (uint8_t*) &ftp_data.ip_addr;
                    ftp_data.dtimeout = 0;
                    snprintf((char*) ftp_data.dBuffer, ftp_buff_size, "(%u,%u,%u,%u,%u,%u)", pip[0], pip[1], pip[2], pip[3], (FTPSERVER_PASIVE_DATA_PORT >> 8),
                            (FTPSERVER_PASIVE_DATA_PORT & 0xFF));
                    ftp_data.substate = E_FTP_STE_SUB_LISTEN_FOR_DATA;
                    ESP_LOGI(FTP_TAG, "Data socket created");
                    ftp_send_reply(227, (char*) ftp_data.dBuffer);
                } else {
                    ESP_LOGW(FTP_TAG, "Error creating data socket");
                    ftp_send_reply(425, NULL);
                }
            }
                break;
            case E_FTP_CMD_LIST:
            case E_FTP_CMD_NLST:
                ftp_get_param_and_open_child(&bufptr);
                if (cmd == E_FTP_CMD_LIST)
                    ftp_nlist = 0;
                else
                    ftp_nlist = 1;
                if (ftp_open_dir_for_listing(ftp_path) == E_FTP_RESULT_CONTINUE) {
                    ftp_data.state = E_FTP_STE_CONTINUE_LISTING;
                    ftp_send_reply(150, NULL);
                } else
                    ftp_send_reply(550, NULL);
                break;
            case E_FTP_CMD_RETR:
                ftp_data.total = 0;
                ftp_data.time = 0;
                ftp_get_param_and_open_child(&bufptr);
                if ((strlen(ftp_path) > 0) && (ftp_path[strlen(ftp_path) - 1] != '/')) {
                    if (ftp_open_file(ftp_path, "rb")) {
                        ftp_data.state = E_FTP_STE_CONTINUE_FILE_TX;
                        vTaskDelay(20 / portTICK_PERIOD_MS);
                        ftp_send_reply(150, NULL);
                    } else {
                        ftp_data.state = E_FTP_STE_END_TRANSFER;
                        ftp_send_reply(550, NULL);
                    }
                } else {
                    ftp_data.state = E_FTP_STE_END_TRANSFER;
                    ftp_send_reply(550, NULL);
                }
                break;
            case E_FTP_CMD_APPE:
                ftp_data.total = 0;
                ftp_data.time = 0;
                ftp_get_param_and_open_child(&bufptr);
                if ((strlen(ftp_path) > 0) && (ftp_path[strlen(ftp_path) - 1] != '/')) {
                    if (ftp_open_file(ftp_path, "ab")) {
                        ftp_data.state = E_FTP_STE_CONTINUE_FILE_RX;
                        vTaskDelay(20 / portTICK_PERIOD_MS);
                        ftp_send_reply(150, NULL);
                    } else {
                        ftp_data.state = E_FTP_STE_END_TRANSFER;
                        ftp_send_reply(550, NULL);
                    }
                } else {
                    ftp_data.state = E_FTP_STE_END_TRANSFER;
                    ftp_send_reply(550, NULL);
                }
                break;
            case E_FTP_CMD_STOR:
                ftp_data.total = 0;
                ftp_data.time = 0;
                ftp_get_param_and_open_child(&bufptr);
                if ((strlen(ftp_path) > 0) && (ftp_path[strlen(ftp_path) - 1] != '/')) {
                    ESP_LOGI(FTP_TAG, "E_FTP_CMD_STOR ftp_path=[%s]", ftp_path);
                    if (ftp_open_file(ftp_path, "wb")) {
                        ftp_data.state = E_FTP_STE_CONTINUE_FILE_RX;
                        vTaskDelay(20 / portTICK_PERIOD_MS);
                        ftp_send_reply(150, NULL);
                    } else {
                        ftp_data.state = E_FTP_STE_END_TRANSFER;
                        ftp_send_reply(550, NULL);
                    }
                } else {
                    ftp_data.state = E_FTP_STE_END_TRANSFER;
                    ftp_send_reply(550, NULL);
                }
                break;
            case E_FTP_CMD_DELE:
                ftp_get_param_and_open_child(&bufptr);
                if ((strlen(ftp_path) > 0) && (ftp_path[strlen(ftp_path) - 1] != '/')) {
                    ESP_LOGI(FTP_TAG, "E_FTP_CMD_DELE ftp_path=[%s]", ftp_path);

                    strcat(fullname, ftp_path);
                    ESP_LOGI(FTP_TAG, "E_FTP_CMD_DELE fullname=[%s]", fullname);

                    if (unlink(fullname) == 0) {
                        vTaskDelay(20 / portTICK_PERIOD_MS);
                        ftp_send_reply(250, NULL);
                    } else
                        ftp_send_reply(550, NULL);
                } else
                    ftp_send_reply(250, NULL);
                break;
            case E_FTP_CMD_RMD:
                ftp_get_param_and_open_child(&bufptr);
                if ((strlen(ftp_path) > 0) && (ftp_path[strlen(ftp_path) - 1] != '/')) {
                    ESP_LOGI(FTP_TAG, "E_FTP_CMD_RMD ftp_path=[%s]", ftp_path);

                    strcat(fullname, ftp_path);
                    ESP_LOGI(FTP_TAG, "E_FTP_CMD_MKD fullname=[%s]", fullname);

                    if (rmdir(fullname) == 0) {
                        vTaskDelay(20 / portTICK_PERIOD_MS);
                        ftp_send_reply(250, NULL);
                    } else
                        ftp_send_reply(550, NULL);
                } else
                    ftp_send_reply(250, NULL);
                break;
            case E_FTP_CMD_MKD:
                ftp_get_param_and_open_child(&bufptr);
                if ((strlen(ftp_path) > 0) && (ftp_path[strlen(ftp_path) - 1] != '/')) {
                    ESP_LOGI(FTP_TAG, "E_FTP_CMD_MKD ftp_path=[%s]", ftp_path);

                    strcat(fullname, ftp_path);
                    ESP_LOGI(FTP_TAG, "E_FTP_CMD_MKD fullname=[%s]", fullname);

                    if (mkdir(fullname, 0755) == 0) {
                        vTaskDelay(20 / portTICK_PERIOD_MS);
                        ftp_send_reply(250, NULL);
                    } else
                        ftp_send_reply(550, NULL);
                } else
                    ftp_send_reply(250, NULL);
                break;
            case E_FTP_CMD_RNFR:
                ftp_get_param_and_open_child(&bufptr);
                ESP_LOGI(FTP_TAG, "E_FTP_CMD_RNFR ftp_path=[%s]", ftp_path);

                strcat(fullname, ftp_path);
                ESP_LOGI(FTP_TAG, "E_FTP_CMD_MKD fullname=[%s]", fullname);

                res = stat(fullname, &buf);
                if (res == 0) {
                    ftp_send_reply(350, NULL);
                    // save the path of the file to rename
                    strcpy((char*) ftp_data.dBuffer, ftp_path);
                } else {
                    ftp_send_reply(550, NULL);
                }
                break;
            case E_FTP_CMD_RNTO:
                ftp_get_param_and_open_child(&bufptr);
                // the path of the file to rename was saved in the data buffer
                ESP_LOGI(FTP_TAG, "E_FTP_CMD_RNTO ftp_path=[%s], ftp_data.dBuffer=[%s]", ftp_path, (char* ) ftp_data.dBuffer);
                strcat(fullname, (char*) ftp_data.dBuffer);
                ESP_LOGI(FTP_TAG, "E_FTP_CMD_RNTO fullname=[%s]", fullname);
                strcat(fullname2, ftp_path);
                ESP_LOGI(FTP_TAG, "E_FTP_CMD_RNTO fullname2=[%s]", fullname2);

                if (rename(fullname, fullname2) == 0) {
                    ftp_send_reply(250, NULL);
                } else {
                    ftp_send_reply(550, NULL);
                }
                break;
            case E_FTP_CMD_NOOP:
                ftp_send_reply(200, NULL);
                break;
            case E_FTP_CMD_QUIT:
                ftp_send_reply(221, NULL);
                break;
            default:
                // command not implemented
                ftp_send_reply(502, NULL);
                break;
        }

        if (ftp_data.closechild) {
            remove_fname_from_path(ftp_path, ftp_scratch_buffer);
        }
    } else if (result == E_FTP_RESULT_CONTINUE) {
        if (ftp_data.ctimeout > ftp_timeout) {
            ftp_send_reply(221, NULL);
            ESP_LOGW(FTP_TAG, "Connection timeout");
        }
    } else {
        ftp_close_cmd_data();
    }
}

static void ftp_wait_for_enabled(void) {
    // Check if the telnet service has been enabled
    if (ftp_data.enabled) {
        ftp_data.state = E_FTP_STE_START;
    }
}

void ftpserver_deinit(void) {
    if (ftp_path)
        free(ftp_path);
    if (ftp_cmd_buffer)
        free(ftp_cmd_buffer);
    if (ftp_data.dBuffer)
        free(ftp_data.dBuffer);
    if (ftp_scratch_buffer)
        free(ftp_scratch_buffer);
    ftp_path = NULL;
    ftp_cmd_buffer = NULL;
    ftp_data.dBuffer = NULL;
    ftp_scratch_buffer = NULL;
}

bool ftpserver_init(void) {
    ftp_stop = 0;
    // Allocate memory for the data buffer, and the file system structures (from the RTOS heap)
    ftpserver_deinit();

    memset(&ftp_data, 0, sizeof(ftp_data_t));
    ftp_data.dBuffer = malloc(ftp_buff_size + 1);
    if (ftp_data.dBuffer == NULL)
        return false;
    ftp_path = malloc(FTPSERVER_MAX_PARAM_SIZE);
    if (ftp_path == NULL) {
        free(ftp_data.dBuffer);
        return false;
    }
    ftp_scratch_buffer = malloc(FTPSERVER_MAX_PARAM_SIZE);
    if (ftp_scratch_buffer == NULL) {
        free(ftp_path);
        free(ftp_data.dBuffer);
        return false;
    }
    ftp_cmd_buffer = malloc(FTPSERVER_MAX_PARAM_SIZE + FTPSERVER_CMD_SIZE_MAX);
    if (ftp_cmd_buffer == NULL) {
        free(ftp_scratch_buffer);
        free(ftp_path);
        free(ftp_data.dBuffer);
        return false;
    }

    ftp_data.c_sd = -1;
    ftp_data.d_sd = -1;
    ftp_data.lc_sd = -1;
    ftp_data.ld_sd = -1;
    ftp_data.e_open = E_FTP_NOTHING_OPEN;
    ftp_data.state = E_FTP_STE_DISABLED;
    ftp_data.substate = E_FTP_STE_SUB_DISCONNECTED;

    return true;
}

int ftpserver_run(uint32_t elapsed) {
    if (ftp_stop)
        return -2;

    ftp_data.dtimeout += elapsed;
    ftp_data.ctimeout += elapsed;
    ftp_data.time += elapsed;

    switch (ftp_data.state) {
        case E_FTP_STE_DISABLED:
            ftp_wait_for_enabled();
            break;
        case E_FTP_STE_START:
            if (ftp_create_listening_socket(&ftp_data.lc_sd, FTPSERVER_CMD_PORT, FTPSERVER_CMD_CLIENTS_MAX - 1)) {
                ftp_data.state = E_FTP_STE_READY;
            }
            break;
        case E_FTP_STE_READY:
            if (ftp_data.c_sd < 0 && ftp_data.substate == E_FTP_STE_SUB_DISCONNECTED) {
                if (E_FTP_RESULT_OK == ftp_wait_for_connection(ftp_data.lc_sd, &ftp_data.c_sd, &ftp_data.ip_addr)) {
                    ftp_data.txRetries = 0;
                    ftp_data.logginRetries = 0;
                    ftp_data.ctimeout = 0;
                    ftp_data.loggin.uservalid = false;
                    ftp_data.loggin.passvalid = false;
                    strcpy(ftp_path, "/");
                    ESP_LOGI(FTP_TAG, "Connected.");
                    ftp_send_reply(220, "ESP FTP Server");
                    break;
                }
            }
            if (ftp_data.c_sd > 0 && ftp_data.substate != E_FTP_STE_SUB_LISTEN_FOR_DATA) {
                ftp_process_cmd();
                if (ftp_data.state != E_FTP_STE_READY) {
                    break;
                }
            }
            break;
        case E_FTP_STE_END_TRANSFER:
            if (ftp_data.d_sd >= 0) {
                closesocket(ftp_data.d_sd);
                ftp_data.d_sd = -1;
            }
            break;
        case E_FTP_STE_CONTINUE_LISTING:
            // go on with listing
        {
            uint32_t listsize = 0;
            ftp_result_t list_res = ftp_list_dir((char*) ftp_data.dBuffer, ftp_buff_size, &listsize);
            if (listsize > 0)
                ftp_send_list(listsize);
            if (list_res == E_FTP_RESULT_OK) {
                ftp_send_reply(226, NULL);
                ftp_data.state = E_FTP_STE_END_TRANSFER;
            }
            ftp_data.ctimeout = 0;
        }
            break;
        case E_FTP_STE_CONTINUE_FILE_TX:
            // read and send the next block from the file
        {
            uint32_t readsize;
            ftp_result_t result;
            ftp_data.ctimeout = 0;
            result = ftp_read_file((char*) ftp_data.dBuffer, ftp_buff_size, &readsize);
            if (result == E_FTP_RESULT_FAILED) {
                ftp_send_reply(451, NULL);
                ftp_data.state = E_FTP_STE_END_TRANSFER;
            } else {
                if (readsize > 0) {
                    ftp_send_file_data(readsize);
                    ftp_data.total += readsize;
                    ESP_LOGI(FTP_TAG, "Sent %"PRIu32", total: %"PRIu32, readsize, ftp_data.total);
                }
                if (result == E_FTP_RESULT_OK) {
                    ftp_send_reply(226, NULL);
                    ftp_data.state = E_FTP_STE_END_TRANSFER;
                    ESP_LOGI(FTP_TAG, "File sent (%"PRIu32" bytes in %"PRIu32" msec).", ftp_data.total, ftp_data.time);
                }
            }
        }
            break;
        case E_FTP_STE_CONTINUE_FILE_RX: {
            int32_t len;
            ftp_result_t result = E_FTP_RESULT_OK;

            result = ftp_recv_non_blocking(ftp_data.d_sd, ftp_data.dBuffer, ftp_buff_size, &len);
            if (result == E_FTP_RESULT_OK) {
                // block of data received
                ftp_data.dtimeout = 0;
                ftp_data.ctimeout = 0;
                // save received data to file
                if (E_FTP_RESULT_OK != ftp_write_file((char*) ftp_data.dBuffer, len)) {
                    ftp_send_reply(451, NULL);
                    ftp_data.state = E_FTP_STE_END_TRANSFER;
                    ESP_LOGW(FTP_TAG, "Error writing to file");
                } else {
                    ftp_data.total += len;
                    ESP_LOGI(FTP_TAG, "Received %"PRIu32", total: %"PRIu32, len, ftp_data.total);
                }
            } else if (result == E_FTP_RESULT_CONTINUE) {
                // nothing received
                if (ftp_data.dtimeout > FTPSERVER_DATA_TIMEOUT_MS) {
                    ftp_close_files_dir();
                    ftp_send_reply(426, NULL);
                    ftp_data.state = E_FTP_STE_END_TRANSFER;
                    ESP_LOGW(FTP_TAG, "Receiving to file timeout");
                }
            } else {
                // File received (E_FTP_RESULT_FAILED)
                ftp_close_files_dir();
                ftp_send_reply(226, NULL);
                ftp_data.state = E_FTP_STE_END_TRANSFER;
                ESP_LOGI(FTP_TAG, "File received (%"PRIu32" bytes in %"PRIu32" msec).", ftp_data.total, ftp_data.time);
                break;
            }
        }
            break;
        default:
            break;
    }

    switch (ftp_data.substate) {
        case E_FTP_STE_SUB_DISCONNECTED:
            break;
        case E_FTP_STE_SUB_LISTEN_FOR_DATA:
            if (E_FTP_RESULT_OK == ftp_wait_for_connection(ftp_data.ld_sd, &ftp_data.d_sd, NULL)) {
                ftp_data.dtimeout = 0;
                ftp_data.substate = E_FTP_STE_SUB_DATA_CONNECTED;
                ESP_LOGI(FTP_TAG, "Data socket connected");
            } else if (ftp_data.dtimeout > FTPSERVER_DATA_TIMEOUT_MS) {
                ESP_LOGW(FTP_TAG, "Waiting for data connection timeout (%"PRIi32")", ftp_data.dtimeout);
                ftp_data.dtimeout = 0;
                // close the listening socket
                closesocket(ftp_data.ld_sd);
                ftp_data.ld_sd = -1;
                ftp_data.substate = E_FTP_STE_SUB_DISCONNECTED;
            }
            break;
        case E_FTP_STE_SUB_DATA_CONNECTED:
            if (ftp_data.state == E_FTP_STE_READY && (ftp_data.dtimeout > FTPSERVER_DATA_TIMEOUT_MS)) {
                // close the listening and the data socket
                closesocket(ftp_data.ld_sd);
                closesocket(ftp_data.d_sd);
                ftp_data.ld_sd = -1;
                ftp_data.d_sd = -1;
                ftp_close_filesystem_on_error();
                ftp_data.substate = E_FTP_STE_SUB_DISCONNECTED;
                ESP_LOGW(FTP_TAG, "Data connection timeout");
            }
            break;
        default:
            break;
    }

    // check the state of the data sockets
    if (ftp_data.d_sd < 0 && (ftp_data.state > E_FTP_STE_READY)) {
        ftp_data.substate = E_FTP_STE_SUB_DISCONNECTED;
        ftp_data.state = E_FTP_STE_READY;
        ESP_LOGI(FTP_TAG, "Data socket disconnected");
    }

    return 0;
}

bool ftpserver_enable(void) {
    bool res = false;
    if (ftp_data.state == E_FTP_STE_DISABLED) {
        ftp_data.enabled = true;
        res = true;
    }
    return res;
}

bool ftpserver_isenabled(void) {
    bool res = (ftp_data.enabled == true);
    return res;
}

bool ftpserver_disable(void) {
    bool res = false;
    if (ftp_data.state == E_FTP_STE_READY) {
        _ftp_reset();
        ftp_data.enabled = false;
        ftp_data.state = E_FTP_STE_DISABLED;
        res = true;
    }
    return res;
}

bool ftpserver_reset(void) {
    _ftp_reset();
    return true;
}

int ftpserver_getstate() {
    int fstate = ftp_data.state | (ftp_data.substate << 8);
    if ((ftp_data.state == E_FTP_STE_READY) && (ftp_data.c_sd > 0))
        fstate = E_FTP_STE_CONNECTED;
    return fstate;
}

bool ftpserver_terminate(void) {
    bool res = false;
    if (ftp_data.state == E_FTP_STE_READY) {
        ftp_stop = 1;
        _ftp_reset();
        res = true;
    }
    return res;
}

bool ftpserver_stop_requested(void) {
    bool res = (ftp_stop == 1);
    return res;
}

////////////////////////////////////////////////////////////////

static void ftp_task(void *pvParameters) {
    uint64_t elapsed, time_ms = mp_hal_ticks_ms();
    // Initialize ftp, create rx buffer and mutex
    if (!ftpserver_init()) {
        ESP_LOGE(FTP_TAG, "Init Error");
        vTaskDelete(NULL);
    }

    // We have network connection, enable ftp
    ftpserver_enable();

    time_ms = mp_hal_ticks_ms();
    while (1) {
        // Calculate time between two ftp_run() calls
        elapsed = mp_hal_ticks_ms() - time_ms;
        time_ms = mp_hal_ticks_ms();

        int res = ftpserver_run(elapsed);
        if (res < 0) {
            if (res == -1) {
                ESP_LOGE(FTP_TAG, "\nRun Error");
            }
            // -2 is returned if Ftp stop was requested by user
            break;
        }

        vTaskDelay(1);

    }

    ESP_LOGW(FTP_TAG, "\nTask terminated!");
    vTaskDelete(NULL);
}

void ftpserver_start(const char *_ftp_user, const char *_ftp_password, const char *mount_point) {
    ESP_LOGI(FTP_TAG, "ftp_task start");
    strcpy(ftp_user, _ftp_user);
    strcpy(ftp_pass, _ftp_password);
    strcpy(ftpserver_mount_point, mount_point);
    ESP_LOGI(FTP_TAG, "ftp_user:[%s] ftp_pass:[%s]", ftp_user, ftp_pass);

    xTaskCreate(
            ftp_task,    //
            "ftpserver", //
            1024 * 6,    //
            NULL,        //
            2,           //
            NULL         //
            );
}
