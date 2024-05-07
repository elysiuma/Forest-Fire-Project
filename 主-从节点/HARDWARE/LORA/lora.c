#include "lora.h"
#include "usart2.h"
#include "delay.h"
#include "gps.h"
#include "rtc.h"
#include "string.h"
// #include "mqtt4g.h"
#include "tool.h"

// 定义全局变量
SubNodeSetStruct SubNodeSet;
// u16 last_time_gps = 999; 
u8 is_lora_init = 0;
u8 is_need_query_data = 0;
u8 SelfAddress[6] = {0x99, 0x99, 0x99, 0x99};
u8 query[3] = {0x11, 0x22, 0x33}; // 用于向子节点发送，查询数据
u8 current_query_node_idx = 200;
// u8 SubNodeAddress[120] = {
//         // 0x36, 0x49, 0x01, 0x00, 0x00, 0x00,
//         // 0x28, 0x49, 0x01, 0x00, 0x00, 0x00,
//         0x26, 0x49, 0x01, 0x00, 0x00, 0x00,
//         // 0x27, 0x49, 0x01, 0x00, 0x00, 0x00,
//         // 0x29, 0x49, 0x01, 0x00, 0x00, 0x00,
//                               };

// 主模块与从模块对应关系
typedef struct {
    u8 masterAddress[6];
    u8 _nNode;          // 从节点个数
    u8 slaveAddress[60];
} NodeMappingStruct;

u8 NodeMappingLen = 10;      // 共有多少组地址映射

NodeMappingStruct NodeMapping[] = {
    {{0x99, 0x99, 0x99, 0x99, 0x05, 0x50}, 2,  {0x00, 0x00, 0x00, 0x01, 0x58, 0x67,         // 1号主从
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x64,
                                                // 0x00, 0x00, 0x00, 0x01, 0x58, 0x66,
                                                // 0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                // 0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                // 0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                // 0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                // 0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                }},
    {{0x99, 0x99, 0x99, 0x99, 0x05, 0x51}, 8,  {0x00, 0x00, 0x00, 0x01, 0x58, 0x63,      // 2号主从
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                }},
    {{0x99, 0x99, 0x99, 0x99, 0x05, 0x52}, 8,  {0x00, 0x00, 0x00, 0x01, 0x58, 0x63,      // 3号主从
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                }},
    {{0x99, 0x99, 0x99, 0x99, 0x05, 0x53}, 8,  {0x00, 0x00, 0x00, 0x01, 0x58, 0x63,      // 4号主从
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                }},
    {{0x99, 0x99, 0x99, 0x99, 0x05, 0x54}, 8,  {0x00, 0x00, 0x00, 0x01, 0x58, 0x63,      // 5号主从
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                }},                                      
    {{0x99, 0x99, 0x99, 0x99, 0x05, 0x55}, 8,  {0x00, 0x00, 0x00, 0x01, 0x58, 0x63,      // 6号主从
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                }},
    {{0x99, 0x99, 0x99, 0x99, 0x05, 0x56}, 8,  {0x00, 0x00, 0x00, 0x01, 0x58, 0x63,      // 7号主从
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                }},
    {{0x99, 0x99, 0x99, 0x99, 0x05, 0x57}, 8,  {0x00, 0x00, 0x00, 0x01, 0x58, 0x63,      // 8号主从
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                }},
    {{0x99, 0x99, 0x99, 0x99, 0x05, 0x58}, 8,  {0x00, 0x00, 0x00, 0x01, 0x58, 0x63,      // 主主
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                0x00, 0x00, 0x00, 0x01, 0x58, 0x63,
                                                }},
    {{0x99, 0x99, 0x99, 0x99, 0x00, 0x00}, 0,  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00,      // 未知
                                                }},
};

u8 LORA_Init(void)
{
    u8 flag=0;
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE); //???GPIOD???

    // EN????????????????
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;      //????????
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;     //???????
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz; // 100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;       //????
    GPIO_Init(GPIOD, &GPIO_InitStructure);             //?????
    GPIO_SetBits(GPIOD, GPIO_Pin_4);                   // PD4?????????????

    //????????
    uart2_init(9600);

    SubNodeSet.nNode = 0;
    is_lora_init = 0;
    last_time_gps = 999;

    // lora的第一个指令不会响应，要发两次
    flag = LORA_Network_Clear();
    if (flag == 1)
    {
        printf("Network Data Clear Success\r\n");
    }
    else
    {
        flag = LORA_Network_Clear();
        if (flag == 1)
        {
            printf("Network Data Clear Success\r\n");
        }
        else
        {
            printf("Network Data Clear Fail\r\n");
            return 0;
        }
    }
    delay_ms(1000);
    flag = LORA_Network_Init();
    return flag;
}

void LORA_Send(u8 *buf, u8 len)
{
    USART2_DATA(buf, len);
}

void LORA_Receive(u8 *buf, u8 *len)
{
    // printf("len: %i", *len);
    USART2_Receive_Data(buf, len);
}

