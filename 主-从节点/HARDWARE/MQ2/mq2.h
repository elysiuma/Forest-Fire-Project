#ifndef __MQ2_H
#define __MQ2_H
#include "sys.h"

//MQ2二氧化碳（烟雾）传感器

extern uint8_t flag_mq2;

//MQ2开关引脚定义
#define MQ2 PAout(6)

void MQ2_Init(void); //初始化
void MQ2_Switch(u8 flag); //传感器开关
float MQ2_Scan(void);   //读取数据
#endif
