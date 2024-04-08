#ifndef __LORA_H
#define __LORA_H
#include "sys.h"

//RS066/067 Lora模块，芯片RX1278

#define LORA PDout(4);  //lora模块开关

extern u8 is_need_send_lora_data;  //是否需要发送LORA数据

void LORA_Init(void);
void LORA_Send(u8 *buf,u8 len);
void LORA_Receive(u8 *buf,u8 *len);
int check_LORA_Receive(void);
void LORA_Get_Common_Send_Msg(u8 *msg, u8 *msg_len, u8 *command, u8 *content, u8 content_len, u8 is_send_main);
void LORA_DATA_Transfer(u8 *buf, u8 buf_len, u8 *address, u8 *receive_buf, u8 *receive_len); //数据传输
u8 LORA_Query_Slave_Node_Status(void); //查询自身从节点状态
#endif

