#ifndef __USART3_H
#define __USART3_H
#include "stdio.h"	
#include "stm32f4xx_conf.h"
#include "sys.h" 

#define USART3_REC_LEN  300  	    //定义最大接收字节数 100
#define EN_USART3_RX 	1			//使能（1）/禁止（0）串口1接收

extern u8 USART3_RX_BUF[USART3_REC_LEN];    //接收缓冲,最大USART_REC_LEN个字节.末字节为换行符 
extern u8 USART3_TX_BUF[USART3_REC_LEN];    //发送缓冲,最大USART_REC_LEN个字节 
extern u16 USART3_RX_STA;                    //接收缓冲的长度
void uart3_init(u32 bound);
void USART3_DATA(u8 *buf,u8 len);           //发送数据
void USART3_CMD(unsigned char *lb);         //发送指令
void USART3_Receive_Data(u8 *buf,u8 *len);  //接受缓存
void Usart3CommandAnalysis(void);
#endif
