#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "usart2.h"
#include "led.h"
#include "mq2.h"
#include "adc.h"
#include "battery.h"
#include "lora.h"
#include "timer.h"
#include "usart3.h"
#include <string.h>
#include <stdlib.h>

uint8_t EnableMaster=0;//主从选择 1为主机，0为从机

float co2;	//烟雾浓度
float battery;	//电源电压
u8 temp_rec[50];
u8 rec_len=0;
u8 query_rec[50];
u8 query_rec_len=0;
float co2_queue[10] = {0}; //最近10次的烟雾浓度
u8 co2_idx = 0;
//float data[6]={0};	//风速data[0] 风向1 温度2 气压3 湿度4 烟雾5 电量6
u8 data_u8[28] = {0};
union data{
	float f;
	u8 ch[4];
} data_1;
u8 query_windsensor[11] = {0x24, 0x41, 0x44, 0x2C, 0x30, 0x34, 0x2A, 0x36, 0x33, 0x0D, 0x0A};	// 风速风向请求
u8 cab_windsensor[11] = {0x24, 0x41, 0x5A, 0x2C, 0x30, 0x34, 0x2A, 0x37, 0x39, 0x0D, 0x0A};		// 风速风向校准
void get_data(char*);

int main(void)
{ 
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置系统中断优先级分组2
	delay_init(168);    //初始化延时函数
	uart_init(9600);	//初始化串口波特率为115200
	uart3_init(9600);
	LED_Init();					//初始化LED 
	//Adc_Init();				//初始化ADC

	MQ2_Init();
	BATTERY_Init();
	LORA_Init();
	Timer_Init();
	LED = 1;
	MQ2 = 1;
	delay_ms(500);
	
	
	while(1)
	{ 
		u8 len, t, i;
		float data_f;
		float co2_sum=0, co2;
		
		if(MQ2)	//工作时才采数据
		{
			co2 = MQ2_Scan();
			//printf("co2: %f\r\n", co2);
		
			co2_queue[co2_idx] = co2;
			co2_idx++;
			if(co2_idx==10) co2_idx=0;
			for(i = 0; i < 10; i++)
			{
				co2_sum = co2_sum + co2_queue[i];
				//printf("co2 queue: %f\r\n", co2_queue[i]);
			}
			co2 = co2_sum / 10;
			data_1.f = co2;
			//printf("ave co2 queue: %f\r\n", data_1.f);
			//data_1.f = 123.53;
			for(i = 0; i < 4; i++)
			{
				data_u8[20+i] = data_1.ch[i]; //共用体中ch与真实的16进制是倒过来的
			}
		}
		
		battery = BATTERY_Scan();
		printf("battery: %.2f%%\r\n", battery);
		printf("smoke: %f\r\n", co2);
		data_1.f = battery;
		for(i=0;i<4;i++)
			data_u8[i+24] = data_1.ch[i];
		/*
		if (is_need_send_lora_data)
		{
			LORA_Send(data_u8, 24);
			is_need_send_lora_data = 0;
		}
		*/
		
		printf("query windsensor...\r\n");
		USART3_DATA(query_windsensor, 11);
		delay_ms(1000);
		if (USART3_RX_CNT&0x8000)
		{
			printf("get windsensor data!\r\n");
			USART3_Receive_Data(temp_rec, &rec_len);
			puts(temp_rec);
			printf("\r\n");
			printf("data analyzing...\r\n");
			get_data(temp_rec);
		}
		else
			printf("no data!!!\r\n");
		printf("windsensor query finished...\r\n");
		for(i=0;i<28;i++)
			printf("%02x ", data_u8[i]);
		printf("\r\n");
		rec_len=0;
		
		if(USART_RX_STA&0x8000) 
		{         
			len=USART_RX_STA&0x3fff;//得到此次接收到的数据长度		
			if (USART_RX_BUF[0]==0x6C && USART_RX_BUF[len-1]==0x16)
			{
				printf("LORA CMD, len=%d\r\n", len);
				LORA_Send(USART_RX_BUF, len);
			}
			else
			{
				// 测试发送
				printf("USART CMD: ");
				for(t=0;t<len;t++)
				{
					// USART_SendData(USART1, USART_RX_BUF[t]);         //向串口1发送数据
					// while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);//等待发送结束
					printf("%02x ", USART_RX_BUF[t]);
				}
				printf("\r\nlen=%d\r\n",len);
				
				printf("\r\n");//插入换行 

				// 当输入为00,01时，一键初始化LORA模块
				if (USART_RX_BUF[0]==0x00 && USART_RX_BUF[1]==0x01)
				{
					LORA_Query_Slave_Node_Status();
				}
			}
			USART_RX_STA=0; 
		}
		
		//检查是否收到主模块的数据请求11 22 33
		if (check_LORA_Receive())
		{
			LORA_Receive(query_rec, &query_rec_len);
			//printf("lora msg: ");
			//for(i=0;i<rec_len;i++)
				//printf("%02x ", temp_rec[i]);
		
			if (query_rec_len==3 & query_rec[0] == 0x11 & query_rec[1] == 0x22 & query_rec[2] == 0x33)
			{
				printf("data reporting...\r\n");
				LORA_Send(data_u8, 28);
				printf("data reported...\r\n");
				query_rec_len=0;
			}
		}
		
		delay_ms(1000);
		DebugLed();	//LED闪烁说明程序正常运行
	}
}


void get_data(char* data_str)
{
	u8 i = 0, flag = 0, j = 0, k = 0, t = 0;
	float data_float;
	char float_str[20];
	char* substr = strstr(data_str, "\"T\"");
	
	for(k=0;k<5;k++)
	{
		while(1)
		{	
			if(substr[i] == ',' || substr[i] == '}') {i++;break;}
			if(flag)
			{
				float_str[j] = substr[i];
				j++;
			}
			if(substr[i] == ':') flag = 1;
			i++;
		}
		float_str[j] = '\0';
		//puts(float_str);
		//printf("\r\n");
		flag=0;
		j=0;
		data_float = atof(float_str);
		data_1.f = data_float;
		for(t=0;t<4;t++)
			data_u8[k*4+t] = data_1.ch[t];
	}
}
