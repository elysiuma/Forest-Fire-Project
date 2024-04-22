#ifndef __TIMER_H
#define __TIMER_H

#include "stdio.h"
#include "sys.h" 

// extern u8 data_u8[24];

void Timer_Init(u16 interval); //初始化, interval为定时器中断时间间隔，单位为秒

#endif
