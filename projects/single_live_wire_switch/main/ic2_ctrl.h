
#pragma once

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

void adc_cheak(void);
int Is_ADC_CHECK_OK(void);
void single_fire_led_blink_on_adc();
void single_fire_led_blink_off_adc();   

#ifdef __cplusplus
}
#endif /**< _cplusplus */