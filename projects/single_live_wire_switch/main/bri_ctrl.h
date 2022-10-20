#ifndef BRI_CTRL_H
#define BRI_CTRL_H	 

#ifdef __cplusplus
extern "C" {
#endif

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Public constants ----------------------------------------------------------*/

#define		Light_OFF	0
#define		Light_ON	1

#define		BRI_ONOFF  	0
#define		BRI_SUB	   	1
#define		BRI_ADD    	2

#define ZCD_START_ANGLE  25
#define ZCD_END_ANGLE  	125

uint16_t Get_Bri_Status(void); 

void ZCD_CTRL_Time(unsigned char T100ns);

void Brightness_Add_or_Sub(int cmd);
void Toggle_The_Lights(void);
void Open_The_Lights(void);
void Close_The_Lights(void);
int Get_Light_state(void);
void Light_Init(void);
void DIM_Ctrl(void);
void Bti_Ctrl_Init(void);
void ZCD_Adjust(void);
uint8_t Is_bri_off(void);

void Set_Btight_Min_Pct(unsigned int angle_val);
unsigned int Get_Btight_Min_Pct(void);
void Set_Btight_Max_Pct(unsigned int angle_val);
unsigned int Get_Btight_Max_Pct(void);

void Bright_Add_Long_Press(void);
void Bright_Add_Short_Press(void);
void Bright_Sub_Long_Press(void);
void Bright_Sub_Short_Press(void);

void Bri_Ctrl_Init(void);
void dimmer_rmaker_init(void);

void esp_rmaker_update(uint8_t cmd_tyed, uint32_t dat);

void sys_check();

void Set_Bri_Status(uint16_t status);
void Mcu_Set_Bri_Status(uint16_t status);

void Set_Btight_Pct(unsigned int angle_val);
void Mcu_Set_Btight_Pct(unsigned int Pct);
unsigned int Get_Btight_Pct(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif
