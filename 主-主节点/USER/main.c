#include "sys.h"
#include "delay.h"
#include "usart4.h"
#include "usart5.h"
#include "usart6.h"
#include "led.h"
#include "mq2.h"
#include "mq7.h"
#include "adc.h"
#include "battery.h"
#include "lora.h"
#include "gps.h"
#include "timer.h"
#include "timer2.h"
#include "timer3.h"
#include "rtc.h"
#include "mqtt4g.h"
// #include "i2c.h"
// #include "bmp280.h"
// #include "SHT2X.h"
#include <string.h>
#include <stdlib.h>
#define MQ2PreheatInterval 10 // MQ2预热时间间隔，单位为秒  至少为20秒
#define GPSTimeInterval 120	// GPS时间校时间隔，单位为秒  测试时2分钟一次，正式为5分钟一次
#define QueryTimeInterval 120	// 查询从节点数据时间间隔，单位为秒 测试时为5分钟，正式为30分钟
#define nMSnode 1

uint8_t EnableMaster = 1;		  	// 主从选择 1为主机，0为从机
u8 is_debug = 1;				  	// 是否调试模式，1为调试模式，0为正常模式
u8 query_node_data_max_times = 5; 	// 查询节点数据最大次数
u8 is_lora = 1;					  	// 是否启动lora模块
u8 is_biglora = 1;					// 是否启动大功率lora模块
u8 is_gps = 0;					  	// 是否启动GPS模块
u8 is_4g = 0;					  	// 是否启动4G模块,需要先启动lora和gps
u8 is_wind_sensor = 0;				// 是否启动风速风向传感器
u8 is_battery = 0;					// 是否启动电池电压检测
u8 is_calibration = 0;				// 是否启动风速风向校准
u8 query_windsensor[11] = {0x24, 0x41, 0x44, 0x2C, 0x30, 0x34, 0x2A, 0x36, 0x33, 0x0D, 0x0A};	// 向风速传感器请求数据
u8 cab_windsensor[11] = {0x24, 0x41, 0x5A, 0x2C, 0x30, 0x34, 0x2A, 0x37, 0x39, 0x0D, 0x0A};		// 风速风向校准
u8 MSNodeAddress[120] = {
        // 0x36, 0x49, 0x01, 0x00, 0x00, 0x00,
        // 0x28, 0x49, 0x01, 0x00, 0x00, 0x00,
        0x99, 0x99, 0x99, 0x99, 0x05, 0x50,
        // 0x27, 0x49, 0x01, 0x00, 0x00, 0x00,
        // 0x29, 0x49, 0x01, 0x00, 0x00, 0x00,
};									// 主从节点的地址

union data{
	float f;
	u8 ch[4];
} data_1;							// 用于转换数据格式： char -> float

