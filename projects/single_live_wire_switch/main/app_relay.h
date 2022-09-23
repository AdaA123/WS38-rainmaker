/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once
enum {
	SWITCH_STATE_OFF,
	SWITCH_STATE_ON,
	SWITCH_STATE_NA,
};

int app_relay_set_state(int chan, int onoff);
int app_relay_get_state(int chan);
int app_relay_get_name(int chan);
int app_relay_init(void);
