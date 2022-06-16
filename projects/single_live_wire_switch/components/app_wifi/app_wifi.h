/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once
#include <esp_err.h>

#define SSID_PREFIX_LEN_MAX 8

typedef struct {
    uint8_t mfg[12];
    char* broadcast_prefix;
    uint8_t prov_timeout;  /* If prov_timeout is zero, will not exit the provisioning */
    bool enable_prov;
} app_wifi_prov_config_t;

/**
 * @brief 
 * 
 */
void app_wifi_init(void);

/**
 * @brief Start wifi (if there is no wifi authentication info, it will automatically start the network configuration process)
 * 
 * @param config 
 * @return esp_err_t 
 */
esp_err_t app_wifi_start(app_wifi_prov_config_t* config);
