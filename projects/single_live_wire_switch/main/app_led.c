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

#include "app_priv.h"

static const char *TAG = "app_led";


/*led gpio*/
#define LED_GPIO_PROVISION   (GPIO_NUM_10)
#define LED_BLINK_PERIOD     (1000 * 1000) // 1s

typedef struct {
	int chan;
	gpio_num_t gpio_num;
	bool active_level;
	int  state;
	esp_timer_handle_t led_timer;
} app_led_t;

static app_led_t leds[1];

static void app_led_timer_callback(void* arg)
{
	app_led_t *led = arg;
	gpio_hold_dis(led->gpio_num);
	gpio_set_level(led->gpio_num, gpio_get_level(led->gpio_num) ? false : true);
	gpio_hold_en(led->gpio_num);
}

static int _app_led_init(app_led_t *led)
{
	gpio_config_t io_conf;

	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_INPUT_OUTPUT;
	io_conf.pin_bit_mask = (1ULL << (led->gpio_num));
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 0;
	gpio_config(&io_conf);	

	//init state
	return app_led_set_state(led->state);
}

int app_led_init(void)
{
	//only one led
	app_led_t *led = &leds[0];
	led->chan = 0;
	led->gpio_num = LED_GPIO_PROVISION;
	led->active_level = 1;//high level
	//led timer init
	const esp_timer_create_args_t led_timer_args = {
		.callback = &app_led_timer_callback,
		.arg = led,
		.name = "device_led",
	};
	esp_timer_create(&led_timer_args, &(led->led_timer));
	_app_led_init(led);
	return ESP_OK;
}

int app_led_set_state(int state)
{
	if (state < LED_STATE_OFF || state >= LED_STATE_NA) {
		ESP_LOGE(TAG,"invalid state %d", state);
		return ESP_FAIL;
	}
	
	app_led_t *led = &leds[0];
	if (led->state == LED_STATE_BLINK) {
		esp_timer_stop(led->led_timer);
	}
	
	switch (state) {
		case LED_STATE_OFF:
			gpio_hold_dis(led->gpio_num);
			gpio_set_level(led->gpio_num, !(led->active_level));
			gpio_hold_en(led->gpio_num);
			break;
		case LED_STATE_ON:
			gpio_hold_dis(led->gpio_num);
			gpio_set_level(led->gpio_num, (led->active_level));
			gpio_hold_en(led->gpio_num);
			break;
		case LED_STATE_BLINK:
			ESP_LOGI(TAG,"led blink start");
			esp_timer_start_periodic(led->led_timer, LED_BLINK_PERIOD);
			break;
	}
	//update state
	led->state = state;
	return ESP_OK;
}

int app_led_get_state(void)
{
	app_led_t *led = &leds[0];
	return led->state;
}



void provisioning_indicator_start(void)
{
    app_led_set_state(LED_STATE_BLINK);
    return;
}

void provisioning_indicator_stop(void)
{
    app_led_set_state(LED_STATE_OFF);
    return;
}

