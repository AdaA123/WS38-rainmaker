#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_sleep.h"
#include "driver/gpio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <esp_rmaker_utils.h>
//#include  "app_reset.h"

#include "app_priv.h"

#define RELAY_TIMER_MS 7
#define RELAY_BUFFER_TIMER_MS 300

esp_timer_handle_t device_relay_timer = NULL;
esp_timer_handle_t device_relay_buffer_timer = NULL;

uint64_t relay_mask = RELAY_OFF;
uint64_t relay_mask_temp = RELAY_OFF;
bool relay_buffer_end = true;

typedef struct PLUG
{
    /* data */
    uint8_t flag;
    uint8_t led_switch;
}PLUG_Type;

uint16_t ConfigDataBuff = 0;

PLUG_Type Device_Param = {0};
nvs_handle_t Dimm_Handle = {0};

esp_err_t err;

bool innotech_plug_relay_status = false;//true;

void MX_NVS_Init(void)
{
    // Initialize NVS
    esp_err_t err;

    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

}

void Read_Parameter(void)
{
    // Open
    printf("\n");
    printf("Opening Non-Volatile Storage (NVS) handle... ");
    //nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READWRITE, &Dimm_Handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else 
        printf("Done\n");        

    err = nvs_get_u16(Dimm_Handle, "ConfigDataBuff", &ConfigDataBuff);

    memcpy(&Device_Param,&ConfigDataBuff,sizeof(Device_Param)); 

    if(Device_Param.flag != 0xAA)
    {
        Device_Param.flag = 0xAA;
        Device_Param.led_switch = false; 
    }           

    relay_mask = (Device_Param.led_switch == true) ? RELAY_ON : RELAY_OFF;

    gpio_hold_dis(relay_mask);
    gpio_set_level(relay_mask, 1);
    gpio_hold_en(relay_mask);
    vTaskDelay(pdMS_TO_TICKS(7));
    gpio_hold_dis(relay_mask);
    gpio_set_level(relay_mask, 0);
    gpio_hold_en(relay_mask);
    
    printf("\n\nRead_Parameter  innotech_plug_relay_status = %d\n\n\n", innotech_plug_relay_status);

}

void Write_Parameter(void)
{
    // Open
    // printf("Opening Non-Volatile Storage (NVS) handle... ");
    //nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READWRITE, &Dimm_Handle);

        Device_Param.flag = 0xAA;
        Device_Param.led_switch = (relay_mask == RELAY_ON) ? true : false;

        memcpy(&ConfigDataBuff,&Device_Param,sizeof(Device_Param)); 

    printf("\n\nConfigDataBuff = %d\n\n\n", ConfigDataBuff);

    err = nvs_set_u16(Dimm_Handle, "ConfigDataBuff", ConfigDataBuff);
        // printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        // printf("Committing updates in NVS ... ");
        err = nvs_commit(Dimm_Handle);
        // printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        // Close
        nvs_close(Dimm_Handle);        
}

bool app_get_relay_status(void)
{
    printf("app_relay_onoff status: %s\n", (relay_mask == RELAY_ON) ? "ON" : "OFF");
    return (relay_mask == RELAY_ON) ? true : false;
}

void app_relay_toggle(void)
{
    if (true == relay_buffer_end)
    {
        relay_buffer_end = false;

        relay_mask = (relay_mask == RELAY_ON) ? RELAY_OFF : RELAY_ON;
        relay_mask_temp = relay_mask;

        gpio_hold_dis(relay_mask);
        gpio_set_level(relay_mask, 1);
        gpio_hold_en(relay_mask);
        esp_timer_start_once(device_relay_timer, RELAY_TIMER_MS*1000);

        esp_timer_start_once(device_relay_buffer_timer, RELAY_BUFFER_TIMER_MS*1000);
    }
    else
    {
        relay_mask_temp = (relay_mask_temp == RELAY_ON) ? RELAY_OFF : RELAY_ON;
    }
}

void app_relay_onoff(bool relay_onoff)
{
     if (true == relay_buffer_end)
    {
        relay_buffer_end = false;

        relay_mask = (relay_onoff == true) ? RELAY_ON : RELAY_OFF;
        relay_mask_temp = relay_mask;

        gpio_hold_dis(relay_mask);
        gpio_set_level(relay_mask, 1);
        gpio_hold_en(relay_mask);
        esp_timer_start_once(device_relay_timer, RELAY_TIMER_MS*1000);
        printf("wifi app_relay_onoff status: %s\n", (relay_mask == RELAY_ON) ? "ON" : "OFF");

        esp_timer_start_once(device_relay_buffer_timer, RELAY_BUFFER_TIMER_MS*1000);
    }
    else
    {
        relay_mask_temp = (relay_onoff == true) ? RELAY_ON : RELAY_OFF;
    }
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

void device_relay_timer_buffer_callback(void *args)
{
    if (relay_mask_temp != relay_mask)
    {
        relay_mask = relay_mask_temp;

        gpio_hold_dis(relay_mask);
        gpio_set_level(relay_mask, 1);
        gpio_hold_en(relay_mask);
        esp_timer_start_once(device_relay_timer, RELAY_TIMER_MS*1000);
        printf("buffer app_relay_onoff status: %s\n", (relay_mask == RELAY_ON) ? "ON" : "OFF");

        esp_timer_start_once(device_relay_buffer_timer, (RELAY_BUFFER_TIMER_MS+500)*1000);
    }
    else
    {
        relay_buffer_end = true;
        Write_Parameter();
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

    const esp_timer_create_args_t device_relay_buffer_timer_args = {
            .callback = &device_relay_timer_buffer_callback,
            .name = "device_relay_buffer"
    };
    esp_timer_create(&device_relay_buffer_timer_args, &device_relay_buffer_timer);

    MX_NVS_Init();
    Read_Parameter();

    return;
}

