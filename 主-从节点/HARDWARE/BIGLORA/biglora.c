#include "lora.h"
#include "usart5.h"
#include "delay.h"
#include "string.h"
#include <string.h> // 引入memcmp函数的头文件
#define nMSnode 1

u8 cur_addr[6] = {0};
u8 MSNodeAddress[120] = {
        // 0x36, 0x49, 0x01, 0x00, 0x00, 0x00,
        // 0x28, 0x49, 0x01, 0x00, 0x00, 0x00,
        0x99, 0x99, 0x99, 0x99, 0x05, 0x50,
        // 0x27, 0x49, 0x01, 0x00, 0x00, 0x00,
        // 0x29, 0x49, 0x01, 0x00, 0x00, 0x00,
};									// 主从节点的地址

void BIGLORA_send_query(void)
{
    u8 i, j;
    printf("query M-S node data...\r\n");
    for (i = 0; i < nMSnode; i++)
        {
            for (j = 0; j < 6; j++)
                cur_addr[j] = MSNodeAddress[i * 6 + j];
            USART5_DATA(cur_addr, 6);
            printf("query sent...node: %d\r\n", i);
        }
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

void BIGLORA_Send(u8 *buf, u8 len)
{
    USART5_DATA(buf, len);
}

void BIGLORA_init(void)
{
    uart5_init(9600);
}

u8 check_addr(u8 *addr, u8 addr_len, u8 *cur_addr) {
    const u8 CUR_ADDR_LEN = 6; // cur_addr的长度
    u8 i;

    // 遍历addr数组
    for (i = 0; i <= addr_len - CUR_ADDR_LEN; i++) {
        // 直接使用memcmp比较整个6位地址
        if (memcmp(addr + i, cur_addr, CUR_ADDR_LEN) == 0) {
            // 如果找到匹配的地址，返回1
            return 1;
        }
    }
    // 如果未找到匹配的地址，返回0
    return 0;
}
