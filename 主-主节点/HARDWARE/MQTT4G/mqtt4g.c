#include "mqtt4g.h"
#include "usart1.h"
#include "delay.h"

u8 is_need_send_4g = 0;   // 是否需要发送4g数据

// 初始化mqtt4g
void mqtt4g_init(void)
{
	// 配置4G模块指令
	u8 work_mode[13] = {'A', 'T', '*', 'W', 'K', 'M', 'O', 'D', 'E', '=', '9', '#', 0x0d};//工作模式：MQTT模式
	u8 reg_pkg[13] = {'A', 'T', '*', 'R', 'E', 'G', 'P', 'K', 'G', '=', '5', '#', 0x0d};//注册包模式：注册包（MQTT模式发）
	u8 server[33] = {'A','T','*','S','E','R','V','E','R','1','=','0',',','1','2','1','.','3','6','.','1','8','.','1','6','4',',','1','8','8','3','#', 0x0d};//主中心地址，TCP
	//u8 mqtt_topic[30] = {'A','T','*','M','Q','T','O','P','=','t','e','s','t','t','o','p','i','c',',','t','e','s','t','t','o','p','i','c','#', 0x0d};
	u8 mqtt_pub_topic[20] = {'A','T','*','M','Q','P','U','B','=','t','e','s','t','t','o','p','i','c','#', 0x0d};
	u8 check_sim_signal[8] = {'A','T','*','C','S','Q','?', 0x0d};
	u8 check_gstate[11] = {'A','T','*','G','S','T','A','T','E','?', 0x0d};
	u8 test_msg[8] = {'t','e','s','t','d','a','t','a'};
	u8 receive_buf[30];
	u8 receive_len = 0;
	u8 fail_count = 0;
	u8 i=0;
	u8 init_cnt = 0;
	
    // 初始化串口1
    usart1_init(9600);
	delay_ms(30000);
	/*
	while(1)
	{
		delay_ms(1000);
		if(mqtt4g_check())
		{
			mqtt4g_receive(receive_buf, &receive_len);
			for(i=0;i<receive_len;i++)
			{
				printf("%c", receive_buf[i]);
			}
			receive_len = 0;
			if(receive_buf[7] == 'S') break;
		}
		printf("4G Module turning on...\r\n");
	}
	*/
	receive_len = 0;
	USART1_RX_STA = 0;
	
	//配置模块
	mqtt4g_send(work_mode, 13);//工作模式
	printf("config work mode...\r\n");
	delay_ms(2000);
	if(mqtt4g_check())
	{
		mqtt4g_receive(receive_buf, &receive_len);
		for(i=0;i<receive_len;i++)
		{
			printf("%c", receive_buf[i]);
		}
	}
	receive_len = 0;
	
	mqtt4g_send(reg_pkg, 13);//注册包
	printf("config reg package...\r\n");
	delay_ms(2000);
	if(mqtt4g_check())
	{
		mqtt4g_receive(receive_buf, &receive_len);
		for(i=0;i<receive_len;i++)
		{
			printf("%c", receive_buf[i]);
		}
	}
	receive_len = 0;
	
	mqtt4g_send(server, 33);//服务器
	printf("config center server...\r\n");
	delay_ms(2000);
	if(mqtt4g_check())
	{
		mqtt4g_receive(receive_buf, &receive_len);
		for(i=0;i<receive_len;i++)
		{
			printf("%c", receive_buf[i]);
		}
	}
	receive_len = 0;
	
	mqtt4g_send(mqtt_pub_topic, 20);//仅配置发布的主题
	delay_ms(3000);
	if(mqtt4g_check())
	{
		mqtt4g_receive(receive_buf, &receive_len);
		for(i=0;i<receive_len;i++)
		{
			printf("%c", receive_buf[i]);
		}
	}
	receive_len = 0;
	USART1_RX_STA = 0;
	
	//检查sim信号质量
	printf("check sim card signal quality...\r\n");
	while(1)
	{
		mqtt4g_send(check_sim_signal, 8);
		delay_ms(3000);
		if(mqtt4g_check())
		{
			mqtt4g_receive(receive_buf, &receive_len);
			for(i=0;i<receive_len;i++)
			{
				printf("%c", receive_buf[i]);
			}
			if(receive_len>6) {receive_len=0;break;} //信号强度大于等于10时，信号较好
			else
			{	
				fail_count++;
				printf("signal weak! fail=%d\r\n", fail_count);
				receive_len=0;
			}
		}
		else printf("4G module no receive!\r\n");
		
		if(fail_count>=10) break;
	}
	fail_count = 0;
	USART1_RX_STA = 0;
	
	//检查与中心服务器连接状态
	printf("check if connected with center server...\r\n");
	while(1)
	{
		mqtt4g_send(check_gstate, 11);
		delay_ms(3000);
		if(mqtt4g_check())
		{
			mqtt4g_receive(receive_buf, &receive_len);
			for(i=0;i<receive_len;i++)
			{
				printf("%c", receive_buf[i]);
			}
			receive_len=0;
			if(receive_buf[8]==0x31) break; //+gstate=1
			else
			{	
				fail_count++;
				printf("server not connected! fail=%d\r\n", fail_count);
			}
		}
		else printf("4G module no receive!\r\n");
		if(fail_count>=10) break;
	}
	
	printf("4G module initials successfully!\r\n");
	
	/*
	//向服务器发送测试数据
	printf("send test msg to center server...\r\n");
	mqtt4g_send(test_msg, 8);
	printf("over...\r\n");
	*/
}

// 发送数据
void mqtt4g_send(u8 *data, u16 len)
{
    // 发送数据
    usart1_send(data, len);
}

// 接收数据
void mqtt4g_receive(u8 *data, u8 *len)
{
    // 接收数据
    USART1_Receive_Data(data, len);
}

// 检查是否有数据
u8 mqtt4g_check(void)
{
    // 检查是否有数据
    if (USART1_RX_STA & 0x8000)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
