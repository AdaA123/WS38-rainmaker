/* Switch Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_event.h>

#include <esp_rmaker_core.h>
#include <esp_rmaker_mqtt.h>
#include <esp_rmaker_standard_types.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_devices.h>
#include <esp_rmaker_ota.h>
#include <esp_rmaker_schedule.h>
#include <esp_rmaker_console.h>

#include <esp_rmaker_common_events.h>
//#include <app_insights.h>
#include <esp_timer.h>
#include <esp_sntp.h>

#include "app_priv.h"

#define APP_RAINMAKER_MQTT_RECONNECT_TIMEOUT (60)  //seconds
static const char *TAG = "app_rmaker";
static esp_rmaker_device_t* switch_device;
static esp_timer_handle_t rmaker_reconnect_timer;

/* Callback to handle commands received from the RainMaker cloud */
static esp_err_t write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
                                                const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    if (ctx) {
        ESP_LOGI(TAG, "Received write request via : %s", esp_rmaker_device_cb_src_to_str(ctx->src));
    }

    const char *device_name = esp_rmaker_device_get_name(device);

    const char *param_name = esp_rmaker_param_get_name(param);
    if (strcmp(param_name, ESP_RMAKER_DEF_POWER_NAME) == 0) {
        ESP_LOGI(TAG, "Received value = %s for %s - %s", val.val.b? "true" : "false", device_name, param_name);
            if (val.val.b)
            {
                Open_The_Lights();
            }
            else
            {
                Close_The_Lights();
            }
         //app_light_set_power(val.val.b);
    } else if (strcmp(param_name, ESP_RMAKER_DEF_BRIGHTNESS_NAME) == 0) {
        ESP_LOGI(TAG, "Received value = %d for %s - %s",
                val.val.i, device_name, param_name);
            ESP_LOGI(TAG, "Set_Btight_Pct");
            if(Get_Bri_Status())
            {
                Set_Btight_Pct( val.val.i);//app_light_set_brightness(val.val.i);
            }
            else
            {
                esp_rmaker_param_update_and_report(  
                            esp_rmaker_device_get_param_by_name(switch_device, ESP_RMAKER_DEF_BRIGHTNESS_NAME),  
                            esp_rmaker_int(Get_Btight_Pct())
                            );
            }
    } else {
        /* Silently ignoring invalid params */
        esp_rmaker_param_update_and_report(param, val);
        return ESP_OK;
    }
    return ESP_OK;
}

void esp_rmaker_update(uint8_t cmd_tyed, uint32_t dat)
{
    switch (cmd_tyed)
    {
    case OnOffCMD:
        {
            esp_rmaker_param_update_and_report(  
                            esp_rmaker_device_get_param_by_name(switch_device, ESP_RMAKER_DEF_POWER_NAME),  
                            esp_rmaker_bool(dat)
                            );
        }
        /* code */
        break;
    
    case BriNowCMD:         
        {
            esp_rmaker_param_update_and_report(  
                            esp_rmaker_device_get_param_by_name(switch_device, ESP_RMAKER_DEF_BRIGHTNESS_NAME),  
                            esp_rmaker_int(dat)
                            );
        }
        break;

    default:
        break;
    }

    return;
}


esp_err_t app_rainmaker_update_relay_state(int chan, int state)
{
	// if (switch_device == NULL) return ESP_FAIL;
	// esp_rmaker_param_t *param = esp_rmaker_device_get_param_by_name(switch_device,app_relay_get_name(chan));
	// if (param == NULL) return ESP_FAIL;
	// esp_rmaker_param_val_t val = esp_rmaker_bool(chan);
    // return esp_rmaker_param_update_and_notify(param, val);
    return -1;
}

static void rmaker_reconnect_timer_callback(void* arg)
{
	if (esp_rmaker_mqtt_is_connected() || !wifi_is_connected())
		return;
	ESP_LOGI(TAG, "rainmaker reconnecting");
	esp_rmaker_mqtt_reconnect();
}

void app_rainmaker_reconnect_timer_start(void)
{
	if (rmaker_reconnect_timer == NULL) {
		const esp_timer_create_args_t rmaker_reconnect_timer_args = {
            .callback = &rmaker_reconnect_timer_callback,
            .name = "rmaker_reconnect"
    	};
		esp_timer_create(&rmaker_reconnect_timer_args, &rmaker_reconnect_timer);
	}
	ESP_LOGI(TAG, "rainmaker reconnect in %d secs",APP_RAINMAKER_MQTT_RECONNECT_TIMEOUT);
	esp_timer_start_once(rmaker_reconnect_timer, APP_RAINMAKER_MQTT_RECONNECT_TIMEOUT * 1000 * 1000);
}

void app_rainmaker_reconnect_timer_stop(void)
{
	if (rmaker_reconnect_timer) {
		ESP_LOGI(TAG, "rainmaker reconnect timer canceled");
		esp_timer_stop(rmaker_reconnect_timer);
	}
}


