#ifndef __GPS_H
#define __GPS_H
#include "sys.h"

extern float node_position[4]; // 节点位置,纬度和经度
extern char node_lati_longi_str[2]; // 节点纬度、经度标识（N/S,E/W）

void GPS_Init(void);
void GPS_Send(u8 *buf, u8 len);
void GPS_Receive(u8 *buf, u8 *len);
u8 check_GPS_Receive(void);
u8 GPS_get_time(u8 *time);
u8 GPS_get_time_and_pos(u8 *time, float *position, char *lati_longi_str);
#endif
