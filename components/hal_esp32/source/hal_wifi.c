/*
 * @file hal_wifi.c
 * @brief WIFI port
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see
 * @note This is based on other projects. See license files
 */

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_event_base.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "hal_wifi.h"

static const char *TAG = "hal_wifi";

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry_num = 0;
esp_netif_t *sta_netif;
bool wifi_connected = false;

void wifi_stop(void) {
    esp_wifi_stop();
    esp_netif_destroy_default_wifi(sta_netif);
    esp_event_loop_delete_default();
    esp_netif_deinit();
    wifi_connected = false;
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

uint32_t wifi_scan(hal_wifi_ap_record_t **ap_record) {
    if (!wifi_connected) {
        ESP_ERROR_CHECK(esp_netif_init());

        ESP_ERROR_CHECK(esp_event_loop_create_default());
        sta_netif = esp_netif_create_default_wifi_sta();
        assert(sta_netif);

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_start());
    }

    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));

    esp_wifi_scan_start(NULL, true);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));

    *ap_record = malloc(DEFAULT_SCAN_LIST_SIZE * sizeof(hal_wifi_ap_record_t));

    for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_count); i++) {
        for (int n = 0; n < 6; n++)
            (*ap_record)[i].bssid[n] = ap_info[i].bssid[n];
        for (int n = 0; n < 33; n++)
            (*ap_record)[i].ssid[n] = ap_info[i].ssid[n];
        (*ap_record)[i].primary = ap_info[i].primary;
        (*ap_record)[i].second = ap_info[i].second;
        (*ap_record)[i].rssi = ap_info[i].rssi;
        (*ap_record)[i].authmode = ap_info[i].authmode;
        (*ap_record)[i].pairwise_cipher = ap_info[i].pairwise_cipher;
        (*ap_record)[i].group_cipher = ap_info[i].group_cipher;
        (*ap_record)[i].ant = ap_info[i].ant;
        (*ap_record)[i].phy_11b = (bool)ap_info[i].phy_11b;
        (*ap_record)[i].phy_11g = (bool)ap_info[i].phy_11g;
        (*ap_record)[i].phy_11n = (bool)ap_info[i].phy_11n;
        (*ap_record)[i].phy_lr = (bool)ap_info[i].phy_lr;
        (*ap_record)[i].wps = (bool)ap_info[i].wps;
        (*ap_record)[i].ftm_responder = (bool)ap_info[i].ftm_responder;
        (*ap_record)[i].ftm_initiator = (bool)ap_info[i].ftm_initiator;
        (*ap_record)[i].country = (*(hal_wifi_country_t*)(&ap_info[i].country));
    }
    if (!wifi_connected)
        wifi_stop();

    return ap_count;
}

static void _wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 10) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            wifi_connected = false;
        }
        ESP_LOGI(TAG, "connect to the AP fail");
        wifi_connected = false;
        //wifi_stop();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip: %d.%d.%d.%d", IP2STR(&event->ip_info.ip));
        wifi_connected = true;
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_connect_sta(const char *ssid, const char *pass) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    s_wifi_event_group = xEventGroupCreate();

    sta_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &_wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &_wifi_event_handler, NULL, &instance_got_ip));

    wifi_config_t wifi_config = {
            .sta = {
                    .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            },
    };
    memcpy(wifi_config.sta.ssid, ssid, strlen(ssid) > 32 ? 32 : strlen(ssid));
    memcpy(wifi_config.sta.password, pass, strlen(pass) > 32 ? 32 : strlen(pass));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(
            s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY
            );

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", ssid, pass);
        wifi_connected = true;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", ssid, pass);
        wifi_connected = false;
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
        wifi_connected = false;
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
}
