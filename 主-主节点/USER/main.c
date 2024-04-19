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
#include "rtc.h"
#include "mqtt4g.h"
#include "i2c.h"
#include "bmp280.h"
#include "SHT2X.h"
#include <string.h>
#include <stdlib.h>
#define MQ2PreheatInterval 10 // MQ2Ԥ��ʱ��������λΪ��  ����Ϊ20��
#define GPSTimeInterval 120	// GPSʱ��Уʱ�������λΪ��  ����ʱ2����һ�Σ���ʽΪ5����һ��

uint8_t EnableMaster = 1;		  	// ����ѡ�� 1Ϊ������0Ϊ�ӻ�
u8 is_debug = 1;				  	// �Ƿ����ģʽ��1Ϊ����ģʽ��0Ϊ����ģʽ
u8 query_node_data_max_times = 5; 	// ��ѯ�ڵ�����������
u8 is_lora = 0;					  	// �Ƿ�����loraģ��
u8 is_gps = 1;					  	// �Ƿ�����GPSģ��
u8 is_4g = 1;					  	// �Ƿ�����4Gģ��,��Ҫ������lora��gps
u8 rec_len=0;						//
u8 query_windsensor[11] = {0x24, 0x41, 0x44, 0x2C, 0x30, 0x34, 0x2A, 0x36, 0x33, 0x0D, 0x0A};	// ����ٴ�������������
u8 cab_windsensor[11] = {0x24, 0x41, 0x5A, 0x2C, 0x30, 0x34, 0x2A, 0x37, 0x39, 0x0D, 0x0A};		// ���ٷ���У׼
u8 data_u8[28] = {0};				// ���ڴ洢����
union data{
	float f;
	u8 ch[4];
} data_1;							// ����ת�����ݸ�ʽ�� char -> float

