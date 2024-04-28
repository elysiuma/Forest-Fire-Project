#ifndef __MQTT4G_H
#define __MQTT4G_H
#include "sys.h"

extern u8 is_need_send_4g;   // 是否需要发送4g数据

void mqtt4g_init(void);
void mqtt4g_send(u8 *data, u16 len);
void mqtt4g_receive(u8 *data, u8 *len);
u8 mqtt4g_check(void);


#endif
