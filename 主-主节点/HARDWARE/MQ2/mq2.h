#ifndef __MQ2_H
#define __MQ2_H
#include "sys.h"

//MQ2������̼������������

extern uint8_t flag_mq2;    // MQ2���������ر�ʶ
extern u8 mq2_state_count;  // MQ2������״̬���� ����(0:���� 1:Ԥ�� 2+:��������)�ر�(0:���� 1+:�رռ���)
extern u8 flag_mq2_is_need_measure; // MQ2�������Ƿ���Ҫ������ʶ

#define MQ2 PAout(6)        // MQ2�������Ŷ���
#define MQ2_ON_MAX 11       // MQ2����������������(Ԥ��1+��������)
#define MQ2_OFF_MAX 10      // MQ2�������ر�������(�ر����ڴ���)


void MQ2_Init(void); //��ʼ��
void MQ2_Switch(u8 flag); //����������
float MQ2_Scan(void);   //��ȡ����
void MQ2_PPM_Calibration(float RS); //У׼
#endif
