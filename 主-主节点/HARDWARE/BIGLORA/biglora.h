#ifndef __BIGLORA_H
#define __BIGLORA_H
#include "sys.h"

#define MAX_MSNode 15

typedef struct
{
    u8 address[6];   //主从节点地址
    u8 NodeStatus;   //从节点状态， 0：初始化，1：正常，2：已发送查询命令, 3: 已接收数据
    u8 fail_count;  // 查询失败次数
    // float wind_speed; //风速
    // float wind_direction; //风向
    // float temperature;    //温度
    // float pressure;       //气压
    // float humidity;       //湿度
    // float smoke;          //烟雾
    // float co;             //一氧化碳
    // float battery;        //电量
    // u8 sample_time[3];  //采样时间
    // u16 last_gps;   //距离上一次同步gps时间多少时间周期，5分钟一个周期，12次为1小时， 999为未同步
} MSNode;
// 主从节点集合
typedef struct
{
    u8 nNode;   //主从节点数量
    MSNode MSNode_list[MAX_MSNode];    //主从节点集合
} MSNodeSetStruct;


extern MSNodeSetStruct MSNodeSet;
extern u8 current_query_MSnode_idx;   // 当前查询的主从节点idx
extern u8 is_need_query_MSnode;   // 是否需要查询主从节点数据


void BIGLORA_send_query(void);                   // 查询所有主从节点数据
void BIGLORA_send_query_MSNode(u8 *address);    // 查询单个主从节点数据
u8 check_BIGLORA_Receive(void);
void BIGLORA_Receive(u8 *buf, u16 *len);
void BIGLORA_Send(u8 *buf, u8 len);
void BIGLORA_init(void);
void BIGLORA_Add_MSNode(u8 nNode, u8 *_addr);
void BIGLORA_Update_All_MSNode_Status(void);  // 更新所有主从节点状态和失败次数

#endif