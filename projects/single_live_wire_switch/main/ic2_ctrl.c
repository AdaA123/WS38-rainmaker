#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "driver/adc.h"

#include <esp_wifi.h>

#include "ic2_ctrl.h"
#include "app_priv.h"

#define TIMES 256

#define GET_ADC_TIMES 100 //次数

#define GET_ADC_DELAY 2*60 //s

static int ADC_CHECK_OK = 0;

int Is_ADC_CHECK_OK(void)
{
    return ADC_CHECK_OK;
}

static void single_read(void* arg)
{
    uint32_t adc1_reading_sum = 0;
    uint16_t adc1_reading;
    uint16_t adc_delay_timer_s = GET_ADC_DELAY;
    int n, ic2_cnt = 0;

    adc1_config_width(ADC_WIDTH_BIT_DEFAULT);
    adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_11);

#if 1
    while(1)
    {
        n = GET_ADC_TIMES;

        while (n--) 
        {
            adc1_reading_sum += adc1_get_raw(ADC1_CHANNEL_3);
            vTaskDelay(pdMS_TO_TICKS(20));
        }

        adc1_reading = adc1_reading_sum / GET_ADC_TIMES;
        ESP_LOGI("single_read ADC1_CH3", "%d", adc1_reading);
        adc1_reading_sum = 0;

        if (2750 < adc1_reading) // 1.95v (0~2.9v)
        {
            printf("ADC check end\n");
            break;
        }

        printf("ADC checking... \n");

        gpio_hold_dis(QUICK_CHARGE_PIN);
        gpio_set_level(QUICK_CHARGE_PIN, 1);
        gpio_hold_en(QUICK_CHARGE_PIN);

        vTaskDelay(pdMS_TO_TICKS(3 * 1000));
    }
#endif

    ADC_CHECK_OK = 1;

    do {
        n = GET_ADC_TIMES;

        while (n--) 
        {
            adc1_reading_sum += adc1_get_raw(ADC1_CHANNEL_3);
            vTaskDelay(pdMS_TO_TICKS(20));
        }

        adc1_reading = adc1_reading_sum / GET_ADC_TIMES;
        ESP_LOGI("single_read ADC1_CH3", "%d", adc1_reading);
        adc1_reading_sum = 0;

        if (2189 < adc1_reading && 0 == ic2_cnt) // 1.55v (0~2.9v)
        {
            vTaskDelay(pdMS_TO_TICKS(GET_ADC_DELAY * 1000));
        } else 
        if (2330 < adc1_reading) // 1.65v (0~2.9v)
        {
            if (1 == ic2_cnt)
            {
                esp_wifi_start();
	            single_fire_led_blink_off_adc();
            }
            ic2_cnt--;
            vTaskDelay(pdMS_TO_TICKS(30 * 1000));
        } else 
        if (2189 > adc1_reading)
        {
            if (0 == ic2_cnt)
            {
                esp_wifi_stop();
	            single_fire_led_blink_on_adc();
            }
            ic2_cnt = 3;
        }

        vTaskDelay(pdMS_TO_TICKS(30 * 1000));
    } while (1);

    vTaskDelete(NULL);
}

void adc_cheak(void)
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << CCHARGING_PIN | 1ULL << QUICK_CHARGE_PIN);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    gpio_hold_dis(CCHARGING_PIN);
    gpio_set_level(CCHARGING_PIN, 0);
    gpio_hold_en(CCHARGING_PIN);

    gpio_hold_dis(QUICK_CHARGE_PIN);
    gpio_set_level(QUICK_CHARGE_PIN, 0);
    gpio_hold_en(QUICK_CHARGE_PIN);

    xTaskCreate(single_read, "single_read_adc_task", 2*1024, NULL, 5, NULL);

    while(0 == ADC_CHECK_OK)
    {
        vTaskDelay(pdMS_TO_TICKS(1500));
    }
}

