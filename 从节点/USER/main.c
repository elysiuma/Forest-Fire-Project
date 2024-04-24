#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "usart2.h"
#include "led.h"
#include "mq2.h"
#include "mq7.h"
#include "adc.h"
#include "bmp280.h"
#include "SHT2X.h"
#include "battery.h"
#include "lora.h"
#include "timer.h"
#include "usart3.h"
#include <string.h>
#include <stdlib.h>

#define MQ2PreheatInterval 10 // MQ2预热时间间隔，单位为秒  至少为20秒
#define NumOfData 6 		// 数据个数, 温度0 气压1 湿度2 烟雾3 一氧化碳4 电量5

uint8_t EnableMaster=0;				//主从选择 1为主机，0为从机	(FIX:很久没用了，基本无效)
u8 is_battery = 1;					// 是否启动电池电压检测
u8 is_debug = 1;					// 是否启动调试模式

u8 temp_rec[50];
u8 rec_len=0;
u8 query_rec[50];
u8 query_rec_len=0;
//float data[6]={0};	//风速data[0] 风向1 温度2 气压3 湿度4 烟雾5 电量6
u8 data_u8[NumOfData*4] = {0};
u8 query_windsensor[11] = {0x24, 0x41, 0x44, 0x2C, 0x30, 0x34, 0x2A, 0x36, 0x33, 0x0D, 0x0A};	// 风速风向请求
u8 cab_windsensor[11] = {0x24, 0x41, 0x5A, 0x2C, 0x30, 0x34, 0x2A, 0x37, 0x39, 0x0D, 0x0A};		// 风速风向校准
// void get_data(char*);
float u8_to_float(u8*);
void float_to_u8(float, u8*);

int main(void)
{ 
	float co2;	//烟雾浓度
	float battery=0;	//电源电压
	float co_latest = 0; // 最新的CO浓度
	float BMP280_P = 100000; // pressure of BMP280
	float BMP280_T = 25.00;	// temperature of BMP280，用于校正BMP280
	float SHT2X_T = 25.00; // temperature of SHT2X
	float SHT2X_H = 40.00; // humidity of SHT2X
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置系统中断优先级分组2
	delay_init(168);    //初始化延时函数
	uart_init(9600);	//初始化串口波特率为115200
	uart3_init(9600);
	LED_Init();					//初始化LED 
	Adc_Init();				//初始化ADC

	MQ2_Init();
	MQ7_Init();
	if(is_battery) BATTERY_Init();
	LORA_Init();
	Timer_mq2_Init(MQ2PreheatInterval);		// 初始化MQ2定时器，每MQ2PreheatInterval秒状态位递增，用于MQ2的预热（20秒）和测量，MQ7共用一个定时器
	LED = 1;
	MQ2 = 1;
	MQ7 = 1;

	// i2c温湿度传感器初始化
	IIC_Init(); // I2C initialize
	SHT2X_Init();
	bmp280_uint();

	delay_ms(500);
	
	
	while(1)
	{ 
		u8 len, t, i;
		
		// 读取烟雾浓度
		if (flag_mq2_is_need_measure) // 需要测量时采集MQ2数据
		{
			printf("MQ2_Scan state=%d\r\n", mq2_state_count);
			co2 = MQ2_Scan();
			co_latest = MQ7_Scan();
			printf("CO2: %.2f, CO: %.2f\r\n", co2, co_latest);
			flag_mq2_is_need_measure = 0;
		}
		
		// I2C传感器执行
		SHT2X_T = SHT2X_TEST_T(); // get temperature of SHT2X.
		// if (is_debug) printf("raw T: %f\r\n", SHT2X_T);
		SHT2X_T = 1.055 * SHT2X_T - 3.455;
		SHT2X_T += 0.4;
		BMP280_T = bmp280_get_temperature();
		// if (is_debug) printf("bmp_t:%f\r\n", BMP280_T);
		BMP280_P = bmp280_get_pressure(); // get pressure of bmp280.
		BMP280_P = (BMP280_P - 1.19) / 100;
		SHT2X_H = SHT2X_TEST_RH(); // get humidity of SHT2X.
		// if (is_debug) printf("raw H: %f\r\n", SHT2X_H);
		SHT2X_H = 0.976 * SHT2X_H + 6.551;

		if (is_debug) printf("pressure: %f\r\n", BMP280_P);
		if (is_debug) printf("temperature: %f\r\n", SHT2X_T);
		if (is_debug) printf("humidity: %f\r\n", SHT2X_H);

		if(is_battery)	// 电池电压检测
		{
			battery = BATTERY_Scan();
			printf("battery: %.2f%%\r\n", battery);
		}

		/*
		if (is_need_send_lora_data)
		{
			LORA_Send(data_u8, 24);
			is_need_send_lora_data = 0;
		}
		*/
		
		// 串口查询风速风向
		// printf("query windsensor...\r\n");
		// USART3_DATA(query_windsensor, 11);
		// delay_ms(1000);
		// if (USART3_RX_CNT&0x8000)
		// {
		// 	printf("get windsensor data!\r\n");
		// 	USART3_Receive_Data(temp_rec, &rec_len);
		// 	puts(temp_rec);
		// 	printf("\r\n");
		// 	printf("data analyzing...\r\n");
		// 	get_data(temp_rec);
		// }
		// else
		// 	printf("no data!!!\r\n");
		// printf("windsensor query finished...\r\n");
		// for(i=0;i<28;i++)
		// 	printf("%02x ", data_u8[i]);
		// printf("\r\n");
		// rec_len=0;
		
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
				// 将float数据转成u8列表
				float_to_u8(SHT2X_T, data_u8);
				float_to_u8(BMP280_P, data_u8+4);
				float_to_u8(SHT2X_H, data_u8+8);
				float_to_u8(co2, data_u8+12);
				float_to_u8(co_latest, data_u8+16);
				float_to_u8(battery, data_u8+20);
				LORA_Send(data_u8, NumOfData*4);
				for (i=0; i<NumOfData*4; i++)
					printf("%02x ", data_u8[i]);
				printf("data reported!\r\n");
				query_rec_len=0;
			}
		}
		
		delay_ms(1000);
		DebugLed();	//LED闪烁说明程序正常运行
		printf("\r\n");
	}
}