// ��������
void UART4_Handler(void); 	// ������4PCͨ�ŵ�����
void LORA_Handler(void);  	// ����LORAͨ�ŵ�����
void GPS_Handler(void);	  	// ����GPSͨ�ŵ�����
// ���ڽ�������
void DATA_Handler(float *temp, float *pres, float *humi, float *wind_sp, float *wind_dir, float *smoke, float *batt);																																	
void get_data(char*);		// ���ڽ�������
int main(void)
{
	float co2 = 0; // ����Ũ��
	float BMP280_P = 100000;
	float BMP280_T = 25.00;
	float SHT2X_T = 25.00; // temperature of SHT2X
	float SHT2X_H = 40.00; // humidity of SHT2X
	float battery = 0;	   // ��Դ��ѹ
	float wind_speed = 0;
	float wind_direction = 0;
	float co_latest = 0; // ���µ�COŨ��

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); // ����ϵͳ�ж����ȼ�����2
	delay_init(168);								// ��ʼ����ʱ����
	uart4_init(9600);								// ��ʼ�����ڲ�����Ϊ9600
	uart5_init(9600);								// ��ʼ�����ڲ�����Ϊ9600������Lora
	uart6_init(9600);								// ��ʼ�����ڲ�����Ϊ9600��������
	LED_Init();										// ��ʼ��LED

	MQ2_Init();
	MQ7_Init();
	Timer_mq2_Init(MQ2PreheatInterval); 	// ��ʼ��MQ2��ʱ����ÿMQ2PreheatInterval��״̬λ����������MQ2��Ԥ�ȣ�20�룩�Ͳ���
	BATTERY_Init();
	if (is_4g)
		mqtt4g_init();
	customRTC_Init();
	if (is_gps)
		GPS_Init();
	Timer_Init(GPSTimeInterval); // ��ʼ����ʱ����TIM2���ڶ�ȡgpsʱ���RTCУʱ, �����λΪ�룬GPSTimeInterval*12ΪУʱ������
	
	if (is_lora)
	{
		is_lora_init = LORA_Init();
		if (is_debug) printf("LORA Init OK\r\n");
	}

	LED = 1;
	MQ2 = 1;
	MQ7 = 1;

	// TODO: ��ʱ����I2C������
	// IIC_Init(); // I2C initialize
	// SHT2X_Init();
	// bmp280_uint();
	delay_ms(500);
	if (is_debug) printf("calibrating windsensor...please ensure NO WIND\r\n");
	USART6_DATA(cab_windsensor, 11);
	printf("calibration DONE\r\n");
	delay_ms(1000);

	while (1)
	{
		u8 time[3];
		u8 i, j;
		u8 current_addr[6] = {0};
		u8 query[3] = {0x11, 0x22, 0x33}; // �������ӽڵ㷢�ͣ���ѯ����
		u8 is_query_node_success = 0;	  // �Ƿ�ɹ���ѯ���ڵ�����
		u8 data_str[300];				  // ���ڴ洢���͸�������������

		// ��ȡ����Ũ�����10��ƽ������
		if (flag_mq2_is_need_measure) // ��Ҫ����ʱ�ɼ�MQ2����
		{
			printf("MQ2_Scan state=%d\r\n", mq2_state_count);
			co2 = MQ2_Scan();
			co_latest = MQ7_Scan();
			printf("CO2: %.2f, CO: %.2f\r\n", co2, co_latest);
			flag_mq2_is_need_measure = 0;
		}
		// if (is_debug) printf("bmp T: %f\r\n", bmp280_get_temperature());

		// I2C������ִ��
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

		// ��ȡ��ص�ѹ
		battery = BATTERY_Scan();
		if (is_debug) printf("battery: %.2f%%\r\n", battery);
		if (is_debug) printf("\r\n");

		//	���loraģ��δ��ʼ���ɹ����������³�ʼ��
		if (is_lora && !is_lora_init)
			is_lora_init = LORA_Network_Init();

		// �Ƿ���Ҫ����ʱ��
		// is_need_update_time =1;	// �����ã�ÿ�ζ�����ʱ��
		if (is_gps && is_need_update_time)
		{
			// ����RTCʱ����ҪGPS����GPSδ������ʱ�򲻽���ʱ�����
			RTC_update_device_time();
			is_need_update_time = 0;
		}

		// if (UART4_RX_STA & 0x8000)
			// UART4_Handler(); // ������4PCͨ�ŵ�����

		if (is_lora && check_LORA_Receive())
			LORA_Handler(); // ����LORAͨ�ŵ�����

		// TODO: ��һ������ʵ��û��Ч����УʱΪ����Уʱ
		if (is_gps && check_GPS_Receive())
			GPS_Handler(); // ����GPSͨ�ŵ�����

		// �򴫸���Ҫ����
		printf("query windsensor...\r\n");
		USART6_DATA(query_windsensor, 11);
		delay_ms(3000);
		if (USART6_RX_STA&0x8000)
		{
			DATA_Handler(&SHT2X_T, &BMP280_P, &SHT2X_H, &wind_speed, &wind_direction, &co2, &battery);
		}
		rec_len=0;

		if (is_lora)
		{
			// ��ӽڵ�Ҫ����
			if (is_debug) printf("query data...\r\n");
			for (i = 0; i < nNode; i++)
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
						LORA_Handler(); // ����LORAͨ�ŵ�����
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
							LORA_Handler(); // ����LORAͨ�ŵ�����
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
			// ���ڵ㷢����������
			// ��ӡʱ��ʹ���������
			RTC_Get_Time(time);
			sprintf(data_str, "address: 999999990505\r\ntime: %02d:%02d:%02d\r\ntemperature: %.2f\r\npressure: %.2f\r\nhumidity: %.2f\r\nwind_speed: %.2f\r\nwind_direction: %.2f\r\nsmoke: %.2f\r\nbattery: %.2f%%\r\nisTimeTrue: %d\r\n",
					time[0], time[1], time[2], SHT2X_T, BMP280_P, SHT2X_H, wind_speed, wind_direction, co2, battery, RTC_check_device_time());
			if (is_debug) puts(data_str);
			if (is_debug) printf("sending main node data to server...\r\n");
			mqtt4g_send(data_str, strlen(data_str));
			if (is_debug) printf("data sent...\r\n");
		}

		delay_ms(1000);
		DebugLed(); // LED��˸˵��������������
	}
}

