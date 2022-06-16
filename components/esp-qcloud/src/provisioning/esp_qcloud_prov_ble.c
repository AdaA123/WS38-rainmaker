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
#ifdef CONFIG_QCLOUD_PROVISIONING_BLECONFIG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "cJSON.h"
#include "sys/param.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_blufi.h"
#include "esp_blufi_api.h"
#include "esp_qcloud_prov_blufi_init.h"

#include "esp_qcloud_log.h"
#include "esp_qcloud_storage.h"
#include "esp_qcloud_iothub.h"
#include "esp_qcloud_prov_tencent.h"
#include "esp_qcloud_prov.h"
#include "esp_qcloud_prov_tencent.h"

#define CONFIG_QCLOUD_LOG_MAX_SIZE 1024

static const char* TAG = "esp_qcloud_prov_ble";

static char *g_local_name = NULL;

static bool g_wifi_is_connected = false;
static bool g_ble_is_connected = false;

static wifi_config_t sta_config = {0};
static bool g_blufi_is_started = false;

bool esp_qcloud_prov_ble_is_connected(void)
{
    return g_ble_is_connected;
}

bool esp_qcloud_prov_wifi_is_connected(void)
{
    return g_wifi_is_connected;
}

void esp_qcloud_prov_wifi_set_status(bool status)
{
    g_wifi_is_connected = status;
}


static uint8_t blufi_service_uuid128[32] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,
};

static esp_ble_adv_data_t blufi_adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x100, //slave connection min interval, Time = min_interval * 1.25 msec
    .max_interval = 0x200, //slave connection max interval, Time = max_interval * 1.25 msec
    .appearance = 0x00,
    .manufacturer_len = 0,
    .p_manufacturer_data =  NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = 16,
    .p_service_uuid = blufi_service_uuid128,
    .flag = 0x6,
};

static esp_ble_adv_params_t blufi_adv_params = {
    .adv_int_min        = 0x640,
    .adv_int_max        = 0x640,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static void esp_ble_gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        esp_ble_gap_start_advertising(&blufi_adv_params);
        break;
    default:
        break;
    }
}

static esp_err_t esp_qcloud_prov_ble_handle_recv_data(char *data, uint32_t len)
{
    ESP_LOGW(TAG, "esp_qcloud_prov_ble_handle_recv_data");
    esp_err_t err = ESP_FAIL;
	cJSON *root   = cJSON_Parse(data);
    ESP_QCLOUD_ERROR_GOTO(!root, EXIT, "The data format is wrong");

	cJSON *cmd_json = cJSON_GetObjectItem(root, "cmdType");
    ESP_QCLOUD_ERROR_GOTO(!cmd_json, EXIT, "The data format is wrong, the 'cmdType' field is not included");

	switch (cmd_json->valueint) {
		/*device ble token */
		case CMD_BLE_TOKEN: {
            cJSON *token_json = cJSON_GetObjectItem(root, "token");
            ESP_QCLOUD_ERROR_GOTO(!cmd_json, EXIT, "The data format is wrong, the 'cmdType' token is not included");
            err = esp_qcloud_prov_set_token(token_json->valuestring, 32);
		}
		break;
		
		/*device ble log upload */
		case CMD_BLE_LOG_UPLOAD: {
            bool result = cJSON_HasObjectItem(root, "log");
            ESP_QCLOUD_ERROR_GOTO(!result, EXIT, "The data format is wrong, the 'log' token is not included");

			int log_size = esp_qcloud_log_flash_size();
			char *data_upload_log = ESP_QCLOUD_MALLOC(CONFIG_QCLOUD_LOG_MAX_SIZE - 17);
				
		    ESP_LOGI(TAG, "The flash partition that stores the log size: %d", log_size);
				
			for (size_t size = MIN(CONFIG_QCLOUD_LOG_MAX_SIZE - 17, log_size);
					size > 0 && esp_qcloud_log_flash_read(data_upload_log, &size) == ESP_OK;
					log_size -= size, size = MIN(CONFIG_QCLOUD_LOG_MAX_SIZE - 17, log_size)) {
				ESP_LOGI(TAG, "upload_log:%s.", data_upload_log);
				fflush(stdout);
			}
            if(g_ble_is_connected){
                err = esp_blufi_send_custom_data((uint8_t *)data_upload_log, strlen(data_upload_log));
            }else{
                err = ESP_ERR_INVALID_STATE;
            }
			
		    ESP_QCLOUD_FREE(data_upload_log);
		}
        break;
		
        default:
            ESP_LOGI(TAG,"invalid cmd: %s", data);
        break;
	}

EXIT:
    cJSON_Delete(root);
    
	return err;
}

