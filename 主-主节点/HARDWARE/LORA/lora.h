#ifndef __LORA_H
#define __LORA_H
#include "sys.h"

//RS066/067 Lora模块，芯片RX1278

#define LORA PDout(4);  //lora模块开关

// 定义从节点数据结构，包含从节点地址和从节点状态，以及数据，和获取时间
typedef struct
{
    u8 address[6];   //从节点地址
    u8 SubNodeStatus;   //从节点状态， 0：初始化，1：正常，2：已发送查询命令, 3: 已接收数据
    u8 fail_count;  // 查询失败次数
    // float wind_speed; //风速
    // float wind_direction; //风向
    float temperature;    //温度
    float pressure;       //气压
    float humidity;       //湿度
    float smoke;          //烟雾
    float co;             //一氧化碳
    float battery;        //电量
    u8 sample_time[3];  //采样时间
    u16 last_gps;   //距离上一次同步gps时间多少时间周期，5分钟一个周期，12次为1小时， 999为未同步
} SubNode;
// 从节点集合
typedef struct
{
    u8 nNode;   //从节点数量
    SubNode SubNode_list[30];    //从节点集合
} SubNodeSetStruct;

// 初始化从节点集合对象
extern SubNodeSetStruct SubNodeSet;
// extern u16 last_time_gps;     // 距离上一次同步gps时间多少时间周期，5分钟一个周期，12次为1小时， 999为未同步
extern u8 is_lora_init;     // 是否已经初始化网络
extern u8 is_need_query_data;   // 是否需要查询子节点数据
// extern u8 nNode;
extern u8 SubNodeAddress[120];  //从节点地址集合
extern u8 SelfAddress[6];    //自身地址
extern u8 current_query_node_idx;   // 当前查询的节点idx

u8 LORA_Init(void);
void LORA_Send(u8 *buf,u8 len);
void LORA_Receive(u8 *buf,u8 *len);
void LORA_Get_Common_Send_Msg(u8 *msg, u8 *msg_len, u8 *command, u8 *content, u8 content_len);
u8 check_LORA_Receive(void);
u8 LORA_Network_Init(void); //一键初始化网络
u8 LORA_Init_Time(u8 *time);
u8 LORA_Add_Slave_Node(u8 nNode, u8 *SubNodeAddress);
u8 LORA_Query_Network_Status(u8 *address, u8 *time, u8 is_debug);     // 查询网络状态
u8 LORA_Query_Slave_Node_Status(u8 is_debug); //查询从节点状态
void LORA_DATA_Transfer(u8 *buf, u8 buf_len, u8 *address); //数据传输
u8 LORA_Network_Clear(void);    //清除网络
u8 LORA_Receive_Data_Analysis(u8 *buf, u8 buf_len);  //接收数据解析
u8 LORA_Network_Naming(void);   // 全网点名
u8 LORA_Network_Start(void);    // 启动组网
u8 LORA_Find_SubNode(u8 *address);  // 查找从节点
void LORA_Query_SubNode_Data(u8 *address);  // 查询从节点数据, 只查询不等待解析
void LORA_Query_All_SubNode_Data(void);  // 查询所有从节点数据
void LORA_Get_All_SubNode_Data(u8 *_all_data_str);  // 获取所有从节点数据
u8 LORA_Get_SubNode_Data_idx(u8 idx, u8 *_data_str);  // 根据索引idx获取从节点数据
void LORA_Update_All_SubNode_Status(void);  // 更新所有从节点状态和失败次数
// u8 LORA_update_device_time(void);   // 更新设备时间

#endif

