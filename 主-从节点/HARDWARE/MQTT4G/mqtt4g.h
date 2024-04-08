#ifndef __MQTT4G_H
#define __MQTT4G_H
#include "sys.h"
void mqtt4g_init(void);
void mqtt4g_send(u8 *data, u16 len);
void mqtt4g_receive(u8 *data, u8 *len);
u8 mqtt4g_check(void);


#endif