esp_err_t esp_qcloud_prov_ble_report_bind_status(bool token_status)
{
    // ESP_QCLOUD_ERROR_CHECK(g_ble_is_connected, ESP_ERR_INVALID_STATE, "ble not connecte");

	esp_err_t err      = ESP_FAIL;
    char json_str[256] = {0};
	cJSON *reply_data  = cJSON_CreateObject();

	cJSON_AddNumberToObject(reply_data, "status", token_status);
	cJSON_AddStringToObject(reply_data, "productId", esp_qcloud_get_product_id()); 
	cJSON_AddStringToObject(reply_data, "deviceName", esp_qcloud_get_device_name());
    cJSON_PrintPreallocated(reply_data, json_str, sizeof(json_str), 0);
    strcat(json_str, "\r\n");

	err = esp_blufi_send_custom_data((uint8_t *)&json_str,strlen(json_str));
    cJSON_Delete(reply_data);

	return err;
}

#include "driver/gpio.h"

static void ble_event_callback(esp_blufi_cb_event_t event, esp_blufi_cb_param_t *param)
{
    /* actually, should post to blufi_task handle the procedure,
     * now, as a example, we do it more simply */
    switch (event) {
    case ESP_BLUFI_EVENT_INIT_FINISH:
        ESP_LOGI(TAG,"BLUFI init finish");
        esp_ble_gap_set_device_name(g_local_name);
        esp_ble_gap_config_adv_data(&blufi_adv_data);
        break;

    case ESP_BLUFI_EVENT_DEINIT_FINISH:
        ESP_LOGI(TAG,"BLUFI deinit finish");
        break;

    case ESP_BLUFI_EVENT_BLE_CONNECT:
        ESP_LOGW(TAG,"ESP_BLUFI_EVENT_BLE_CONNECT");
        g_ble_is_connected = true;
        esp_ble_conn_update_params_t conn_params = {0};
        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
        conn_params.latency = 0;
        conn_params.min_int = 0x100;    // min_int = 0x10*1.25ms = 20ms
        conn_params.max_int = 0x200;    // max_int = 0x20*1.25ms = 40ms
        conn_params.timeout = 400;     // timeout = 400*10ms = 4000ms
        //start sent the update connection parameters to the peer device.
        esp_ble_gap_update_conn_params(&conn_params);

        esp_blufi_adv_stop();
        blufi_security_init();
        break;

    case ESP_BLUFI_EVENT_BLE_DISCONNECT:
        ESP_LOGI(TAG,"BLUFI ble disconnect");
        g_ble_is_connected = false;
        blufi_security_deinit();
        esp_blufi_adv_start();
        break;

    case ESP_BLUFI_EVENT_SET_WIFI_OPMODE:
        BLUFI_INFO("BLUFI Set WIFI opmode %d\n", param->wifi_mode.op_mode);
        ESP_ERROR_CHECK( esp_wifi_set_mode(param->wifi_mode.op_mode) );
        break;
    case ESP_BLUFI_EVENT_REQ_CONNECT_TO_AP:
        BLUFI_INFO("BLUFI requset wifi connect to AP\n");
        /* there is no wifi callback when the device has already connected to this wifi
        so disconnect wifi before connection.
        */
        esp_wifi_disconnect();
        esp_wifi_start();
        esp_wifi_connect();
        break;
    case ESP_BLUFI_EVENT_REQ_DISCONNECT_FROM_AP:
        BLUFI_INFO("BLUFI requset wifi disconnect from AP\n");
        esp_wifi_disconnect();
        break;
    case ESP_BLUFI_EVENT_REPORT_ERROR:
        BLUFI_ERROR("BLUFI report error, error code %d\n", param->report_error.state);
        esp_blufi_send_error_info(param->report_error.state);
        break;
    case ESP_BLUFI_EVENT_GET_WIFI_STATUS: {
        if (esp_qcloud_prov_wifi_is_connected()) {
            esp_qcloud_prov_ble_post_success();
            BLUFI_INFO("esp_qcloud_prov_ble_post_success\n");
        } else {
            esp_qcloud_prov_ble_post_fail();
            BLUFI_INFO("esp_qcloud_prov_ble_post_fail\n");
        }
        break;
    }
    case ESP_BLUFI_EVENT_RECV_SLAVE_DISCONNECT_BLE:
        BLUFI_INFO("blufi close a gatt connection");
        esp_blufi_disconnect();
        break;
    case ESP_BLUFI_EVENT_DEAUTHENTICATE_STA:
        /* TODO */
        break;
	case ESP_BLUFI_EVENT_RECV_STA_BSSID:
        memcpy(sta_config.sta.bssid, param->sta_bssid.bssid, 6);
        sta_config.sta.bssid_set = true;
        esp_wifi_set_config(WIFI_IF_STA, &sta_config);
        BLUFI_INFO("Recv STA BSSID %s\n", sta_config.sta.ssid);
        break;
	case ESP_BLUFI_EVENT_RECV_STA_SSID:
        strncpy((char *)sta_config.sta.ssid, (char *)param->sta_ssid.ssid, param->sta_ssid.ssid_len);
        sta_config.sta.ssid[param->sta_ssid.ssid_len] = '\0';
        esp_wifi_set_config(WIFI_IF_STA, &sta_config);
        ESP_LOGI(TAG,"Recv STA SSID %s", sta_config.sta.ssid);
        break;
	case ESP_BLUFI_EVENT_RECV_STA_PASSWD:
        strncpy((char *)sta_config.sta.password, (char *)param->sta_passwd.passwd, param->sta_passwd.passwd_len);
        sta_config.sta.password[param->sta_passwd.passwd_len] = '\0';
        esp_wifi_set_config(WIFI_IF_STA, &sta_config);
        ESP_LOGI(TAG,"Recv STA PASSWORD %s", sta_config.sta.password);
        break;
    case ESP_BLUFI_EVENT_GET_WIFI_LIST:{
        wifi_scan_config_t scanConf = {
            .ssid = NULL,
            .bssid = NULL,
            .channel = 0,
            .show_hidden = false
        };
        esp_wifi_scan_start(&scanConf, true);
        ESP_LOGI(TAG,"esp_wifi_scan_start");
        break;
    }
    case ESP_BLUFI_EVENT_RECV_CUSTOM_DATA:
		ESP_LOGI(TAG,"Recv Custom Data %d", param->custom_data.data_len);
        ESP_LOG_BUFFER_HEXDUMP("Custom Data", param->custom_data.data, param->custom_data.data_len, ESP_LOG_INFO);
        esp_qcloud_prov_ble_handle_recv_data((char *)param->custom_data.data, param->custom_data.data_len);
        break;
    default:
        break;
    }
}

