#ifndef __LORA_H
#define __LORA_H
#include "sys.h"

//RS066/067 Lora模块，芯片RX1278

#define LORA PDout(4);  //lora模块开关

// 定义从节点数据结构，包含从节点地址和从节点状态，以及数据，和获取时间
typedef struct
{
    u8 address[6];   //从节点地址
    u8 SubNodeStatus;   //从节点状态
    float wind_speed; //风速
    float wind_direction; //风向
    float temperature;    //温度
    float pressure;       //气压
    float humidity;       //湿度
    float smoke;          //烟雾
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
extern u8 get_data_flag;
extern u8 nNode;
extern u8 SubNodeAddress[120];

u8 LORA_Init(void);
void LORA_Send(u8 *buf,u8 len);
void LORA_Receive(u8 *buf,u8 *len);
void LORA_Get_Common_Send_Msg(u8 *msg, u8 *msg_len, u8 *command, u8 *content, u8 content_len);
u8 check_LORA_Receive(void);
u8 LORA_Network_Init(void); //一键初始化网络
u8 LORA_Init_Time(u8 *time);
u8 LORA_Add_Slave_Node(u8 nNode, u8 *SubNodeAddress);
u8 LORA_Query_Network_Status(u8 *time, u8 is_debug);     // 查询网络状态
u8 LORA_Query_Slave_Node_Status(u8 is_debug); //查询从节点状态
void LORA_DATA_Transfer(u8 *buf, u8 buf_len, u8 *address); //数据传输
u8 LORA_Network_Clear(void);    //清除网络
u8 LORA_Receive_Data_Analysis(u8 *buf, u8 buf_len);  //接收数据解析
u8 LORA_Network_Naming(void);   // 全网点名
u8 LORA_Network_Start(void);    // 启动组网
u8 LORA_Find_SubNode(u8 *address);  // 查找从节点
u8 LORA_update_device_time(void);   // 更新设备时间

#endif

