/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once

#define OnOff_CMD		   	     0x00    //开关命令
#define BRI_NOW_CMD		   0x01    //当前亮度
#define BRI_MAX_CMD		    0x02    //最大亮度
#define BRI_MIN_CMD		   	 0x03    //最小亮度
#define WIFI_RESET_CMD		                 0x05    //重置wifi
#define WIFI_INIT_END_CMD		          0x06    //wifi初始化结束
#define UART_SEND_ALLOW_CMD		 0xcc    //可以发送消息
#define UART_ACCEPT_END_CMD	      0xdd    //消息接受结束
#define UART_ACK_CMD	  0xee    //uart ack
#define ERROR_CMD		   	   0xff    //错误

int app_uart_send(unsigned char dat);
int app_uart_event_init(void);


