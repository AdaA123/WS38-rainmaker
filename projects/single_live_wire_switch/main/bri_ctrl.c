#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_sleep.h"

#include <esp_wifi.h>

#include "bri_ctrl.h"
// #include "app_led.h"
#include "app_priv.h"

static const char *TAG = "uart_events";

/* External variables --------------------------------------------------------*/
uint16_t Bri_Status = 0;

char Bri_Min_Pct = 0;	//最小亮度
char Bri_Max_Pct = 100;      //最大亮度
char Bri_Now_Pct =50;      //当前亮度

extern __uint8_t s_wifi_init_end_flag;

static uint8_t s_bri_add_flag = 0;
static uint8_t s_bri_sub_flag = 0;

void Set_Bri_Status(uint16_t status)
{
	Bri_Status = status;

	return;
}

/*-----------------------------------------------------------------------------
Function: Get_Bri_Status()
Description: 获取开关状态
Input:
Output:
Return:
Others:
-----------------------------------------------------------------------------*/
uint16_t Get_Bri_Status(void)  //
{
	return Bri_Status;
}

/*-----------------------------------------------------------------------------
Function: 获取当前亮度()
Description: 获取当前亮度
Input:
Output:
Return:
Others:
-----------------------------------------------------------------------------*/
unsigned int Get_Btight_Pct(void) //
{
    return Bri_Now_Pct;
}

/*-----------------------------------------------------------------------------
Function: Set_Btight_Pct(unsigned int Pct)
Description: 设置当前亮度
Input:  Pct ：亮度百分比
Output:
Return:
Others:
-----------------------------------------------------------------------------*/
void Set_Btight_Pct(unsigned int Pct)
{
	if (Bri_Max_Pct < Pct || Bri_Min_Pct > Pct) {
		return;	
	}

	Bri_Now_Pct = Pct;

	send_bri_ctrl_info(BriNowCMD, Bri_Now_Pct);

    printf("Bri_Now_Pct = %d\n", Bri_Now_Pct);
}

/*-----------------------------------------------------------------------------
Function: Set_Btight_Min_Pct(unsigned int Pct)
Description: 设置最小亮度
Input:  Pct ：最小亮度百分比
Output:
Return:
Others:
-----------------------------------------------------------------------------*/
void Set_Btight_Min_Pct(unsigned int pct)
{
	if (100 < pct) {
		return;	
	}

	Bri_Min_Pct = pct;

	if (Bri_Now_Pct < Bri_Min_Pct)
	{
		Bri_Now_Pct = Bri_Min_Pct;
		
		send_bri_ctrl_info(BriNowCMD, Bri_Now_Pct);
	}

	send_bri_ctrl_info(BriMinCMD, Bri_Now_Pct);
}

/*-----------------------------------------------------------------------------
Function: Set_Btight_Max_Pct(unsigned int Pct)
Description: 设置最大亮度
Input:  Pct ：最大亮度百分比
Output:
Return:
Others:
-----------------------------------------------------------------------------*/
void Set_Btight_Max_Pct(unsigned int pct)
{
	if (100 < pct) {
		return;	
	}

	Bri_Max_Pct = pct;

	if (Bri_Now_Pct > Bri_Max_Pct)
	{
		Bri_Now_Pct = Bri_Max_Pct;
		
		send_bri_ctrl_info(BriNowCMD, Bri_Now_Pct);
        vTaskDelay(pdMS_TO_TICKS(1));
	}

	send_bri_ctrl_info(BriMaxCMD, Bri_Now_Pct);
        vTaskDelay(pdMS_TO_TICKS(1));
}

/*-----------------------------------------------------------------------------
Function: Get_Btight_Min_Pct(void)
Description: 获取最小亮度
Input:  Pct ：最小亮度百分比
Output:
Return:
Others:
-----------------------------------------------------------------------------*/
unsigned int Get_Btight_Min_Pct(void)
{
	return Bri_Min_Pct;
}

