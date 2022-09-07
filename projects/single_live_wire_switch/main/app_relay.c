#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_sleep.h"
#include "driver/gpio.h"

#include <esp_rmaker_utils.h>
//#include  "app_reset.h"

#include "app_priv.h"

#define RELAY_TIMER_MS 7

static esp_timer_handle_t device_relay_timer = NULL;

uint64_t relay_mask = RELAY_ON;

bool app_get_relay_status(void)
{
    printf("app_relay_onoff status: %s\n", (relay_mask == RELAY_ON) ? "true" : "false");
   return (relay_mask == RELAY_ON) ? true : false;
}

void app_relay_toggle(void)
{
    relay_mask = (relay_mask == RELAY_ON) ? RELAY_OFF : RELAY_ON;

    gpio_hold_dis(relay_mask);
    gpio_set_level(relay_mask, 1);
    gpio_hold_en(relay_mask);
    esp_timer_start_once(device_relay_timer, RELAY_TIMER_MS*1000);
}

void app_relay_onoff(bool relay_onoff)
{
    relay_mask = (relay_onoff == true) ? RELAY_ON : RELAY_OFF;

    gpio_hold_dis(relay_mask);
    gpio_set_level(relay_mask, 1);
    gpio_hold_en(relay_mask);
    esp_timer_start_once(device_relay_timer, RELAY_TIMER_MS*1000);

    printf("wifi app_relay_onoff status: %s\n", (relay_mask == RELAY_ON) ? "true" : "false");
}

void device_relay_timer_callback(void *args)
{
    gpio_hold_dis(relay_mask);
    gpio_set_level(relay_mask, 0);
    gpio_hold_en(relay_mask);

    esp_rmaker_update((relay_mask == RELAY_ON) ? true : false);

    if(relay_mask == RELAY_ON)
    {
        app_ccharging_en();
    }
    else{
        app_ccharging_dis();
    }
}

void app_relay_init(void)
{

    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << RELAY_ON | 1ULL << RELAY_OFF);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    gpio_hold_dis(RELAY_ON);
    gpio_set_level(RELAY_ON, 0);
    gpio_hold_en(RELAY_ON);

    gpio_hold_dis(RELAY_OFF);
    gpio_set_level(RELAY_OFF, 0);
    gpio_hold_en(RELAY_OFF);

    const esp_timer_create_args_t device_relay_timer_args = {
            .callback = &device_relay_timer_callback,
            .name = "device_relay"
    };
    esp_timer_create(&device_relay_timer_args, &device_relay_timer);

    

    return;
}

