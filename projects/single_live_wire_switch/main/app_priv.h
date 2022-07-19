/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_timer.h>
#include <esp_sleep.h>
#include "driver/gpio.h"

#include "app_led.h"
#include "app_button.h"
#include "bri_ctrl.h"
#include "ic2_ctrl.h"
#include "app_uart.h"

enum {
	SWITCH_CHANNEL_1,
	SWITCH_CHANNEL_2,
	SWITCH_CHANNEL_3,
	SWITCH_CHANNEL_MAX,
	SWITCH_CHANNEL_ALL,
};

int app_rainmaker_init(void);
void app_event_register_init(void);
int wifi_is_connected(void);
esp_err_t app_rainmaker_update_relay_state(int chan, int state);
void app_rainmaker_reconnect_timer_start(void);
void app_rainmaker_reconnect_timer_stop(void);
void start_power_save(void);
void stop_power_save(void);
void app_wifi_init_end(void);

