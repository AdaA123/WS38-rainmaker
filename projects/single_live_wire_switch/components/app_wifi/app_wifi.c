/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <esp_event.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <string.h>

#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>

#include "app_wifi.h"
#include <esp_timer.h>
#include <nvs.h>
#include <nvs_flash.h>

#include "app_priv.h"

static const char* TAG = "app_wifi";

static TimerHandle_t g_prov_timer;

#define CREDENTIALS_NAMESPACE "rmaker_creds"
#define RANDOM_NVS_KEY "random"
#define POP_STR_SIZE 9
#define TIMER_UNIT_1_MIN (60 * 1000LL)

__attribute__((weak)) void provisioning_indicator_start(void)
{
    ESP_LOGW(TAG, "undefined public function");
    return;
}

__attribute__((weak)) void provisioning_indicator_stop(void)
{
    ESP_LOGW(TAG, "undefined public function");
    return;
}

static void app_wifi_prov_timer_reset(void);
static TickType_t app_wifi_prov_timer_expiry_get(void);
static void app_wifi_prov_timer_stop(void);
static void app_wifi_prov_stop(void);

extern void scan_set_pas_duration(uint32_t time);

int app_wifi_sta_ps_config(void)
{
   // set wifi station listen interval
   wifi_config_t wifi_cfg;
   ESP_ERROR_CHECK (esp_wifi_get_config(WIFI_IF_STA, &wifi_cfg));
   wifi_cfg.sta.listen_interval = CONFIG_EXAMPLE_WIFI_LISTEN_INTERVAL;
   ESP_ERROR_CHECK (esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));

   //
   scan_set_pas_duration(280); //magic number from my workmate
   // set wifi ps mode
   wifi_ps_type_t ps_mode = WIFI_PS_NONE;
   #if CONFIG_EXAMPLE_POWER_SAVE_MIN_MODEM
   ps_mode = WIFI_PS_MIN_MODEM;
   #elif CONFIG_EXAMPLE_POWER_SAVE_MAX_MODEM
   ps_mode = WIFI_PS_MAX_MODEM;
   #endif /*CONFIG_POWER_SAVE_MODEM*/

   return esp_wifi_set_ps(ps_mode);
}


/* Event handler for catching system events */
static void
prov_event_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data)
{
    if (event_base == WIFI_PROV_EVENT) {
        app_wifi_prov_timer_reset();
        switch (event_id) {
        case WIFI_PROV_START:
            ESP_LOGI(TAG, "Provisioning started");
            provisioning_indicator_start();
            break;
        case WIFI_PROV_CRED_RECV: {
            wifi_sta_config_t* wifi_sta_cfg = (wifi_sta_config_t*)event_data;
            ESP_LOGI(TAG, "Received Wi-Fi credentials"
                          "\n\tSSID     : %s\n\tPassword : %s",
                (const char*)wifi_sta_cfg->ssid,
                (const char*)wifi_sta_cfg->password);
            break;
        }
        case WIFI_PROV_CRED_FAIL: {
            wifi_prov_sta_fail_reason_t* reason = (wifi_prov_sta_fail_reason_t*)event_data;
            ESP_LOGE(TAG, "Provisioning failed!\n\tReason : %s",
                (*reason == WIFI_PROV_STA_AUTH_ERROR) ? "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");
            wifi_prov_mgr_reset_sm_state_on_failure();
            break;
        }
        case WIFI_PROV_CRED_SUCCESS:
            ESP_LOGI(TAG, "Provisioning successful");
            break;
        case WIFI_PROV_END:
            app_wifi_prov_timer_stop();
            /* De-initialize manager once provisioning is finished */
            wifi_prov_mgr_deinit();
            provisioning_indicator_stop();
            break;
        default:
            break;
        }
    } 
}

static void wifi_init_sta()
{
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(app_wifi_sta_ps_config());
    ESP_ERROR_CHECK(esp_wifi_start());
}