static esp_blufi_callbacks_t ble_callbacks = {
    .event_cb = ble_event_callback,
    .negotiate_data_handler = blufi_dh_negotiate_data_handler,
    .encrypt_func = blufi_aes_encrypt,
    .decrypt_func = blufi_aes_decrypt,
    .checksum_func = blufi_crc_checksum,
};

esp_err_t esp_qcloud_prov_bleconfig_start(const char *local_name, uint16_t listen_interval)
{
    esp_err_t ret = ESP_FAIL;
    ESP_QCLOUD_ERROR_CHECK(local_name == NULL, ESP_ERR_INVALID_ARG, "blulocal_namefi is NULL");
    ESP_QCLOUD_ERROR_CHECK(g_blufi_is_started, ESP_ERR_INVALID_STATE, "blufi is started");

    g_blufi_is_started = true;
    g_local_name = strdup(local_name);
    sta_config.sta.listen_interval = listen_interval;
    //sta_config.sta.channel = 13;    //设置起始扫描信道

    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    ESP_QCLOUD_ERROR_CHECK(ret != ESP_OK, ret, "%s initialize bt controller failed: %s", __func__, esp_err_to_name(ret));

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    ESP_QCLOUD_ERROR_CHECK(ret != ESP_OK, ret, "%s initialize bt controller failed: %s", __func__, esp_err_to_name(ret));

    ret = esp_blufi_host_init();
    ESP_QCLOUD_ERROR_CHECK(ret != ESP_OK, ret, "%s init bluedroid failed: %s", __func__, esp_err_to_name(ret));

    BLUFI_INFO("BLUFI VERSION %04x", esp_blufi_get_version());
    ret = esp_ble_gap_register_callback(esp_ble_gap_event_handler);
    ESP_QCLOUD_ERROR_CHECK(ret != ESP_OK, ret, "%s gap register failed, error code = %x", __func__, ret);

    ret = esp_blufi_register_callbacks(&ble_callbacks);
    ESP_QCLOUD_ERROR_CHECK(ret != ESP_OK, ret, "%s gap register failed, error code = %x", __func__, ret);

    ret = esp_blufi_profile_init();
    ESP_QCLOUD_ERROR_CHECK(ret != ESP_OK, ret, "%s profile init, error code = %x", __func__, ret);

	return ret;
}

