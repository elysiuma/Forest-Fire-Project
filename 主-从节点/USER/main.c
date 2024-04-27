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
#include "tool.h"
#include "wind.h"
// #include "mqtt4g.h"
// #include "i2c.h"
// #include "bmp280.h"
// #include "SHT2X.h"
#include <string.h>
#include <stdlib.h>
#define MQ2PreheatInterval			20 				// MQ2预热时间间隔，单位为秒  至少为20秒 （传感器采集数据共用这个）
#define GPSTimeInterval 			120				// GPS时间校时间隔，单位为秒  测试时2分钟一次，正式为5分钟一次
#define QueryTimeInterval			120				// 查询从节点数据时间间隔，单位为秒 测试时为5分钟，正式为30分钟
#define is_debug					1				// 是否调试模式，1为调试模式，0为正常模式
#define query_node_data_max_times	5 				// 查询节点数据最大次数
#define is_lora						1				// 是否启动lora模块
#define is_gps						1				// 是否启动GPS模块
#define is_battery					1				// 是否启动电池电压检测
#define is_wind_sensor				1				// 是否启动风速风向传感器
#define is_calibration				0				// 是否启动风速风向校准 1为校准，0为不校准

uint8_t EnableMaster = 1;		  // 主从选择 1为主机，0为从机

u8 data_str[300];				  // 用于存储发送给服务器的单条数据
u8 temp_buf[20];
static u8 all_data_str[3000] = {0};			  // 用于存储发送给服务器的所有数据(按最大10个节点算)

// u8 is_4g = 0;					  // 是否启动4G模块,需要先启动lora和gps

union data{
	float f;
	u8 ch[4];
} data_1;							// 用于转换数据格式： char -> float

// 函数申明
void UART4_Handler(void); // 处理串口4PC通信的内容
void LORA_Handler(void);  // 处理LORA通信的内容
void GPS_Handler(void);	  // 处理GPS通信的内容