// 函数申明
void UART4_Handler(void); 	// 处理串口4PC通信的内容
void LORA_Handler(void);  	// 处理LORA通信的内容
void GPS_Handler(void);	  	// 处理GPS通信的内容
// 用于解析数据
void DATA_Handler(float *temp, float *pres, float *humi, float *wind_sp, float *wind_dir, float *smoke, float *batt);																																	
void get_data(char*, u8*);		// 用于解析数据
int main(void)
{
	float co2 = 0; // 烟雾浓度
	float BMP280_P = 100000;
	float BMP280_T = 25.00;
	float SHT2X_T = 25.00; // temperature of SHT2X
	float SHT2X_H = 40.00; // humidity of SHT2X
	float battery = 0;	   // 电源电压
	float wind_speed = 0;
	float wind_direction = 0;
	float co_latest = 0; // 最新的CO浓度
	u8 time[3] = {0};
	u8 flag = 0;
	u8 i = 0;
	u8 MSrec[300];
	u8 MSrec_len = 0;

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); // 设置系统中断优先级分组2
	delay_init(168);								// 初始化延时函数
	uart4_init(9600);								// 初始化串口波特率为9600
	uart5_init(9600);								// 初始化串口波特率为9600，大功率Lora
	uart6_init(9600);								// 初始化串口波特率为9600，传感器
	LED_Init();										// 初始化LED

	MQ2_Init();
	MQ7_Init();
	Timer_mq2_Init(MQ2PreheatInterval); 	// 初始化MQ2定时器，每MQ2PreheatInterval秒状态位递增，用于MQ2的预热（20秒）和测量，MQ7共用一个定时器
	if (is_battery)	BATTERY_Init();
	if (is_4g)	mqtt4g_init();
	customRTC_Init();
	if (is_gps)	GPS_Init();
	Timer_Init(GPSTimeInterval); // 初始化定时器，TIM2用于读取gps时间给RTC校时, 间隔单位为秒，GPSTimeInterval*12为校时总周期
	
	if (is_lora)
	{
		is_lora_init = LORA_Init();
		Timer_querydata_Init(QueryTimeInterval); // 初始化定时器，TIM5用于查询从节点数据，间隔单位为秒
		if (is_debug) printf("LORA Init OK\r\n");
	}

	LED = 1;
	MQ2 = 1;
	MQ7 = 1;

	// TODO: 临时禁用I2C传感器
	// IIC_Init(); // I2C initialize
	// SHT2X_Init();
	// bmp280_uint();
	delay_ms(500);
	if (is_calibration)
	{
		if (is_debug) printf("calibrating windsensor...please ensure NO WIND\r\n");
		USART6_DATA(cab_windsensor, 11);
		if (is_debug) printf("calibration DONE\r\n");
		delay_ms(1000);
	}

	while (1)
	{
		u8 time[3] = {0};
		u8 flag = 0;
		u8 i, j;
		u8 current_addr[6] = {0};
		u8 query[3] = {0x11, 0x22, 0x33}; // 用于向子节点发送，查询数据
		u8 is_query_node_success = 0;	  // 是否成功查询到节点数据
		u8 data_str[300];				  // 用于存储发送给服务器的数据

		// 读取烟雾浓度
		if (flag_mq2_is_need_measure) // 需要测量时采集MQ2数据
		{
			printf("MQ2_Scan state=%d\r\n", mq2_state_count);
			co2 = MQ2_Scan();
			co_latest = MQ7_Scan();
			printf("CO2: %.2f, CO: %.2f\r\n", co2, co_latest);
			flag_mq2_is_need_measure = 0;
		}
		// if (is_debug) printf("bmp T: %f\r\n", bmp280_get_temperature());

		// I2C传感器执行
		// SHT2X_T = SHT2X_TEST_T(); // get temperature of SHT2X.
		// // if (is_debug) printf("raw T: %f\r\n", SHT2X_T);
		// SHT2X_T = 1.055 * SHT2X_T - 3.455;
		// SHT2X_T += 0.4;

		// BMP280_T = bmp280_get_temperature();
		// // if (is_debug) printf("bmp_t:%f\r\n", BMP280_T);
		// BMP280_P = bmp280_get_pressure(); // get pressure of bmp280.
		// BMP280_P = (BMP280_P - 1.19) / 100;

		// SHT2X_H = SHT2X_TEST_RH(); // get humidity of SHT2X.
		// // if (is_debug) printf("raw H: %f\r\n", SHT2X_H);
		// SHT2X_H = 0.976 * SHT2X_H + 6.551;

		// if (is_debug) printf("smoke: %f\r\n", co2);
		// if (is_debug) printf("pressure: %f\r\n", BMP280_P);
		// if (is_debug) printf("temperature: %f\r\n", SHT2X_T);
		// if (is_debug) printf("humidity: %f\r\n", SHT2X_H);

		// 读取电池电压
		if (is_battery)
		{
			battery = BATTERY_Scan();
			if (is_debug) printf("battery: %.2f%%\r\n", battery);
			if (is_debug) printf("\r\n");
		}

		//	如果lora模块未初始化成功，尝试重新初始化
		if (is_lora && !is_lora_init)
			is_lora_init = LORA_Network_Init();

		// 是否需要更新时间
		// is_need_update_time =1;	// 调试用，每次都更新时间
		if (is_gps && is_need_update_time)
		{
			// 更新RTC时间需要GPS，在GPS未启动的时候不进行时间更新
			RTC_update_device_time();
			is_need_update_time = 0;
		}

		// if (UART4_RX_STA & 0x8000)
		// 	UART4_Handler(); // 处理串口4PC通信的内容

		if (is_lora && check_LORA_Receive())
			LORA_Handler(); // 处理LORA通信的内容

		// TODO: 这一步操作实际没有效果，校时为按需校时
		if (is_gps && check_GPS_Receive())
			GPS_Handler(); // 处理GPS通信的内容

		// 向风速风向传感器串口要数据
		if (is_wind_sensor)
		{
			printf("query windsensor...\r\n");
			USART6_DATA(query_windsensor, 11);
			delay_ms(3000);
			if (USART6_RX_STA&0x8000)
			{
				DATA_Handler(&SHT2X_T, &BMP280_P, &SHT2X_H, &wind_speed, &wind_direction, &co2, &battery);
			}
		}
		

		if (is_lora & is_need_query_data)
		{
			// 向从节点要数据
			if (is_debug) printf("query data...\r\n");
			for (i = 0; i < SubNodeSet.nNode; i++)
			{
				for (j = 0; j < 6; j++)
					current_addr[j] = SubNodeAddress[i * 6 + j];
				LORA_DATA_Transfer(query, 3, current_addr);
				if (is_debug) printf("query sent...node: %d\r\n", i);
				delay_ms(5000);
				for (j = 0; j < query_node_data_max_times; j++)
				{
					if (check_LORA_Receive())
					{
						if (is_debug) printf("data received!\r\n");
						LORA_Handler(); // 处理LORA通信的内容
						is_query_node_success = 1;
						break;
					}
					delay_ms(2000);
				}
				if (is_query_node_success)
				{
					continue;
				}
				else
				{
					if (is_debug) printf("query data again... node: %d\r\n", i);
					LORA_DATA_Transfer(query, 3, current_addr);
					if (is_debug) printf("query sent...node: %d\r\n", i);
					delay_ms(5000);
					for (j = 0; j < query_node_data_max_times; j++)
					{
						if (check_LORA_Receive())
						{
							if (is_debug) printf("data received!\r\n");
							LORA_Handler(); // 处理LORA通信的内容
							is_query_node_success = 1;
							break;
						}
						delay_ms(2000);
					}
				}
				if (!is_query_node_success)
					if (is_debug) printf("query data failed... node: %d\r\n", i);
				is_query_node_success = 0;
			}
		}

		if (is_4g)
		{
			// 主节点发送自身数据
			// 打印时间和传感器数据
			RTC_Get_Time(time);
			sprintf(data_str, "address: 999999990551\r\ntime: %02d:%02d:%02d\r\ntemperature: %.2f\r\npressure: %.2f\r\nhumidity: %.2f\r\nwind_speed: %.2f\r\nwind_direction: %.2f\r\nsmoke: %.2f\r\nbattery: %.2f%%\r\nisTimeTrue: %d\r\n",
					time[0], time[1], time[2], SHT2X_T, BMP280_P, SHT2X_H, wind_speed, wind_direction, co2, battery, RTC_check_device_time());
			if (is_debug) puts(data_str);
			if (is_debug) printf("sending main node data to server...\r\n");
			mqtt4g_send(data_str, strlen(data_str));
			if (is_debug) printf("data sent...\r\n");
		}

		if (is_biglora)
		{
			if (is_debug) printf("query M-S node data...\r\n");
			for (i = 0; i < nMSnode; i++)
			{
				for (j = 0; j < 6; j++)
					current_addr[j] = MSNodeAddress[i * 6 + j];
				USART5_DATA(current_addr, 6);
				if (is_debug) printf("query sent...node: %d\r\n", i);
				delay_ms(5000);
				for (j = 0; j < query_node_data_max_times; j++)
				{
					if (USART5_RX_STA & 0x8000)				// 如果大功率LORA模块收到数据
					{
						if (is_debug) printf("data received!\r\n");
						USART5_Receive_Data(MSrec, &MSrec_len);
						is_query_node_success = 1;
						mqtt4g_send(MSrec, strlen(MSrec));
						MSrec_len = 0;
						if (is_debug) printf("MS data sent...\r\n");
						break;
					}
					delay_ms(2000);
				}
				if (is_query_node_success)
				{
					continue;
				}
				else
				{
					if (is_debug) printf("query data again... node: %d\r\n", i);
					USART5_DATA(current_addr, 6);
					if (is_debug) printf("query sent...node: %d\r\n", i);
					delay_ms(5000);
					for (j = 0; j < query_node_data_max_times; j++)
					{
						if (USART5_RX_STA & 0x8000)				// 如果大功率LORA模块收到数据
						{
							if (is_debug) printf("data received!\r\n");
							USART5_Receive_Data(MSrec, &MSrec_len);
							is_query_node_success = 1;
							mqtt4g_send(MSrec, strlen(MSrec));
							MSrec_len = 0;
							if (is_debug) printf("MS data sent...\r\n");
							break;
						}
						delay_ms(2000);
					}
				}
				if (!is_query_node_success)
					if (is_debug) printf("query data failed... node: %d\r\n", i);
				is_query_node_success = 0;
			}
		}

		delay_ms(1000);
		DebugLed(); // LED闪烁说明程序正常运行
	}
}

