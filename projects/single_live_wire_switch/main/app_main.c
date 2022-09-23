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
#include <nvs_flash.h>

#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_types.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_devices.h>
#include <esp_rmaker_ota.h>
#include <esp_rmaker_schedule.h>
#include <esp_rmaker_console.h>

#include <esp_rmaker_common_events.h>

#include <app_wifi.h>
//#include <app_insights.h>

#include "app_priv.h"

#include <esp_pm.h>

static const char *TAG = "app_main";

int app_driver_init(void)
{
	app_led_init();
	app_relay_init();
	app_button_init();
	return ESP_OK;
}

int app_pm_config(void)
{
#if CONFIG_PM_ENABLE
   // Configure dynamic frequency scaling:
   // maximum and minimum frequencies are set in sdkconfig,
   // automatic light sleep is enabled if tickless idle support is enabled.
#if CONFIG_IDF_TARGET_ESP32
   esp_pm_config_esp32_t pm_config = {
#elif CONFIG_IDF_TARGET_ESP32S2
   esp_pm_config_esp32s2_t pm_config = {
#elif CONFIG_IDF_TARGET_ESP32C3
   esp_pm_config_esp32c3_t pm_config = {
#endif
           .max_freq_mhz = CONFIG_EXAMPLE_MAX_CPU_FREQ_MHZ,
           .min_freq_mhz = CONFIG_EXAMPLE_MIN_CPU_FREQ_MHZ,
#if CONFIG_FREERTOS_USE_TICKLESS_IDLE
           .light_sleep_enable = true
#endif
   };

   ESP_ERROR_CHECK (esp_pm_configure(&pm_config));
   #endif // CONFIG_PM_ENABLE
   return ESP_OK;
}

#define swap_16(x) ((uint16_t)((((x)&0xff00) >> 8) | (((x)&0x00ff) << 8)))
static esp_err_t fill_mfg_info(uint8_t* mfg)
{
	uint8_t _mfg[12] = { 0xe5, 0x02 };
    const char* app_id = "Nova";
    sscanf(app_id, "%s", &_mfg[2]);

    uint16_t manufacturer_id = swap_16(1);
    memcpy(&_mfg[6], &manufacturer_id, 2);

    uint16_t device_code = swap_16(128);
    memcpy(&_mfg[8], &device_code, 2);

    uint8_t device_subtype_code = 1;
    memcpy(&_mfg[10], &device_subtype_code, 1);

    uint8_t device_extra_code = 1;
    memcpy(&_mfg[11], &device_extra_code, 1);
	
    ESP_LOG_BUFFER_HEXDUMP(TAG, _mfg, 12, ESP_LOG_INFO);
    memcpy(mfg, _mfg, sizeof(_mfg));
	return ESP_OK;
}


void app_main()
{
    /* Initialize NVS. */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
	app_pm_config();
	app_driver_init();
    /* Initialize Wi-Fi. Note that, this should be called before esp_rmaker_node_init()
     */
    app_wifi_init();
	app_rainmaker_init();
	/* Start the Wi-Fi.
     * If the node is provisioned, it will start connection attempts,
     * else, it will start Wi-Fi provisioning. The function will return
     * after a connection has been successfully established
     */
    app_wifi_prov_config_t pro_config = { 0 };
    fill_mfg_info(pro_config.mfg);
    pro_config.broadcast_prefix = "Nova";
    pro_config.prov_timeout = 2;  //配网等待时间
	pro_config.enable_prov = 1;
    
    /* Start the Wi-Fi.
     * If the node is provisioned, it will start connection attempts,
     * else, it will start Wi-Fi provisioning. The function will return
     * after a connection has been successfully established
     */
    err = app_wifi_start(&pro_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Could not start Wifi. Aborting!!!");
        vTaskDelay(5000/portTICK_PERIOD_MS);
        abort();
    }
}
