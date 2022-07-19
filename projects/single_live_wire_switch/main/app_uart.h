/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once

#ifdef __cplusplus
extern "C" {
#endif


#define OnOffCMD		   	0x00    //开关命令
#define BriNowCMD		   	0x01    //当前亮度
#define BriMaxCMD		   	0x02    //最大亮度
#define BriMinCMD		   	0x03    //最小亮度
#define CChargingCMD		   0x04    //开启充电
#define WIFIRESETCMD		   0x05    //重置wifi
#define WIFIINITENDCMD		0x06    //wifi初始化结束
#define WIFIINITSTARTCMD	0x07    //wifi init
#define UART_SEND_ALLOW		0xcc    //可以发送消息
#define UART_ACCEPT_END		0xdd    //消息接受结束
#define UART_ACK		     	0xee    //uart ack
#define ERRORCMD		   	0xff    //错误

void mcu_wakeup_io_init(void);
void bri_ctrl_uart_init(void);
void wifi_start_init(void);

void send_bri_ctrl_info(unsigned char cmd, unsigned char dat); 

void wakeup_mcu_info(void);
#ifdef __cplusplus
}
#endif