// 处理串口4PC通信的内容
void UART4_Handler(void)
{
	// 支持调试指令（结尾需要\r\n）
	// 00,00 设置LORA模块时间
	// 00,01 一键初始化LORA模块
	// 00,02 查询网络状态
	// 00,03 启动数据透传，向从节点发送传感器数据请求
	// 00,04 查询从节点状态
	// 00,05 网络数据清除
	// 00,06 全网点名
	// 00,07 启动组网
	// 01,00 查询GPS时间
	// 01,01 读取rtc时间
	// 01,02 设置rtc时间

	u8 t, len;
	// 接收数据
	len = UART4_RX_STA & 0x3fff; // 得到此次接收到的数据长度
	// 接收数据长度为0时，直接清空并返回
	if (len == 0)
	{
		UART4_RX_STA = 0;
		return;
	}
	// 检测是否是LORA的指令，如果是再发给lora模块
	if (UART4_RX_BUF[0] == 0x6C && UART4_RX_BUF[len - 1] == 0x16)
	{
		if (is_debug) printf("LORA CMD\r\n");
		LORA_Send(UART4_RX_BUF, len);
	}

	else
	{
		// 测试发送
		if (is_debug) printf("UART4 CMD: ");
		for (t = 0; t < len; t++)
		{
			// USART_SendData(USART1, USART_RX_BUF[t]);         //向串口1发送数据
			// while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);//等待发送结束
			if (is_debug) printf("%02x ", UART4_RX_BUF[t]);
		}
		if (is_debug) printf("\r\nlen=%d", len);
		if (is_debug) printf("\r\n"); // 插入换行

		// 当输入为00,00时，设置LORA模块时间
		if (UART4_RX_BUF[0] == 0x00 && UART4_RX_BUF[1] == 0x00)
		{
			u8 time[3] = {17, 33, 11};
			LORA_Init_Time(time);
		}
		// 当输入为00,01时，一键初始化LORA模块
		else if (UART4_RX_BUF[0] == 0x00 && UART4_RX_BUF[1] == 0x01)
		{
			LORA_Network_Init();
		}
		// 当输入为00,02时，查询网络状态
		else if (UART4_RX_BUF[0] == 0x00 && UART4_RX_BUF[1] == 0x02)
		{
			u8 time[3] = {0};
			u8 flag = 0;
			flag = LORA_Query_Network_Status(SelfAddress, time, is_debug);
			if (flag)
			{
				if (is_debug) printf("LORA Network Status: OK\r\n");
			}
			else
			{
				if (is_debug) printf("LORA Network Status: ERROR\r\n");
			}
		}
		// 当输入00,03时，启动数据透传，向从节点发送传感器数据请求
		else if (UART4_RX_BUF[0] == 0x00 && UART4_RX_BUF[1] == 0x03)
		{
			u8 transfer_buf[6] = {0x00, 0x03, 0x01, 0x02, 0x03, 0x04};
			u8 tmp_addr[6] = {0x05, 0x49, 0x01, 0x00, 0x00, 0x00};
			u8 receive_buf[50];
			u8 receive_len = 0;
			LORA_DATA_Transfer(
				transfer_buf,
				6,
				tmp_addr);
			// 打印接收到的数据
			for (t = 0; t < receive_len; t++)
			{
				USART_SendData(UART4, receive_buf[t]); // 向串口4发送数据
				while (USART_GetFlagStatus(UART4, USART_FLAG_TC) != SET)
					; // 等待发送结束
			}
		}
		// 当输入00,04时，查询从节点状态
		else if (UART4_RX_BUF[0] == 0x00 && UART4_RX_BUF[1] == 0x04)
		{
			u8 flag = 0;
			flag = LORA_Query_Slave_Node_Status(1);
			if (flag == 1)
			{
				if (is_debug) printf("Slave Node Online\r\n");
			}
			else
			{
				if (is_debug) printf("Slave Node Offline\r\n");
			}
		}
		// 当输入00,05时，网络数据清除
		else if (UART4_RX_BUF[0] == 0x00 && UART4_RX_BUF[1] == 0x05)
		{
			u8 flag = 0;
			flag = LORA_Network_Clear();
			if (flag == 1)
			{
				if (is_debug) printf("Network Data Clear Success\r\n");
			}
			else
			{
				if (is_debug) printf("Network Data Clear Fail\r\n");
			}
		}
		// 当输入00, 06时，全网点名
		else if (UART4_RX_BUF[0] == 0x00 && UART4_RX_BUF[1] == 0x06)
		{
			u8 flag = 0;
			flag = LORA_Network_Naming();
			if (flag == 1)
			{
				if (is_debug) printf("Network Naming Success\r\n");
			}
			else
			{
				if (is_debug) printf("Network Naming Fail\r\n");
			}
		}
		// 当输入00,07时，启动组网
		else if (UART4_RX_BUF[0] == 0x00 && UART4_RX_BUF[1] == 0x07)
		{
			u8 flag = 0;
			flag = LORA_Network_Start();
			if (flag == 1)
			{
				delay_ms(5000);
				if (is_debug) printf("Network Start Success\r\n");
			}
			else
			{
				if (is_debug) printf("Network Start Fail\r\n");
			}
		}
		// 当输入01,00时，读取gps时间
		else if (UART4_RX_BUF[0] == 0x01 && UART4_RX_BUF[1] == 0x00)
		{
			u8 time[3] = {0};
			u8 flag = 0;
			flag = GPS_get_time(time);
			if (flag == 1)
			{
				if (is_debug) printf("GPS Time: %d:%d:%d\r\n", time[0], time[1], time[2]);
			}
			else
			{
				if (is_debug) printf("GPS Time Read Fail\r\n");
			}
		}
		// 当输入01,01时，读取RTC时钟
		else if (UART4_RX_BUF[0] == 0x01 && UART4_RX_BUF[1] == 0x01)
		{
			u8 time[3] = {0};
			RTC_Get_Time(time);
			if (is_debug) printf("RTC Time: %d:%d:%d\r\n", time[0], time[1], time[2]);
		}
		// 当输入01,02时，设置RTC时钟
		else if (UART4_RX_BUF[0] == 0x01 && UART4_RX_BUF[1] == 0x02)
		{
			u8 time[3] = {0};
			time[0] = UART4_RX_BUF[2];
			time[1] = UART4_RX_BUF[3];
			time[2] = UART4_RX_BUF[4];
			RTC_Set_Time(time[0], time[1], time[2]);
			if (is_debug) printf("RTC Time Set Success\r\n");
		}
	}
	UART4_RX_STA = 0; // 清空接收缓存长度
}

