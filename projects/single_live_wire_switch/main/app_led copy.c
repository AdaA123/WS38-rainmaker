
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "app_led.h"

static esp_timer_handle_t device_led_timer = NULL;

void single_fire_led_on()
{
    gpio_hold_dis(PROVISION_LED);
    gpio_set_level(PROVISION_LED, 0);
    gpio_hold_en(PROVISION_LED);
}

void single_fire_led_off()
{
    gpio_hold_dis(PROVISION_LED);
    gpio_set_level(PROVISION_LED, 1);
    gpio_hold_en(PROVISION_LED);
}

static void device_led_timer_callback(void* arg)
{
    gpio_hold_dis(PROVISION_LED);
    gpio_set_level(PROVISION_LED, gpio_get_level(PROVISION_LED) ? false : true);
    gpio_hold_en(PROVISION_LED);
}

void single_fire_led_blink_on()
{
    esp_timer_start_periodic(device_led_timer, 1000 * 1000);  
}

void single_fire_led_blink_off()            
{
    esp_timer_stop(device_led_timer);
    single_fire_led_off();
}

void app_led_init()
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << PROVISION_LED);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    const esp_timer_create_args_t device_led_timer_args = {
            .callback = &device_led_timer_callback,
            .name = "device_led"
    };
    esp_timer_create(&device_led_timer_args, &device_led_timer);

    gpio_hold_dis(PROVISION_LED);
    gpio_set_level(PROVISION_LED, 1);
    gpio_hold_en(PROVISION_LED);

    return;
}

