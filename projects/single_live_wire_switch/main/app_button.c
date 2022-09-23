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
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <esp_rmaker_utils.h>
#include "app_priv.h"

static const char *TAG = "app_button";

/*button gpios*/
#define BUTTON_GPIO_CHANNEL_1 (GPIO_NUM_8)
#define BUTTON_GPIO_CHANNEL_2 (GPIO_NUM_3)
#define BUTTON_GPIO_CHANNEL_3 (GPIO_NUM_4)

#define BUTTON_RESET_TIMEOUT (4 * 1000 * 1000) 

typedef struct {
	int chan;
	int level;
} button_event;

typedef struct {
	int chan;
	gpio_num_t gpio_num;
	gpio_int_type_t gpio_intr;
	void *data;
	int64_t press_time_start;
} app_button_t;

typedef struct {
	TaskHandle_t       task_handle;
	QueueHandle_t      queue_handle;
	app_button_t       buttons[SWITCH_CHANNEL_MAX];
} app_button_impl_t;

static app_button_impl_t button_impl;


/**************************buttons********************************/
void IRAM_ATTR app_button_isr(void *arg)
{
	button_event ev;
	ev.chan = (int)arg;// button channel
	ev.level = gpio_get_level(button_impl.buttons[ev.chan].gpio_num);
	app_button_t *btn = &(button_impl.buttons[ev.chan]);
	if (btn->gpio_intr == GPIO_INTR_LOW_LEVEL) {
		btn->gpio_intr = GPIO_INTR_HIGH_LEVEL;
	} else if (btn->gpio_intr == GPIO_INTR_HIGH_LEVEL) {
		btn->gpio_intr = GPIO_INTR_LOW_LEVEL;
	}
	gpio_set_intr_type(btn->gpio_num, btn->gpio_intr);
	gpio_wakeup_enable(btn->gpio_num, btn->gpio_intr);
	portBASE_TYPE pxHigherPriorityTaskWoken = pdFALSE;
	if (button_impl.queue_handle) {
		xQueueSendFromISR(button_impl.queue_handle, &ev, pxHigherPriorityTaskWoken);
	}
}

/*buttons init*/
static int _app_button_init(app_button_t *btn)
{
	gpio_config_t io_conf;
    io_conf.intr_type = btn->gpio_intr;
    io_conf.mode      = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << btn->gpio_num);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = true;
    ESP_ERROR_CHECK (gpio_config(&io_conf));
	return ESP_OK;
}

static void app_button_task(void *arg)
{
	button_event event;
	app_button_t *btn;
	while (xQueueReceive(button_impl.queue_handle,&event,portMAX_DELAY) == pdTRUE) {
		//ESP_LOGI(TAG,"chan %d", chan);
		int chan = event.chan;
		int level = event.level;
		btn = &(button_impl.buttons[chan]);

		if (level){
			int64_t now = esp_timer_get_time();
			//ESP_LOGI(TAG,"--level %d , time: %d now: %d diff: %d",level, btn->press_time_start, now,now - btn->press_time_start);
			if (now - btn->press_time_start < BUTTON_RESET_TIMEOUT){
				int onoff = app_relay_get_state(chan);
				onoff = (onoff == SWITCH_STATE_OFF) ? SWITCH_STATE_ON : SWITCH_STATE_OFF;
				ESP_LOGI(TAG,"==>set relay %d to %d",chan,onoff);
				app_relay_set_state(chan, onoff);
			} else {
				// doing reset
				ESP_LOGI(TAG,"==>reset");
				esp_rmaker_factory_reset(0, 2);
			}
		} else {
			//ESP_LOGI(TAG,"++level %d , time: %d",level, btn->press_time_start);
			btn->press_time_start = esp_timer_get_time();
		}
	}
}

int app_button_init(void)
{
	button_impl.queue_handle = xQueueCreate(2, sizeof(button_event));
	if (xTaskCreate(app_button_task, "btn_task",
                    1024 * 4, NULL, 3, &button_impl.task_handle) != pdPASS) {
        return ESP_FAIL;
    }

	/*3 buttons in total*/
	app_button_t *btn;
	/*button 1*/
	btn = &(button_impl.buttons[SWITCH_CHANNEL_1]);
	btn->chan         = SWITCH_CHANNEL_1;
	btn->gpio_num     = BUTTON_GPIO_CHANNEL_1;
	btn->gpio_intr    = GPIO_INTR_LOW_LEVEL;
	btn->data         = (void *)SWITCH_CHANNEL_1;
	_app_button_init(btn);

	gpio_install_isr_service(0);
	gpio_isr_handler_add(btn->gpio_num, app_button_isr, (void *)btn->data);
	gpio_wakeup_enable(btn->gpio_num, btn->gpio_intr);

	// /*button 2*/
	// btn = &(button_impl.buttons[SWITCH_CHANNEL_2]);
	// btn->chan         = SWITCH_CHANNEL_2;
	// btn->gpio_num     = BUTTON_GPIO_CHANNEL_2;
	// btn->gpio_intr    = GPIO_INTR_LOW_LEVEL;
	// btn->data         = (void *)SWITCH_CHANNEL_2;
	// _app_button_init(btn);
	// gpio_isr_handler_add(btn->gpio_num, app_button_isr, (void *)btn->data);
	// gpio_wakeup_enable(btn->gpio_num, btn->gpio_intr);

	// /*button 3*/
	// btn = &(button_impl.buttons[SWITCH_CHANNEL_3]);
	// btn->chan         = SWITCH_CHANNEL_3;
	// btn->gpio_num     = BUTTON_GPIO_CHANNEL_3;
	// btn->gpio_intr    = GPIO_INTR_LOW_LEVEL;
	// btn->data         = (void *)SWITCH_CHANNEL_3;
	// _app_button_init(btn);
	// gpio_isr_handler_add(btn->gpio_num, app_button_isr, (void *)btn->data);
	// gpio_wakeup_enable(btn->gpio_num, btn->gpio_intr);
	
	esp_sleep_enable_gpio_wakeup();
	return ESP_OK;
}

