#ifndef __BIGLORA_H
#define __BIGLORA_H
#include "sys.h"

extern u8 MSNodeAddress[120];

void BIGLORA_send_query(void);
u8 check_BIGLORA_Receive(void);
void BIGLORA_Receive(u8 *buf, u16 *len);
void BIGLORA_Send(u8 *buf, u16 len);
void BIGLORA_init(void);
u8 check_addr(u8 *addr,u16 addr_len, u8 *cur_addr);  // 查看地址串中是否包含自身地址

#endif
