#ifndef APP_RELAY_H_
#define APP_RELAY_H_


#define RELAY_ON       GPIO_NUM_6
#define RELAY_OFF      GPIO_NUM_5

void app_relay_onoff(bool relay_onoff);
bool app_get_relay_status(void);

void app_relay_init(void);
void app_relay_toggle(void);

#endif
