#ifndef __MQ7_H
#define __MQ7_H
#include "sys.h"

//MQ7一氧化碳传感器

extern uint8_t flag_mq7;   // MQ7传感器开关标识
// MQ7开关跟MQ2保持同步，在MQ2.h中实现
// extern u8 mq7_state_count;  // MQ7传感器状态计数 开启(0:正常 1:预热 2+:测量计数)关闭(0:正常 1+:关闭计数)
// extern u8 flag_mq7_is_need_measure; // MQ7传感器是否需要测量标识

#define MQ7 PCout(1)        // MQ7开关引脚定义PC1
// 同MQ2.h中的定义
// #define MQ7_ON_MAX 11       // MQ7传感器开启最大计数(预热1+测量次数)
// #define MQ7_OFF_MAX 10      // MQ7传感器关闭最大计数(关闭周期次数)


void MQ7_Init(void); //初始化
void MQ7_Switch(u8 flag); //传感器开关
float MQ7_Scan(void);   //读取数据
void MQ7_PPM_Calibration(float RS); //校准
float MQ7_Get_R0(void); //获取R0
#endif
