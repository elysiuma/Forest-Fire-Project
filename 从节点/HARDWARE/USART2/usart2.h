#ifndef __USART2_H
#define __USART2_H
#include "stdio.h"	
#include "stm32f4xx_conf.h"
#include "sys.h" 

#define USART2_REC_LEN  100  	    //�����������ֽ��� 100
#define EN_USART2_RX 	1			//ʹ�ܣ�1��/��ֹ��0������1����

extern u8 USART2_RX_BUF[USART2_REC_LEN];    //���ջ���,���USART_REC_LEN���ֽ�.ĩ�ֽ�Ϊ���з� 
extern u8 USART2_TX_BUF[USART2_REC_LEN];    //���ͻ���,���USART_REC_LEN���ֽ� 
extern u16 USART2_RX_CNT;                    //���ջ���ĳ���
void uart2_init(u32 bound);
void USART2_DATA(u8 *buf,u8 len);           //��������
void USART2_CMD(unsigned char *lb);         //����ָ��
void USART2_Receive_Data(u8 *buf,u8 *len);  //���ܻ���
void Usart2CommandAnalysis(void);
#endif

