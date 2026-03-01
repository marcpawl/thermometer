/* OTA example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_app_desc.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "protocol_examples_common.h"
#include "string.h"
#ifdef CONFIG_EXAMPLE_USE_CERT_BUNDLE
#include "esp_crt_bundle.h"
#endif

#include "nvs.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include <sys/socket.h>
#if CONFIG_EXAMPLE_CONNECT_WIFI
#include "esp_wifi.h"
#endif
}

#include "ota_task.hpp"



#ifdef CONFIG_EXAMPLE_FIRMWARE_UPGRADE_BIND_IF
/* The interface name value can refer to if_desc in esp_netif_defaults.h */
#if CONFIG_EXAMPLE_FIRMWARE_UPGRADE_BIND_IF_ETH
static const char *bind_interface_name = EXAMPLE_NETIF_DESC_ETH;
#elif CONFIG_EXAMPLE_FIRMWARE_UPGRADE_BIND_IF_STA
static const char *bind_interface_name = EXAMPLE_NETIF_DESC_STA;
#endif
#endif

static const char *TAG = "simple_ota_example";
extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

#define OTA_URL_SIZE 256

esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            break;
    }
    return ESP_OK;
}

static esp_http_client_config_t http_client_config()
{
    esp_http_client_config_t config;
    memset(&config, 0, sizeof(config));
    config.url = CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL;
    config.host = nullptr;
    config.port= 0;
    config.username  = nullptr;
    config.password = nullptr;
    config.auth_type = HTTP_AUTH_TYPE_NONE;
    config.path = nullptr;
    config.query = nullptr;
    config.cert_pem  = nullptr;
    config.cert_len = 0;
    config.client_cert_len = 0;
    config.client_key_pem = nullptr;
    config.client_key_len = 0;
    config.client_key_password = nullptr;
    config.client_key_password_len = 0;
    config.tls_version = ESP_HTTP_CLIENT_TLS_VER_TLS_1_3;
    config.user_agent = "marcpawl thermometer";
    config.method = HTTP_METHOD_GET;
    config.timeout_ms = 10000;
    config.disable_auto_redirect = false;
    config.max_redirection_count = 0;
    config.max_authorization_retries = 0;
    config.event_handler = _http_event_handler;


#ifdef CONFIG_EXAMPLE_USE_CERT_BUNDLE
        config.crt_bundle_attach = esp_crt_bundle_attach;
#else
        config.cert_pem = (char *) server_cert_pem_start;
#endif /* CONFIG_EXAMPLE_USE_CERT_BUNDLE */
        config.keep_alive_enable = true;
#ifdef CONFIG_EXAMPLE_FIRMWARE_UPGRADE_BIND_IF
        config.if_name = &ifr;
#endif
#if CONFIG_EXAMPLE_TLS_DYN_BUF_RX_STATIC
        /* This part applies static buffer strategy for rx dynamic buffer.
         * This is to avoid frequent allocation and deallocation of dynamic buffer.
         */
        config.tls_dyn_buf_strategy = HTTP_TLS_DYN_BUF_RX_STATIC;
#endif /* CONFIG_EXAMPLE_TLS_DYN_BUF_RX_STATIC */

    return config;
}


static esp_https_ota_config_t https_ota_config(esp_http_client_config_t* config)
{
    esp_https_ota_config_t ota_config;
    memset(&ota_config, 0, sizeof(ota_config));
    ota_config.http_config = config;
    return ota_config;
};

static void do_ota(void *pvParameter) {
#ifdef CONFIG_EXAMPLE_FIRMWARE_UPGRADE_BIND_IF
    esp_netif_t *netif = get_example_netif_from_desc(bind_interface_name);
    if (netif == NULL) {
        ESP_LOGE(TAG, "Can't find netif from interface description");
        abort();
    }
    struct ifreq ifr;
    esp_netif_get_netif_impl_name(netif, ifr.ifr_name);
    ESP_LOGI(TAG, "Bind interface name is %s", ifr.ifr_name);
#endif
    esp_http_client_config_t config = http_client_config();

#ifdef CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL_FROM_STDIN
    char url_buf[OTA_URL_SIZE];
    if (strcmp(config.url, "FROM_STDIN") == 0) {
        example_configure_stdin_stdout();
        fgets(url_buf, OTA_URL_SIZE, stdin);
        int len = strlen(url_buf);
        url_buf[len - 1] = '\0';
        config.url = url_buf;
    } else {
        ESP_LOGE(TAG, "Configuration mismatch: wrong firmware upgrade image url");
        abort();
    }
#endif

#ifdef CONFIG_EXAMPLE_SKIP_COMMON_NAME_CHECK
    config.skip_cert_common_name_check = true;
#endif

    esp_https_ota_config_t ota_config = https_ota_config(&config);
    esp_https_ota_handle_t ota_handle = NULL;

    // 1. Connect and start the OTA session
    esp_err_t err = esp_https_ota_begin(&ota_config, &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ESP HTTPS OTA Begin failed: %s", esp_err_to_name(err));
        return;
    }

    // 2. Extract the new app's header information from the stream
    esp_app_desc_t app_desc;
    err = esp_https_ota_get_img_desc(ota_handle, &app_desc);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get image description: %s", esp_err_to_name(err));
        esp_https_ota_abort(ota_handle);
        return;
    }

    // 3. Compare with currently running version
    const esp_app_desc_t *running_desc = esp_app_get_description();

    if (strcmp(app_desc.version, running_desc->version) == 0) {
        ESP_LOGW(TAG, "Versions are identical (%s). Stopping OTA.", app_desc.version);
        esp_https_ota_abort(ota_handle);
        return;
    }

    // 4. Versions are different -> Proceed with download
    ESP_LOGI(TAG, "Running version %s", app_desc.version);
    ESP_LOGI(TAG, "New version found: %s. Downloading...", app_desc.version);
    while (1) {
        err = esp_https_ota_perform(ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        } else {
            ESP_LOGD(TAG, "%04x %s", err, esp_err_to_name(err));
        }
    }

    // 5. Cleanup and Reboot
    err = esp_https_ota_finish(ota_handle);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "OTA upgrade completed. Rebooting ...");
        esp_restart();
    }
    ESP_LOGW(TAG, "OTA upgrade failed. %04X %s", err, esp_err_to_name(err) );
}

void ota_task(void *pvParameter)
{
    ESP_LOGI(TAG, "Starting OTA task");
    do_ota(pvParameter);
    ESP_LOGI(TAG, "Ending OTA task");
    vTaskDelete(NULL);
}




