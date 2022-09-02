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

#include <esp_rmaker_utils.h>
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
#define UART_GPIO_TX    GPIO_NUM_10
#define UART_GPIO_RX    GPIO_NUM_7

#define MCU_WAJEUP_IO   GPIO_NUM_6
#define WAJEUP_MCU_IO   GPIO_NUM_5

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

static uint8_t ucSendBuff[LENGTH] = { SendHead1, SendHead2, 0, 0, 0};
static uint8_t ucSendFlag = 0;
// static uint8_t ucReceiveData[LENGTH] = {ReceivedHead1, ReceivedHead2, 0, 0, 0};
// static uint8_t ucRxCut = 0;

extern __uint8_t s_wifi_init_end_flag;

TaskHandle_t ucSendTaskHandle = NULL;
static int8_t ucSendTaskHandle_Flag = 0;
static int8_t ucRxTaskHandle_Flag = 0;
static int8_t re_wkup_mcu_flag = 0;

esp_err_t wait_for_mcu_info_config(gpio_num_t gpio_num);
void wakeup_mcu_info(void);
void wakeup_mcu_info_config(gpio_num_t gpio_num);

void send_uart_info_task(void *arg)
{
    uint8_t ucSendBuff_temp[LENGTH] = {0};

    while(1)
    {
        memcpy(ucSendBuff_temp, ucSendBuff, sizeof(ucSendBuff_temp));
        
        ucSendFlag = 0;
        
        if (
            ucSendBuff_temp[0] != SendHead1
            || ucSendBuff_temp[1] != SendHead2
            //|| ucSendBuff_temp[4] != (ucSendBuff_temp[0] + ucSendBuff_temp[1] + ucSendBuff_temp[2] + ucSendBuff_temp[3])
        )
        {
            ucSendBuff_temp[2] = ERRORCMD;
            break;
        }

        while(ucRxTaskHandle_Flag)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        if(OnOffCMD == ucSendBuff_temp[2] 
            || WIFIINITSTARTCMD == ucSendBuff_temp[2]
            || WIFIINITENDCMD == ucSendBuff_temp[2]) 
        {
            // wakeup_mcu_info_config();
            wakeup_mcu_info();
        }

        stop_power_save();
        uart_write_bytes(EX_UART_NUM, (const char*) ucSendBuff_temp, sizeof(ucSendBuff_temp));//uart_tx_chars
        uart_wait_tx_done(EX_UART_NUM, 100);
        start_power_save();

        if (s_wifi_init_end_flag)
        {
            esp_rmaker_update(ucSendBuff_temp[2], ucSendBuff_temp[3]);
        }

        ucSendFlag = 0;

        vTaskDelay(pdMS_TO_TICKS(100));

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
    ucSendBuff[2] = cmd;
    ucSendBuff[3] = dat;
    ucSendBuff[4] = ucSendBuff[0] + ucSendBuff[1] + ucSendBuff[2]  + ucSendBuff[3];
    ucSendFlag = 1;
    
    if (0 == ucSendTaskHandle_Flag)
    {
         if (xTaskCreate(send_uart_info_task, "send_uart_info_task", 4*1024, NULL, 12, &ucSendTaskHandle) != pdTRUE) {
            ESP_LOGE(TAG, "Error create websocket task");
            return;
        }
        ucSendTaskHandle_Flag = 1;
    }
}

void uart_deal_with(uint8_t* str, uint8_t num)
{
    uint8_t i;
    uint8_t SendBuff[LENGTH] = { SendHead1, SendHead2, UART_ACCEPT_END, 0, 0};
    
    for (i = 0; i < num - 4; ) 
    { 
        printf("i == %d\n", i);
        printf("str[i] = %x \tstr[i+1] = %x\n", str[i], str[i+1]);
        if (ReceivedHead1 == str[i] && ReceivedHead2 == str[i+1])
        {
            printf("str[i+2] = %x \tstr[i+3] = %x\n", str[i + 2], str[i + 2]);
            switch(str[i + 2])
            {
                case OnOffCMD:
                    if (str[i+3] != Get_Bri_Status())
                    {
                        Set_Bri_Status(str[i+3]);
                                            
                        SendBuff[3] = str[i + 2];
                        SendBuff[4] = SendBuff[0] + SendBuff[1] + SendBuff[2]  + SendBuff[3];  

                        stop_power_save();
                        uart_write_bytes(EX_UART_NUM, (const char*) SendBuff, sizeof(SendBuff));//uart_tx_chars
                        uart_wait_tx_done(EX_UART_NUM, 100);
                        start_power_save();

                        printf("start_power_save\n");
                        ucRxTaskHandle_Flag = 0;

                        if (s_wifi_init_end_flag)
                        {
                            esp_rmaker_update(str[i + 2], (str[i+3])?1:0);
                        }
                    }
                    else
                    {
                        start_power_save();
                        printf("start_power_save\n");
                        printf("Bri_Status same\n");
                    }
                    printf("Bri_Status %d\n", str[i+3]);
                    i += 4;
                break;

                case BriNowCMD:
                    if (str[i+3] != Get_Btight_Pct())
                    {
                        SendBuff[3] = str[i + 2];
                        SendBuff[4] = SendBuff[0] + SendBuff[1] + SendBuff[2]  + SendBuff[3];  

                        stop_power_save();
                        uart_write_bytes(EX_UART_NUM, (const char*) SendBuff, sizeof(SendBuff));//uart_tx_chars
                        uart_wait_tx_done(EX_UART_NUM, 100);
                        start_power_save();

                        printf("start_power_save\n");
                        ucRxTaskHandle_Flag = 0;

                        if (str[i+3] <= 100) 
                        {
                            Set_Btight_Pct(str[i+3]);
                                    
                            if (s_wifi_init_end_flag)
                            {
                                esp_rmaker_update(str[i + 2], str[i+3]);
                            }
                        }
                    }
                    else
                    {
                        start_power_save();
                        printf("start_power_save\n");
                        printf("Btight_Pct same\n");
                    }
                    printf("\t\t\t\tBtight_Pct %d\n", str[i+3]);
                    i += 4;
                break;

                case WIFIRESETCMD:
                    SendBuff[3] = str[i + 2];
                    SendBuff[4] = SendBuff[0] + SendBuff[1] + SendBuff[2]  + SendBuff[3];  

                    stop_power_save();
                    uart_write_bytes(EX_UART_NUM, (const char*) SendBuff, sizeof(SendBuff));//uart_tx_chars
                    uart_wait_tx_done(EX_UART_NUM, 100);
                    start_power_save();

                    printf("start_power_save\n");
                    ucRxTaskHandle_Flag = 0;

                    esp_rmaker_factory_reset(0, REBOOT_DELAY);
                    i += 4;
                break;

                case BriMaxCMD:
                case BriMinCMD:
                case CChargingCMD:
                case WIFIINITENDCMD:
                case WIFIINITSTARTCMD:
                case UART_SEND_ALLOW:
                case UART_ACCEPT_END:
                case UART_ACK:
                case ERRORCMD:
                    i += 4;
                break;

                default:
                    i++;
                break;
            }
        }
        else
        {
            i++;
        }
    } 

    start_power_save();
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
                    uart_deal_with(dtmp, event.size);
                    re_wkup_mcu_flag = 0;
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

static esp_timer_handle_t wakeup_io_start_timer = NULL;
static esp_timer_handle_t wakeup_io_stop_timer = NULL;

static gpio_int_type_t wakeup_io_intr_type = GPIO_INTR_HIGH_LEVEL;

static void IRAM_ATTR wakeup_io_isr_handler(void* arg)
{
    if(wakeup_io_intr_type == GPIO_INTR_LOW_LEVEL){
        wakeup_io_intr_type = GPIO_INTR_HIGH_LEVEL;
        gpio_set_intr_type(MCU_WAJEUP_IO, GPIO_INTR_HIGH_LEVEL);
        ucRxTaskHandle_Flag = 0;
        esp_timer_stop(wakeup_io_start_timer);
        esp_timer_start_once(wakeup_io_start_timer, 1 * 1000);
    }else if (wakeup_io_intr_type == GPIO_INTR_HIGH_LEVEL) {
        wakeup_io_intr_type = GPIO_INTR_LOW_LEVEL;
        gpio_set_intr_type(MCU_WAJEUP_IO, GPIO_INTR_LOW_LEVEL);
        ucRxTaskHandle_Flag = 1;
        esp_timer_stop(wakeup_io_stop_timer);
        esp_timer_start_once(wakeup_io_stop_timer, 1 * 1000);  
    }else {
        wakeup_io_intr_type = GPIO_INTR_LOW_LEVEL;
        gpio_set_intr_type(MCU_WAJEUP_IO, GPIO_INTR_LOW_LEVEL);
    }  
}

static void wakeup_io_stop_power_timer_callback(void* arg)
{
    uint8_t SendBuff[LENGTH] = { SendHead1, SendHead2, 0, 0, 0};

    stop_power_save();
    printf("stop_power_save\n");

    SendBuff[2] = UART_SEND_ALLOW;
    SendBuff[3] = 1;
    SendBuff[4] = SendBuff[0] + SendBuff[1] + SendBuff[2]  + SendBuff[3];
    
    uart_write_bytes(EX_UART_NUM, (const char*) SendBuff, sizeof(SendBuff));//uart_tx_chars
    uart_wait_tx_done(EX_UART_NUM, 100);

    printf("SendBuff[2] = %d\n", SendBuff[2]);
}

static void wakeup_io_start_power_timer_callback(void* arg)
{    
    uint8_t SendBuff[LENGTH] = { SendHead1, SendHead2, 0, 0, 0};
    SendBuff[2] = UART_ACCEPT_END;
    SendBuff[3] = 1;
    SendBuff[4] = SendBuff[0] + SendBuff[1] + SendBuff[2]  + SendBuff[3];
    
    uart_write_bytes(EX_UART_NUM, (const char*) SendBuff, sizeof(SendBuff));//uart_tx_chars
    uart_wait_tx_done(EX_UART_NUM, 100);

    start_power_save();
    printf("start_power_save\n");
    printf("SendBuff[2] = %d\n", SendBuff[2]);
}   

void wakeup_mcu_info_config(gpio_num_t gpio_num)
{        
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << gpio_num);
    io_conf.pull_down_en = 1;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    printf("wakeup_mcu_info_config\n");
}

void wakeup_mcu_info(void)
{    
    gpio_hold_dis(WAJEUP_MCU_IO);
    gpio_set_level(WAJEUP_MCU_IO, 1);
    gpio_hold_en(WAJEUP_MCU_IO);

    vTaskDelay(pdMS_TO_TICKS(2));

    gpio_hold_dis(WAJEUP_MCU_IO);
    gpio_set_level(WAJEUP_MCU_IO, 0);
    gpio_hold_en(WAJEUP_MCU_IO);
}

esp_err_t wait_for_mcu_info_config(gpio_num_t gpio_num)
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_HIGH_LEVEL;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << gpio_num);
    io_conf.pull_down_en = 1;
    io_conf.pull_up_en = 0;
    return gpio_config(&io_conf);
    printf("wait_for_mcu_info_config\n");
}

