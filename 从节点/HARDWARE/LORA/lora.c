#include "lora.h"
#include "usart2.h"
#include "delay.h"

u8 is_need_send_lora_data = 0; //是否需要发送LORA数据

void LORA_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);//使能GPIOD时钟

    //EN使能引脚，高电平启动
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
    GPIO_Init(GPIOD, &GPIO_InitStructure);//初始化
    GPIO_SetBits(GPIOD,GPIO_Pin_4);//PD4设置高，启动模块

    //串口通信口
    uart2_init(9600);

    is_need_send_lora_data = 0;
    
}

void LORA_Send(u8 *buf,u8 len)
{
    USART2_DATA(buf, len);
}

void LORA_Receive(u8 *buf,u8 *len)
{
    USART2_Receive_Data(buf, len);
}

int check_LORA_Receive(void)
{   
    // 用于判断是否接收数据完毕，返回1表示接收完毕，返回0表示未接收完毕
    if (USART2_RX_CNT&0x8000)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

// 获取LORA模块通用发送内容
void LORA_Get_Common_Send_Msg(
    u8 *send_msg,   // 待发送的报文
    u8 *sen_msg_len,    // 报文长度
    u8 *command,    // 命令，两位
    u8 *content_buf,    // 内容
    u8 content_len,  // 内容长度
    u8 is_send_main // 是否是像主节点发送数据
    )
{
    u8 p = 0, i;
    *sen_msg_len = 10 + content_len;
    
    send_msg[p] = 0x6C;	p++;
    send_msg[p] = 5 + content_len;	p++;
    send_msg[p] = 0x09;	p++;
    send_msg[p] = command[0];	p++;
    send_msg[p] = command[1];	p++;
    if(is_send_main == 1)
    {
        send_msg[p] = 0x80;	p++;
        send_msg[p] = 0x88;	p++;
        send_msg[p] = 0x88; p++;
    }
    else
    {
        send_msg[p] = 0x00;	p++;
        send_msg[p] = 0x99;	p++;
        send_msg[p] = 0x99; p++;
    }
    for(i = 0; i < content_len; i++)
    {	
        send_msg[p] = content_buf[i];
        p++;
    }
    send_msg[p] = 0;
    for(i = 0; i < p; i++)
    {
        send_msg[p] = send_msg[p] + send_msg[i];
    }
    p++;
    send_msg[p] = 0x16;
}

// 获取从节点网络信息
u8 LORA_Query_Slave_Node_Status(void)
{
    
    //报文长度:10
    u8 p = 0;
    u8 sen_msg_len = 10;
    u8 msg[10];
    u8 command[2] = {0x01, 0x01};
    u8 fail_count = 0;

    u8 receive_buf[50];
    u8 receive_len=0;

    // 从节点地址
    u8 address[6] = {0x00};
    // 归属主节点地址
    u8 main_address[2] = {0x00};
    // 从节点层次
    u8 level = 0;
    // 从节点信号强度
    u8 signal_strength = 0;
    
    LORA_Get_Common_Send_Msg(msg, &sen_msg_len, command, NULL, 0, 0);
    // 打印一下msg
    printf("msg: ");
    for(p = 0; p < sen_msg_len; p++)
    {
        printf("%02x ", msg[p]);
    }
    printf("\r\n");
    LORA_Send(msg, sen_msg_len);
    // 等待接受指令状态
    while (1)
    {
        delay_ms(500);
        if (check_LORA_Receive())
        {
            printf("received！\r\n");
            LORA_Receive(receive_buf, &receive_len);
            address[0] = receive_buf[8];
            address[1] = receive_buf[9];
            address[2] = receive_buf[10];
            address[3] = receive_buf[11];
            address[4] = receive_buf[12];
            address[5] = receive_buf[13];
            main_address[0] = receive_buf[14];
            main_address[1] = receive_buf[15];
            level = receive_buf[16];
            signal_strength = receive_buf[17];
            printf("address: %02x %02x %02x %02x %02x %02x\r\n", address[0], address[1], address[2], address[3], address[4], address[5]);
            printf("main_address: %02x %02x\r\n", main_address[0], main_address[1]);
            printf("level: %d\r\n", level);
            printf("signal_strength: %d\r\n", signal_strength);
            return 1;
        }
        else
        {
            printf("LORA_Query_Slave_Node_Status: No receive, fail=%d\r\n", ++fail_count);
            if (fail_count >= 10)
            {
                break;
            }
        }
    }
}

// 数据透传
void LORA_DATA_Transfer(u8 *buf, u8 buf_len, u8 *address, u8 *receive_buf, u8 *receive_len)
{
    //报文长度	msg_len = 8 + 6 + 2 + 1 + len + 2
	//address:数据传输收方的地址
	//buf:待发送数据缓存器
	//buf_len:待发送数据长度
    //receive_buf:接收数据缓存器
    //receive_len:接收数据长度
	u8 p = 0, i;
    u8 sen_msg_len = 8 + 6 + 2 + 1 + buf_len + 2;
    u8 msg[200];
    u8 command[2] = {0x03, 0x05};
    u8 content_len = 6 + 2 + 1 + buf_len;	//address + 固定 + 长度位 + buf_len
    u8 content[200];

    u8 all_receive_buf[100];
    u8 all_receive_len=0;

    for(i = 0; i < 6; i = i + 1)
	{
		content[p] = address[i];					//8--13
		p++;
	}
	content[p] = 0x00;	p++;						//14
	content[p] = 0x30;	p++;						//15
	content[p] = buf_len;	p++;						//16
	for(i = 0; i < buf_len; i = i + 1)
	{
		content[p] = buf[i];						//17--17+len-1
		p++;
	}
    
    LORA_Get_Common_Send_Msg(msg, &sen_msg_len, command, content, content_len,1);

    LORA_Send(msg, sen_msg_len);
    // 等待接受指令状态
    while (1)
    {
        if (check_LORA_Receive())
        {
            LORA_Receive(all_receive_buf, &all_receive_len);
            *receive_len = all_receive_buf[16];
            printf("receive_len: %d\r\n", *receive_len);
            printf("receive_buf: ");
            for(i = 0; i < *receive_len; i = i + 1)
            {
                receive_buf[i] = all_receive_buf[17+i];
                printf("%x ", receive_buf[i]);
            }
            break;
        }
    }
}



