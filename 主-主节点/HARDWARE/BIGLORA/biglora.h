#ifndef __BIGLORA_H
#define __BIGLORA_H
#include "sys.h"

extern u8 MSNodeAddress[120];

void BIGLORA_send_query(void);
u8 check_BIGLORA_Receive(void);
void BIGLORA_Receive(u8 *buf, u8 *len);
void BIGLORA_init(void);

#endif