/*-----------------------------------------------------------------------------
Function: Get_Btight_Max_Pct(void)
Description: 获取最大亮度
Input:  
Output: 最大亮度百分比
Return:
Others:
-----------------------------------------------------------------------------*/
unsigned int Get_Btight_Max_Pct(void)
{
	return Bri_Max_Pct;
}

/*-----------------------------------------------------------------------------
Function: Bright_Sub_Short_Press()
Description: 短按减亮度键
Input:
Output:
Return:
Others:
-----------------------------------------------------------------------------*/
void Bright_Sub_Short_Press()
{
	Bri_Now_Pct -= 10;

    if (Bri_Now_Pct < Bri_Min_Pct || Bri_Now_Pct > 101) {
		Bri_Now_Pct = Bri_Min_Pct;
	} 
		
	send_bri_ctrl_info(BriNowCMD, Bri_Now_Pct);
}

/*-----------------------------------------------------------------------------
Function: Bright_Sub_Long_Press()
Description: 长按减亮度键
Input:
Output:
Return:
Others:
-----------------------------------------------------------------------------*/
void Bright_Sub_Long_Press()
{
    if (Bri_Now_Pct > Bri_Min_Pct)
    {
        Bri_Now_Pct -= 1;
	    send_bri_ctrl_info(BriNowCMD, Bri_Now_Pct);
    }
}

/*-----------------------------------------------------------------------------
Function: Bright_Add_Short_Press(void)
Description: 短按加亮度键
Input:
Output:
Return:
Others:
-----------------------------------------------------------------------------*/
void Bright_Add_Short_Press()
{
    Bri_Now_Pct += 10;

    if (Bri_Now_Pct > Bri_Max_Pct) {
		Bri_Now_Pct = Bri_Max_Pct;
	}

	send_bri_ctrl_info(BriNowCMD, Bri_Now_Pct);
}

/*-----------------------------------------------------------------------------
Function: Bright_Add_Long_Press(void)
Description: 长按加亮度键
Input:
Output:
Return:
Others:
-----------------------------------------------------------------------------*/
void Bright_Add_Long_Press(void)
{   
    if (Bri_Now_Pct < Bri_Max_Pct)
    {
        Bri_Now_Pct += 1;
	    send_bri_ctrl_info(BriNowCMD, Bri_Now_Pct);
    }
    
}

void sys_check()
{
    if (Bri_Status == 0)
    {
        //stop_button_check();
        start_power_save();
    } 
}

extern __uint8_t s_wifi_init_end_flag;
/*-----------------------------------------------------------------------------
Function: Open_The_Lights(void)
Description: 开灯
Input:
Output:
Return:
Others:
-----------------------------------------------------------------------------*/
void Open_The_Lights(void)
{
	Bri_Status = 1;
    
    send_bri_ctrl_info(OnOffCMD, Bri_Status);

    //start_button_check();
}

/*-----------------------------------------------------------------------------
Function: Close_The_Lights(void)
Description: 关灯
Input:
Output:
Return:
Others:
-----------------------------------------------------------------------------*/
void Close_The_Lights(void)
{
	Bri_Status = 0;
    stop_button_check();
    //single_fire_led_off();
	//send close  signal
    send_bri_ctrl_info(OnOffCMD, Bri_Status);
}

/*-----------------------------------------------------------------------------
Function: Toggle_The_Lights(void)
Description: 切换灯光状态
Input:
Output:
Return:
Others:
-----------------------------------------------------------------------------*/
void Toggle_The_Lights(void)
{
	if (Bri_Status) {
		Close_The_Lights();
	} else {
		Open_The_Lights();
	}
}

void dimmer_rmaker_init(void)
{
	if (Bri_Status) {
		Open_The_Lights();
	} else {
		Close_The_Lights();
	}
}

/*-----------------------------------------------------------------------------
Function: Bri_Ctrl_Init(void)
Description: 灯光初始化
Input:
Output:
Return:
Others:
-----------------------------------------------------------------------------*/
void Bri_Ctrl_Init(void)
{
	Bri_Status = 0;

    //mcu_wakeup_io_init();
    bri_ctrl_uart_init();

    //Close_The_Lights();

    return;
}

