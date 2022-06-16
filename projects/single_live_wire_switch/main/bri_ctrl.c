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
#include "app_led.h"
#include "app_priv.h"

static const char *TAG = "uart_events";

/**
 * This example shows how to use the UART driver to handle special UART events.
 *
 * It also reads data from UART0 directly, and echoes it to console.
 *
 * - Port: UART0
 * - Receive (Rx) buffer: on
 * - Transmit (Tx) buffer: off
 * - Flow control: off
 * - Event queue: on
 * - Pin assignment: TxD (default), RxD (default)
 */

#define EX_UART_NUM UART_NUM_1
#define PATTERN_CHR_NUM    (3)         /*!< Set the number of consecutive and identical characters received by receiver which defines a UART pattern*/
#define UART_GPIO_TX    10
#define UART_GPIO_RX    7

#define MCU_WAJEUP_IO   GPIO_NUM_6

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)

#define LENGTH													5
	//0xaa55 
#define SendHead1												 0xaa
#define SendHead2												0x55
	//0x55aa
#define ReceivedHead1											0x55
#define ReceivedHead2											0xaa

static QueueHandle_t uart0_queue;

/* External variables --------------------------------------------------------*/
uint16_t Bri_Status = 0;

char Bri_Min_Pct = 0;	//最小亮度
char Bri_Max_Pct = 100;      //最大亮度
char Bri_Now_Pct =60;      //当前亮度

extern __uint8_t s_wifi_init_end_flag;

static uint8_t ucSendBuff[LENGTH] = { SendHead1, SendHead2, 0, 0, 0};
static uint8_t ucSendFlag = 0;
static uint8_t ucReceiveData[LENGTH] = {ReceivedHead1, ReceivedHead2, 0, 0, 0};
static uint8_t ucRxCut = 0;

static uint8_t s_bri_add_flag = 0;
static uint8_t s_bri_sub_flag = 0;

TaskHandle_t ucSendTaskHandle = NULL;
static int8_t ucSendTaskHandle_Flag = 0;
/*-----------------------------------------------------------------------------
Function: receive_mcu_ack()
Description: 接受MCU反馈信号
Input:
Output:
Return:
Others:
-----------------------------------------------------------------------------*/
void receive_mcu_ack(void)  //
{
        
}

void send_uart_info_task(void *arg)
{
    uint8_t ucSendBuff_temp[LENGTH] = {0};

    while(1)
    {
        memcpy(ucSendBuff_temp, ucSendBuff, sizeof(ucSendBuff_temp));

        ucSendFlag = 0;
        
        if(OnOffCMD == ucSendBuff_temp[2] || CChargingCMD == ucSendBuff_temp[2]) //&& 1 == ucSendBuff_temp[3]
        {
            gpio_hold_dis(MCU_WAJEUP_IO);
            gpio_set_level(MCU_WAJEUP_IO, 1);
            gpio_hold_en(MCU_WAJEUP_IO);

            vTaskDelay(pdMS_TO_TICKS(2));

            gpio_hold_dis(MCU_WAJEUP_IO);
            gpio_set_level(MCU_WAJEUP_IO, 0);
            gpio_hold_en(MCU_WAJEUP_IO);
        }

        stop_power_save();
        uart_write_bytes(EX_UART_NUM, (const char*) ucSendBuff_temp, sizeof(ucSendBuff_temp));//uart_tx_chars
        uart_wait_tx_done(EX_UART_NUM, 100);
        start_power_save();

        if (s_wifi_init_end_flag)
        {
            esp_rmaker_update(ucSendBuff_temp[2], ucSendBuff_temp[3]);
        }

        vTaskDelay(pdMS_TO_TICKS(150));

        if (0 == ucSendFlag)
        {
            ucSendBuff[2] = ERRORCMD;
            break;
        }
    }
    
    ucSendTaskHandle_Flag = 0;

    vTaskDelete(ucSendTaskHandle);
}