// LORA模块通信处理
void LORA_Handler(void)
{
	u8 t;
	u8 temp_rec[200];
	u8 rec_len = 0;
	// 接收数据
	LORA_Receive(temp_rec, &rec_len);
	if (rec_len == 0)
	{
		return;
	}
	// 测试打印数据
	for (t = 0; t < rec_len; t++)
	{
		if (is_debug) printf("%02X ", temp_rec[t]);
		// USART_SendData(UART4, temp_rec[t]); // 向串口4发送数据
		// while (USART_GetFlagStatus(UART4, USART_FLAG_TC) != SET); // 等待发送结束
	}
	if (is_debug) printf("\r\n");
	// 检测是否是完整的LORA数据包
	if (temp_rec[0] == 0x6C && temp_rec[2] == 0x09 && temp_rec[rec_len - 1] == 0x16)
	{
		// 检测接收数据是否是来自从节点的数据透传
		if (temp_rec[3] == 0x03 && temp_rec[4] == 0x05)
		{
			u8 flag;
			if (is_debug) printf("\r\n");
			flag = LORA_Receive_Data_Analysis(temp_rec, rec_len);		// 在这一步中将收集到的从节点数据通过4G上传
			if (flag == 1)
			{
				if (is_debug) printf("LORA Receive Data Analysis Success\r\n");
			}
			else
			{
				if (is_debug) printf("LORA Receive Data Analysis Fail\r\n");
			}
		}
	}
}

