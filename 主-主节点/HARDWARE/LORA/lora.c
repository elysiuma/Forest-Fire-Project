#include "lora.h"
#include "usart2.h"
#include "delay.h"
#include "gps.h"
#include "rtc.h"
#include "string.h"
#include "mqtt4g.h"

// ����ȫ�ֱ���
SubNodeSetStruct SubNodeSet;
// u16 last_time_gps = 999; 
u8 is_lora_init = 0;
u8 get_data_flag = 0;
u8 nNode = 1;
u8 SubNodeAddress[120] = {
        // 0x36, 0x49, 0x01, 0x00, 0x00, 0x00,
        // 0x28, 0x49, 0x01, 0x00, 0x00, 0x00,
        0x26, 0x49, 0x01, 0x00, 0x00, 0x00,
        // 0x27, 0x49, 0x01, 0x00, 0x00, 0x00,
        // 0x29, 0x49, 0x01, 0x00, 0x00, 0x00,
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

    // lora�ĵ�һ��ָ�����Ӧ��Ҫ������
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
    // ��������������򷵻�1
    if (USART2_RX_CNT & 0x8000)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

// ��ȡLORAģ��ͨ�÷�������
void LORA_Get_Common_Send_Msg(
    u8 *send_msg,    // �����͵ı���
    u8 *sen_msg_len, // ���ĳ���
    u8 *command,     // �����λ
    u8 *content_buf, // ����
    u8 content_len   // ���ݳ���
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

// LORAģ���ʼ��ʱ��
u8 LORA_Init_Time(u8 *time)
{
    // ���ĳ���:13
    // time:ʱ���֡���
    u8 i;
    u8 sen_msg_len = 13;
    u8 msg[13];
    u8 content[3];
    u8 command[2] = {0x02, 0x04};
    u8 fail_count = 0; // ʧ�ܴ���

    u8 receive_buf[30];
    u8 receive_len = 0;
    u8 set_time_flag = 0; // 0:����ʱ��ʧ�� 1:����ʱ��ɹ�

    for (i = 0; i < 3; i++)
    {
        // 10����ת16���ƣ�16��������10����������ͬ
        content[i] = time[i] / 10 * 16 + time[i] % 10;
    }
    LORA_Get_Common_Send_Msg(msg, &sen_msg_len, command, content, 3);

    // ��ӡһ��msg
    printf("msg: ");
    for (i = 0; i < sen_msg_len; i++)
    {
        printf("%02x ", msg[i]);
    }
    printf("\r\n");
    LORA_Send(msg, sen_msg_len);
    // �ȴ�����ָ��״̬
    while (1)
    {
        delay_ms(1000);
        if (check_LORA_Receive())
        {
            LORA_Receive(receive_buf, &receive_len);
            // 3,4λΪȷ��λ��00,01ȷ�ϣ�00,02����
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

// LORAģ̬��Ӵӽڵ��ַ����
u8 LORA_Add_Slave_Node(u8 nNode, u8 *SubNodeAddress)
{
    // ���ĳ���	msg_len = 8 + 1 + nNode*6 + 2
    // nNode:�ڵ�����
    // SubNodeAddress:ÿ���ڵ�ĵ�ַ ��ά����
    u8 n, m;
    u8 sen_msg_len = 8 + 1 + nNode * 6 + 2;
    u8 msg[150];
    u8 content[150];
    u8 command[2] = {0x02, 0x08};
    u8 fail_count = 0; // ʧ�ܴ���

    u8 receive_buf[30];
    u8 receive_len = 0;
    u8 add_slave_node_flag = 0;

    // һ֡�������30���ӽڵ��ַ
    if (nNode > 30)
    {
        return 0;
    }

    content[0] = nNode;
    for (n = 0; n < nNode; n = n + 1)
    {
        for (m = 0; m < 6; m = m + 1)
        {
            content[1 + n * 6 + m] = SubNodeAddress[n * 6 + m];
        }
    }
    LORA_Get_Common_Send_Msg(msg, &sen_msg_len, command, content, 1 + nNode * 6);

    LORA_Send(msg, sen_msg_len);

    while (1)
    {
        if (check_LORA_Receive())
        {
            delay_ms(1000);
            LORA_Receive(receive_buf, &receive_len);
            // 3,4λΪȷ��λ��00,01ȷ�ϣ�00,02����
            if (receive_buf[3] == 0x00 && receive_buf[4] == 0x01) // Confirm command
            {
                add_slave_node_flag = 1;
                // ��ʼ���ӽڵ㵵��
                for (m = 0; m < nNode; m++)
                {
                    u8 new_address[6];
                    u8 node_location = 255;
                    for (n = 0; n < 6; n++)
                    {
                        new_address[n] = SubNodeAddress[m * 6 + n];
                    }
                    node_location = LORA_Find_SubNode(new_address);
                    // ��ǰ�ӽڵ㲻�����ٽ������
                    if (node_location == 255)
                    {
                        SubNode new_node;
                        for (n = 0; n < 6; n++)
                        {
                            new_node.address[n] = SubNodeAddress[m * 6 + n];
                        }
                        new_node.SubNodeStatus = 0;
                        new_node.wind_speed = 0;
                        new_node.wind_direction = 0;
                        new_node.temperature = 0;
                        new_node.humidity = 0;
                        new_node.pressure = 0;
                        new_node.smoke = 0;
                        new_node.sample_time[0] = 0;
                        new_node.sample_time[1] = 0;
                        new_node.sample_time[2] = 0;
                        SubNodeSet.SubNode_list[SubNodeSet.nNode] = new_node;
                        SubNodeSet.nNode++;
                        printf("add new SubNodeAddress[%d] = %02x %02x %02x %02x %02x %02x\r\n", m, SubNodeAddress[m * 6 + 5], SubNodeAddress[m * 6 + 4], SubNodeAddress[m * 6 + 3], SubNodeAddress[m * 6 + 2], SubNodeAddress[m * 6 + 1], SubNodeAddress[m * 6]);
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

// LORAģ�������������
u8 LORA_Network_Clear(void)
{
    // ���ĳ���:10
    u8 sen_msg_len;
    u8 msg[10];
    u8 command[2] = {0x02, 0x02};
    u8 fail_count = 0; // ʧ�ܴ���

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
            // 3,4λΪȷ��λ��00,01ȷ�ϣ�00,02����
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

// LORAģ��һ����ʼ��
u8 LORA_Network_Init(void)
{
    u8 time[3];
    u8 flag_set_time = 0;       // 0:����ʱ��ʧ�� 1:����ʱ��ɹ�
    u8 flag_add_slave_node = 0; // 0:��Ӵӽڵ�ʧ�� 1:��Ӵӽڵ�ɹ�
    u8 flag_network_start = 0;  // 0:��������ʧ�� 1:���������ɹ�

    printf("LORA network init start!\r\n");
    // 1.����ͬ��ʱ�䣬ֱ������ʱ��
    time[0]=0x00;
    time[1]=0x00;
    time[2]=0x00;
    flag_set_time = LORA_Init_Time(time);
    printf("flag_set_time=%d\r\n", flag_set_time);
    //if (flag_set_time != 1)
       // return 0;

    // 2.��Ӵӽڵ��ַ����
    SubNodeSet.nNode = 0; // �ӽڵ���������
    flag_add_slave_node = LORA_Add_Slave_Node(nNode, SubNodeAddress);
    if (flag_add_slave_node == 1)
    {
        printf("Add slave node success!\r\n");
    }
    else
    {
        printf("Add slave node fail!\r\n");
        return 0;
    }

    // 3.��������
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
    // ��Ҫ�ӳ�15�룬�ȴ��������
    delay_ms(15 * 1000);
    printf("LORA network init success!\r\n");
    return 1;
}

// ��ѯ����״̬
u8 LORA_Query_Network_Status(u8 *time, u8 is_debug)
{
    // time: ʱ����
    // is_debug: 0:����ӡ������Ϣ 1:��ӡ������Ϣ
    // ���ĳ���:8
    u8 i;
    u8 append_len = 8; // ʵ�ʷ�������ǰ���ַ���
    u8 sen_msg_len = 10;
    u8 msg[10];
    u8 command[2] = {0x01, 0x01};
    u8 fail_count = 0;         // ʧ�ܴ���
    u8 flag_query_network = 0; // 0:��ѯ����״̬ʧ�� 1:��ѯ����״̬�ɹ�

    u8 receive_buf[50];
    u8 receive_len = 0;

    // ģ���ڲ�ʱ��
    // u8 time[3]; // ʱ���� ʹ�ò��������з���
    // ��������״̬��0=���У�1=������2=����
    u8 network_status = 0;
    // �����������û������ӽڵ���ܸ���
    u8 total_number = 0;
    // ʵʱ��������
    u8 online_number = 0;

    LORA_Get_Common_Send_Msg(msg, &sen_msg_len, command, NULL, 0);

    LORA_Send(msg, sen_msg_len);
    // �ȴ�����ָ��״̬
    while (1)
    {
        delay_ms(1000);
        if (check_LORA_Receive())
        {
            LORA_Receive(receive_buf, &receive_len);
            // ��ȡʱ��
            time[0] = receive_buf[append_len + 2] / 16 * 10 + receive_buf[append_len + 2] % 16;
            time[1] = receive_buf[append_len + 3] / 16 * 10 + receive_buf[append_len + 3] % 16;
            time[2] = receive_buf[append_len + 4] / 16 * 10 + receive_buf[append_len + 4] % 16;
            // ��ȡ��������״̬
            network_status = receive_buf[append_len + 6];
            // ��ȡ��������������λ��ɣ���ֻȡ�˵�λ(���ڷ����������ǵߵ��ģ��������ȡ�˸�λ������01 00)
            total_number = receive_buf[append_len + 7];
            // ��ȡʵʱ��������������λ��ɣ���ֻȡ�˵�λ
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
    // �ӽڵ�״̬��ѯ
    u8 sen_msg_len, i;
    u8 append_len = 8; // ʵ�ʷ�������ǰ���ַ���
    u8 msg[20];
    u8 command[2] = {0x01, 0x02};
    u8 content_len = 2;
    u8 content[2];     // ��ʼ��ţ����ֽ�
    u8 fail_count = 0; // ʧ�ܴ���

    u8 receive_buf[50];
    u8 receive_len = 0;
    // ��ʼ���
    u8 start_num[2];
    // ��ǰҳ����
    u8 current_page_num;

    content[0] = 0x00;
    content[1] = 0x00;

    LORA_Get_Common_Send_Msg(msg, &sen_msg_len, command, content, content_len);

    LORA_Send(msg, sen_msg_len);
    // �ȴ�����ָ��״̬
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

            // ��ʼ���
            start_num[0] = receive_buf[append_len];
            start_num[1] = receive_buf[append_len + 1];
            // ��ǰҳ����
            current_page_num = receive_buf[append_len + 2];
            if (is_debug)
            {
                printf("start_num: %d, current_page_num: %d\r\n", start_num[0] * 256 + start_num[1], current_page_num);
                // ��ӡÿ����ģ���ַ6λ��״̬2λ���汾2λ
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

// ����͸��
void LORA_DATA_Transfer(u8 *buf, u8 buf_len, u8 *address)
{
    // ���ĳ���	msg_len = 8 + 6 + 2 + 1 + len + 2(0d0a) + 2
    // ��Ҫ�Զ����������������0x0d��0x0a
    // address:���ݴ����շ��ĵ�ַ
    // buf:���������ݻ�����
    // buf_len:���������ݳ���
    // receive_buf:�������ݻ�����
    // receive_len:�������ݳ���
    u8 p = 0, i;
    u8 sen_msg_len = 8 + 6 + 2 + 1 + buf_len + 2 + 2;
    u8 msg[200];
    u8 command[2] = {0x03, 0x05};
    u8 content_len = 6 + 2 + 1 + buf_len + 2; // address + �̶� + ����λ + buf_len + 0d0a
    u8 content[200];
    u8 fail_count = 0; // ʧ�ܴ���
    //u8 result_state = 99; // 0=ģ��ͨ�ųɹ���1=��ģ��ͨ��ʧ��;2=��ģ������ʧ�ܡ�3=���ڵ�����æ��4=���ڵ���ܴ���,99=�޷���

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
    // ���ݲ��ֵĽ�β��־
    content[p++] = 0x0d;
    content[p++] = 0x0a;

    LORA_Get_Common_Send_Msg(msg, &sen_msg_len, command, content, content_len);

    // ��ӡһ��͸�����͵�msg
    printf("LORA_DATA_Transfer_send_msg: \r\n");
    for (i = 0; i < sen_msg_len; i = i + 1)
    {
        printf("%02x ", msg[i]);
    }
    printf("\r\n");
    LORA_Send(msg, sen_msg_len);
	/*
    // �ȴ�����ָ��״̬
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

// LORA��ģ�����͸�����ݽ���
u8 LORA_Receive_Data_Analysis(u8 *buf, u8 buf_len)
{
    // eg. 6C 10 09 03 06 80 B3 16 02 49 01 00 00 00 00 00 02 01 01 27 16
    u8 i;
    u8 address[6];
    u8 data_len;
    u8 data[100];
    u8 flag = 0;
    u8 node_location = 255; // �ڴ��д洢�Ķ�Ӧ�ôӽڵ��λ��

    // Ŀǰʱ����LORA��ģ���ѯ��Լ��0.5~1����ӳ�
    u8 time[3] = {0};
    // ���ٷ��򣨷���4�ֽڣ�����4���ֽڣ����¶ȣ����ֽڣ�����ѹ�����ֽڣ�ʪ�ȣ����ֽڣ����������ֽڣ�
    u8 wind_speed[4] = {0};
    u8 wind_direction[4] = {0};
    u8 temperature[4] = {0};
    u8 pressure[4] = {0};
    u8 humidity[4] = {0};
    u8 smoke[4] = {0};
	u8 battery[4] = {0};
	u8 data_str[300];
	u8 isTime;
    float wind_speed_f = 0;
    float wind_direction_f = 0;
    float temperature_f = 0;
    float pressure_f = 0;
    float humidity_f = 0;
    float smoke_f = 0;
	float battery_f = 0;

    for (i = 0; i < 6; i = i + 1)
    {
        address[i] = buf[8 + i];
    }
    data_len = buf[16];
    for (i = 0; i < data_len; i = i + 1)
    {
        data[i] = buf[17 + i];
    }

    printf("address: %02x%02x%02x%02x%02x%02x, data_len: %d, data: ",
           address[5], address[4], address[3], address[2], address[1], address[0], data_len);
    for (i = 0; i < data_len; i = i + 1)
    {
        printf("%x ", data[i]);
    }
    printf("\r\n");

    // ��ȡ����ʱ��
    RTC_Get_Time(time);
    printf("time: %02d:%02d:%02d\r\n", time[0], time[1], time[2]);
    // flag = LORA_Query_Network_Status(time, 0);
    // printf("LORA_Query_Time, Status=%d\r\n", flag);
    // if (flag != 1)
    //     return flag;

    flag = 0; // ����flag
    // ��������������
    for (i = 0; i < 4; i = i + 1)
    {	//�� ѹ ʪ ���ٷ���
        temperature[i] = data[i];
        pressure[i] = data[4 + i];
        humidity[i] = data[8 + i];
        wind_speed[i] = data[12 + i];
        wind_direction[i] = data[16 + i];
        smoke[i] = data[20 + i];
		battery[i] = data[24 + i];
    }
    // ���ֽ�u8תfloat
    wind_speed_f = *(float *)wind_speed;
    wind_direction_f = *(float *)wind_direction;
    temperature_f = *(float *)temperature;
    pressure_f = *(float *)pressure;
    humidity_f = *(float *)humidity;
    smoke_f = *(float *)smoke;
	battery_f = *(float *)battery;
	isTime = RTC_check_device_time();
    // ��ӡʱ��ʹ���������
	sprintf(data_str, "address: %02x%02x%02x%02x%02x%02x\r\ntime: %02d:%02d:%02d\r\ntemperature: %f\r\npressure: %f\r\nhumidity: %f\r\nwind_speed: %f\r\n"
	"wind_direction: %f\r\nsmoke: %f\r\nbattery: %f\r\nisTimeTrue: %d\r\n",
           address[5], address[4], address[3], address[2], address[1], address[0], time[0], time[1], time[2], temperature_f, pressure_f, humidity_f, wind_speed_f, 
	wind_direction_f, smoke_f, battery_f, isTime);
	puts(data_str);
	printf("data_str len:%d\r\n", strlen(data_str));
	printf("sending data to server...\r\n");
	mqtt4g_send(data_str, strlen(data_str));
	printf("data sent...\r\n");
    //printf("time: %02d:%02d:%02d, wind_speed: %f, wind_direction: %f, temperature: %f, pressure: %f, humidity: %f, smoke: %f\r\n",
           //time[0], time[1], time[2], wind_speed_f, wind_direction_f, temperature_f, pressure_f, humidity_f, smoke_f);
    // ���洫��������
    // ��ȫ�ֱ���SubNodeSet�в����Ƿ��иõ�ַ�Ĵӽڵ�

    node_location = LORA_Find_SubNode(address);
    if (node_location == 255)
    {
        // δ�ҵ��ôӽڵ㣬��Ӹôӽڵ�
        SubNode new_node;
        for (i = 0; i < 6; i++)
        {
            new_node.address[i] = address[i];
        }
        SubNodeSet.SubNode_list[SubNodeSet.nNode] = new_node;
        SubNodeSet.nNode++;
        printf("Add new node,new address%02x %02x %02x %02x %02x %02x, nNode: %d\r\n",
               address[5], address[4], address[3], address[2], address[1], address[0], SubNodeSet.nNode);
        node_location = SubNodeSet.nNode - 1;
    }
    // �ҵ��ôӽڵ㣬���¸ôӽڵ������
    SubNodeSet.SubNode_list[node_location].wind_speed = wind_speed_f;
    SubNodeSet.SubNode_list[node_location].wind_direction = wind_direction_f;
    SubNodeSet.SubNode_list[node_location].temperature = temperature_f;
    SubNodeSet.SubNode_list[node_location].pressure = pressure_f;
    SubNodeSet.SubNode_list[node_location].humidity = humidity_f;
    SubNodeSet.SubNode_list[node_location].smoke = smoke_f;
    SubNodeSet.SubNode_list[node_location].sample_time[0] = time[0];
    SubNodeSet.SubNode_list[node_location].sample_time[1] = time[1];
    SubNodeSet.SubNode_list[node_location].sample_time[2] = time[2];
    SubNodeSet.SubNode_list[node_location].last_gps = last_time_gps;
    flag = 1; // ���³ɹ�
    return flag;
}

// ȫ������ 03 01
u8 LORA_Network_Naming(void)
{
    // ���ĳ���:10
    u8 sen_msg_len;
    u8 msg[10];
    u8 command[2] = {0x03, 0x01};
    u8 fail_count = 0; // ʧ�ܴ���

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
            // 3,4λΪȷ��λ��00,01ȷ�ϣ�00,02����
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

// �������� 03 03
u8 LORA_Network_Start(void)
{
    // ���ĳ���:10
    u8 sen_msg_len;
    u8 msg[10];
    u8 command[2] = {0x03, 0x03};
    u8 fail_count = 0; // ʧ�ܴ���
    u8 i;

    u8 receive_buf[30];
    u8 receive_len = 0;
    u8 network_start_flag = 0;

    LORA_Get_Common_Send_Msg(msg, &sen_msg_len, command, NULL, 0);

    LORA_Send(msg, sen_msg_len);
    // ��������ָ���û�з��أ�ֱ�Ӳ����з���֡�����
    delay_ms(1000);
    network_start_flag = 1;
    // while (1)
    // {
    //     delay_ms(1000);
    //     if (check_LORA_Receive())
    //     {
    //         LORA_Receive(receive_buf, &receive_len);
    //         // ��ӡһ��receive_buf
    //         for (i = 0; i < receive_len; i++)
    //         {
    //             printf("%02x ", receive_buf[i]);
    //         }
            
    //         // 3,4λΪȷ��λ��00,01ȷ�ϣ�00,02����
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
    //         if (fail_count >= 10)
    //         {
    //             break;
    //         }
    //     }
    // }
    return network_start_flag;
}

// ��⵱ǰ��ַ�Ĵӽڵ��Ƿ���ڣ��Լ�λ��
u8 LORA_Find_SubNode(u8 *address)
{
    u8 i;
    u8 flag = 255; // 255�����ڣ�����Ϊ��Ӧ�Ĵ洢λ��
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

// // �Զ�ͬ��gpsʱ�䲢Уʱ
// // ���ж��н���Уʱһֱʧ�ܣ��������øú�������Ϊ����RTCʱ��������Уʱ
// u8 LORA_update_device_time()
// {
//     u8 flag_get_time = 0;   // 0:��ȡʱ��ʧ�� 1:��ȡʱ��ɹ� 2:δ�ҵ�����ʱ�䲻׼
//     u8 flag_set_time = 0;
//     u8 time[3];
//     u8 set_time_fail_count = 0; // ����ʱ��ʧ�ܴ���
//     u8 try_set_time_num;    // ��������ʱ��Ĵ���
//     printf("last_time_gps=%d\r\n",last_time_gps);
//     last_time_gps++;
//     if(last_time_gps > 999)
//         // ��ֹlast_time_gps���,999��Ϊ��δ��ɹ�Уʱ
//         last_time_gps = 999;
//     // ��������ϴ�gpsͬ��ʱ�䳬����12�����ڣ�������ͬ��һ��ʱ��
//     if(last_time_gps >= 12)
//     {
//         printf("Try to get time from GPS!\r\n");
//         // 0.��ȡʱ��
//         flag_get_time = GPS_get_time(time);
//         printf("flag_get_time=%d\r\n",flag_get_time);
//         if (flag_get_time == 1)
//         {
//             printf("Get time success!\r\n");
//             try_set_time_num = 3;   // �����ȡʱ��ɹ����ͳ�������ʱ��3��
//         }
//         else if (flag_get_time == 2)
//         {
//             printf("Get time fail, no satellite!\r\n");
            
//             try_set_time_num = 2;   // �����ȡʱ��ʧ�ܣ��ͳ�������ʱ��1��
//             if(last_time_gps<999)
//                 // ���û�����ǣ������豸���й������й�һ�γɹ���ʱ��ͬ�����Ͳ���ͬ��ʱ��
//                 return 0;
//         }
//         else
//         {
//             printf("Get time fail!\r\n");
//             return 0;
//         }
//         // 1.����ʱ��
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
