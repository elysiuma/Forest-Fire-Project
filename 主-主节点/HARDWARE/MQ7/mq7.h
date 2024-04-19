#ifndef __MQ7_H
#define __MQ7_H
#include "sys.h"

//MQ7һ����̼������

extern uint8_t flag_mq7;   // MQ7���������ر�ʶ
// MQ7���ظ�MQ2����ͬ������MQ2.h��ʵ��
// extern u8 mq7_state_count;  // MQ7������״̬���� ����(0:���� 1:Ԥ�� 2+:��������)�ر�(0:���� 1+:�رռ���)
// extern u8 flag_mq7_is_need_measure; // MQ7�������Ƿ���Ҫ������ʶ

#define MQ7 PCout(1)        // MQ7�������Ŷ���PC1
// ͬMQ2.h�еĶ���
// #define MQ7_ON_MAX 11       // MQ7����������������(Ԥ��1+��������)
// #define MQ7_OFF_MAX 10      // MQ7�������ر�������(�ر����ڴ���)


void MQ7_Init(void); //��ʼ��
void MQ7_Switch(u8 flag); //����������
float MQ7_Scan(void);   //��ȡ����
void MQ7_PPM_Calibration(float RS); //У׼
float MQ7_Get_R0(void); //��ȡR0
#endif