// GPS模块通信处理
void GPS_Handler(void)
{
	u8 t;
	u8 temp_rec[300];
	u8 rec_len = 0;
	u8 time[3] = {0};
	// 接收数据
	GPS_Receive(temp_rec, &rec_len);
	if (rec_len == 0)
	{
		return;
	}
	// 测试打印数据
	
	// for (t = 0; t < rec_len; t++)
	// {
	// 	USART_SendData(UART4, temp_rec[t]); // 向串口4发送数据
	// 	while (USART_GetFlagStatus(UART4, USART_FLAG_TC) != SET); // 等待发送结束
	// }
	
	// 检测是否是完整的GPS数据包
	if (temp_rec[3] == 0x47 && temp_rec[4] == 0x47 && temp_rec[5] == 0x41)
	{
		// 时
		time[0] = 10 * (temp_rec[7] - 48) + (temp_rec[8] - 48); // UTC时间
		time[0] += 8;											// 北京时间
		if (time[0] >= 24)
			time[0] -= 24;
		time[0] = (time[0] / 10) * 16 + time[0] % 10; // 转换为16进制
		// set_lora_clk[8] = time[0];
		// 分
		time[1] = 16 * (temp_rec[9] - 48) + (temp_rec[10] - 48);
		// set_lora_clk[9] = time[1];
		// 秒
		time[2] = 16 * (temp_rec[11] - 48) + (temp_rec[12] - 48);

		// 打印时间
		// if (is_debug) printf("GPS Time: %d:%d:%d\r\n", time[0], time[1], time[2]);
		//  u8 flag;
		//  if (is_debug) printf("\r\n");
		//  flag = GPS_Receive_Data_Analysis(temp_rec, rec_len);
		//  if (flag == 1)
		//  {
		//  	if (is_debug) printf("GPS Receive Data Analysis Success\r\n");
		//  }
		//  else
		//  {
		//  	if (is_debug) printf("GPS Receive Data Analysis Fail\r\n");
		//  }
	}
	// if (is_debug) printf("\r\n");//插入换行
	//  if (is_debug) printf("Receive %d,%d, len=%d", temp_rec[0],temp_rec[1],rec_len);
}

