/*
 * @file ftpserver.h
 *
 * @brief FTP Server
 * @details
 * This is based on other projects:
 *   MicroPython ESP32 (https://github.com/loboris/MicroPython_ESP32_psRAM_LoBo)
 *   Others (see individual files)
 *
 *   please contact their authors for more information.
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @see https://github.com/hiperiondev/esp32-ftpserver
 * @date 2023
 * @copyright The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef FTPSERVER_H_
#define FTPSERVER_H_

#include <dirent.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_netif_types.h"

/**
 * @enum
 * @brief
 *
 */
typedef enum {
    E_FTP_STE_DISABLED = 0,     /**< E_FTP_STE_DISABLED */
    E_FTP_STE_START,            /**< E_FTP_STE_START */
    E_FTP_STE_READY,            /**< E_FTP_STE_READY */
    E_FTP_STE_END_TRANSFER,     /**< E_FTP_STE_END_TRANSFER */
    E_FTP_STE_CONTINUE_LISTING, /**< E_FTP_STE_CONTINUE_LISTING */
    E_FTP_STE_CONTINUE_FILE_TX, /**< E_FTP_STE_CONTINUE_FILE_TX */
    E_FTP_STE_CONTINUE_FILE_RX, /**< E_FTP_STE_CONTINUE_FILE_RX */
    E_FTP_STE_CONNECTED         /**< E_FTP_STE_CONNECTED */
} ftp_state_t;                  /**<  */

/**
 * @enum
 * @brief
 *
 */
typedef enum {
    E_FTP_STE_SUB_DISCONNECTED = 0, /**< E_FTP_STE_SUB_DISCONNECTED */
    E_FTP_STE_SUB_LISTEN_FOR_DATA,  /**< E_FTP_STE_SUB_LISTEN_FOR_DATA */
    E_FTP_STE_SUB_DATA_CONNECTED    /**< E_FTP_STE_SUB_DATA_CONNECTED */
} ftp_substate_t;                   /**<  */

/**
 * @enum
 * @brief
 *
 */
typedef enum {
    E_FTP_RESULT_OK = 0,   /**< E_FTP_RESULT_OK */
    E_FTP_RESULT_CONTINUE, /**< E_FTP_RESULT_CONTINUE */
    E_FTP_RESULT_FAILED    /**< E_FTP_RESULT_FAILED */
} ftp_result_t;            /**<  */

/**
 * @struct ftp_loggin_s
 * @brief
 *
 */
typedef struct ftp_loggin_s {
    bool uservalid :1; /**<  */
    bool passvalid :1; /**<  */
} ftp_loggin_t;        /**<  */

/**
 * @enum
 * @brief
 *
 */
typedef enum {
    E_FTP_NOTHING_OPEN = 0, /**< E_FTP_NOTHING_OPEN */
    E_FTP_FILE_OPEN,        /**< E_FTP_FILE_OPEN */
    E_FTP_DIR_OPEN          /**< E_FTP_DIR_OPEN */
} ftp_e_open_t;

/**
 * @enum
 * @brief
 *
 */
typedef enum {
    E_FTP_CLOSE_NONE = 0,     /**< E_FTP_CLOSE_NONE */
    E_FTP_CLOSE_DATA,         /**< E_FTP_CLOSE_DATA */
    E_FTP_CLOSE_CMD_AND_DATA, /**< E_FTP_CLOSE_CMD_AND_DATA */
} ftp_e_closesocket_t;

/**
 * @enum
 * @brief
 *
 */
