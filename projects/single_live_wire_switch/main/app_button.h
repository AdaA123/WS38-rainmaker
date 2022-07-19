/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once

#define BUTTON_1        GPIO_NUM_8
#define BUTTON_2        GPIO_NUM_5
#define BUTTON_3        GPIO_NUM_4

#define REBOOT_DELAY        2

esp_err_t app_button_init(void);

void start_button_check(void);

void stop_button_check(void);
