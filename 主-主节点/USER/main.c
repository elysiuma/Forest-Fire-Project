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
#include "timer4.h"
#include "rtc.h"
#include "mqtt4g.h"
#include "biglora.h"
#include "wind.h"
// #include "i2c.h"
// #include "bmp280.h"
// #include "SHT2X.h"
#include <string.h>
#include <stdlib.h>

#define MQ2PreheatInterval			20 				// MQ2预热时间间隔，单位为秒  至少为20秒 （传感器采集数据共用这个）
#define GPSTimeInterval 			120				// GPS时间校时间隔，单位为秒  测试时2分钟一次，正式为5分钟一次
#define QueryTimeInterval			90				// 查询从节点数据时间间隔，单位为秒 测试时为5分钟，正式为30分钟
#define QueryDelayTime				5				// 查询从节点数据延时时间，单位为秒,大功率时为QueryDelayTime*24
#define is_debug					1				// 是否调试模式，1为调试模式，0为正常模式
#define query_node_data_max_times	5 				// 查询节点数据最大次数
#define is_lora						1				// 是否启动lora模块
#define is_gps						1				// 是否启动GPS模块
#define is_4g 						1				// 是否启动4G模块,需要先启动lora和gps
#define is_battery					1				// 是否启动电池电压检测
#define is_wind_sensor				1				// 是否启动风速风向传感器
#define is_calibration				0				// 是否启动风速风向校准 1为校准，0为不校准
#define is_biglora 					1				// 是否启动大功率lora模块
#define is_quick_biglora			0				// 是否启动快速查询大功率lora模块，测试用

uint8_t EnableMaster = 1;		  	// 主从选择 1为主机，0为从机
u8 data_str[200];				  	// 用于存储主主自身发送给服务器的数据
static u8 all_data_str[3000] = {0};	// 用于存储发送给服务器的所有数据(按最大10个节点算)