esp_err_t wakeup_io_intr_init(void)
{
    esp_err_t ret = ESP_FAIL;

    wakeup_mcu_info_config(WAJEUP_MCU_IO);

    ret = wait_for_mcu_info_config(MCU_WAJEUP_IO);

    if(gpio_get_level(MCU_WAJEUP_IO) == 0){
        wakeup_io_intr_type = GPIO_INTR_POSEDGE;
    }

    ret = gpio_hold_en(MCU_WAJEUP_IO);

    ret = gpio_install_isr_service(0);

    ret = gpio_isr_handler_add(MCU_WAJEUP_IO, wakeup_io_isr_handler, (void *)MCU_WAJEUP_IO);

    ret = gpio_wakeup_enable(MCU_WAJEUP_IO, GPIO_INTR_HIGH_LEVEL);

    ret = esp_sleep_enable_gpio_wakeup();

    return ret;
}

void wifi_start_init(void)
{
    ucRxTaskHandle_Flag = 0;
    send_bri_ctrl_info(WIFIINITSTARTCMD, 1);

    return;
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
    uart_driver_install(EX_UART_NUM, 1024, 0, 40, &uart0_queue, 0);//BUF_SIZE *  4
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
    xTaskCreate(uart_event_task, "uart_event_task", 4*1024, NULL, 12, NULL);

    const esp_timer_create_args_t wakeup_io_stop_power_timer_args = {
            .callback = &wakeup_io_stop_power_timer_callback,
            .name = "wakeup_io_timer_callback"
    };
    esp_timer_create(&wakeup_io_stop_power_timer_args, &wakeup_io_stop_timer);
    const esp_timer_create_args_t wakeup_io_start_power_timer_args = {
            .callback = &wakeup_io_start_power_timer_callback,
            .name = "wakeup_io_timer_callback"
    };
    esp_timer_create(&wakeup_io_start_power_timer_args, &wakeup_io_start_timer);

    wakeup_io_intr_init();

    printf("uart init ok\n");
}

