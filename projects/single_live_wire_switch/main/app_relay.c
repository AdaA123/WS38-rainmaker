/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/* It is recommended to copy this code in your example so that you can modify as
 * per your application's needs, especially for the indicator calbacks,
 * wifi_reset_indicate() and factory_reset_indicate().
 */
#include "nvs.h"
#include "nvs_flash.h"

#include "app_priv.h"
esp_err_t app_relay_state_nvs_set(int chan, int32_t state);
esp_err_t app_relay_state_nvs_get(int chan, int32_t *state);


static const char *TAG = "app_relay";

#define NVS_NAMESPACE_APP_RELAY_STATE "app_cfg"
#define NVS_APP_RELAY_STATE_KEY_FMT   "relay.%d"

#define NVS_APP_DEFAULT_RELAY_STATE  (1)
/*relay gpios*/
#define RELAY_GPIO_CHANNEL_1 (GPIO_NUM_5)
#define RELAY_GPIO_CHANNEL_2 (GPIO_NUM_6)
#define RELAY_GPIO_CHANNEL_3 (GPIO_NUM_7)

typedef struct {
	int chan;
	const char *name;
	gpio_num_t gpio_num_on;
	gpio_num_t gpio_num_off;
	bool active_level;
	int onoff;
} app_relay_t;
static app_relay_t relays[SWITCH_CHANNEL_MAX];

static int _app_relay_init(app_relay_t *relay)
{
	gpio_config_t io_conf;

	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask = (1ULL << (relay->gpio_num_on)) |  (1ULL << (relay->gpio_num_off));
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 0;
	gpio_config(&io_conf);	

	//init state
	return app_relay_set_state(relay->chan,relay->onoff);
}

int app_relay_init(void)
{
	app_relay_t *relay;
	int32_t state;
	/*relay 1*/
	app_relay_state_nvs_get(SWITCH_CHANNEL_1,&state);
	relay = &(relays[SWITCH_CHANNEL_1]);
	relay->chan = SWITCH_CHANNEL_1;
	relay->name = "Power ";
	relay->gpio_num_on = RELAY_GPIO_CHANNEL_1;
	relay->gpio_num_off = RELAY_GPIO_CHANNEL_2;
	relay->active_level = 1;//high level
	relay->onoff = state; //default to off , or you can save previous state in nvs
	_app_relay_init(relay);
	ESP_LOGI(TAG,"relay %d init to %d", relay->chan, relay->onoff);

	return ESP_OK;
}

static int _app_relay_set_state(int chan, int onoff)
{
	// TODO -- SWITCH_CHANNEL_ALL support
	if (chan < SWITCH_CHANNEL_1 || chan > SWITCH_CHANNEL_MAX) {
		ESP_LOGE(TAG,"invalid chan %d", chan);
		return ESP_FAIL;
	}
	
	app_relay_t *relay = &(relays[SWITCH_CHANNEL_1]);
	gpio_num_t relay_gpio_num = onoff ? relay->gpio_num_on : relay->gpio_num_off ;
	
	gpio_hold_dis(relay_gpio_num);
	gpio_set_level(relay_gpio_num, 1);
	gpio_hold_en(relay_gpio_num);
	vTaskDelay(7/portTICK_PERIOD_MS);
	gpio_hold_dis(relay_gpio_num);
	gpio_set_level(relay_gpio_num, 0);
	gpio_hold_en(relay_gpio_num);
	
	//update state
	relay->onoff = onoff;
	app_relay_state_nvs_set(SWITCH_CHANNEL_1, onoff);
	app_rainmaker_update_relay_state(SWITCH_CHANNEL_1, onoff);
	return ESP_OK;
}

static int _app_relay_get_state(int chan)
{
	if (chan < SWITCH_CHANNEL_1 || chan >= SWITCH_CHANNEL_MAX) {
		ESP_LOGE(TAG,"invalid chan %d", chan);
		return SWITCH_STATE_NA;
	}
	
	app_relay_t *relay = &(relays[chan]);
	return relay->onoff;
}

int app_relay_set_state(int chan, int onoff)
{
	if (chan == SWITCH_CHANNEL_ALL) {
		for (int i=SWITCH_CHANNEL_1 ; i < SWITCH_CHANNEL_MAX; i++) {
			_app_relay_set_state(i, onoff);
		}
	} else {
		_app_relay_set_state(chan, onoff);
	}
	return ESP_OK;
}

int app_relay_get_state(int chan)
{
	int state;
	if (chan == SWITCH_CHANNEL_ALL) {
		int onoff = 0;
		for (int i=SWITCH_CHANNEL_1 ; i < SWITCH_CHANNEL_MAX; i++) {
			if (_app_relay_get_state(i)) {
				onoff++;
			}
		}

		state = (onoff == 0) ? false : true;
	} else {
		state = _app_relay_get_state(chan);
	}
	return state;
}

int app_relay_get_name(int chan)
{
	if (chan < SWITCH_CHANNEL_1 || chan >= SWITCH_CHANNEL_MAX) {
		ESP_LOGE(TAG,"invalid chan %d", chan);
		return SWITCH_STATE_NA;
	}
	
	app_relay_t *relay = &(relays[chan]);
	return relay->name;
}

esp_err_t app_relay_state_nvs_set(int chan, int32_t state)
{
	esp_err_t ret;
	nvs_handle handle;
	char key[12];	

	ret = nvs_open(NVS_NAMESPACE_APP_RELAY_STATE, NVS_READWRITE, &handle);
	snprintf(key,sizeof(key),NVS_APP_RELAY_STATE_KEY_FMT,chan);
	ret = nvs_set_i32(handle, key, state);
	nvs_commit(handle);
	nvs_close(handle);
	return ESP_OK;
}

esp_err_t app_relay_state_nvs_get(int chan, int32_t *state)
{
	esp_err_t ret;
	nvs_handle handle;
	char key[12];	

	ret = nvs_open(NVS_NAMESPACE_APP_RELAY_STATE, NVS_READWRITE, &handle);
	snprintf(key,sizeof(key),NVS_APP_RELAY_STATE_KEY_FMT,chan);
	ret = nvs_get_i32(handle, key, state);
	nvs_close(handle);

	if (ret == ESP_ERR_NVS_NOT_FOUND) {
		ESP_LOGI(TAG,"relay state set to default %d", NVS_APP_DEFAULT_RELAY_STATE);
		app_relay_state_nvs_set(chan, NVS_APP_DEFAULT_RELAY_STATE);
		*state = NVS_APP_DEFAULT_RELAY_STATE;
	}
	return ESP_OK;
}

