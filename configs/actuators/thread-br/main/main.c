/*
 * SPDX-FileCopyrightText: 2021-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 *
 * OpenThread Border Router Example
 *
 * This example code is in the Public Domain (or CC0 licensed, at your option.)
 *
 * Unless required by applicable law or agreed to in writing, this
 * software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.
 */

#include <stdio.h>
#include <string.h>

#include "sdkconfig.h"
#include "esp_check.h"
#include "esp_coexist.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_openthread.h"
#include "esp_openthread_lock.h"
#include "esp_openthread_netif_glue.h"
#include "esp_openthread_spinel.h"
#include "esp_openthread_types.h"
#if CONFIG_OPENTHREAD_CLI_ESP_EXTENSION
#include "esp_ot_cli_extension.h"
#endif // CONFIG_OPENTHREAD_CLI_ESP_EXTENSION
#include "esp_ot_config.h"
#include "esp_vfs_dev.h"
#include "esp_vfs_eventfd.h"
#include "esp_wifi.h"
#include "esp_http_server.h"
#include "mdns.h"
#include "nvs_flash.h"
#include "ot_examples_br.h"
#include "ot_examples_common.h"
#include "protocol_examples_common.h"

#if CONFIG_OPENTHREAD_STATE_INDICATOR_ENABLE
#include "ot_led_strip.h"
#endif

#define TAG "esp_ot_br"

#if CONFIG_OPENTHREAD_SUPPORT_HW_RESET_RCP
#define PIN_TO_RCP_RESET CONFIG_OPENTHREAD_HW_RESET_RCP_PIN
static void rcp_failure_hardware_reset_handler(void)
{
    gpio_config_t reset_pin_config;
    memset(&reset_pin_config, 0, sizeof(reset_pin_config));
    reset_pin_config.intr_type = GPIO_INTR_DISABLE;
    reset_pin_config.pin_bit_mask = BIT(PIN_TO_RCP_RESET);
    reset_pin_config.mode = GPIO_MODE_OUTPUT;
    reset_pin_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    reset_pin_config.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&reset_pin_config);
    gpio_set_level(PIN_TO_RCP_RESET, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(PIN_TO_RCP_RESET, 1);
    vTaskDelay(pdMS_TO_TICKS(30));
    gpio_reset_pin(PIN_TO_RCP_RESET);
}
#endif

// === OTBR REST API (port 8081) for Home Assistant ===
#include "openthread/dataset.h"
#include "openthread/instance.h"
#include "openthread/thread.h"

static void get_dataset_hex(char *buf, size_t buflen)
{
    otInstance *instance = esp_openthread_get_instance();
    otOperationalDatasetTlvs dataset;
    otError err = otDatasetGetActiveTlvs(instance, &dataset);
    if (err != OT_ERROR_NONE) { buf[0] = '\0'; return; }
    for (int i = 0; i < dataset.mLength; i++)
        sprintf(buf + i * 2, "%02x", dataset.mTlvs[i]);
    buf[dataset.mLength * 2] = '\0';
}

static esp_err_t node_handler(httpd_req_t *req)
{
    otInstance *instance = esp_openthread_get_instance();
    otDeviceRole role = otThreadGetDeviceRole(instance);
    int state = (role >= OT_DEVICE_ROLE_CHILD) ? 4 : 1;
    char resp[32];
    snprintf(resp, sizeof(resp), "{\"State\":%d}", state);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, resp);
    return ESP_OK;
}

static esp_err_t active_dataset_handler(httpd_req_t *req)
{
    char hex[OT_OPERATIONAL_DATASET_MAX_LENGTH * 2 + 1];
    get_dataset_hex(hex, sizeof(hex));
    if (hex[0] == '\0') {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No active dataset");
        return ESP_FAIL;
    }
    char resp[OT_OPERATIONAL_DATASET_MAX_LENGTH * 2 + 32];
    snprintf(resp, sizeof(resp), "{\"ActiveDataset\":\"%s\"}", hex);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, resp);
    return ESP_OK;
}

static void start_otbr_api(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 8081;
    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start REST API");
        return;
    }
    httpd_uri_t uris[] = {
        { .uri = "/node", .method = HTTP_GET, .handler = node_handler, .user_ctx = NULL },
        { .uri = "/networks/dataset/active", .method = HTTP_GET, .handler = active_dataset_handler, .user_ctx = NULL },
        { .uri = "/dataset/active", .method = HTTP_GET, .handler = active_dataset_handler, .user_ctx = NULL },
    };
    for (int i = 0; i < 3; i++) httpd_register_uri_handler(server, &uris[i]);

    // Register mDNS services for Home Assistant discovery
    mdns_service_add(NULL, "_meshcop", "_tcp", 8081, NULL, 0);
    mdns_service_add(NULL, "_otbr", "_tcp", 8081, NULL, 0);
    ESP_LOGI(TAG, "REST API on port 8081");
}

void app_main(void)
{
    // Used eventfds:
    // * netif
    // * task queue
    // * border router
    size_t max_eventfd = 3;

#if CONFIG_OPENTHREAD_RADIO_NATIVE || CONFIG_OPENTHREAD_RADIO_SPINEL_SPI
    max_eventfd++;
#endif
#if CONFIG_OPENTHREAD_RADIO_TREL
    max_eventfd++;
#endif
    esp_vfs_eventfd_config_t eventfd_config = {
        .max_fds = max_eventfd,
    };
    ESP_ERROR_CHECK(esp_vfs_eventfd_register(&eventfd_config));
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set("esp-ot-br"));
#if CONFIG_OPENTHREAD_SUPPORT_HW_RESET_RCP
    esp_openthread_register_rcp_failure_handler(rcp_failure_hardware_reset_handler);
#endif

#if CONFIG_OPENTHREAD_CLI
    ot_console_start();
#endif

    // Start REST API (needed by Home Assistant)
    start_otbr_api();

    static esp_openthread_config_t config = {
        .netif_config = ESP_NETIF_DEFAULT_OPENTHREAD(),
        .platform_config = {
            .radio_config = ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG(),
            .host_config = ESP_OPENTHREAD_DEFAULT_HOST_CONFIG(),
            .port_config = ESP_OPENTHREAD_DEFAULT_PORT_CONFIG(),
        },
    };

    ESP_ERROR_CHECK(esp_openthread_start(&config));
    esp_netif_set_default_netif(esp_openthread_get_netif());
#if CONFIG_OPENTHREAD_CLI_ESP_EXTENSION
    esp_cli_custom_command_init();
#endif
#if CONFIG_OPENTHREAD_BORDER_ROUTER_AUTO_START
    ESP_ERROR_CHECK(esp_openthread_border_router_start());
#if CONFIG_ESP_COEX_SW_COEXIST_ENABLE && CONFIG_SOC_IEEE802154_SUPPORTED
    ESP_ERROR_CHECK(esp_coex_wifi_i154_enable());
#endif
#endif
#if CONFIG_OPENTHREAD_NETWORK_AUTO_START
    ot_network_auto_start();
#endif
}