/* Event handler for catching RainMaker events */
static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == RMAKER_EVENT) {
        switch (event_id) {
            case RMAKER_EVENT_INIT_DONE:
                ESP_LOGI(TAG, "RainMaker Initialised.");
                break;
            case RMAKER_EVENT_CLAIM_STARTED:
                ESP_LOGI(TAG, "RainMaker Claim Started.");
                break;
            case RMAKER_EVENT_CLAIM_SUCCESSFUL:
                ESP_LOGI(TAG, "RainMaker Claim Successful.");
                break;
            case RMAKER_EVENT_CLAIM_FAILED:
                ESP_LOGI(TAG, "RainMaker Claim Failed.");
                break;
			case RMAKER_EVENT_USER_NODE_MAPPING_DONE:
				ESP_LOGI(TAG, "RainMaker User Mapping Done");
				break;
            default:
                ESP_LOGW(TAG, "Unhandled RainMaker Event: %d", event_id);
        }
    } else if (event_base == RMAKER_COMMON_EVENT) {
        switch (event_id) {
            case RMAKER_EVENT_REBOOT:
                ESP_LOGI(TAG, "Rebooting in %d seconds.", *((uint8_t *)event_data));
                break;
            case RMAKER_EVENT_WIFI_RESET:
                ESP_LOGI(TAG, "Wi-Fi credentials reset.");
                break;
            case RMAKER_EVENT_FACTORY_RESET:
                ESP_LOGI(TAG, "Node reset to factory defaults.");
                break;
            case RMAKER_MQTT_EVENT_CONNECTED:
                ESP_LOGI(TAG, "MQTT Connected.");
				#if CONFIG_ESP_RMAKER_MQTT_DISABLE_AUTO_RECONNECT
				if (sntp_enabled()) {
					sntp_init();
				}
				#endif
                break;
            case RMAKER_MQTT_EVENT_DISCONNECTED:
                ESP_LOGI(TAG, "MQTT Disconnected.");
				#if CONFIG_ESP_RMAKER_MQTT_DISABLE_AUTO_RECONNECT
				if (wifi_is_connected()) {
					sntp_stop();
					app_rainmaker_reconnect_timer_start();
				}
				#endif
                break;
            case RMAKER_MQTT_EVENT_PUBLISHED:
                ESP_LOGI(TAG, "MQTT Published. Msg id: %d.", *((int *)event_data));
                break;
            default:
                ESP_LOGW(TAG, "Unhandled RainMaker Common Event: %d", event_id);
        }
    } else {
        ESP_LOGW(TAG, "Invalid event received!");
    }
}

int app_rainmaker_init(void)
{
    #if 0  //console acts abnormal when enter light sleep, chentao2@espressif.com
	esp_rmaker_console_init();
    #endif
	/* Register an event handler to catch RainMaker events */
    ESP_ERROR_CHECK(esp_event_handler_register(RMAKER_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(RMAKER_COMMON_EVENT, RMAKER_MQTT_EVENT_DISCONNECTED, &event_handler, NULL));
	
	/* Initialize the ESP RainMaker Agent.
     * Note that this should be called after app_wifi_init() but before app_wifi_start()
     * */
    esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = false,
    };
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, "ESP RainMaker Device", "Lightbulb");
    if (!node) {
        ESP_LOGE(TAG, "Could not initialise node. Aborting!!!");
        vTaskDelay(5000/portTICK_PERIOD_MS);
        abort();
    }

    /* Create a device and add the relevant parameters to it */
    switch_device = esp_rmaker_lightbulb_device_create("Light", NULL, 0);
    esp_rmaker_device_add_cb(switch_device, write_cb, NULL);

    esp_rmaker_device_add_param(switch_device, esp_rmaker_brightness_param_create(ESP_RMAKER_DEF_BRIGHTNESS_NAME, 25));
    //esp_rmaker_device_add_param(light_device, esp_rmaker_hue_param_create(ESP_RMAKER_DEF_HUE_NAME, DEFAULT_HUE));
    //esp_rmaker_device_add_param(light_device, esp_rmaker_saturation_param_create(ESP_RMAKER_DEF_SATURATION_NAME, DEFAULT_SATURATION));

    esp_rmaker_node_add_device(node, switch_device);
	
    /* Enable OTA */
    esp_rmaker_ota_config_t ota_config = {
        .server_cert = ESP_RMAKER_OTA_DEFAULT_SERVER_CERT,
    };
    esp_rmaker_ota_enable(&ota_config, OTA_USING_PARAMS);

    /* Enable timezone service which will be require for setting appropriate timezone
     * from the phone apps for scheduling to work correctly.
     * For more information on the various ways of setting timezone, please check
     * https://rainmaker.espressif.com/docs/time-service.html.
     */
    esp_rmaker_timezone_service_enable();

    /* Enable scheduling. */
    esp_rmaker_schedule_enable();

    /* Enable Insights. Requires CONFIG_ESP_INSIGHTS_ENABLED=y */
    //app_insights_enable();

    /* Start the ESP RainMaker Agent */
    return esp_rmaker_start();
}
