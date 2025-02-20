// Copyright 2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "sdkconfig.h"
#if (CONFIG_QCLOUD_PROVISIONING_SMARTCONFIG) || (CONFIG_QCLOUD_PROVISIONING_SOFTAPCONFIG) || (CONFIG_QCLOUD_PROVISIONING_BLECONFIG)

#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include <cJSON.h>

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include "esp_smartconfig.h"

#include "esp_qcloud_prov.h"
#include "esp_qcloud_storage.h"
#include "esp_qcloud_iothub.h"
#include "esp_qcloud_prov.h"
#include <qrcode.h>

#define PROV_QR_VERSION            "v1"

#if (CONFIG_QCLOUD_PROVISIONING_SMARTCONFIG) || (CONFIG_QCLOUD_PROVISIONING_SOFTAPCONFIG)
#define APP_SERVER_PORT            8266
#define UDP_SERVER_BUFFER_MAX_SIZE 128
static bool g_prov_server_start_flag = false;
#endif

static const char *TAG = "esp_qcloud_prov";

#define ESP_QCLOUD_TOKEN_LEN 64
static char g_prov_token[ESP_QCLOUD_TOKEN_LEN] = {0};

void esp_qcloud_prov_print_wechat_qr(const char *name, const char *transport)
{
    if (!name || !transport) {
        ESP_LOGW(TAG, "Cannot generate QR code payload. Data missing.");
        return;
    }

    char *qcloud_payload = NULL;
    char *terminal_payload = NULL;

    asprintf(&terminal_payload, "https://iot.cloud.tencent.com/iotexplorer/device?page=%s&productId=%s&ver=%s&name=%s",
             transport, esp_qcloud_get_product_id(), PROV_QR_VERSION, name);
    ESP_LOGI(TAG, "Scan this QR code from the Wechat for Provisioning.");
    qrcode_display(terminal_payload);
    
    asprintf(&qcloud_payload, "https://iot.cloud.tencent.com/iotexplorer/device?page=%s%%26productId=%s%%26ver=%s%%26name=%s",
             transport, esp_qcloud_get_product_id(), PROV_QR_VERSION, name);
    ESP_LOGI(TAG, "If QR code is not visible, copy paste the below URL in a browser.\n%s?data=%s",
             "https://rainmaker.espressif.com/qrcode.html", qcloud_payload);

    ESP_QCLOUD_FREE(qcloud_payload);
    ESP_QCLOUD_FREE(terminal_payload);
}

esp_err_t esp_qcloud_prov_data_handler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen,
                                       uint8_t **outbuf, ssize_t *outlen, void *priv_data)
{
    if (inbuf) {
        ESP_LOGI(TAG, "Received data: %.*s", inlen, (char *)inbuf);
    }

    cJSON *root = cJSON_Parse((char *)inbuf);
    ESP_QCLOUD_ERROR_GOTO(!root, EXIT, "The data format is wrong");

    cJSON *token_json = cJSON_GetObjectItem(root, "token");
    ESP_QCLOUD_ERROR_GOTO(!token_json, EXIT, "The data format is wrong, the 'token' field is not included");

    *outlen = asprintf((char **)outbuf,
                       "{\"cmdType\":2,\"productId\":\"%s\",\"deviceName\":\"%s\",\"protoVersion\":\"2.0\"}",
                       esp_qcloud_get_product_id(), esp_qcloud_get_device_name());

EXIT:
    cJSON_Delete(root);
    return ESP_OK;
}

#if (CONFIG_QCLOUD_PROVISIONING_SMARTCONFIG) || (CONFIG_QCLOUD_PROVISIONING_SOFTAPCONFIG)

