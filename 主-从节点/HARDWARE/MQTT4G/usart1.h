#ifndef __USART1_H
#define __USART1_H
#include "sys.h"
#include "stdio.h"

#define USART1_REC_LEN  			200  	//定义最大接收字节数 200
#define EN_USART1_RX 			1		//使能（1）/禁止（0）串口1接收
	  	
extern u8  USART1_RX_BUF[USART1_REC_LEN]; //接收缓冲,最大USART_REC_LEN个字节.末字节为换行符 
extern u16 USART1_RX_STA;         		//接收状态标记	
//如果想串口中断接收，请不要注释以下宏定义
void usart1_init(u32 bound);
void usart1_send(u8 *buf,u16 len);
void USART1_Receive_Data(u8 *buf,u8 *len);
#endif