// ������4PCͨ�ŵ�����
void UART4_Handler(void)
{
	// ֧�ֵ���ָ���β��Ҫ\r\n��
	// 00,00 ����LORAģ��ʱ��
	// 00,01 һ����ʼ��LORAģ��
	// 00,02 ��ѯ����״̬
	// 00,03 ��������͸������ӽڵ㷢�ʹ�������������
	// 00,04 ��ѯ�ӽڵ�״̬
	// 00,05 �����������
	// 00,06 ȫ������
	// 00,07 ��������
	// 01,00 ��ѯGPSʱ��
	// 01,01 ��ȡrtcʱ��
	// 01,02 ����rtcʱ��

	u8 t, len;
	// ��������
	len = UART4_RX_STA & 0x3fff; // �õ��˴ν��յ������ݳ���
	// �������ݳ���Ϊ0ʱ��ֱ����ղ�����
	if (len == 0)
	{
		UART4_RX_STA = 0;
		return;
	}
	// ����Ƿ���LORA��ָ�������ٷ���loraģ��
	if (UART4_RX_BUF[0] == 0x6C && UART4_RX_BUF[len - 1] == 0x16)
	{
		if (is_debug) printf("LORA CMD\r\n");
		LORA_Send(UART4_RX_BUF, len);
	}

	else
	{
		// ���Է���
		if (is_debug) printf("UART4 CMD: ");
		for (t = 0; t < len; t++)
		{
			// USART_SendData(USART1, USART_RX_BUF[t]);         //�򴮿�1��������
			// while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);//�ȴ����ͽ���
			if (is_debug) printf("%02x ", UART4_RX_BUF[t]);
		}
		if (is_debug) printf("\r\nlen=%d", len);
		if (is_debug) printf("\r\n"); // ���뻻��

		// ������Ϊ00,00ʱ������LORAģ��ʱ��
		if (UART4_RX_BUF[0] == 0x00 && UART4_RX_BUF[1] == 0x00)
		{
			u8 time[3] = {17, 33, 11};
			LORA_Init_Time(time);
		}
		// ������Ϊ00,01ʱ��һ����ʼ��LORAģ��
		else if (UART4_RX_BUF[0] == 0x00 && UART4_RX_BUF[1] == 0x01)
		{
			LORA_Network_Init();
		}
		// ������Ϊ00,02ʱ����ѯ����״̬
		else if (UART4_RX_BUF[0] == 0x00 && UART4_RX_BUF[1] == 0x02)
		{
			u8 time[3] = {0};
			u8 flag = 0;
			flag = LORA_Query_Network_Status(time, is_debug);
			if (flag)
			{
				if (is_debug) printf("LORA Network Status: OK\r\n");
			}
			else
			{
				if (is_debug) printf("LORA Network Status: ERROR\r\n");
			}
		}
		// ������00,03ʱ����������͸������ӽڵ㷢�ʹ�������������
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
			// ��ӡ���յ�������
			for (t = 0; t < receive_len; t++)
			{
				USART_SendData(UART4, receive_buf[t]); // �򴮿�4��������
				while (USART_GetFlagStatus(UART4, USART_FLAG_TC) != SET)
					; // �ȴ����ͽ���
			}
		}
		// ������00,04ʱ����ѯ�ӽڵ�״̬
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
		// ������00,05ʱ�������������
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
		// ������00, 06ʱ��ȫ������
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
		// ������00,07ʱ����������
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
		// ������01,00ʱ����ȡgpsʱ��
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
		// ������01,01ʱ����ȡRTCʱ��
		else if (UART4_RX_BUF[0] == 0x01 && UART4_RX_BUF[1] == 0x01)
		{
			u8 time[3] = {0};
			RTC_Get_Time(time);
			if (is_debug) printf("RTC Time: %d:%d:%d\r\n", time[0], time[1], time[2]);
		}
		// ������01,02ʱ������RTCʱ��
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
	UART4_RX_STA = 0; // ��ս��ջ��泤��
}

