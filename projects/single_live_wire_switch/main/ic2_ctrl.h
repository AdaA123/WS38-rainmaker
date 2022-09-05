
#pragma once

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

#define CCHARGING_PIN       GPIO_NUM_4
#define QUICK_CHARGE_PIN    GPIO_NUM_10

void adc_cheak(void);
int Is_ADC_CHECK_OK(void);
void single_fire_led_blink_on_adc();
void single_fire_led_blink_off_adc();   

#ifdef __cplusplus
}
#endif /**< _cplusplus */