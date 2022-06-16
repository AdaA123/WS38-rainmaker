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

#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <esp_log.h>
#include <mqtt_client.h>

#include <esp_qcloud_mqtt.h>

#include "esp_qcloud_iothub.h"

static const char *TAG = "esp_qcloud_mqtt";

#define MAX_MQTT_SUBSCRIPTIONS      6

typedef struct {
    char *topic;
    esp_qcloud_mqtt_subscribe_cb_t cb;
    void *priv;
} esp_qcloud_mqtt_subscription_t;

typedef struct {
    esp_mqtt_client_handle_t mqtt_client;
    esp_qcloud_mqtt_config_t *config;
    esp_qcloud_mqtt_subscription_t *subscriptions[MAX_MQTT_SUBSCRIPTIONS];
} esp_qcloud_mqtt_data_t;

esp_qcloud_mqtt_data_t g_mqtt_data = {0};

const int MQTT_CONNECTED_EVENT = BIT1;

#define QCLOUD_MQTT_ERROR_CHECK(con, err, format, ...) do { \
        if (con) { \
            if(*format != '\0') \
                ESP_LOGE(TAG,"line:%d, <ret: %d> " format, __LINE__, err,  ##__VA_ARGS__); \
            return err; \
        } \
    } while(0)

static void esp_qcloud_mqtt_subscribe_callback(const char *topic, int topic_len, const char *data, int data_len)
{
    esp_qcloud_mqtt_subscription_t **subscriptions = g_mqtt_data.subscriptions;
    for (int i = 0; i < MAX_MQTT_SUBSCRIPTIONS; i++) {
        if (subscriptions[i]) {
            if (strncmp(topic, subscriptions[i]->topic, topic_len) == 0) {
                subscriptions[i]->cb(subscriptions[i]->topic, (void *)data, data_len, subscriptions[i]->priv);
            }
        }
    }
}

esp_err_t esp_qcloud_mqtt_subscribe(const char *topic, esp_qcloud_mqtt_subscribe_cb_t cb, void *priv_data)
{
    QCLOUD_MQTT_ERROR_CHECK(topic == NULL, ESP_ERR_INVALID_ARG, "topic is NULL ");
    QCLOUD_MQTT_ERROR_CHECK(cb == NULL, ESP_ERR_INVALID_ARG, "cb is NULL ");
    QCLOUD_MQTT_ERROR_CHECK(g_mqtt_data.mqtt_client == NULL, ESP_ERR_INVALID_ARG, "g_mqtt_data.mqtt_client is NULL ");

    for (int i = 0; i < MAX_MQTT_SUBSCRIPTIONS; i++) {
        if (!g_mqtt_data.subscriptions[i]) {
            esp_qcloud_mqtt_subscription_t *subscription = calloc(1, sizeof(esp_qcloud_mqtt_subscription_t));

            if (!subscription) {
                return ESP_FAIL;
            }
            subscription->topic = strdup(topic);
            if (!subscription->topic) {
                free(subscription);
                return ESP_FAIL;
            }
            int ret = esp_mqtt_client_subscribe(g_mqtt_data.mqtt_client, subscription->topic, 1);
            if (ret < 0) {
                free(subscription->topic);
                free(subscription);
                return ESP_FAIL;
            }

            subscription->priv = priv_data;
            subscription->cb = cb;
            g_mqtt_data.subscriptions[i] = subscription;
            ESP_LOGD(TAG, "Subscribed to topic: %s", topic);
            return ESP_OK;
        }
    }
    return ESP_FAIL;
}

esp_err_t esp_qcloud_mqtt_unsubscribe(const char *topic)
{
    QCLOUD_MQTT_ERROR_CHECK(topic == NULL, ESP_ERR_INVALID_ARG, "topic is NULL ");
    QCLOUD_MQTT_ERROR_CHECK(g_mqtt_data.mqtt_client == NULL, ESP_ERR_INVALID_ARG, "g_mqtt_data.mqtt_client is NULL ");

    int ret = esp_mqtt_client_unsubscribe(g_mqtt_data.mqtt_client, topic);
    QCLOUD_MQTT_ERROR_CHECK(ret != ESP_OK, ret, "esp_mqtt_client_unsubscribe() failed with err = %d",ret);

    esp_qcloud_mqtt_subscription_t **subscriptions = g_mqtt_data.subscriptions;
    for (int i = 0; i < MAX_MQTT_SUBSCRIPTIONS; i++) {
        if (subscriptions[i]) {
            if (strncmp(topic, subscriptions[i]->topic, strlen(topic)) == 0) {
                free(subscriptions[i]->topic);
                free(subscriptions[i]);
                subscriptions[i] = NULL;
                return ESP_OK;
            }
        }
    }

    return ESP_FAIL;
}