/*-----------------------------------------------------------------------------
Function: send_bri_ctrl_info()
Description: 发送控制MCU命令
Input:
Output:
Return:
Others:
-----------------------------------------------------------------------------*/
void send_bri_ctrl_info(unsigned char cmd, unsigned char dat) //
{
    if (0 == ucSendFlag)
    {
        ucSendBuff[2] = cmd;
        ucSendBuff[3] = dat;
        ucSendBuff[4] = ucSendBuff[0] + ucSendBuff[1] + ucSendBuff[2]  + ucSendBuff[3];

        ucSendFlag = 1;
    }

    // gpio_hold_dis(MCU_WAJEUP_IO);
    // gpio_set_level(MCU_WAJEUP_IO, 1);
    // gpio_hold_en(MCU_WAJEUP_IO);
    
    // vTaskDelay(pdMS_TO_TICKS(1));

    // gpio_hold_dis(MCU_WAJEUP_IO);
    // gpio_set_level(MCU_WAJEUP_IO, 0);
    // gpio_hold_en(MCU_WAJEUP_IO);

    // uart_write_bytes(EX_UART_NUM, (const char*) ucSendBuff, sizeof(ucSendBuff));

    //esp_rmaker_update(cmd, dat);
    
    if (0 == ucSendTaskHandle_Flag)
    {
         if (xTaskCreate(send_uart_info_task, "send_uart_info_task", 4*1024, NULL, 12, &ucSendTaskHandle) != pdTRUE) {
            ESP_LOGE(TAG, "Error create websocket task");
            return;
        }
        ucSendTaskHandle_Flag = 1;
    }
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

    start_button_check();
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

/*-----------------------------------------------------------------------------
Function: uart_event_task(void)
Description: 串口处理进程
Input:
Output:
Return:
Others:
-----------------------------------------------------------------------------*/
static void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    size_t buffered_size;
    uint8_t* dtmp = (uint8_t*) malloc(RD_BUF_SIZE);
    for(;;) {
        //Waiting for UART event.
        if(xQueueReceive(uart0_queue, (void * )&event, (portTickType)portMAX_DELAY)) {
            bzero(dtmp, RD_BUF_SIZE);
            ESP_LOGI(TAG, "uart[%d] event:", EX_UART_NUM);
            switch(event.type) {
                //Event of UART receving data
                /*We'd better handler data event fast, there would be much more data events than
                other types of events. If we take too much time on data event, the queue might
                be full.*/
                case UART_DATA:
                    ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
                    uart_read_bytes(EX_UART_NUM, dtmp, event.size, portMAX_DELAY);
                    ESP_LOGI(TAG, "[DATA EVT]: %s", dtmp);
                    //uart_write_bytes(EX_UART_NUM, (const char*) dtmp, event.size);
                    break;
                //Event of HW FIFO overflow detected
                case UART_FIFO_OVF:
                    ESP_LOGI(TAG, "hw fifo overflow");
                    // If fifo overflow happened, you should consider adding flow control for your application.
                    // The ISR has already reset the rx FIFO,
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(EX_UART_NUM);
                    xQueueReset(uart0_queue);
                    break;
                //Event of UART ring buffer full
                case UART_BUFFER_FULL:
                    ESP_LOGI(TAG, "ring buffer full");
                    // If buffer full happened, you should consider encreasing your buffer size
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(EX_UART_NUM);
                    xQueueReset(uart0_queue);
                    break;
                //Event of UART RX break detected
                case UART_BREAK:
                    ESP_LOGI(TAG, "uart rx break");
                    break;
                //Event of UART parity check error
                case UART_PARITY_ERR:
                    ESP_LOGI(TAG, "uart parity error");
                    break;
                //Event of UART frame error
                case UART_FRAME_ERR:
                    ESP_LOGI(TAG, "uart frame error");
                    break;
                //UART_PATTERN_DET
                case UART_PATTERN_DET:
                    uart_get_buffered_data_len(EX_UART_NUM, &buffered_size);
                    int pos = uart_pattern_pop_pos(EX_UART_NUM);
                    ESP_LOGI(TAG, "[UART PATTERN DETECTED] pos: %d, buffered size: %d", pos, buffered_size);
                    if (pos == -1) {
                        // There used to be a UART_PATTERN_DET event, but the pattern position queue is full so that it can not
                        // record the position. We should set a larger queue size.
                        // As an example, we directly flush the rx buffer here.
                        uart_flush_input(EX_UART_NUM);
                    } else {
                        uart_read_bytes(EX_UART_NUM, dtmp, pos, 100 / portTICK_PERIOD_MS);
                        uint8_t pat[PATTERN_CHR_NUM + 1];
                        memset(pat, 0, sizeof(pat));
                        uart_read_bytes(EX_UART_NUM, pat, PATTERN_CHR_NUM, 100 / portTICK_PERIOD_MS);
                        ESP_LOGI(TAG, "read data: %s", dtmp);
                        ESP_LOGI(TAG, "read pat : %s", pat);
                    }
                    break;
                //Others切换灯光状态
                default:
                    ESP_LOGI(TAG, "uart event type: %d", event.type);
                    break;
            }
        }
    }
    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
}

void mcu_wakeup_io_init(void)
{    
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << MCU_WAJEUP_IO);
    io_conf.pull_down_en = true;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
}

/*-----------------------------------------------------------------------------
Function: bri_ctrl_uart_init(void)
Description: 串口初始化
Input:
Output:
Return:
Others:
-----------------------------------------------------------------------------*/
void bri_ctrl_uart_init(void)
{
	    esp_log_level_set(TAG, ESP_LOG_INFO);
printf("uart init state\n");
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,//UART_SCLK_RTC UART_SCLK_APB
    };
    //Install UART driver, and get the queue.
    uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, 0, 40, &uart0_queue, 0);//BUF_SIZE *  4
    uart_param_config(EX_UART_NUM, &uart_config);

    //Set UART log level
    esp_log_level_set(TAG, ESP_LOG_INFO);
    //Set UART pins (using UART0 default pins ie no changes.)
    uart_set_pin(EX_UART_NUM, UART_GPIO_TX, UART_GPIO_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    //Set uart pattern detect function.
    uart_enable_pattern_det_baud_intr(EX_UART_NUM, '+', PATTERN_CHR_NUM, 9, 0, 0);
    //Reset the pattern queue length to record at most 20 pattern positions.
    uart_pattern_queue_reset(EX_UART_NUM, 20);

    //Create a task to handler UART event from ISR
    xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 12, NULL);
printf("uart init ok\n");
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

    mcu_wakeup_io_init();
    bri_ctrl_uart_init();

    Close_The_Lights();

    printf("Bri_Ctrl_Init \n");

    return;
}