static void udp_server_task(void *pvParameters)
{
    esp_err_t err     = ESP_FAIL;
    char *rx_buffer   = ESP_QCLOUD_MALLOC(UDP_SERVER_BUFFER_MAX_SIZE);
    socklen_t socklen = sizeof(struct sockaddr_in);
    struct sockaddr_in client_addr = {0};
    struct sockaddr_in server_addr = {
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_family      = AF_INET,
        .sin_port        = htons(APP_SERVER_PORT),
    };

    int udp_server_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    ESP_QCLOUD_ERROR_GOTO(udp_server_sockfd < 0, EXIT, "Unable to create socket, errno %d, err_str: %s", errno, strerror(errno));

    err = bind(udp_server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    ESP_QCLOUD_ERROR_GOTO(err < 0, EXIT, "Socket unable to bind: errno %d, err_str: %s", errno, strerror(errno));

    ESP_LOGI(TAG, "Socket bound, port %d", APP_SERVER_PORT);

    while (g_prov_server_start_flag) {
        fd_set read_fd;
        struct timeval timeout = {.tv_sec = 1,};
        FD_ZERO(&read_fd);
        FD_SET(udp_server_sockfd, &read_fd);
        err = select(udp_server_sockfd + 1, &read_fd, NULL, NULL, &timeout);
        ESP_QCLOUD_ERROR_GOTO(err < 0, EXIT, "recvfrom failed, errno %d, err_str: %s", errno, strerror(errno));
        ESP_QCLOUD_ERROR_CONTINUE(err == 0 || !FD_ISSET(udp_server_sockfd, &read_fd), "");

        memset(rx_buffer, 0, UDP_SERVER_BUFFER_MAX_SIZE);
        int len = recvfrom(udp_server_sockfd, rx_buffer, UDP_SERVER_BUFFER_MAX_SIZE, 0, (struct sockaddr *)&client_addr, &socklen);
        ESP_QCLOUD_ERROR_GOTO(len < 0, EXIT, "recvfrom failed, errno %d, err_str: %s", errno, strerror(errno));

        ESP_LOGI(TAG, "recvfrom, data: %s", rx_buffer);

        cJSON *json_root = cJSON_Parse(rx_buffer);
        ESP_QCLOUD_ERROR_CONTINUE(!json_root, "cJSON_Parse failed, recv data: %.*s", len, rx_buffer);

        memset(g_prov_token, 0, sizeof(g_prov_token));
        memcpy(g_prov_token, cJSON_GetObjectItem(json_root, "token")->valuestring, 
        strlen(cJSON_GetObjectItem(json_root, "token")->valuestring) > ESP_QCLOUD_TOKEN_LEN ? 
          ESP_QCLOUD_TOKEN_LEN : strlen(cJSON_GetObjectItem(json_root, "token")->valuestring));

        if (cJSON_GetObjectItem(json_root, "cmdType")->valueint == CMD_SSID_PW_TOKEN) {
            wifi_config_t wifi_cfg = {0};
            strcpy((char *)wifi_cfg.sta.ssid, cJSON_GetObjectItem(json_root, "ssid")->valuestring);
            strcpy((char *)wifi_cfg.sta.password, cJSON_GetObjectItem(json_root, "password")->valuestring);

            // /* Configure Wi-Fi as both AP and/or Station */
            ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_cfg));
            ESP_ERROR_CHECK(esp_wifi_connect());
        }

        cJSON_Delete(json_root);

        if (1) {
            wifi_config_t wifi_cfg = {0};
            ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_cfg));
            ESP_ERROR_CHECK(esp_wifi_disconnect());
            ESP_LOGW(TAG, "Timeout waiting for connection router, please try again");
            continue;
        }

        xEventGroupSetBits(g_wifi_event_group, QCLOUD_PROV_EVENT_GET_TOKEN);

        char *tx_buffer = NULL;
        asprintf(&tx_buffer, "{\"cmdType\":%d,\"productId\":\"%s\",\"deviceName\":\"%s\",\"protoVersion\":\"2.0\"}",
                 CMD_DEVICE_REPLY, esp_qcloud_get_product_id(), esp_qcloud_get_device_name());

        ESP_LOGI(TAG, "sendto, data: %s", tx_buffer);

        for (int i = 0; i < 5; i++) {
            vTaskDelay(pdMS_TO_TICKS(i * 10));
            err = sendto(udp_server_sockfd, tx_buffer, strlen(tx_buffer), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
            ESP_QCLOUD_ERROR_CONTINUE(err < 0, "sendto failed, errno %d, err_str: %s", errno, strerror(errno));
            
            break;
        }

        ESP_QCLOUD_FREE(tx_buffer);

        break;
    }

EXIT:

    if (udp_server_sockfd != -1) {
        ESP_LOGI(TAG, "Shutting down socket");
        shutdown(udp_server_sockfd, 0);
        close(udp_server_sockfd);
    }

    ESP_QCLOUD_FREE(rx_buffer);
    vTaskDelete(NULL);
}

esp_err_t esp_qcloud_prov_udp_server_start()
{
    if (!g_prov_server_start_flag) {
        g_prov_server_start_flag = true;
        xTaskCreate(udp_server_task, "prov_udp_server", 4096, NULL, 5, NULL);
    }
    return ESP_OK;
}

esp_err_t esp_qcloud_prov_udp_server_stop()
{
    g_prov_server_start_flag = false;
    return ESP_OK;
}
#endif

esp_err_t esp_qcloud_prov_set_token(char *token, size_t token_len)
{
    ESP_LOGW(TAG, "esp_qcloud_prov_set_token:%s",token);
    memset(g_prov_token, 0, sizeof(g_prov_token));
    memcpy(g_prov_token, token, token_len > ESP_QCLOUD_TOKEN_LEN ? ESP_QCLOUD_TOKEN_LEN : token_len);
    return esp_event_post(QCLOUD_EVENT, QCLOUD_EVENT_IOTHUB_BLE_GET_TOKEN, NULL, 0, portMAX_DELAY);
}

char *esp_qcloud_prov_get_token(void)
{
    return g_prov_token;
}

#endif