/* Free random_bytes after use only if function returns ESP_OK */
static esp_err_t read_random_bytes_from_nvs(uint8_t** random_bytes, size_t* len)
{
    nvs_handle handle;
    esp_err_t err;
    *len = 0;

    if ((err = nvs_open_from_partition(CONFIG_ESP_RMAKER_FACTORY_PARTITION_NAME, CREDENTIALS_NAMESPACE,
             NVS_READONLY, &handle))
        != ESP_OK) {
        ESP_LOGD(TAG, "NVS open for %s %s %s failed with error %d", CONFIG_ESP_RMAKER_FACTORY_PARTITION_NAME, CREDENTIALS_NAMESPACE, RANDOM_NVS_KEY, err);
        return ESP_FAIL;
    }

    if ((err = nvs_get_blob(handle, RANDOM_NVS_KEY, NULL, len)) != ESP_OK) {
        ESP_LOGD(TAG, "Error %d. Failed to read key %s.", err, RANDOM_NVS_KEY);
        nvs_close(handle);
        return ESP_ERR_NOT_FOUND;
    }

    *random_bytes = calloc(*len, 1);
    if (*random_bytes) {
        nvs_get_blob(handle, RANDOM_NVS_KEY, *random_bytes, len);
        nvs_close(handle);
        return ESP_OK;
    }
    nvs_close(handle);
    return ESP_ERR_NO_MEM;
}

static esp_err_t get_device_service_name(const char* prefix, char* service_name, size_t max)
{
    uint8_t* nvs_random = NULL;
    size_t nvs_random_size = 0;
    if ((read_random_bytes_from_nvs(&nvs_random, &nvs_random_size) != ESP_OK) || nvs_random_size < 3) {
        uint8_t eth_mac[6];
        esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
        snprintf(service_name, max, "%s_%02x%02x%02x", prefix, eth_mac[3], eth_mac[4], eth_mac[5]);
    } else {
        snprintf(service_name, max, "%s_%02x%02x%02x", prefix, nvs_random[nvs_random_size - 3],
            nvs_random[nvs_random_size - 2], nvs_random[nvs_random_size - 1]);
    }
    if (nvs_random) {
        free(nvs_random);
    }
    return ESP_OK;
}

void app_wifi_init(void)
{
    esp_netif_init();

    /* Initialize the event loop */
    esp_event_loop_create_default();

    /* Register our event handler for Wi-Fi, IP and Provisioning related events */
    esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &prov_event_handler, NULL);
    app_event_register_init();
	
    /* Initialize Wi-Fi including netif with default config */
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
}

static void app_wifi_prov_timer_reset(void)
{
    if (g_prov_timer) {
        xTimerReset(g_prov_timer, 0);
    }
}

static TickType_t app_wifi_prov_timer_expiry_get(void)
{
    if (g_prov_timer) {
        TickType_t tick = xTimerGetExpiryTime(g_prov_timer);
        return tick;
    }
    return 0;
}

static void app_wifi_prov_timer_stop(void)
{
    if (g_prov_timer) {
        xTimerDelete(g_prov_timer, 0);
        g_prov_timer = NULL;
    }
}

static void app_wifi_prov_stop(void)
{
    wifi_prov_mgr_stop_provisioning();
}

static void prov_timeout_timer_cb(void* priv)
{
    ESP_LOGW(TAG, "Provisioning timed out.");
    app_wifi_prov_stop();
    app_wifi_prov_timer_stop();
}

esp_err_t app_wifi_start_timer(uint16_t timeout)
{
    if (timeout == 0) {
        return ESP_OK;
    }

    g_prov_timer = xTimerCreate("prov timeout timer", pdMS_TO_TICKS(timeout * TIMER_UNIT_1_MIN), false, NULL, prov_timeout_timer_cb);
    xTimerStart(g_prov_timer, 0);
    ESP_LOGI(TAG, "Provisioning will auto stop after %d minute(s).", timeout);
    return ESP_OK;
}

esp_err_t app_wifi_start(app_wifi_prov_config_t* config)
{
    wifi_prov_mgr_config_t mgr_config = {
        .scheme = wifi_prov_scheme_ble,
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM
    };

    provisioning_indicator_start();

    wifi_prov_mgr_init(mgr_config);
    bool provisioned = false;
    wifi_prov_mgr_is_provisioned(&provisioned);
    if (!provisioned) {
        if (config->enable_prov == true) {
            ESP_LOGI(TAG, "Starting provisioning");
            char service_name[12];
            wifi_prov_scheme_ble_set_mfg_data(config->mfg, sizeof(config->mfg));
            get_device_service_name((const char*)config->broadcast_prefix, service_name, sizeof(service_name));
            wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_1, NULL, service_name, NULL);
            app_wifi_start_timer(config->prov_timeout);
        } else {
            ESP_LOGW(TAG, "Exit provisioning");
            return ESP_FAIL;
        }
    } else {
        ESP_LOGI(TAG, "Already provisioned, starting Wi-Fi STA");
        wifi_prov_mgr_deinit();
        wifi_init_sta();
    }
    return ESP_OK;
}
