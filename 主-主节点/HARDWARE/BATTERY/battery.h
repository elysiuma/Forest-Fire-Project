#ifndef __BATTERY_H
#define __BATTERY_H
#include "sys.h"

//电源电压管理模块

void BATTERY_Init(void);    //初始化
float BATTERY_Scan(void);   //读取电源数据
#endif