esp_err_t esp_qcloud_prov_ble_stop(void)
{
    esp_err_t ret;
    ESP_QCLOUD_ERROR_CHECK(!g_blufi_is_started, ESP_ERR_INVALID_STATE, "blufi is stopped");
    g_blufi_is_started = false;
    ESP_QCLOUD_FREE(g_local_name);

    esp_blufi_disconnect();

    esp_blufi_adv_stop();

    ret = esp_blufi_profile_deinit();
    ESP_QCLOUD_ERROR_CHECK(ret != ESP_OK, ret, "profile deinit failed, error code = %x", ret);
	
    ret = esp_bluedroid_disable();
    ESP_QCLOUD_ERROR_CHECK(ret != ESP_OK, ret, "bluedroid disable failed, error code = %x", ret);

    ret = esp_bluedroid_deinit();
    ESP_QCLOUD_ERROR_CHECK(ret != ESP_OK, ret, "bluedroid deinit failed, error code = %x", ret);

    ret = esp_bt_controller_disable();
    ESP_QCLOUD_ERROR_CHECK(ret != ESP_OK, ret, "controller disable failed, error code = %x", ret);

    ret = esp_bt_controller_deinit();
    ESP_QCLOUD_ERROR_CHECK(ret != ESP_OK, ret, "controller deinit failed, error code = %x", ret);

    esp_bt_controller_mem_release(ESP_BT_MODE_BTDM);

    ESP_LOGI(TAG, "esp_qcloud_prov_ble_stop called successfully");

    return ret;
}

esp_err_t esp_qcloud_prov_ble_post_success(void)
{
    // ESP_QCLOUD_ERROR_CHECK(g_ble_is_connected, ESP_ERR_INVALID_STATE, "ble not connecte");

    wifi_mode_t mode;
    esp_blufi_extra_info_t info = { 0 };
    wifi_config_t sta_cfg   = { 0 };

    esp_wifi_get_mode(&mode);
    esp_wifi_get_config(ESP_IF_WIFI_STA, &sta_cfg);

    info.sta_bssid_set = false;
    memcpy(info.sta_bssid, sta_cfg.sta.bssid, 6);
    info.sta_ssid_len = strlen((char*)(sta_cfg.sta.ssid));
    info.sta_ssid = sta_cfg.sta.ssid;
    return esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONN_SUCCESS, 0, &info);
}

esp_err_t esp_qcloud_prov_ble_post_fail(void)
{
    ESP_QCLOUD_ERROR_CHECK(g_ble_is_connected, ESP_ERR_INVALID_STATE, "ble not connecte");
    return esp_blufi_send_wifi_conn_report(WIFI_MODE_STA, ESP_BLUFI_STA_CONN_FAIL, 0, NULL);
}

#endif
