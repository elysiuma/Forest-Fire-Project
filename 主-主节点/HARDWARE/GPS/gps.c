#include "gps.h"
#include "usart3.h"
#include "delay.h"
#include "stdlib.h"

float node_position[4] = {0,0,0,0}; // �ڵ�λ��,γ�ȡ��ֺ;��ȡ���
char node_lati_longi_str[2] = {'\0','\0'}; // �ڵ�γ�ȡ����ȱ�ʶ��N/S,E/W��

void GPS_Init(void)
{
    // ����GGAЭ�鴫����λ1��
    u8 GPS_GGA_MODE[16] = {0xA0, 0xA1, 0x00, 0x09, 0x08, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x08, 0x0D, 0x0A};
    uart3_init(38400);
    GPS_Send(GPS_GGA_MODE, 16); // ��GPS����Э������ΪGGA
}
/*
u8 * GPS_gettime(void)
{

}
*/
void GPS_Send(u8 *buf, u8 len)
{
    USART3_DATA(buf, len);
}

void GPS_Receive(u8 *buf, u8 *len)
{
    // printf("len: %i", *len);
    USART3_Receive_Data(buf, len);
}

u8 check_GPS_Receive(void)
{
    // printf("USART2_RX_CNT: %i", USART2_RX_CNT);
    // ��������������򷵻�1
    if (USART3_RX_STA & 0x8000)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

// GPS��ȡ���ݣ����ص�����ȷ��ʱ��
u8 GPS_get_time(u8 *time)
{
    u8 t;
    u8 temp_rec[300];
    u8 rec_len = 0;
    u8 fail_count = 0;
    u8 start_idx=0;     // ��ʼλ�ã���ʱ����յ�����$����Ҫ����һ�¿�ʼλ��

    // GPS����
    u8 GPS_Quality = 0; // 0:��Ч 1:GPS���㶨λ 2:DGPS��ֶ�λ 43λ
    // ��������
    u8 GPS_Satellite_Num = 0;   // 45,46λ

    while (1)
    {
        printf("GPS_get_time\r\n");
        // ��������
        if(check_GPS_Receive())
        {
            printf("check_GPS_Receive\r\n");
            GPS_Receive(temp_rec, &rec_len);
        }
        printf("rec_len: %i\r\n", rec_len);
        // ��ӡtemp_rec
        for (t = 0; t < rec_len; t++)
        {
            printf("%c", temp_rec[t]); // �򴮿�4��������
        }
        printf("\r\n");
        if (rec_len == 0)
        {
            fail_count++;
            if (fail_count > 3)
            {
                return 0;
            }
            delay_ms(1500);
            continue;
        }
        // // ���Դ�ӡ����
        // printf("GPS Receive: ");
        // for (t = 0; t < rec_len; t++)
        // {
        //     printf("%c", temp_rec[t]); // �򴮿�4��������
        // }
        // printf("\r\n");
        // ����Ƿ���������GPS���ݰ�
        // $GNGGA��$$GNGGA
        if ((temp_rec[3] == 0x47 && temp_rec[4] == 0x47 && temp_rec[5] == 0x41) ||
            (temp_rec[0] == '$' && temp_rec[4] == 0x48 && temp_rec[5] == 0x47 && temp_rec[6] == 0x41))
        {
            // �ҵ���ʼλ��
            if(temp_rec[3] == 0x47 && temp_rec[4] == 0x47 && temp_rec[5] == 0x41)
                start_idx = 0;
            else
                start_idx = 1;
            // ʱ
            time[0] = 10 * (temp_rec[7+start_idx] - '0') + (temp_rec[8+start_idx] - '0'); // UTCʱ��
            time[0] += 8;                                           // ����ʱ��
            if (time[0] >= 24)
                time[0] -= 24;
            // time[0] = (time[0] / 10) * 16 + time[0] % 10; // ת��Ϊ16����
            // ��
            time[1] = 10 * (temp_rec[9+start_idx] - '0') + (temp_rec[10+start_idx] - '0');
            // ��
            time[2] = 10 * (temp_rec[11+start_idx] - '0') + (temp_rec[12+start_idx] - '0');
            // GPS����
            GPS_Quality = temp_rec[43+start_idx]-'0';
            // ��������
            GPS_Satellite_Num = 10 * (temp_rec[45+start_idx] - '0') + (temp_rec[46]+start_idx - '0');

            // ��ӡʱ��
            //printf("GPS Time: %d:%d:%d quality:%d, gps_num:%d\r\n", time[0], time[1], time[2],GPS_Quality,GPS_Satellite_Num);
           
            if (GPS_Quality == 0)
            {
                printf("GPS Quality is invalid\r\n");
                return 2;
            }
            return 1;
            // u8 flag;
            // printf("\r\n");
            // flag = GPS_Receive_Data_Analysis(temp_rec, rec_len);
            // if (flag == 1)
            // {
            // 	printf("GPS Receive Data Analysis Success\r\n");
            // }
            // else
            // {
            // 	printf("GPS Receive Data Analysis Fail\r\n");
            // }
        }
        // printf("\r\n");//���뻻��
        //  printf("Receive %d,%d, len=%d", temp_rec[0],temp_rec[1],rec_len);
        rec_len = 0;
        fail_count++;
        if (fail_count > 10)
        {
            return 0;
        }
        delay_ms(2000);
    }
}

// GPS��ȡ���ݣ����ص�����ȷ��ʱ��,λ����Ϣ
u8 GPS_get_time_and_pos(u8 *time, float *_position, char *_lati_longi_str)
{
    u8 t;
    u8 temp_rec[300];
    char temp_pos_char[9] = {'\0'}; // ��ʱλ����Ϣ����
    u8 rec_len = 0;
    u8 fail_count = 0;
    u8 start_idx=0;     // ��ʼλ�ã���ʱ����յ�����$����Ҫ����һ�¿�ʼλ��

    // GPS����
    u8 GPS_Quality = 0; // 0:��Ч 1:GPS���㶨λ 2:DGPS��ֶ�λ 43λ
    // ��������
    u8 GPS_Satellite_Num = 0;   // 45,46λ

    while (1)
    {
        printf("GPS_get_time\r\n");
        // ��������
        if(check_GPS_Receive())
        {
            printf("check_GPS_Receive\r\n");
            GPS_Receive(temp_rec, &rec_len);
        }
        printf("rec_len: %i\r\n", rec_len);
        // ��ӡtemp_rec
        for (t = 0; t < rec_len; t++)
        {
            printf("%c", temp_rec[t]); // �򴮿�4��������
        }
        printf("\r\n");
        if (rec_len == 0)
        {
            fail_count++;
            if (fail_count > 3)
            {
                return 0;
            }
            delay_ms(1500);
            continue;
        }
        // // ���Դ�ӡ����
        // printf("GPS Receive: ");
        // for (t = 0; t < rec_len; t++)
        // {
        //     printf("%c", temp_rec[t]); // �򴮿�4��������
        // }
        // printf("\r\n");
        // ����Ƿ���������GPS���ݰ�
        // $GNGGA��$$GNGGA
        if ((temp_rec[3] == 0x47 && temp_rec[4] == 0x47 && temp_rec[5] == 0x41) ||
            (temp_rec[0] == '$' && temp_rec[4] == 0x48 && temp_rec[5] == 0x47 && temp_rec[6] == 0x41))
        {
            // �ҵ���ʼλ��
            if(temp_rec[3] == 0x47 && temp_rec[4] == 0x47 && temp_rec[5] == 0x41)
                start_idx = 0;
            else
                start_idx = 1;
            // ʱ
            time[0] = 10 * (temp_rec[7+start_idx] - '0') + (temp_rec[8+start_idx] - '0'); // UTCʱ��
            time[0] += 8;                                           // ����ʱ��
            if (time[0] >= 24)
                time[0] -= 24;
            // time[0] = (time[0] / 10) * 16 + time[0] % 10; // ת��Ϊ16����
            // ��
            time[1] = 10 * (temp_rec[9+start_idx] - '0') + (temp_rec[10+start_idx] - '0');
            // ��
            time[2] = 10 * (temp_rec[11+start_idx] - '0') + (temp_rec[12+start_idx] - '0');
            // GPS����
            GPS_Quality = temp_rec[45+start_idx]-'0';
            // ��������
            GPS_Satellite_Num = 10 * (temp_rec[47+start_idx] - '0') + (temp_rec[48+start_idx] - '0');

            // ��ӡʱ��
            //printf("GPS Time: %d:%d:%d quality:%d, gps_num:%d\r\n", time[0], time[1], time[2],GPS_Quality,GPS_Satellite_Num);
            // ��γ��
            _position[0] = 10 * (temp_rec[18+start_idx] - '0') + (temp_rec[19+start_idx] - '0'); // γ��
            strncpy(temp_pos_char, temp_rec + 20+start_idx, 8); // γ��8λ����ʽΪmm.mmmmm
            _position[1] = atof(temp_pos_char); // γ��
            _lati_longi_str[0] = temp_rec[29+start_idx]; // γ�ȱ�ʶ
            _position[2] = 100 * (temp_rec[31+start_idx] - '0') + 10*(temp_rec[32+start_idx] - '0') + (temp_rec[33+start_idx] - '0'); // ����
            strncpy(temp_pos_char, temp_rec + 34+start_idx, 8); // ����8λ����ʽΪmm.mmmmm
            _position[3] = atof(temp_pos_char); // ����
            _lati_longi_str[1] = temp_rec[43+start_idx]; // ���ȱ�ʶ

            
            if (GPS_Quality == 0)
            {
                printf("GPS Quality is invalid\r\n");
                return 2;
            }
            return 1;
            // u8 flag;
            // printf("\r\n");
            // flag = GPS_Receive_Data_Analysis(temp_rec, rec_len);
            // if (flag == 1)
            // {
            // 	printf("GPS Receive Data Analysis Success\r\n");
            // }
            // else
            // {
            // 	printf("GPS Receive Data Analysis Fail\r\n");
            // }
        }
        // printf("\r\n");//���뻻��
        //  printf("Receive %d,%d, len=%d", temp_rec[0],temp_rec[1],rec_len);
        rec_len = 0;
        fail_count++;
        if (fail_count > 10)
        {
            return 0;
        }
        delay_ms(2000);
    }
}
