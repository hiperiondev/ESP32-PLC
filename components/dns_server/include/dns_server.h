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

#ifndef DNS_SERVER_H
#define DNS_SERVER_H

#ifndef DNS_SERVER_MAX_ITEMS
#define DNS_SERVER_MAX_ITEMS 1
#endif

#define DNS_SERVER_CONFIG_SINGLE(queried_name, netif_key)                                                                                                      \
    {                                                                                                                                                          \
        .num_of_entries = 1, .item = { { .name = queried_name, .if_key = netif_key } }                                                                         \
    }

/**
 * @brief Definition of one DNS entry: NAME - IP (or the netif whose IP to answer)
 *
 * @note Please use string literals (or ensure they are valid during dns_server lifetime) as names, since
 * we don't take copies of the config values `name` and `if_key`
 */
typedef struct dns_entry_pair {
    const char *name;   /**<! Exact match of the name field of the DNS query to answer */
    const char *if_key; /**<! Use this network interface IP to answer, only if NULL, use the static IP below */
    esp_ip4_addr_t ip;  /**<! Constant IP address to answer this query, if "if_key==NULL" */
} dns_entry_pair_t;

/**
 * @brief DNS server config struct defining the rules for answering DNS (A type) queries
 *
 * @note If you want to define more rules, you can set `DNS_SERVER_MAX_ITEMS` before including this header
 * Example of using 2 entries with constant IP addresses
 * \code{.c}
 * #define DNS_SERVER_MAX_ITEMS 2
 * #include "dns_server.h"
 *
 * dns_server_config_t config = {
 *   .num_of_entries = 2,
 *   .item = { {.name = "my-esp32.com", .ip = { .addr = ESP_IP4TOADDR( 192, 168, 4, 1) } } ,
 *             {.name = "my-utils.com", .ip = { .addr = ESP_IP4TOADDR( 192, 168, 4, 100) } } } };
 * start_dns_server(&config);
 * \endcode
 */
typedef struct dns_server_config {
    int num_of_entries;                          /**<! Number of rules specified in the config struct */
    dns_entry_pair_t item[DNS_SERVER_MAX_ITEMS]; /**<! Array of pairs */
} dns_server_config_t;

/**
 * @brief DNS server handle
 */
typedef struct dns_server_handle *dns_server_handle_t;

/**
 * @brief Set ups and starts a simple DNS server that will respond to all A queries (IPv4)
 * based on configured rules, pairs of name and either IPv4 address or a netif ID (to respond by it's IPv4 add)
 *
 * @param config Configuration structure listing the pairs of (name, IP/netif-id)
 * @return dns_server's handle on success, NULL on failure
 */
dns_server_handle_t start_dns_server(dns_server_config_t *config);

/**
 * @brief Stops and destroys DNS server's task and structs
 * @param handle DNS server's handle to destroy
 */
void stop_dns_server(dns_server_handle_t handle);

#endif
