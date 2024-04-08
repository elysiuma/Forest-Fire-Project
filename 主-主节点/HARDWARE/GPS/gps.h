#ifndef __GPS_H
#define __GPS_H
#include "sys.h"

void GPS_Init(void);
void GPS_Send(u8 *buf, u8 len);
void GPS_Receive(u8 *buf, u8 *len);
u8 check_GPS_Receive(void);
u8 GPS_get_time(u8 *time);
#endif
