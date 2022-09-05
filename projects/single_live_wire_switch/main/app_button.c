#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_sleep.h"

#include <esp_rmaker_utils.h>
//#include  "app_reset.h"

#include "app_priv.h"

#define BUTTON_DELAY_MIN_CONFIG  50000
#define BUTTON_DELAY_MAX_CONFIG  3000000
#define BUTTON_DELAY_CONFIG    2000
#define QCLOUD_NETWORK_CONFIG  5000000

static esp_timer_handle_t button1_timer = NULL;

static gpio_int_type_t button1_intr_type = GPIO_INTR_HIGH_LEVEL;

void IRAM_ATTR single_fire_button1_isr(void *arg)
{
    static int64_t button_delay_time = -BUTTON_DELAY_MAX_CONFIG;
    if(button1_intr_type == GPIO_INTR_LOW_LEVEL){
        button1_intr_type = GPIO_INTR_HIGH_LEVEL;
        gpio_set_intr_type(BUTTON_1, GPIO_INTR_HIGH_LEVEL);
        esp_timer_start_once(button1_timer, QCLOUD_NETWORK_CONFIG);
        button_delay_time = esp_timer_get_time();
    }else if (button1_intr_type == GPIO_INTR_HIGH_LEVEL) {
        esp_timer_stop(button1_timer);
        if(esp_timer_get_time() > button_delay_time + BUTTON_DELAY_MIN_CONFIG && 
               esp_timer_get_time() < button_delay_time + BUTTON_DELAY_MAX_CONFIG ){
            app_relay_toggle(); //single_fire_set_leval(RELAY_1, SINGLE_FIRE_NEGATE);
        }
        button_delay_time = esp_timer_get_time();
        button1_intr_type = GPIO_INTR_LOW_LEVEL;
        gpio_set_intr_type(BUTTON_1, GPIO_INTR_LOW_LEVEL);
    }else {
        button1_intr_type = GPIO_INTR_HIGH_LEVEL;
        gpio_set_intr_type(BUTTON_1, GPIO_INTR_HIGH_LEVEL);
    }
}

esp_err_t single_fire_button_config(gpio_num_t gpio_num)
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << gpio_num);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    return gpio_config(&io_conf);
}

static void button_long_timer_callback(void* arg)
{
    printf("\n\n-------------------reset!!!!------------------\n\n\n");
	single_fire_led_blink_on();
    esp_rmaker_factory_reset(0, REBOOT_DELAY);
}

esp_err_t app_button_init()
{
    esp_err_t ret = ESP_FAIL;

    const esp_timer_create_args_t button_long_timer_args = {
            .callback = &button_long_timer_callback,
            .name = "button_long"
    };
    esp_timer_create(&button_long_timer_args, &button1_timer);

    ret = single_fire_button_config(BUTTON_1);

    if(gpio_get_level(BUTTON_1) == 0){
        button1_intr_type = GPIO_INTR_NEGEDGE;
    }
    

    ret = gpio_hold_en(BUTTON_1);

    ret = gpio_install_isr_service(0);

    ret = gpio_isr_handler_add(BUTTON_1, single_fire_button1_isr, (void *)BUTTON_1);

    ret = gpio_wakeup_enable(BUTTON_1, GPIO_INTR_LOW_LEVEL);

    ret = esp_sleep_enable_gpio_wakeup();

    return ret;
}