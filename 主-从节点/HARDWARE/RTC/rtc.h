#ifndef __RTC_H
#define __RTC_H
#include "sys.h"
#include "stdio.h"

extern u16 last_time_gps;     // 距离上一次同步gps时间多少时间周期，5分钟一个周期，12次为1小时， 999为未同步
extern u8 is_need_update_time;  // 是否需要更新时间, 1为需要更新，0为不需要更新,用timer改变状态，在主循环中更新
extern u8 flag_get_time;

u8 customRTC_Init(void);
void RTC_Set_Time(u8 hour,u8 min,u8 sec);
void RTC_Set_Date(u8 year,u8 month,u8 date,u8 week);
void RTC_Get_Time(u8* time);
u8 RTC_update_device_time(void);
u8 RTC_check_device_time(void);
#endif
