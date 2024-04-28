#include "biglora.h"
#include "usart5.h"
#include "delay.h"
#include <string.h>
#define nMSnode 1

MSNodeSetStruct MSNodeSet;

u8 cur_addr[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0d, 0x0a};
u8 is_need_query_MSnode = 0;
u8 current_query_MSnode_idx = 200;
u8 MSNodeAddress[120] = {
        // 0x36, 0x49, 0x01, 0x00, 0x00, 0x00,
        // 0x28, 0x49, 0x01, 0x00, 0x00, 0x00,
        0x99, 0x99, 0x99, 0x99, 0x05, 0x50,
        // 0x27, 0x49, 0x01, 0x00, 0x00, 0x00,
        // 0x29, 0x49, 0x01, 0x00, 0x00, 0x00,
};									// 主从节点的地址

void BIGLORA_Send(u8 *buf, u8 len);

void BIGLORA_send_query(void)
{
    u8 i, j;
    printf("query M-S node data...\r\n");
    for (i = 0; i < nMSnode; i++)
    {
        for (j = 0; j < 6; j++)
            cur_addr[j] = MSNodeAddress[i * 6 + j];
        BIGLORA_Send(cur_addr, 8);
        printf("query sent...node: %d\r\n", i);
    }
}

void BIGLORA_send_query_MSNode(u8 *address)
{
    u8 i;
    for (i = 0; i < 6; i++)
        cur_addr[i] = address[i];
    BIGLORA_Send(cur_addr, 8);
    printf("query MSNode sent to %02x%02x%02x%02x%02x%02x\r\n", address[0], address[1], address[2], address[3], address[4], address[5]);
}

void BIGLORA_Send(u8 *buf, u8 len)
{
    USART5_DATA(buf, len);
}

u8 check_BIGLORA_Receive(void)
{
    // 若接受完毕数据则返回1
    if (USART5_RX_STA & 0x8000)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void BIGLORA_Receive(u8 *buf, u8 *len)
{
    USART5_Receive_Data(buf, len);
}

void BIGLORA_init(void)
{
    uart5_init(9600);
    MSNodeSet.nNode = 0;
    BIGLORA_Add_MSNode(nMSnode, MSNodeAddress);
}

// 添加主从节点
void BIGLORA_Add_MSNode(u8 nNode, u8 *_addr)
{
    u8 i;
    if (MSNodeSet.nNode > MAX_MSNode)
    {
        printf("MSNodeSet is full!\r\n");
        return;
    }
    
    for (i = 0; i < nNode; i++)
    {
        MSNode new_node;
        memcpy(new_node.address, _addr + i * 6, 6);
        new_node.NodeStatus = 0;
        new_node.fail_count = 0;
        MSNodeSet.MSNode_list[MSNodeSet.nNode++] = new_node;
        if (MSNodeSet.nNode >= MAX_MSNode)
        {
            printf("MSNodeSet is full!\r\n");
            return;
        }
    }
}

// 更新所有主从节点状态和失败次数
void BIGLORA_Update_All_MSNode_Status(void)
{
    // 用于更新所有主从节点状态和失败次数，若在新一轮查询主从节点数据时，某主从节点依然是已发送（未响应）状态，则将其状态重置，失败次数+1
    u8 i;
    for (i = 0; i < MSNodeSet.nNode; i++)
    {
        if (MSNodeSet.MSNode_list[i].NodeStatus == 2)
        {
            if (MSNodeSet.MSNode_list[i].fail_count < 255)
                MSNodeSet.MSNode_list[i].fail_count++;
        }
        MSNodeSet.MSNode_list[i].NodeStatus = 1;   // 已接收数据也要重置为正常状态
    }
}
