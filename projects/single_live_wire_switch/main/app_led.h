/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once

#define PROVISION_LED   GPIO_NUM_7

enum {
	LED_STATE_OFF,
	LED_STATE_ON,
	LED_STATE_BLINK,
	LED_STATE_NA,
};

int app_led_set_state(int state);
void app_led_init(void);
int app_led_get_state(void);
void single_fire_led_on();
void single_fire_led_off();
void single_fire_led_blink_on();
void single_fire_led_blink_off();