// LORAģ��ͨ�Ŵ���
void LORA_Handler(void)
{
	u8 t;
	u8 temp_rec[200];
	u8 rec_len = 0;
	// ��������
	LORA_Receive(temp_rec, &rec_len);
	if (rec_len == 0)
	{
		return;
	}
	// ���Դ�ӡ����
	for (t = 0; t < rec_len; t++)
	{
		if (is_debug) printf("%02X ", temp_rec[t]);
		// USART_SendData(UART4, temp_rec[t]); // �򴮿�4��������
		// while (USART_GetFlagStatus(UART4, USART_FLAG_TC) != SET); // �ȴ����ͽ���
	}
	if (is_debug) printf("\r\n");
	// ����Ƿ���������LORA���ݰ�
	if (temp_rec[0] == 0x6C && temp_rec[2] == 0x09 && temp_rec[rec_len - 1] == 0x16)
	{
		// �����������Ƿ������Դӽڵ������͸��
		if (temp_rec[3] == 0x03 && temp_rec[4] == 0x05)
		{
			u8 flag;
			if (is_debug) printf("\r\n");
			flag = LORA_Receive_Data_Analysis(temp_rec, rec_len);
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

// GPSģ��ͨ�Ŵ���
void GPS_Handler(void)
{
	u8 t;
	u8 temp_rec[300];
	u8 rec_len = 0;
	u8 time[3] = {0};
	// ��������
	GPS_Receive(temp_rec, &rec_len);
	if (rec_len == 0)
	{
		return;
	}
	// ���Դ�ӡ����
	
	// for (t = 0; t < rec_len; t++)
	// {
	// 	USART_SendData(UART4, temp_rec[t]); // �򴮿�4��������
	// 	while (USART_GetFlagStatus(UART4, USART_FLAG_TC) != SET); // �ȴ����ͽ���
	// }
	
	// ����Ƿ���������GPS���ݰ�
	if (temp_rec[3] == 0x47 && temp_rec[4] == 0x47 && temp_rec[5] == 0x41)
	{
		// ʱ
		time[0] = 10 * (temp_rec[7] - 48) + (temp_rec[8] - 48); // UTCʱ��
		time[0] += 8;											// ����ʱ��
		if (time[0] >= 24)
			time[0] -= 24;
		time[0] = (time[0] / 10) * 16 + time[0] % 10; // ת��Ϊ16����
		// set_lora_clk[8] = time[0];
		// ��
		time[1] = 16 * (temp_rec[9] - 48) + (temp_rec[10] - 48);
		// set_lora_clk[9] = time[1];
		// ��
		time[2] = 16 * (temp_rec[11] - 48) + (temp_rec[12] - 48);

		// ��ӡʱ��
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
	// if (is_debug) printf("\r\n");//���뻻��
	//  if (is_debug) printf("Receive %d,%d, len=%d", temp_rec[0],temp_rec[1],rec_len);
}

//	����ģ�鴦��
void DATA_Handler(float *temp, float *pres, float *humi, float *wind_sp, float *wind_dir, float *smoke, float *batt)
{
	u8 i;
	u8 temp_rec[300];
	u8 wind_speed[4] = {0};
    u8 wind_direction[4] = {0};
    u8 temperature[4] = {0};
    u8 pressure[4] = {0};
    u8 humidity[4] = {0};
    u8 _smoke[4] = {0};
	u8 battery[4] = {0};

	if (is_debug) printf("get windsensor data!\r\n");
	USART6_Receive_Data(temp_rec, &rec_len);
	// puts(temp_rec);
	if (is_debug) printf("\r\n");
	if (is_debug) printf("data analyzing...\r\n");
	get_data(temp_rec);
	
	if (is_debug) 
	{
	printf("windsensor query finished...\r\n");
	for(i=0;i<28;i++)
		printf("%02x ", data_u8[i]);
	printf("\r\n");
	}

	for (i = 0; i < 4; i = i + 1)
    {	//�� ѹ ʪ ���ٷ���
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

void get_data(char* data_str)
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
