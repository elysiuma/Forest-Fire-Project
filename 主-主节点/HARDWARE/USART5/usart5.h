#ifndef __USART5_H
#define __USART5_H
#include "stdio.h"	
#include "stm32f4xx_conf.h"
#include "sys.h" 

#define USART5_REC_LEN  3000  	    //定义最大接收字节数 100
#define EN_USART5_RX 	1			//使能（1）/禁止（0）串口1接收

extern u8 USART5_RX_BUF[USART5_REC_LEN];    //接收缓冲,最大USART_REC_LEN个字节.末字节为换行符 
extern u8 USART5_TX_BUF[USART5_REC_LEN];    //发送缓冲,最大USART_REC_LEN个字节 
extern u16 USART5_RX_STA;                    //接收缓冲的长度
void uart5_init(u32 bound);
void USART5_DATA(u8 *buf,u16 len);           //发送数据
void USART5_CMD(unsigned char *lb);         //发送指令
void USART5_Receive_Data(u8 *buf,u16 *len);  //接受缓存
void Usart5CommandAnalysis(void);
#endif
