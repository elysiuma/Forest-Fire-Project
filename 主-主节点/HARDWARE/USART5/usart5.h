#ifndef __USART5_H
#define __USART5_H
#include "stdio.h"	
#include "stm32f4xx_conf.h"
#include "sys.h" 

#define USART5_REC_LEN  300  	    //�����������ֽ��� 100
#define EN_USART5_RX 	1			//ʹ�ܣ�1��/��ֹ��0������1����

extern u8 USART5_RX_BUF[USART5_REC_LEN];    //���ջ���,���USART_REC_LEN���ֽ�.ĩ�ֽ�Ϊ���з� 
extern u8 USART5_TX_BUF[USART5_REC_LEN];    //���ͻ���,���USART_REC_LEN���ֽ� 
extern u16 USART5_RX_STA;                    //���ջ���ĳ���
void uart5_init(u32 bound);
void USART5_DATA(u8 *buf,u8 len);           //��������
void USART5_CMD(unsigned char *lb);         //����ָ��
void USART5_Receive_Data(u8 *buf,u8 *len);  //���ܻ���
void Usart5CommandAnalysis(void);
#endif
