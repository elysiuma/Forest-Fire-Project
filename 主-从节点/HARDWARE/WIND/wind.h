#ifndef __WIND_H
#define __WIND_H
#include "sys.h"

// 风速风向模块
extern u8 flag_wind_is_need_measure;   //是否需要测量风速风向

void Wind_Init(void);    //初始化
void Wind_query(void);   //查询风速风向数据,发送查询命令，不等待
void Wind_analysis(float *temp, float *pres, float *humi, float *wind_sp, float *wind_dir);   //读取风速风向数据
u8 check_Wind_receive(void);    //检查是否接收到风速风向数据
#endif
