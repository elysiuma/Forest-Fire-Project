#ifndef __MQ2_H
#define __MQ2_H
#include "sys.h"

//MQ2������̼������������

extern uint8_t flag_mq2;

//MQ2�������Ŷ���
#define MQ2 PAout(6)

void MQ2_Init(void); //��ʼ��
void MQ2_Switch(u8 flag); //����������
float MQ2_Scan(void);   //��ȡ����
#endif
