#ifndef __USART6_H
#define __USART6_H
#include "stdio.h"	
#include "stm32f4xx_conf.h"
#include "sys.h" 

#define USART6_REC_LEN  300  	    //�����������ֽ��� 100
#define EN_USART6_RX 	1			//ʹ�ܣ�1��/��ֹ��0������1����

extern u8 USART6_RX_BUF[USART6_REC_LEN];    //���ջ���,���USART_REC_LEN���ֽ�.ĩ�ֽ�Ϊ���з� 
extern u8 USART6_TX_BUF[USART6_REC_LEN];    //���ͻ���,���USART_REC_LEN���ֽ� 
extern u16 USART6_RX_STA;                    //���ջ���ĳ���
void uart6_init(u32 bound);
void USART6_DATA(u8 *buf,u8 len);           //��������
void USART6_CMD(unsigned char *lb);         //����ָ��
void USART6_Receive_Data(u8 *buf,u8 *len);  //���ܻ���
void Usart6CommandAnalysis(void);
#endif