//	数据模块处理
void DATA_Handler(float *temp, float *pres, float *humi, float *wind_sp, float *wind_dir, float *smoke, float *batt)
{
	u8 i;
	u8 temp_rec[300];
	u8 rec_len=0;
	u8 wind_speed[4] = {0};
    u8 wind_direction[4] = {0};
    u8 temperature[4] = {0};
    u8 pressure[4] = {0};
    u8 humidity[4] = {0};
    u8 _smoke[4] = {0};
	u8 battery[4] = {0};
	u8 data_u8[28] = {0};				// 用于存储数据

	if (is_debug) printf("get windsensor data!\r\n");
	USART6_Receive_Data(temp_rec, &rec_len);
	// puts(temp_rec);
	if (is_debug) printf("\r\n");
	if (is_debug) printf("data analyzing...\r\n");
	get_data(temp_rec, data_u8);
	
	if (is_debug) 
	{
	printf("windsensor query finished...\r\n");
	for(i=0;i<28;i++)
		printf("%02x ", data_u8[i]);
	printf("\r\n");
	}

	for (i = 0; i < 4; i = i + 1)
    {	//温 压 湿 风速风向
        temperature[i] = data_u8[i];
        pressure[i] = data_u8[4 + i];
        humidity[i] = data_u8[8 + i];
        wind_speed[i] = data_u8[12 + i];
        wind_direction[i] = data_u8[16 + i];
        _smoke[i] = data_u8[20 + i];
		battery[i] = data_u8[24 + i];
    }
	*temp = *(float *)(temperature);
	*pres = *(float *)(pressure);
	*humi = *(float *)(humidity);
	*wind_sp = *(float *)(wind_speed);
	*wind_dir = *(float *)(wind_direction);
	*smoke = *(float *)(_smoke);
	*batt = *(float *)(battery);
}

void get_data(char* data_str, u8* data_u8)
{
	u8 i = 0, flag = 0, j = 0, k = 0, t = 0;
	float data_float;
	char float_str[20];
	char* substr;
	substr = strstr(data_str, "\"T\"");
	
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