typedef enum {
    E_FTP_CMD_NOT_SUPPORTED = -1, /**< E_FTP_CMD_NOT_SUPPORTED */
    E_FTP_CMD_FEAT = 0,           /**< E_FTP_CMD_FEAT */
    E_FTP_CMD_SYST,               /**< E_FTP_CMD_SYST */
    E_FTP_CMD_CDUP,               /**< E_FTP_CMD_CDUP */
    E_FTP_CMD_CWD,                /**< E_FTP_CMD_CWD */
    E_FTP_CMD_PWD,                /**< E_FTP_CMD_PWD */
    E_FTP_CMD_XPWD,               /**< E_FTP_CMD_XPWD */
    E_FTP_CMD_SIZE,               /**< E_FTP_CMD_SIZE */
    E_FTP_CMD_MDTM,               /**< E_FTP_CMD_MDTM */
    E_FTP_CMD_TYPE,               /**< E_FTP_CMD_TYPE */
    E_FTP_CMD_USER,               /**< E_FTP_CMD_USER */
    E_FTP_CMD_PASS,               /**< E_FTP_CMD_PASS */
    E_FTP_CMD_PASV,               /**< E_FTP_CMD_PASV */
    E_FTP_CMD_LIST,               /**< E_FTP_CMD_LIST */
    E_FTP_CMD_RETR,               /**< E_FTP_CMD_RETR */
    E_FTP_CMD_STOR,               /**< E_FTP_CMD_STOR */
    E_FTP_CMD_DELE,               /**< E_FTP_CMD_DELE */
    E_FTP_CMD_RMD,                /**< E_FTP_CMD_RMD */
    E_FTP_CMD_MKD,                /**< E_FTP_CMD_MKD */
    E_FTP_CMD_RNFR,               /**< E_FTP_CMD_RNFR */
    E_FTP_CMD_RNTO,               /**< E_FTP_CMD_RNTO */
    E_FTP_CMD_NOOP,               /**< E_FTP_CMD_NOOP */
    E_FTP_CMD_QUIT,               /**< E_FTP_CMD_QUIT */
    E_FTP_CMD_APPE,               /**< E_FTP_CMD_APPE */
    E_FTP_CMD_NLST,               /**< E_FTP_CMD_NLST */
    E_FTP_CMD_AUTH,               /**< E_FTP_CMD_AUTH */
    E_FTP_NUM_FTP_CMDS            /**< E_FTP_NUM_FTP_CMDS */
} ftp_cmd_index_t;                /**<  */

/**
 * @struct ftp_data_s
 * @brief
 *
 */
typedef struct ftp_data_s {
     uint8_t *dBuffer;          /**<  */
    uint32_t ctimeout;          /**<  */
    union {
         DIR *dp;               /**<  */
        FILE *fp;               /**<  */
    };
         int32_t lc_sd;         /**<  */
         int32_t ld_sd;         /**<  */
         int32_t c_sd;          /**<  */
         int32_t d_sd;          /**<  */
         int32_t dtimeout;      /**<  */
        uint32_t ip_addr;       /**<  */
         uint8_t state;         /**<  */
         uint8_t substate;      /**<  */
         uint8_t txRetries;     /**<  */
         uint8_t logginRetries; /**<  */
    ftp_loggin_t loggin;        /**<  */
         uint8_t e_open;        /**<  */
            bool closechild;    /**<  */
            bool enabled;       /**<  */
            bool listroot;      /**<  */
        uint32_t total;         /**<  */
        uint32_t time;          /**<  */
} ftp_data_t;                   /**<  */

/**
 * @struct
 * @brief
 *
 */
typedef struct ftp_cmd_s {
    char *cmd; /**<  */
} ftp_cmd_t;   /**<  */

///////////////////////////////////////////

   bool ftpserver_init(void);
   void ftpserver_deinit(void);
    int ftpserver_run(uint32_t elapsed);
   bool ftpserver_enable(void);
   bool ftpserver_isenabled(void);
   bool ftpserver_disable(void);
   bool ftpserver_reset(void);
    int ftpserver_getstate();
   bool ftpserver_terminate(void);
   bool ftpserver_stop_requested();
int32_t ftpserver_get_maxstack(void);
   void ftpserver_start(const char *_ftp_user, const char *_ftp_password, const char *mount_point);

#endif /* FTPSERVER_H_ */