float u8_to_float(u8* data)
{
	union data{	
		float f;
		u8 ch[4];
	} data_1;		// 联合体变量
	float data_float;

	data_1.ch[0] = data[0];
	data_1.ch[1] = data[1];
	data_1.ch[2] = data[2];
	data_1.ch[3] = data[3];
	data_float = data_1.f;
	return data_float;
}

void float_to_u8(float data, u8* data_u8)
{
	union data{	
		float f;
		u8 ch[4];
	} data_1;		// 联合体变量

	data_1.f = data;
	data_u8[0] = data_1.ch[0];
	data_u8[1] = data_1.ch[1];
	data_u8[2] = data_1.ch[2];
	data_u8[3] = data_1.ch[3];
}


// // 解析串口风速风向数据
// void get_data(char* data_str)
// {
// 	u8 i = 0, flag = 0, j = 0, k = 0, t = 0;
// 	float data_float;
// 	char float_str[20];
// 	char* substr = strstr(data_str, "\"T\"");
	
// 	for(k=0;k<5;k++)
// 	{
// 		while(1)
// 		{	
// 			if(substr[i] == ',' || substr[i] == '}') {i++;break;}
// 			if(flag)
// 			{
// 				float_str[j] = substr[i];
// 				j++;
// 			}
// 			if(substr[i] == ':') flag = 1;
// 			i++;
// 		}
// 		float_str[j] = '\0';
// 		//puts(float_str);
// 		//printf("\r\n");
// 		flag=0;
// 		j=0;
// 		data_float = atof(float_str);
// 		data_1.f = data_float;
// 		for(t=0;t<4;t++)
// 			data_u8[k*4+t] = data_1.ch[t];
// 	}
// }
