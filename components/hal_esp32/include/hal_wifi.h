/*
 * @file hal_wifi.h
 * @brief WIFI port
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see
 * @note This is based on other projects. See license files
 */

#ifndef HAL_WIFI_H_
#define HAL_WIFI_H_

#define DEFAULT_SCAN_LIST_SIZE 50

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    HAL_WIFI_SECOND_CHAN_NONE = 0, /**< the channel width is HT20 */
    HAL_WIFI_SECOND_CHAN_ABOVE,    /**< the channel width is HT40 and the secondary channel is above the primary channel */
    HAL_WIFI_SECOND_CHAN_BELOW,    /**< the channel width is HT40 and the secondary channel is below the primary channel */
} hal_wifi_second_chan_t;          /**< wifi channel width */

typedef enum {
    HAL_WIFI_AUTH_OPEN = 0,        /**< authenticate mode : open */
    HAL_WIFI_AUTH_WEP,             /**< authenticate mode : WEP */
    HAL_WIFI_AUTH_WPA_PSK,         /**< authenticate mode : WPA_PSK */
    HAL_WIFI_AUTH_WPA2_PSK,        /**< authenticate mode : WPA2_PSK */
    HAL_WIFI_AUTH_WPA_WPA2_PSK,    /**< authenticate mode : WPA_WPA2_PSK */
    HAL_WIFI_AUTH_WPA2_ENTERPRISE, /**< authenticate mode : WPA2_ENTERPRISE */
    HAL_WIFI_AUTH_WPA3_PSK,        /**< authenticate mode : WPA3_PSK */
    HAL_WIFI_AUTH_WPA2_WPA3_PSK,   /**< authenticate mode : WPA2_WPA3_PSK */
    HAL_WIFI_AUTH_WAPI_PSK,        /**< authenticate mode : WAPI_PSK */
    HAL_WIFI_AUTH_OWE,             /**< authenticate mode : OWE */
    HAL_WIFI_AUTH_MAX
} hal_wifi_auth_mode_t;            /**< wifi authenticate mode */

typedef enum {
    HAL_WIFI_CIPHER_TYPE_NONE = 0,    /**< the cipher type is none */
    HAL_WIFI_CIPHER_TYPE_WEP40,       /**< the cipher type is WEP40 */
    HAL_WIFI_CIPHER_TYPE_WEP104,      /**< the cipher type is WEP104 */
    HAL_WIFI_CIPHER_TYPE_TKIP,        /**< the cipher type is TKIP */
    HAL_WIFI_CIPHER_TYPE_CCMP,        /**< the cipher type is CCMP */
    HAL_WIFI_CIPHER_TYPE_TKIP_CCMP,   /**< the cipher type is TKIP and CCMP */
    HAL_WIFI_CIPHER_TYPE_AES_CMAC128, /**< the cipher type is AES-CMAC-128 */
    HAL_WIFI_CIPHER_TYPE_SMS4,        /**< the cipher type is SMS4 */
    HAL_WIFI_CIPHER_TYPE_GCMP,        /**< the cipher type is GCMP */
    HAL_WIFI_CIPHER_TYPE_GCMP256,     /**< the cipher type is GCMP-256 */
    HAL_WIFI_CIPHER_TYPE_AES_GMAC128, /**< the cipher type is AES-GMAC-128 */
    HAL_WIFI_CIPHER_TYPE_AES_GMAC256, /**< the cipher type is AES-GMAC-256 */
    HAL_WIFI_CIPHER_TYPE_UNKNOWN,     /**< the cipher type is unknown */
} hal_wifi_cipher_type_t;             /**< wifi cipher type */

typedef enum {
    HAL_WIFI_COUNTRY_POLICY_AUTO,   /**< Country policy is auto, use the country info of AP to which the station is connected */
    HAL_WIFI_COUNTRY_POLICY_MANUAL, /**< Country policy is manual, always use the configured country info */
} hal_wifi_country_policy_t;        /**< wifi Country policy */

typedef struct {
    char cc[3];                       /**< country code string */
    uint8_t schan;                    /**< start channel */
    uint8_t nchan;                    /**< total channel number */
    int8_t max_tx_power;              /**< This field is used for getting WiFi maximum transmitting power, call esp_wifi_set_max_tx_power to set the maximum transmitting power. */
    hal_wifi_country_policy_t policy; /**< country policy */
} hal_wifi_country_t;                 /**< wifi country */

typedef struct {
    uint8_t bssid[6];                       /**< MAC address of AP */
    uint8_t ssid[33];                       /**< SSID of AP */
    uint8_t primary;                        /**< channel of AP */
    hal_wifi_second_chan_t second;          /**< secondary channel of AP */
    int8_t rssi;                            /**< signal strength of AP */
    hal_wifi_auth_mode_t authmode;          /**< authmode of AP */
    hal_wifi_cipher_type_t pairwise_cipher; /**< pairwise cipher of AP */
    hal_wifi_cipher_type_t group_cipher;    /**< group cipher of AP */
    uint8_t ant;                            /**< antenna used to receive beacon from AP */
    bool phy_11b;                           /**< identify if 11b mode is enabled or not */
    bool phy_11g;                           /**< identify if 11g mode is enabled or not */
    bool phy_11n;                           /**< identify if 11n mode is enabled or not */
    bool phy_lr;                            /**< identify if low rate is enabled or not */
    bool wps;                               /**< identify if WPS is supported or not */
    bool ftm_responder;                     /**< identify if FTM is supported in responder mode */
    bool ftm_initiator;                     /**< identify if FTM is supported in initiator mode */
    hal_wifi_country_t country;             /**< country information of AP */
} hal_wifi_ap_record_t;                     /**< wifi AP record */

extern bool wifi_connected; /**< wifi is connected */

/**
 * @brief Return wifi scan
 *
 * @param ap_record list of APs container
 * @return quantity of APs
 */
uint32_t wifi_scan(hal_wifi_ap_record_t **ap_record);

/**
 * @brief Connect wifi to AP
 *
 * @param name AP name
 * @param pass AP password
 */
void wifi_connect_sta(const char *name, const char *pass);

/**
 * @brief Disconnect wifi and stop all related process
 *
 */
void wifi_stop(void);

#endif /* HAL_WIFI_H_ */