// 函数申明
void UART4_Handler(void); 	// 处理串口4PC通信的内容
void LORA_Handler(void);  	// 处理LORA通信的内容
void GPS_Handler(void);	  	// 处理GPS通信的内容
void get_data(char*, float*);		// 用于解析数据
void BIGLORA_Handler(void);	//处理大功率LORA通信的内容

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
	u8 SEP_STR[] = "[SEP]";
	u8 END_STR[] = {0x0d, 0x0a};
	u8 time[3] = {0};
	u8 i = 0, j = 0;
	u8 MSrec_len = 0;

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); // 设置系统中断优先级分组2
	delay_init(168);								// 初始化延时函数
	uart4_init(9600);								// 初始化串口波特率为9600
	printf("***********Init***********\r\n");
	LED_Init();										// 初始化LED
	#if is_wind_sensor
		Wind_Init();								// 初始化风速风向传感器
	#endif
	

	MQ2_Init();
	MQ7_Init();
	Timer_mq2_Init(MQ2PreheatInterval); 	// 初始化MQ2定时器，每MQ2PreheatInterval秒状态位递增，用于MQ2的预热（20秒）和测量，MQ7共用一个定时器

	#if is_battery
		BATTERY_Init();
	#endif

	#if is_4g
		mqtt4g_init();
	#endif

	customRTC_Init();

	#if is_gps
		GPS_Init();
	#endif
	Timer_Init(GPSTimeInterval); // 初始化定时器，TIM2用于读取gps时间给RTC校时, 间隔单位为秒，GPSTimeInterval*12为校时总周期
	
	#if is_lora	
	{
		is_lora_init = LORA_Init();
		
		if (is_debug) printf("LORA Init OK\r\n");
	}
	#endif

	#if is_biglora
		BIGLORA_init();
	#endif

	Timer_querydata_Init(QueryTimeInterval); // 初始化定时器，TIM5用于查询从节点数据，间隔单位为秒
	Timer_QueryDelay_Init(QueryDelayTime);	// 初始化定时器，TIM4用于查询从节点数据的延时，间隔单位为秒

	LED = 1;
	MQ2 = 1;
	MQ7 = 1;

	// TODO: 临时禁用I2C传感器
	// IIC_Init(); // I2C initialize
	// SHT2X_Init();
	// bmp280_uint();
	
	#if is_calibration
	{
		if (is_debug) printf("calibrating windsensor...please ensure NO WIND\r\n");
		USART6_DATA(cab_windsensor, 11);
		if (is_debug) printf("calibration DONE\r\n");
		delay_ms(1000);
	}
	#endif
	printf("System Init OK\r\n");
	delay_ms(500);

	while (1)
	{	
		printf("***********INTO THE LOOP***********\r\n");

		// 读取烟雾浓度
		if (flag_mq2_is_need_measure) // 需要测量时采集MQ2数据
		{
			printf("***********SMOKE***********\r\n");
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
		#if is_battery
		{
			if (flag_battery_is_need_measure)
			{
				printf("***********BATTERY***********\r\n");
				battery = BATTERY_Scan();
				if (is_debug) printf("battery: %.2f%%\r\n", battery);
				if (is_debug) printf("\r\n");
				flag_battery_is_need_measure = 0;
			}
		}
		#endif

		//	如果lora模块未初始化成功，尝试重新初始化
		if (is_lora && !is_lora_init)
			is_lora_init = LORA_Network_Init();

		// 是否需要更新时间
		// is_need_update_time =1;	// 调试用，每次都更新时间
		if (is_gps && is_need_update_time)
		{
			printf("***********UPDATE TIME***********\r\n");
			// 更新RTC时间需要GPS，在GPS未启动的时候不进行时间更新
			RTC_update_device_time();
			is_need_update_time = 0;
		}

		// if (UART4_RX_STA & 0x8000)
		// 	UART4_Handler(); // 处理串口4PC通信的内容

		if (is_lora && check_LORA_Receive())
		{
			printf("***********LORA HANDLING***********\r\n");
			LORA_Handler(); // 处理LORA通信的内容
		}

		// TODO: 这一步操作实际没有效果，校时为按需校时
		if (is_gps && check_GPS_Receive())
		{
			// printf("***********GPS HANDLING***********\r\n");
			GPS_Handler(); // 处理GPS通信的内容
		}

		// 向风速风向传感器串口要数据
		#if is_wind_sensor
			if (flag_wind_is_need_measure)	// 需要测量时采集风速风向数据
			{
				printf("***********QEURY WINDSENSOR***********\r\n");
				Wind_query();
				flag_wind_is_need_measure = 0;
			}
			
			if (check_Wind_receive())	// 检查是否接收到风速风向数据
			{
				Wind_analysis(&SHT2X_T, &BMP280_P, &SHT2X_H, &wind_speed, &wind_direction);
				printf("temperature: %.2f, pressure: %.2f, humidity: %.2f, wind_speed: %.2f, wind_direction: %.2f\r\n", SHT2X_T, BMP280_P, SHT2X_H, wind_speed, wind_direction);
			}
		#endif
		
		if (is_lora && is_need_query_data)
		{
			// printf("***********QEURY LORA***********\r\n");
			// 向从节点要数据
			if (current_query_node_idx < SubNodeSet.nNode)
			{
				if (SubNodeSet.SubNode_list[current_query_node_idx].SubNodeStatus != 2 && SubNodeSet.SubNode_list[current_query_node_idx].SubNodeStatus != 3)		// 当前子节点已发送查询命令或已接收数据时不再发送查询命令
				{
					LORA_Query_SubNode_Data(SubNodeSet.SubNode_list[current_query_node_idx].address);
					SubNodeSet.SubNode_list[current_query_node_idx].SubNodeStatus = 2;
					if (SubNodeSet.SubNode_list[current_query_node_idx].fail_count > 0)
						printf("Query Node %02x%02x%02x%02x%02x%02x Fail %d Times\r\n", SubNodeSet.SubNode_list[current_query_node_idx].address[0], SubNodeSet.SubNode_list[current_query_node_idx].address[1], SubNodeSet.SubNode_list[current_query_node_idx].address[2], SubNodeSet.SubNode_list[current_query_node_idx].address[3], SubNodeSet.SubNode_list[current_query_node_idx].address[4], SubNodeSet.SubNode_list[current_query_node_idx].address[5], SubNodeSet.SubNode_list[current_query_node_idx].fail_count);
				}
			}
		}

		#if is_4g
			if (is_need_send_4g)
			{
				is_need_send_4g = 0;
				printf("***********4G SENDING SELF DATA***********\r\n");
				// 主节点发送自身数据
				// 打印时间和传感器数据
				memset(all_data_str, 0, sizeof(all_data_str));
				RTC_Get_Time(time);
				sprintf(data_str,	"%02x%02x%02x%02x%02x%02x;"		// 主节点地址
									"%02d:%02d:%02d;"				// 时间
									"%.2f;"							// 温度
									"%.2f;"							// 气压
									"%.2f;"							// 湿度				
									"%.2f;"							// 风速
									"%.2f;"							// 风向
									"%.2f;"							// CO2浓度
									"%.2f;"							// CO浓度
									"%c%.0f*%.5f:%c%.0f*%.5f;"		// 纬度，经度
									"%.2f;"							// 电池电压
									"%d;",							// RTC校时状态
									SelfAddress[0], SelfAddress[1], SelfAddress[2], SelfAddress[3], SelfAddress[4], SelfAddress[5],
					time[0], time[1], time[2], SHT2X_T, BMP280_P, SHT2X_H,wind_speed, wind_direction, co2,co_latest,
					node_lati_longi_str[0], node_position[0], node_position[1], 
					node_lati_longi_str[1], node_position[2], node_position[3],
					battery, RTC_check_device_time());
				strcat(all_data_str, data_str);
				strcat(all_data_str, SEP_STR);
				LORA_Get_All_SubNode_Data(all_data_str);	// 获取所有从节点数据
				printf("all data: \r\n");
				puts(all_data_str);printf("\r\n");
				if (is_debug) printf("sending main node data to server...\r\n");
				mqtt4g_send(all_data_str, strlen(all_data_str));
				if (is_debug) printf("data sent...\r\n");
			}
		#endif

		#if is_quick_biglora
			if (is_need_quick_query_MSnode)
			{
				is_need_quick_query_MSnode = 0;
				printf("***********QEURY BIGLORA***********\r\n");
				BIGLORA_send_query_MSNode(MSNodeSet.MSNode_list[0].address);
				MSNodeSet.MSNode_list[0].NodeStatus = 2;
				if (MSNodeSet.MSNode_list[0].fail_count > 0)
					printf("Query MSNode %02x%02x%02x%02x%02x%02x Fail %d Times\r\n", MSNodeSet.MSNode_list[0].address[0], MSNodeSet.MSNode_list[0].address[1], MSNodeSet.MSNode_list[0].address[2], MSNodeSet.MSNode_list[0].address[3], MSNodeSet.MSNode_list[0].address[4], MSNodeSet.MSNode_list[0].address[5], MSNodeSet.MSNode_list[0].fail_count);
			}
		#else
		if (is_biglora && is_need_query_MSnode)
			{
				printf("***********QEURY BIGLORA***********\r\n");
				if (current_query_MSnode_idx < MSNodeSet.nNode)
				{
					if (MSNodeSet.MSNode_list[current_query_MSnode_idx].NodeStatus != 2 && MSNodeSet.MSNode_list[current_query_MSnode_idx].NodeStatus != 3)		// 当前子节点已发送查询命令或已接收数据时不再发送查询命令
					{
						BIGLORA_send_query_MSNode(MSNodeSet.MSNode_list[current_query_MSnode_idx].address);
						MSNodeSet.MSNode_list[current_query_MSnode_idx].NodeStatus = 2;
						if (MSNodeSet.MSNode_list[current_query_MSnode_idx].fail_count > 0)
							printf("Query MSNode %02x%02x%02x%02x%02x%02x Fail %d Times\r\n", MSNodeSet.MSNode_list[current_query_MSnode_idx].address[0], MSNodeSet.MSNode_list[current_query_MSnode_idx].address[1], MSNodeSet.MSNode_list[current_query_MSnode_idx].address[2], MSNodeSet.MSNode_list[current_query_MSnode_idx].address[3], MSNodeSet.MSNode_list[current_query_MSnode_idx].address[4], MSNodeSet.MSNode_list[current_query_MSnode_idx].address[5], MSNodeSet.MSNode_list[current_query_MSnode_idx].fail_count);
					}
				}
			}
		#endif

		if (is_biglora && check_BIGLORA_Receive())
		{
			printf("***********BIGLORA HANDLING***********\r\n");
			BIGLORA_Handler(); // 处理大功率LORA模块通信的内容
		}

		printf("***********OUT THE LOOP***********\r\n\r\n");
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
	u8 temp_rec[100];
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


void BIGLORA_Handler(void)
{
	u8 all_data_str[3000] = {0};
	u16 rec_len = 0;

	BIGLORA_Receive(all_data_str, &rec_len);
	printf("BIGLORA sending to 4G...");
	mqtt4g_send(all_data_str, rec_len);
	printf("BIGLORA sending to 4G DONE!!!");
}