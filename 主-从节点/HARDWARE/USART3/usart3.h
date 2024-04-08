#ifndef __USART3_H
#define __USART3_H
#include "stdio.h"	
#include "stm32f4xx_conf.h"
#include "sys.h" 

#define USART3_REC_LEN  300  	    //�����������ֽ��� 100
#define EN_USART3_RX 	1			//ʹ�ܣ�1��/��ֹ��0������1����

extern u8 USART3_RX_BUF[USART3_REC_LEN];    //���ջ���,���USART_REC_LEN���ֽ�.ĩ�ֽ�Ϊ���з� 
extern u8 USART3_TX_BUF[USART3_REC_LEN];    //���ͻ���,���USART_REC_LEN���ֽ� 
extern u16 USART3_RX_STA;                    //���ջ���ĳ���
void uart3_init(u32 bound);
void USART3_DATA(u8 *buf,u8 len);           //��������
void USART3_CMD(unsigned char *lb);         //����ָ��
void USART3_Receive_Data(u8 *buf,u8 *len);  //���ܻ���
void Usart3CommandAnalysis(void);
#endif