esp_err_t esp_qcloud_mqtt_publish(const char *topic, void *data, size_t data_len)
{
    QCLOUD_MQTT_ERROR_CHECK(topic == NULL, ESP_ERR_INVALID_ARG, "topic is NULL ");
    QCLOUD_MQTT_ERROR_CHECK(data == NULL, ESP_ERR_INVALID_ARG, "data is NULL ");
    QCLOUD_MQTT_ERROR_CHECK(g_mqtt_data.mqtt_client == NULL, ESP_ERR_INVALID_ARG, "g_mqtt_data.mqtt_client is NULL ");

    int ret = esp_mqtt_client_publish(g_mqtt_data.mqtt_client, topic, data, data_len, 1, 0);
    QCLOUD_MQTT_ERROR_CHECK(ret < 0, ESP_FAIL, "esp_mqtt_client_publish() failed with err = %d",ret);
    return ESP_OK;
}

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Connected");
            esp_event_post(QCLOUD_EVENT, QCLOUD_EVENT_IOTHUB_MQTT_CONNECT, NULL, 0, portMAX_DELAY);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT Disconnected. ");
            esp_event_post(QCLOUD_EVENT, QCLOUD_EVENT_IOTHUB_MQTT_DISCONNECT, NULL, 0, portMAX_DELAY);
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGD(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGD(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGD(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGD(TAG, "MQTT_EVENT_DATA");
            ESP_LOGD(TAG, "TOPIC=%.*s\r\n", event->topic_len, event->topic);
            ESP_LOGD(TAG, "DATA=%.*s\r\n", event->data_len, event->data);
            ESP_LOGI(TAG, "msg_id=%d\r\n", event->msg_id);
            static int last_msg_id = -1;
            if (event->msg_id > last_msg_id) {
                esp_qcloud_mqtt_subscribe_callback(event->topic, event->topic_len, event->data, event->data_len);
                last_msg_id = event->msg_id;
            } else {
                ESP_LOGI("", "mqtt retry transaction message");
            }
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
            break;

        default:
            ESP_LOGD(TAG, "Other event id:%d", event->event_id);
            break;
    }

    return ESP_OK;
}

esp_err_t esp_qcloud_mqtt_reconnect(void)
{
    return esp_mqtt_client_reconnect(g_mqtt_data.mqtt_client);
}

esp_err_t esp_qcloud_mqtt_connect(void)
{
    QCLOUD_MQTT_ERROR_CHECK(g_mqtt_data.mqtt_client == NULL, ESP_ERR_INVALID_ARG, "g_mqtt_data.mqtt_client is NULL ");
    ESP_LOGI(TAG, "Connecting to %s", g_mqtt_data.config->host);
    esp_err_t ret = esp_mqtt_client_start(g_mqtt_data.mqtt_client);
    QCLOUD_MQTT_ERROR_CHECK(ret != ESP_OK, ret, "esp_mqtt_client_start() failed with err = %d",ret);
    ESP_LOGI(TAG, "Waiting for MQTT connection. This may take time.");
    return ESP_OK;
}

esp_err_t esp_qcloud_mqtt_disconnect(void)
{
    esp_err_t ret = ESP_FAIL;
    QCLOUD_MQTT_ERROR_CHECK(g_mqtt_data.mqtt_client == NULL, ESP_ERR_INVALID_ARG, "g_mqtt_data.mqtt_client is NULL ");
    ret = esp_mqtt_client_disconnect(g_mqtt_data.mqtt_client);
    QCLOUD_MQTT_ERROR_CHECK(ret != ESP_OK, ret, "esp_mqtt_client_disconnect() failed with err = %d",ret);
    ret = esp_mqtt_client_stop(g_mqtt_data.mqtt_client);
    QCLOUD_MQTT_ERROR_CHECK(ret != ESP_OK, ret, "esp_mqtt_client_stop() failed with err = %d",ret);
    return ESP_OK;
}

esp_err_t esp_qcloud_mqtt_init(esp_qcloud_mqtt_config_t *config)
{
    if(g_mqtt_data.config == NULL){
        g_mqtt_data.config = malloc(sizeof(esp_qcloud_mqtt_config_t));
    }
    memcpy(g_mqtt_data.config, config, sizeof(esp_qcloud_mqtt_config_t));
    
    const esp_mqtt_client_config_t mqtt_client_cfg = {
        .username  = g_mqtt_data.config->username,
        .password  = g_mqtt_data.config->password,
        .client_id = g_mqtt_data.config->client_id,
        .uri       = g_mqtt_data.config->host,
        .cert_pem  = g_mqtt_data.config->server_cert,
        .client_cert_pem = g_mqtt_data.config->client_cert,
        .client_key_pem  = g_mqtt_data.config->client_key,
        .keepalive       = 180,
        .disable_auto_reconnect = true,
        .event_handle    = mqtt_event_handler,
        .disable_clean_session = true
    };
    if(g_mqtt_data.mqtt_client == NULL){
        g_mqtt_data.mqtt_client = esp_mqtt_client_init(&mqtt_client_cfg);
    }
    return g_mqtt_data.mqtt_client != NULL ? ESP_OK :ESP_FAIL;
}

esp_err_t esp_qcloud_mqtt_deinit(void)
{
    esp_err_t ret = ESP_FAIL;

    if(g_mqtt_data.config != NULL){
        free(g_mqtt_data.config);
        g_mqtt_data.config = NULL;
    }
    if(g_mqtt_data.mqtt_client != NULL){
        ret = esp_mqtt_client_destroy(g_mqtt_data.mqtt_client);
        g_mqtt_data.mqtt_client = NULL;
    }
    return ret;
}