int main(void)
{
	float co2 = 0; // 烟雾浓度
	float co_latest = 0; // 最新的CO浓度
	float BMP280_P = 100000;
	float BMP280_T = 25.00;
	float SHT2X_T = 25.00; // temperature of SHT2X
	float SHT2X_H = 40.00; // humidity of SHT2X
	float battery = 0;	   // 电源电压
	float wind_speed = 0;
	float wind_direction = 0;
	u8 time[3] = {0};
	u8 flag = 0;
	u8 len = 0;
	
	
	

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); // 设置系统中断优先级分组2
	delay_init(168);								// 初始化延时函数
	uart4_init(9600);								// 初始化串口波特率为9600
	uart5_init(9600);								// 初始化串口波特率为9600
	if(is_wind_sensor) Wind_Init();								// 初始化串口波特率为9600
	LED_Init();										// 初始化LED

	MQ2_Init();
	MQ7_Init();
	Timer_mq2_Init(MQ2PreheatInterval);		// 初始化MQ2定时器，每MQ2PreheatInterval秒状态位递增，用于MQ2的预热（20秒）和测量，MQ7共用一个定时器
	if (is_battery) BATTERY_Init();
	// if (is_4g) mqtt4g_init();
	customRTC_Init();
	if (is_gps) GPS_Init();
	Timer_Init(GPSTimeInterval); // 初始化定时器，TIM2用于读取gps时间给RTC校时, 间隔单位为秒，interval*12为校时总周期
	if (is_lora)
	{
		is_lora_init = LORA_Init();
		Timer_querydata_Init(QueryTimeInterval); // 初始化定时器，TIM5用于查询从节点数据，间隔单位为秒
		printf("LORA Init OK\r\n");
	}

	LED = 1;
	MQ2 = 1;
	MQ7 = 1;

	// I2C初始化
	// IIC_Init(); // I2C initialize
	// SHT2X_Init();
	// bmp280_uint();
	delay_ms(500);
	printf("System Init OK\r\n");

	while (1)
	{
		u8 i, j;
		u8 current_addr[6] = {0};
		u8 is_query_node_success = 0;	  // 是否成功查询到节点数据
		

		// 读取烟雾浓度最近10次平均数据
		if (flag_mq2_is_need_measure) // 需要测量时采集MQ2数据
		{
			printf("MQ2_Scan state=%d\r\n", mq2_state_count);
			co2 = MQ2_Scan();
			co_latest = MQ7_Scan();
			printf("CO2: %.2f, CO: %.2f\r\n", co2, co_latest);
			flag_mq2_is_need_measure = 0;
		}
		// printf("bmp T: %f\r\n", bmp280_get_temperature());

		// SHT2X_T = SHT2X_TEST_T(); // get temperature of SHT2X.
		// // printf("raw T: %f\r\n", SHT2X_T);
		// SHT2X_T = 1.055 * SHT2X_T - 3.455;
		// SHT2X_T += 0.4;

		// BMP280_T = bmp280_get_temperature();
		// // printf("bmp_t:%f\r\n", BMP280_T);
		// BMP280_P = bmp280_get_pressure(); // get pressure of bmp280.
		// BMP280_P = (BMP280_P - 1.19) / 100;

		// SHT2X_H = SHT2X_TEST_RH(); // get humidity of SHT2X.
		// // printf("raw H: %f\r\n", SHT2X_H);
		// SHT2X_H = 0.976 * SHT2X_H + 6.551;

		// printf("smoke: %f\r\n", co2);
		// printf("pressure: %f\r\n", BMP280_P);
		// printf("temperature: %f\r\n", SHT2X_T);
		// printf("humidity: %f\r\n", SHT2X_H);

		// 读取电池电压
		if(is_battery && flag_battery_is_need_measure)
		{
			battery = BATTERY_Scan();
			printf("battery: %.2f%%\r\n", battery);
			printf("\r\n");
			flag_battery_is_need_measure = 0;
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

		if (UART4_RX_STA & 0x8000)
			UART4_Handler(); // 处理串口4PC通信的内容

		if (is_lora && check_LORA_Receive())
			LORA_Handler(); // 处理LORA通信的内容

		if (is_gps && check_GPS_Receive())
			GPS_Handler(); // 处理GPS通信的内容

		// 向风速风向传感器串口要数据
		if (is_wind_sensor)
		{
			if (flag_wind_is_need_measure)	// 需要测量时采集风速风向数据
			{
				printf("Send query windsensor...\r\n");
				Wind_query();
				flag_wind_is_need_measure = 0;
			}
			
			if (check_Wind_receive())	// 检查是否接收到风速风向数据
			{
				Wind_analysis(&SHT2X_T, &BMP280_P, &SHT2X_H, &wind_speed, &wind_direction);
				printf("temperature: %.2f, pressure: %.2f, humidity: %.2f, wind_speed: %.2f, wind_direction: %.2f\r\n", SHT2X_T, BMP280_P, SHT2X_H, wind_speed, wind_direction);
			}
		}

		if (is_lora && is_need_query_data)
		// if (0)
		{
			printf("***********QEURY LORA***********\r\n");
			// 向从节点要数据
			is_need_query_data = 0;
			LORA_Query_All_SubNode_Data();
		}

		// 收到大功率lora查询数据
		// if(0)
		if (USART5_RX_STA&0x8000)	
		// if (1)
		{
			len = 0;
			USART5_Receive_Data(temp_buf, &len);	// 接收数据
			if(len>0)
			{
				printf("received data from big lora: ");
				for (i = 0; i < len; i++)
				{
					printf("%02x ", temp_buf[i]);
				}
				printf("\r\n");
			}
		
			if (len == 6 &&  		// 6位地址
				temp_buf[0] == SelfAddress[0] &&
				temp_buf[1] == SelfAddress[1] &&
				temp_buf[2] == SelfAddress[2] &&
				temp_buf[3] == SelfAddress[3] &&
				temp_buf[4] == SelfAddress[4] &&
				temp_buf[5] == SelfAddress[5])
			// if(1)
			{
				// 主从节点发送自身数据
				// 打印时间和传感器数据
				// 清空all_data_str
				memset(all_data_str, 0, sizeof(all_data_str));
				RTC_Get_Time(time);
				sprintf(data_str, "address: %02x%02x%02x%02x%02x%02x\r\ntime: %02d:%02d:%02d\r\ntemperature: %.2f\r\npressure: %.2f\r\nhumidity: %.2f\r\nwind_speed: %.2f\r\n"
								"wind_direction: %.2f\r\nsmoke: %.2f\r\nco: %.2f\r\nbattery: %.2f\r\nisTimeTrue: %d\r\n[SEP]",SelfAddress[0], SelfAddress[1], SelfAddress[2], SelfAddress[3], SelfAddress[4], SelfAddress[5],
						time[0], time[1], time[2], SHT2X_T, BMP280_P, SHT2X_H,wind_speed, wind_direction, co2,co_latest, battery, RTC_check_device_time());
				strcat(all_data_str, data_str);
				for (i = 0; i < SubNodeSet.nNode; i++) 
				{
					SubNode current_node = SubNodeSet.SubNode_list[i];
					// 生成当前子节点的数据字符串
					sprintf(data_str, "address: %02x%02x%02x%02x%02x%02x\r\ntime: %02d:%02d:%02d\r\ntemperature: %.2f\r\npressure: %.2f\r\nhumidity: %.2f\r\n"
							"smoke: %.2f\r\nco: %.2f\r\nbattery: %.2f\r\nisTimeTrue: %d\r\n[SEP]",
							current_node.address[0], current_node.address[1], current_node.address[2], current_node.address[3], current_node.address[4], current_node.address[5],
							current_node.sample_time[0], current_node.sample_time[1], current_node.sample_time[2], current_node.temperature, current_node.pressure, current_node.humidity,
							current_node.smoke, current_node.co, current_node.battery, RTC_check_specified_time(current_node.last_gps));
					// 将当前子节点的数据字符串拼接到all_data_str中
					strcat(all_data_str, data_str);
				}
				printf("all data: \r\n");
				puts(all_data_str);
				printf("sending all node data to MMNode..., len=%d\r\n", strlen(all_data_str));
				USART5_DATA("data_str", strlen(data_str));	// 发送给MMNode
				// mqtt4g_send(data_str, strlen(data_str));
				printf("all node data sent...\r\n");
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
		printf("LORA CMD\r\n");
		LORA_Send(UART4_RX_BUF, len);
	}

	else
	{
		// 测试发送
		printf("UART4 CMD: ");
		for (t = 0; t < len; t++)
		{
			// USART_SendData(USART1, USART_RX_BUF[t]);         //向串口1发送数据
			// while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);//等待发送结束
			printf("%02x ", UART4_RX_BUF[t]);
		}
		printf("\r\nlen=%d", len);
		printf("\r\n"); // 插入换行

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
				printf("LORA Network Status: OK\r\n");
			}
			else
			{
				printf("LORA Network Status: ERROR\r\n");
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
				printf("Slave Node Online\r\n");
			}
			else
			{
				printf("Slave Node Offline\r\n");
			}
		}
		// 当输入00,05时，网络数据清除
		else if (UART4_RX_BUF[0] == 0x00 && UART4_RX_BUF[1] == 0x05)
		{
			u8 flag = 0;
			flag = LORA_Network_Clear();
			if (flag == 1)
			{
				printf("Network Data Clear Success\r\n");
			}
			else
			{
				printf("Network Data Clear Fail\r\n");
			}
		}
		// 当输入00, 06时，全网点名
		else if (UART4_RX_BUF[0] == 0x00 && UART4_RX_BUF[1] == 0x06)
		{
			u8 flag = 0;
			flag = LORA_Network_Naming();
			if (flag == 1)
			{
				printf("Network Naming Success\r\n");
			}
			else
			{
				printf("Network Naming Fail\r\n");
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
				printf("Network Start Success\r\n");
			}
			else
			{
				printf("Network Start Fail\r\n");
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
				printf("GPS Time: %d:%d:%d\r\n", time[0], time[1], time[2]);
			}
			else
			{
				printf("GPS Time Read Fail\r\n");
			}
		}
		// 当输入01,01时，读取RTC时钟
		else if (UART4_RX_BUF[0] == 0x01 && UART4_RX_BUF[1] == 0x01)
		{
			u8 time[3] = {0};
			RTC_Get_Time(time);
			printf("RTC Time: %d:%d:%d\r\n", time[0], time[1], time[2]);
		}
		// 当输入01,02时，设置RTC时钟
		else if (UART4_RX_BUF[0] == 0x01 && UART4_RX_BUF[1] == 0x02)
		{
			u8 time[3] = {0};
			time[0] = UART4_RX_BUF[2];
			time[1] = UART4_RX_BUF[3];
			time[2] = UART4_RX_BUF[4];
			RTC_Set_Time(time[0], time[1], time[2]);
			printf("RTC Time Set Success\r\n");
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
		printf("%02X ", temp_rec[t]);
		// USART_SendData(UART4, temp_rec[t]); // 向串口4发送数据
		// while (USART_GetFlagStatus(UART4, USART_FLAG_TC) != SET); // 等待发送结束
	}
	printf("\r\n");
	// 检测是否是完整的LORA数据包
	if (temp_rec[0] == 0x6C && temp_rec[2] == 0x09 && temp_rec[rec_len - 1] == 0x16)
	{
		// 检测接收数据是否是来自从节点的数据透传
		if (temp_rec[3] == 0x03 && temp_rec[4] == 0x05)
		{
			u8 flag;
			printf("\r\n");
			flag = LORA_Receive_Data_Analysis(temp_rec, rec_len);
			if (flag == 1)
			{
				printf("LORA Receive Data Analysis Success\r\n");
			}
			else
			{
				printf("LORA Receive Data Analysis Fail\r\n");
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
		// printf("GPS Time: %d:%d:%d\r\n", time[0], time[1], time[2]);
		//  u8 flag;
		//  printf("\r\n");
		//  flag = GPS_Receive_Data_Analysis(temp_rec, rec_len);
		//  if (flag == 1)
		//  {
		//  	printf("GPS Receive Data Analysis Success\r\n");
		//  }
		//  else
		//  {
		//  	printf("GPS Receive Data Analysis Fail\r\n");
		//  }
	}
	// printf("\r\n");//插入换行
	//  printf("Receive %d,%d, len=%d", temp_rec[0],temp_rec[1],rec_len);
}
