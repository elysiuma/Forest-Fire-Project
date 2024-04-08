#ifndef __LORA_H
#define __LORA_H
#include "sys.h"

//RS066/067 Loraģ�飬оƬRX1278

#define LORA PDout(4);  //loraģ�鿪��

// ����ӽڵ����ݽṹ�������ӽڵ��ַ�ʹӽڵ�״̬���Լ����ݣ��ͻ�ȡʱ��
typedef struct
{
    u8 address[6];   //�ӽڵ��ַ
    u8 SubNodeStatus;   //�ӽڵ�״̬
    float wind_speed; //����
    float wind_direction; //����
    float temperature;    //�¶�
    float pressure;       //��ѹ
    float humidity;       //ʪ��
    float smoke;          //����
    u8 sample_time[3];  //����ʱ��
    u16 last_gps;   //������һ��ͬ��gpsʱ�����ʱ�����ڣ�5����һ�����ڣ�12��Ϊ1Сʱ�� 999Ϊδͬ��
} SubNode;
// �ӽڵ㼯��
typedef struct
{
    u8 nNode;   //�ӽڵ�����
    SubNode SubNode_list[30];    //�ӽڵ㼯��
} SubNodeSetStruct;

// ��ʼ���ӽڵ㼯�϶���
extern SubNodeSetStruct SubNodeSet;
// extern u16 last_time_gps;     // ������һ��ͬ��gpsʱ�����ʱ�����ڣ�5����һ�����ڣ�12��Ϊ1Сʱ�� 999Ϊδͬ��
extern u8 is_lora_init;     // �Ƿ��Ѿ���ʼ������
extern u8 get_data_flag;
extern u8 nNode;
extern u8 SubNodeAddress[120];

u8 LORA_Init(void);
void LORA_Send(u8 *buf,u8 len);
void LORA_Receive(u8 *buf,u8 *len);
void LORA_Get_Common_Send_Msg(u8 *msg, u8 *msg_len, u8 *command, u8 *content, u8 content_len);
u8 check_LORA_Receive(void);
u8 LORA_Network_Init(void); //һ����ʼ������
u8 LORA_Init_Time(u8 *time);
u8 LORA_Add_Slave_Node(u8 nNode, u8 *SubNodeAddress);
u8 LORA_Query_Network_Status(u8 *time, u8 is_debug);     // ��ѯ����״̬
u8 LORA_Query_Slave_Node_Status(u8 is_debug); //��ѯ�ӽڵ�״̬
void LORA_DATA_Transfer(u8 *buf, u8 buf_len, u8 *address); //���ݴ���
u8 LORA_Network_Clear(void);    //�������
u8 LORA_Receive_Data_Analysis(u8 *buf, u8 buf_len);  //�������ݽ���
u8 LORA_Network_Naming(void);   // ȫ������
u8 LORA_Network_Start(void);    // ��������
u8 LORA_Find_SubNode(u8 *address);  // ���Ҵӽڵ�
u8 LORA_update_device_time(void);   // �����豸ʱ��

#endif

