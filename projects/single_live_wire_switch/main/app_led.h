/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once
enum {
	LED_STATE_OFF,
	LED_STATE_ON,
	LED_STATE_BLINK,
	LED_STATE_NA,
};

int app_led_set_state(int state);
int app_led_init(void);
int app_led_get_state(void);
