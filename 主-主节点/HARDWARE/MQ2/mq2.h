#ifndef __MQ2_H
#define __MQ2_H
#include "sys.h"

//MQ2二氧化碳（烟雾）传感器

extern uint8_t flag_mq2;    // MQ2传感器开关标识
extern u8 mq2_state_count;  // MQ2传感器状态计数 开启(0:正常 1:预热 2+:测量计数)关闭(0:正常 1+:关闭计数)
extern u8 flag_mq2_is_need_measure; // MQ2传感器是否需要测量标识

#define MQ2 PAout(6)        // MQ2开关引脚定义
#define MQ2_ON_MAX 11       // MQ2传感器开启最大计数(预热1+测量次数)
#define MQ2_OFF_MAX 10      // MQ2传感器关闭最大计数(关闭周期次数)


void MQ2_Init(void); //初始化
void MQ2_Switch(u8 flag); //传感器开关
float MQ2_Scan(void);   //读取数据
void MQ2_PPM_Calibration(float RS); //校准
#endif