u8 check_LORA_Receive(void)
{
    // printf("USART2_RX_CNT: %i", USART2_RX_CNT);
    // 若接受完毕数据则返回1
    if (USART2_RX_CNT & 0x8000)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

// 获取LORA模块通用发送内容
void LORA_Get_Common_Send_Msg(
    u8 *send_msg,    // 待发送的报文
    u8 *sen_msg_len, // 报文长度
    u8 *command,     // 命令，两位
    u8 *content_buf, // 内容
    u8 content_len   // 内容长度
)
{
    u8 p = 0, i;
    *sen_msg_len = 10 + content_len;

    send_msg[p] = 0x6C;
    p++;
    send_msg[p] = 5 + content_len;
    p++;
    send_msg[p] = 0x09;
    p++;
    send_msg[p] = command[0];
    p++;
    send_msg[p] = command[1];
    p++;
    send_msg[p] = 0x00;
    p++;
    send_msg[p] = 0x99;
    p++;
    send_msg[p] = 0x99;
    p++;
    for (i = 0; i < content_len; i++)
    {
        send_msg[p] = content_buf[i];
        p++;
    }
    send_msg[p] = 0;
    for (i = 0; i < p; i++)
    {
        send_msg[p] = send_msg[p] + send_msg[i];
    }
    p++;
    send_msg[p] = 0x16;
}

// LORA模块初始化时间
u8 LORA_Init_Time(u8 *time)
{
    // 报文长度:13
    // time:时、分、秒
    u8 i;
    u8 sen_msg_len = 13;
    u8 msg[13];
    u8 content[3];
    u8 command[2] = {0x02, 0x04};
    u8 fail_count = 0; // 失败次数

    u8 receive_buf[30];
    u8 receive_len = 0;
    u8 set_time_flag = 0; // 0:设置时间失败 1:设置时间成功

    for (i = 0; i < 3; i++)
    {
        // 10进制转16进制，16进制下与10进制数字相同
        content[i] = time[i] / 10 * 16 + time[i] % 10;
    }
    LORA_Get_Common_Send_Msg(msg, &sen_msg_len, command, content, 3);

    // 打印一下msg
    printf("msg: ");
    for (i = 0; i < sen_msg_len; i++)
    {
        printf("%02x ", msg[i]);
    }
    printf("\r\n");
    LORA_Send(msg, sen_msg_len);
    // 等待接受指令状态
    while (1)
    {
        delay_ms(1000);
        if (check_LORA_Receive())
        {
            LORA_Receive(receive_buf, &receive_len);
            // 3,4位为确认位，00,01确认，00,02否认
            if (receive_buf[3] == 0x00 && receive_buf[4] == 0x01) // Confirm command
            {
                set_time_flag = 1;
            }
            else if (receive_buf[3] == 0x00 && receive_buf[4] == 0x02) // deny command
            {
                set_time_flag = 0;
            }
            break;
        }
        else
        {
            printf("LORA_Init_Time: No receive, fail=%d\r\n", ++fail_count);
            if (fail_count >= 3)
            {
                break;
            }
        }
    }
    return set_time_flag;
}

// LORA模态添加从节点地址档案
u8 LORA_Add_Slave_Node(u8 nNode, u8 *SubNodeAddress)
{
    // 报文长度	msg_len = 8 + 1 + nNode*6 + 2
    // nNode:节点数量
    // SubNodeAddress:每个节点的地址 二维数组
    u8 n, m;
    u8 sen_msg_len = 8 + 1 + nNode * 6 + 2;
    u8 msg[150];
    u8 content[150];
    u8 command[2] = {0x02, 0x08};
    u8 fail_count = 0; // 失败次数

    u8 receive_buf[30];
    u8 receive_len = 0;
    u8 add_slave_node_flag = 0;

    // 一帧最多下载30个从节点地址
    if (nNode > 30)
    {
        return 0;
    }

    content[0] = nNode;
    // 添加从节点地址时需要将地址倒过来
    for (n = 0; n < nNode; n = n + 1)
    {
        for (m = 0; m < 6; m = m + 1)
        {
            content[1 + n * 6 + m] = SubNodeAddress[(n+1) * 6 - m - 1];
        }
    }
    // 打印content
    printf("content: ");
    for (n = 0; n < 1 + nNode * 6; n++)
    {
        printf("%02x ", content[n]);
    }
    printf("\r\n");
    LORA_Get_Common_Send_Msg(msg, &sen_msg_len, command, content, 1 + nNode * 6);
    printf("msg: ");
    for (n = 0; n < sen_msg_len; n++)
    {
        printf("%02x ", msg[n]);
    }
    printf("\r\n");
    LORA_Send(msg, sen_msg_len);

    while (1)
    {
        if (check_LORA_Receive())
        {
            delay_ms(1000);
            LORA_Receive(receive_buf, &receive_len);
            // 3,4位为确认位，00,01确认，00,02否认
            if (receive_buf[3] == 0x00 && receive_buf[4] == 0x01) // Confirm command
            {
                add_slave_node_flag = 1;
                // 初始化从节点档案
                for (m = 0; m < nNode; m++)
                {
                    u8 new_address[6];
                    u8 node_location = 255;
                    for (n = 0; n < 6; n++)
                    {
                        new_address[n] = SubNodeAddress[m * 6 + n];
                    }
                    node_location = LORA_Find_SubNode(new_address);
                    // 当前从节点不存在再进行添加
                    if (node_location == 255)
                    {
                        SubNode new_node;
                        for (n = 0; n < 6; n++)
                        {
                            new_node.address[n] = SubNodeAddress[m * 6 + n];
                        }
                        new_node.SubNodeStatus = 0;
                        new_node.fail_count = 0;
                        // new_node.wind_speed = 0;
                        // new_node.wind_direction = 0;
                        new_node.temperature = 0;
                        new_node.humidity = 0;
                        new_node.pressure = 0;
                        new_node.smoke = 0;
                        new_node.co = 0;
                        new_node.battery = 0;
                        new_node.sample_time[0] = 0;
                        new_node.sample_time[1] = 0;
                        new_node.sample_time[2] = 0;
                        new_node.last_gps = 999;
                        SubNodeSet.SubNode_list[SubNodeSet.nNode] = new_node;
                        SubNodeSet.nNode++;
                        printf("add new SubNodeAddress[%d] = %02x %02x %02x %02x %02x %02x\r\n", m, SubNodeAddress[m * 6], SubNodeAddress[m * 6 + 1], SubNodeAddress[m * 6 + 2], SubNodeAddress[m * 6 + 3], SubNodeAddress[m * 6 + 4], SubNodeAddress[m * 6+5]);
                    }
                }
            }
            else if (receive_buf[3] == 0x00 && receive_buf[4] == 0x02) // deny command
            {
                add_slave_node_flag = 0;
            }
            break;
        }
        else
        {
            printf("LORA_Add_Slave_Node: No receive, fail=%d\r\n", ++fail_count);
            if (fail_count >= 10)
            {
                break;
            }
        }
    }
    return add_slave_node_flag;
}

// LORA模块网络数据清除
u8 LORA_Network_Clear(void)
{
    // 报文长度:10
    u8 sen_msg_len;
    u8 msg[10];
    u8 command[2] = {0x02, 0x02};
    u8 fail_count = 0; // 失败次数

    u8 receive_buf[30];
    u8 receive_len = 0;
    u8 network_clear_flag = 0;

    LORA_Get_Common_Send_Msg(msg, &sen_msg_len, command, NULL, 0);

    LORA_Send(msg, sen_msg_len);

    while (1)
    {
        delay_ms(1000);
        if (check_LORA_Receive())
        {
            LORA_Receive(receive_buf, &receive_len);
            // 3,4位为确认位，00,01确认，00,02否认
            if (receive_buf[3] == 0x00 && receive_buf[4] == 0x01) // Confirm command
            {
                network_clear_flag = 1;
            }
            else if (receive_buf[3] == 0x00 && receive_buf[4] == 0x02) // deny command
            {
                network_clear_flag = 0;
            }
            break;
        }
        else
        {
            printf("LORA_Network_Clear: No receive, fail=%d\r\n", ++fail_count);
            if (fail_count >= 3)
            {
                break;
            }
        }
    }
    return network_clear_flag;
}

// LORA模块一键初始化
u8 LORA_Network_Init(void)
{
    u8 i, time[3];
    u8 index_mapping = 0;       // 主从地址映射表索引
    u8 flag_set_time = 0;       // 0:设置时间失败 1:设置时间成功
    u8 flag_query_network = 0;  // 0:查询网络状态失败 1:查询网络状态成功
    u8 flag_add_slave_node = 0; // 0:添加从节点失败 1:添加从节点成功
    u8 flag_network_start = 0;  // 0:网络启动失败 1:网络启动成功


    printf("LORA network init start!\r\n");
    // 1.不再同步时间，直接设置时间
    time[0]=0x00;
    time[1]=0x00;
    time[2]=0x00;
    flag_set_time = LORA_Init_Time(time);
    printf("flag_set_time=%d\r\n", flag_set_time);
    //if (flag_set_time != 1)
       // return 0;

    // 2.查询自身地址
    flag_query_network = LORA_Query_Network_Status(SelfAddress, time, 0);
    if (flag_query_network)
    {
        printf("LORA Network Status: OK\r\n");
        printf("LORA Address: ");
        for (i = 0; i < 6; i++)
            printf("%02X ", SelfAddress[i]);
        printf("\r\n");
    }
    else
    {
        printf("LORA Network Status: ERROR\r\n");
    }

    // 3.添加从节点地址档案
    SubNodeSet.nNode = 0; // 从节点数量清零
    for (i = 0; i < NodeMappingLen; i++)
    {
        if (SelfAddress[4] == NodeMapping[i].masterAddress[4] && SelfAddress[5] == NodeMapping[i].masterAddress[5])
        {
            index_mapping = i;
            break;
        }
        index_mapping = i;
    }
    if (index_mapping == NodeMappingLen)
    {
        printf("Can't find the master address in the NodeMapping table\r\n");
    }
    flag_add_slave_node = LORA_Add_Slave_Node(NodeMapping[index_mapping]._nNode, NodeMapping[index_mapping].slaveAddress);
    if (flag_add_slave_node == 1)
    {
        printf("Add slave node success!\r\n");
    }
    else
    {
        printf("Add slave node fail!\r\n");
        return 0;
    }

    // 3.启动组网
    flag_network_start = LORA_Network_Start();
    if (flag_network_start == 1)
    {
        printf("Network start success!\r\n");
    }
    else
    {
        printf("Network start fail!\r\n");
        return 0;
    }

    printf("LORA network init wait 15 seconds\r\n");
    // 需要延迟15秒，等待组网完成
    delay_ms(15 * 1000);
    printf("LORA network init success!\r\n");
    return 1;
}

// 查询网络状态
u8 LORA_Query_Network_Status(u8 *address, u8 *time, u8 is_debug)
{
    // time: 时分秒
    // is_debug: 0:不打印调试信息 1:打印调试信息
    // 报文长度:8
    u8 i;
    u8 append_len = 8; // 实际返回内容前的字符数
    u8 sen_msg_len = 10;
    u8 msg[10];
    u8 command[2] = {0x01, 0x01};
    u8 fail_count = 0;         // 失败次数
    u8 flag_query_network = 0; // 0:查询网络状态失败 1:查询网络状态成功

    u8 receive_buf[50];
    u8 receive_len = 0;

    // 模块内部时间
    // u8 time[3]; // 时分秒 使用参数来进行返回
    // 网络运行状态：0=空闲；1=组网；2=抄表
    u8 network_status = 0;
    // 档案总数：用户档案子节点的总个数
    u8 total_number = 0;
    // 实时在线总数
    u8 online_number = 0;

    LORA_Get_Common_Send_Msg(msg, &sen_msg_len, command, NULL, 0);

    LORA_Send(msg, sen_msg_len);
    // 等待接受指令状态
    while (1)
    {
        delay_ms(1000);
        if (check_LORA_Receive())
        {
            LORA_Receive(receive_buf, &receive_len);
            //获取地址
            address[4] =receive_buf[6];
            address[5] =receive_buf[7];
            // 获取时间
            time[0] = receive_buf[append_len + 2] / 16 * 10 + receive_buf[append_len + 2] % 16;
            time[1] = receive_buf[append_len + 3] / 16 * 10 + receive_buf[append_len + 3] % 16;
            time[2] = receive_buf[append_len + 4] / 16 * 10 + receive_buf[append_len + 4] % 16;
            // 获取网络运行状态
            network_status = receive_buf[append_len + 6];
            // 获取档案总数，由两位组成，先只取了低位(由于返回数据中是颠倒的，因此这里取了高位，例如01 00)
            total_number = receive_buf[append_len + 7];
            // 获取实时在线总数，由两位组成，先只取了低位
            online_number = receive_buf[append_len + 15];

            if (is_debug)
            {
                printf("receive: ");
                for (i = 0; i < receive_len; i++)
                {
                    printf("%02x ", receive_buf[i]);
                }
                printf("\r\n");
                printf("time: %02d:%02d:%02d\r\n", time[0], time[1], time[2]);
                printf("network_status: %d\r\n", network_status);
                printf("total_number: %d\r\n", total_number);
                printf("online_number: %d\r\n", online_number);
            }

            flag_query_network = 1;
            break;
        }
        else
        {
            fail_count++;
            if (is_debug)
                printf("LORA_Query_Network_Status: No receive, fail=%d\r\n", fail_count);
            if (fail_count >= 5)
            {
                break;
            }
        }
    }
    return flag_query_network;
}

u8 LORA_Query_Slave_Node_Status(u8 is_debug)
{
    // 从节点状态查询
    u8 sen_msg_len, i;
    u8 append_len = 8; // 实际返回内容前的字符数
    u8 msg[20];
    u8 command[2] = {0x01, 0x02};
    u8 content_len = 2;
    u8 content[2];     // 起始序号，两字节
    u8 fail_count = 0; // 失败次数

    u8 receive_buf[50];
    u8 receive_len = 0;
    // 起始序号
    u8 start_num[2];
    // 当前页个数
    u8 current_page_num;

    content[0] = 0x00;
    content[1] = 0x00;

    LORA_Get_Common_Send_Msg(msg, &sen_msg_len, command, content, content_len);

    LORA_Send(msg, sen_msg_len);
    // 等待接受指令状态
    while (1)
    {
        delay_ms(1000);
        if (check_LORA_Receive())
        {
            LORA_Receive(receive_buf, &receive_len);

            if (is_debug)
            {
                printf("receive_len: %d\r\n", receive_len);
                printf("receive_buf: ");
                for (i = 0; i < receive_len; i = i + 1)
                {
                    printf("%x ", receive_buf[i]);
                }
                printf("\r\n");
            }

            // 起始序号
            start_num[0] = receive_buf[append_len];
            start_num[1] = receive_buf[append_len + 1];
            // 当前页个数
            current_page_num = receive_buf[append_len + 2];
            if (is_debug)
            {
                printf("start_num: %d, current_page_num: %d\r\n", start_num[0] * 256 + start_num[1], current_page_num);
                // 打印每个从模块地址6位，状态2位，版本2位
                for (i = 0; i < current_page_num; i = i + 1)
                {
                    printf("address: %02x%02x%02x%02x%02x%02x, status: %02x%02x, version: %02x%02x\r\n",
                           receive_buf[append_len + 8 + i * 10], receive_buf[append_len + 7 + i * 10], receive_buf[append_len + 6 + i * 10],
                           receive_buf[append_len + 5 + i * 10], receive_buf[append_len + 4 + i * 10], receive_buf[append_len + 3 + i * 10],
                           receive_buf[append_len + 9 + i * 10], receive_buf[append_len + 10 + i * 10],
                           receive_buf[append_len + 11 + i * 10], receive_buf[append_len + 12 + i * 10]);
                }
            }
            return 1;
        }
        else
        {
            printf("LORA_Query_Slave_Node_Status: No receive, fail=%d\r\n", ++fail_count);
            if (fail_count >= 10)
            {
                return 0;
            }
        }
    }
}

// 数据透传
void LORA_DATA_Transfer(u8 *buf, u8 buf_len, u8 *address)
{
    // 报文长度	msg_len = 8 + 6 + 2 + 1 + len + 2(0d0a) + 2
    // 需要自动给发送内容最后补上0x0d，0x0a
    // address:数据传输收方的地址
    // buf:待发送数据缓存器
    // buf_len:待发送数据长度
    // receive_buf:接收数据缓存器
    // receive_len:接收数据长度
    u8 p = 0, i;
    u8 sen_msg_len = 8 + 6 + 2 + 1 + buf_len + 2 + 2;
    u8 msg[200];
    u8 command[2] = {0x03, 0x05};
    u8 content_len = 6 + 2 + 1 + buf_len + 2; // address + 固定 + 长度位 + buf_len + 0d0a
    u8 content[200];
    u8 fail_count = 0; // 失败次数
    //u8 result_state = 99; // 0=模块通信成功、1=从模块通信失败;2=从模块组网失败、3=主节点任务繁忙、4=主节点解密错误,99=无返回

    u8 all_receive_buf[100];
    u8 all_receive_len = 0;

    for (i = 0; i < 6; i = i + 1)
    {
        content[p] = address[i]; // 8--13
        p++;
    }
    content[p] = 0x00;
    p++; // 14
    content[p] = 0x70;
    p++; // 15
    content[p] = buf_len+2;
    p++; // 16
    for (i = 0; i < buf_len; i = i + 1)
    {
        content[p] = buf[i]; // 17--17+len-1
        p++;
    }
    // 内容部分的结尾标志
    content[p++] = 0x0d;
    content[p++] = 0x0a;

    LORA_Get_Common_Send_Msg(msg, &sen_msg_len, command, content, content_len);

    // 打印一下透传发送的msg
    printf("LORA_DATA_Transfer_send_msg: \r\n");
    for (i = 0; i < sen_msg_len; i = i + 1)
    {
        printf("%02x ", msg[i]);
    }
    printf("\r\n");
    LORA_Send(msg, sen_msg_len);
	/*
    // 等待接受指令状态
    while (1)
    {
        delay_ms(1000);
        if (check_LORA_Receive())
        {

            LORA_Receive(all_receive_buf, &all_receive_len);
            result_state = all_receive_buf[14]&0xF0;
            *receive_len = all_receive_buf[16];
            printf("result_state: %d (0=success,1=communicate fail,2=net fail,3=busy,4=analysis fail)\r\n", result_state);
            printf("receive_len: %d\r\n", all_receive_len);
            printf("receive_buf: ");
            for (i = 0; i < all_receive_len; i = i + 1)
            {
                receive_buf[i] = all_receive_buf[17 + i];
                printf("%02x ", all_receive_buf[i]);
            }
            printf("\r\n");
            break;
        }
        else
        {
            printf("LORA_DATA_Transfer: No receive, fail=%d\r\n", ++fail_count);
            if (fail_count >= 5)
            {
                break;
            }
        }
    }
    return result_state;
	*/
}

// LORA主模块接收透传数据解析
u8 LORA_Receive_Data_Analysis(u8 *buf, u8 buf_len)
{
    // eg. 6C 10 09 03 06 80 B3 16 02 49 01 00 00 00 00 00 02 01 01 27 16
    u8 i;
    u8 address[6];
    u8 data_len;
    u8 data[100];
    u8 flag = 0;
    u8 node_location = 255; // 内存中存储的对应该从节点的位置

    // 目前时间向LORA主模块查询，约有0.5~1秒的延迟
    u8 time[3] = {0};
    // 风温度（四字节）、气压（四字节）湿度（四字节）、烟雾（四字节）、一氧化碳（四字节）、电池电量（四字节）
	u8 data_str[300];
	u8 isTime;
    // float wind_speed_f = 0;
    // float wind_direction_f = 0;
    float temperature_f = 0;
    float pressure_f = 0;
    float humidity_f = 0;
    float smoke_f = 0;
    float co_f = 0;
	float battery_f = 0;

    for (i = 0; i < 6; i = i + 1)
    {
        address[5-i] = buf[8 + i];
    }
    data_len = buf[16];
    for (i = 0; i < data_len; i = i + 1)
    {
        data[i] = buf[17 + i];
    }
    // 接收到的来自从节点的数据，从节点地址也是倒的，需要正过来
    printf("address: %02x%02x%02x%02x%02x%02x, data_len: %d, data: ",
           address[0], address[1], address[2], address[3], address[4], address[5], data_len);
    for (i = 0; i < data_len; i = i + 1)
    {
        printf("%x ", data[i]);
    }
    printf("\r\n");

    // 获取采样时间
    RTC_Get_Time(time);
    // printf("time: %02d:%02d:%02d\r\n", time[0], time[1], time[2]);
    // flag = LORA_Query_Network_Status(time, 0);
    // printf("LORA_Query_Time, Status=%d\r\n", flag);
    // if (flag != 1)
    //     return flag;

    flag = 0; // 重置flag
    // 解析传感器数据
    // for (i = 0; i < 4; i = i + 1)
    // {	//温 压 湿 风速风向
    //     temperature[i] = data[i];
    //     pressure[i] = data[4 + i];
    //     humidity[i] = data[8 + i];
    //     wind_speed[i] = data[12 + i];
    //     wind_direction[i] = data[16 + i];
    //     smoke[i] = data[20 + i];
	// 	battery[i] = data[24 + i];
    // }
    // 四字节u8转float
    // wind_speed_f = *(float *)wind_speed;
    // wind_direction_f = *(float *)wind_direction;
    temperature_f = u8_to_float(data);
    pressure_f = u8_to_float(data + 4);
    humidity_f = u8_to_float(data + 8);
    smoke_f = u8_to_float(data + 12);
    co_f = u8_to_float(data + 16);
	battery_f = u8_to_float(data + 20);
	isTime = RTC_check_device_time();
    // 打印时间和传感器数据
	sprintf(data_str, "address: %02x%02x%02x%02x%02x%02x\r\ntime: %02d:%02d:%02d\r\ntemperature: %f\r\npressure: %f\r\nhumidity: %f\r\n"
            "smoke: %f\r\nco: %f\r\nbattery: %f\r\nisTimeTrue: %d\r\n",
           address[0], address[1], address[2], address[3], address[4], address[5], time[0], time[1], time[2], temperature_f, pressure_f, humidity_f, 
           smoke_f,co_f, battery_f, isTime);
	puts(data_str);
	printf("data_str len:%d\r\n", strlen(data_str));
	printf("collect data from SNode...\r\n");
	// mqtt4g_send(data_str, strlen(data_str));
	// printf("data sent...\r\n");
    //printf("time: %02d:%02d:%02d, wind_speed: %f, wind_direction: %f, temperature: %f, pressure: %f, humidity: %f, smoke: %f\r\n",
           //time[0], time[1], time[2], wind_speed_f, wind_direction_f, temperature_f, pressure_f, humidity_f, smoke_f);
    // 保存传感器数据
    // 从全局变量SubNodeSet中查找是否有该地址的从节点

    node_location = LORA_Find_SubNode(address);
    if (node_location == 255)
    {
        // 未找到该从节点，添加该从节点
        SubNode new_node;
        for (i = 0; i < 6; i++)
        {
            new_node.address[i] = address[i];
        }
        SubNodeSet.SubNode_list[SubNodeSet.nNode] = new_node;
        SubNodeSet.nNode++;
        printf("Add new node,new address%02x %02x %02x %02x %02x %02x, nNode: %d\r\n",
               address[0], address[1], address[2], address[3], address[4], address[5], SubNodeSet.nNode);
        node_location = SubNodeSet.nNode - 1;
    }
    // 找到该从节点，更新该从节点的数据
    // SubNodeSet.SubNode_list[node_location].wind_speed = wind_speed_f;
    // SubNodeSet.SubNode_list[node_location].wind_direction = wind_direction_f;
    SubNodeSet.SubNode_list[node_location].temperature = temperature_f;
    SubNodeSet.SubNode_list[node_location].pressure = pressure_f;
    SubNodeSet.SubNode_list[node_location].humidity = humidity_f;
    SubNodeSet.SubNode_list[node_location].smoke = smoke_f;
    SubNodeSet.SubNode_list[node_location].co = co_f;
    SubNodeSet.SubNode_list[node_location].battery = battery_f;
    SubNodeSet.SubNode_list[node_location].sample_time[0] = time[0];
    SubNodeSet.SubNode_list[node_location].sample_time[1] = time[1];
    SubNodeSet.SubNode_list[node_location].sample_time[2] = time[2];
    SubNodeSet.SubNode_list[node_location].last_gps = last_time_gps;
    SubNodeSet.SubNode_list[node_location].SubNodeStatus = 3;   // 更新状态为已接收数据
    SubNodeSet.SubNode_list[node_location].fail_count = 0;      // 重置失败次数   
    flag = 1; // 更新成功
    return flag;
}

// 全网点名 03 01
u8 LORA_Network_Naming(void)
{
    // 报文长度:10
    u8 sen_msg_len;
    u8 msg[10];
    u8 command[2] = {0x03, 0x01};
    u8 fail_count = 0; // 失败次数

    u8 receive_buf[30];
    u8 receive_len = 0;
    u8 network_naming_flag = 0;

    LORA_Get_Common_Send_Msg(msg, &sen_msg_len, command, NULL, 0);

    LORA_Send(msg, sen_msg_len);

    while (1)
    {
        delay_ms(1000);
        if (check_LORA_Receive())
        {
            LORA_Receive(receive_buf, &receive_len);
            // 3,4位为确认位，00,01确认，00,02否认
            if (receive_buf[3] == 0x00 && receive_buf[4] == 0x01) // Confirm command
            {
                network_naming_flag = 1;
            }
            else if (receive_buf[3] == 0x00 && receive_buf[4] == 0x02) // deny command
            {
                network_naming_flag = 0;
            }
            break;
        }
        else
        {
            printf("LORA_Network_Naming: No receive, fail=%d\r\n", ++fail_count);
            if (fail_count >= 10)
            {
                break;
            }
        }
    }
    return network_naming_flag;
}

// 启动组网 03 03
u8 LORA_Network_Start(void)
{
    // 报文长度:10
    u8 sen_msg_len;
    u8 msg[10];
    u8 command[2] = {0x03, 0x03};
    u8 fail_count = 0; // 失败次数
    u8 i;

    u8 receive_buf[30];
    u8 receive_len = 0;
    u8 network_start_flag = 0;

    LORA_Get_Common_Send_Msg(msg, &sen_msg_len, command, NULL, 0);

    LORA_Send(msg, sen_msg_len);
    // 启动组网指令经常没有返回，直接不进行返回帧检测了
    delay_ms(1000);
    network_start_flag = 1;
    // while (1)
    // {
    //     delay_ms(1000);
    //     if (check_LORA_Receive())
    //     {
    //         LORA_Receive(receive_buf, &receive_len);
    //         // 打印一下receive_buf
    //         for (i = 0; i < receive_len; i++)
    //         {
    //             printf("%02x ", receive_buf[i]);
    //         }
            
    //         // 3,4位为确认位，00,01确认，00,02否认
    //         if (receive_buf[3] == 0x00 && receive_buf[4] == 0x01) // Confirm command
    //         {
    //             network_start_flag = 1;
    //         }
    //         else if (receive_buf[3] == 0x00 && receive_buf[4] == 0x02) // deny command
    //         {
    //             network_start_flag = 0;
    //         }
    //         break;
    //     }
    //     else
    //     {
    //         printf("LORA_Network_Start: No receive, fail=%d\r\n", ++fail_count);
    //         if (fail_count >= 5)
    //         {
    //             break;
    //         }
    //     }
    // }
    return network_start_flag;
}

// 检测当前地址的从节点是否存在，以及位置
u8 LORA_Find_SubNode(u8 *address)
{
    u8 i;
    u8 flag = 255; // 255不存在，其他为对应的存储位置
    for (i = 0; i < SubNodeSet.nNode; i = i + 1)
    {
        if (SubNodeSet.SubNode_list[i].address[0] == address[0] && SubNodeSet.SubNode_list[i].address[1] == address[1] &&
            SubNodeSet.SubNode_list[i].address[2] == address[2] && SubNodeSet.SubNode_list[i].address[3] == address[3] &&
            SubNodeSet.SubNode_list[i].address[4] == address[4] && SubNodeSet.SubNode_list[i].address[5] == address[5])
        {
            flag = i;
            break;
        }
    }
    return flag;
}

// // 自动同步gps时间并校时
// // 在中断中进行校时一直失败，现在弃用该函数，改为利用RTC时钟来进行校时
// u8 LORA_update_device_time()
// {
//     u8 flag_get_time = 0;   // 0:获取时间失败 1:获取时间成功 2:未找到卫星时间不准
//     u8 flag_set_time = 0;
//     u8 time[3];
//     u8 set_time_fail_count = 0; // 设置时间失败次数
//     u8 try_set_time_num;    // 尝试设置时间的次数
//     printf("last_time_gps=%d\r\n",last_time_gps);
//     last_time_gps++;
//     if(last_time_gps > 999)
//         // 防止last_time_gps溢出,999即为从未完成过校时
//         last_time_gps = 999;
//     // 如果距离上次gps同步时间超过了12个周期，就重新同步一次时间
//     if(last_time_gps >= 12)
//     {
//         printf("Try to get time from GPS!\r\n");
//         // 0.获取时间
//         flag_get_time = GPS_get_time(time);
//         printf("flag_get_time=%d\r\n",flag_get_time);
//         if (flag_get_time == 1)
//         {
//             printf("Get time success!\r\n");
//             try_set_time_num = 3;   // 如果获取时间成功，就尝试设置时间3次
//         }
//         else if (flag_get_time == 2)
//         {
//             printf("Get time fail, no satellite!\r\n");
            
//             try_set_time_num = 2;   // 如果获取时间失败，就尝试设置时间1次
//             if(last_time_gps<999)
//                 // 如果没有卫星，且在设备运行过程中有过一次成功的时间同步，就不再同步时间
//                 return 0;
//         }
//         else
//         {
//             printf("Get time fail!\r\n");
//             return 0;
//         }
//         // 1.设置时间
//         while (set_time_fail_count<try_set_time_num)
//         {
//             flag_set_time = LORA_Init_Time(time);
//             if (flag_set_time == 1)
//             {
//                 printf("Set time success!\r\n");
//                 if(flag_get_time == 1)
//                     last_time_gps = 0;
//                 break;
//             }
//             else
//             {
//                 set_time_fail_count++;
//                 printf("Set time fail! Try count=%d, All count=%d\r\n", try_set_time_num, set_time_fail_count);
//             }
//         }
//     }
//     if(flag_set_time == 1)
//         return 1;
//     else
//         return 0;
// }

// 查询从节点数据，address为从节点地址，正向存储
void LORA_Query_SubNode_Data(u8 *address)  
{
    u8 current_addr[6];     // 当前查询的从节点地址，倒过来存储
    u8 i;
    printf("query data from %02x%02x%02x%02x%02x%02x\r\n", address[0], address[1], address[2], address[3], address[4], address[5]);
    for (i = 0; i < 6; i++)
        current_addr[i] = address[5-i];		// 从节点地址要倒过来查询
    LORA_DATA_Transfer(query, 3, current_addr);
}

// 查询所有从节点数据
void LORA_Query_All_SubNode_Data(void)
{
    u8 i;
    for (i = 0; i < SubNodeSet.nNode; i++)
    {
        LORA_Query_SubNode_Data(SubNodeSet.SubNode_list[i].address);
    }
}

// 获取所有从节点数据
void LORA_Get_All_SubNode_Data(u8 *_all_data_str)
{
    u8 i;
    u8 temp_data_str[300];
    u8 end_str[2] = {0x0d, 0x0a};
    for (i = 0; i < SubNodeSet.nNode; i++) 
	{
		SubNode current_node = SubNodeSet.SubNode_list[i];
		// 生成当前子节点的数据字符串
		sprintf(temp_data_str,  "%02x%02x%02x%02x%02x%02x;"		// 节点地址
							    "%02d:%02d:%02d;"				// 时间
							    "%.2f;"							// 温度
							    "%.2f;"							// 气压
							    "%.2f;"							// 湿度				
							    "%.2f;"							// CO2浓度
							    "%.2f;"							// CO浓度
							    "%.2f;"							// 电池电压
							    "%d;",						    // RTC校时状态
                            current_node.address[0], current_node.address[1], current_node.address[2], current_node.address[3], current_node.address[4], current_node.address[5],
                            current_node.sample_time[0], current_node.sample_time[1], current_node.sample_time[2], current_node.temperature, current_node.pressure, current_node.humidity,
                            current_node.smoke, current_node.co, current_node.battery, RTC_check_specified_time(current_node.last_gps));
		// 将当前子节点的数据字符串拼接到all_data_str中
		strcat(_all_data_str, temp_data_str);
	}
    // 将结束符拼接到all_data_str中
    strcat(_all_data_str, end_str);
}

// 根据索引idx获取从节点数据
u8 LORA_Get_SubNode_Data_idx(u8 idx, u8 *_data_str)
{
    SubNode current_node = SubNodeSet.SubNode_list[idx];
    if (idx >= SubNodeSet.nNode)
    {
        printf("LORA_Get_SubNode_Data_idx: idx out of range!\r\n");
        return 0;
    }
    // 生成当前子节点的数据字符串
    sprintf(_data_str,  "%02x%02x%02x%02x%02x%02x;"		// 节点地址
                        "%02d:%02d:%02d;"				// 时间
                        "%.2f;"							// 温度
                        "%.2f;"							// 气压
                        "%.2f;"							// 湿度				
                        "%.2f;"							// CO2浓度
                        "%.2f;"							// CO浓度
                        "%.2f;"							// 电池电压
                        "%d;",						    // RTC校时状态
                        current_node.address[0], current_node.address[1], current_node.address[2], current_node.address[3], current_node.address[4], current_node.address[5],
                        current_node.sample_time[0], current_node.sample_time[1], current_node.sample_time[2], current_node.temperature, current_node.pressure, current_node.humidity,
                        current_node.smoke, current_node.co, current_node.battery, RTC_check_specified_time(current_node.last_gps));
    return 1;
}

void LORA_Update_All_SubNode_Status(void)
{
    // 用于更新所有从节点的状态，若在新一轮查询从节点数据时，某从节点依然是已发送（未响应）状态，则将其状态重置，失败次数+1
    u8 i;
    for (i = 0; i < SubNodeSet.nNode; i++)
    {
        if (SubNodeSet.SubNode_list[i].SubNodeStatus == 2)
        {
            
            if (SubNodeSet.SubNode_list[i].fail_count < 255)
                SubNodeSet.SubNode_list[i].fail_count ++;
        }
        SubNodeSet.SubNode_list[i].SubNodeStatus = 1;   // 已接收数据也要重置为正常状态
    }
}
