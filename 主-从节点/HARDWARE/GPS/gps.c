#include "gps.h"
#include "usart3.h"
#include "delay.h"
#include "stdlib.h"

float node_position[4] = {0,0,0,0}; // 节点位置,纬度、分和经度、分
char node_lati_longi_str[2] = {'\0','\0'}; // 节点纬度、经度标识（N/S,E/W）

u8 gps_temp_rec[300];   // GPS接收数据缓存

void GPS_Init(void)
{
    // 设置GGA协议传输间隔位1秒
    u8 GPS_GGA_MODE[16] = {0xA0, 0xA1, 0x00, 0x09, 0x08, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x08, 0x0D, 0x0A};
    uart3_init(38400);
    GPS_Send(GPS_GGA_MODE, 16); // 将GPS传回协议设置为GGA
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
    // 若接受完毕数据则返回1
    if (USART3_RX_STA & 0x8000)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

// GPS获取数据，返回的是正确的时间
u8 GPS_get_time(u8 *time)
{
    u8 t;
    u8 rec_len = 0;
    u8 fail_count = 0;
    u8 start_idx=0;     // 开始位置，有时候会收到两个$，需要设置一下开始位置

    // GPS质量
    u8 GPS_Quality = 0; // 0:无效 1:GPS单点定位 2:DGPS差分定位 43位
    // 卫星数量
    u8 GPS_Satellite_Num = 0;   // 45,46位

    while (1)
    {
        printf("GPS_get_time\r\n");
        // 接收数据
        if(check_GPS_Receive())
        {
            printf("check_GPS_Receive\r\n");
            GPS_Receive(gps_temp_rec, &rec_len);
        }
        printf("rec_len: %i\r\n", rec_len);
        // 打印temp_rec
        for (t = 0; t < rec_len; t++)
        {
            printf("%c", gps_temp_rec[t]); // 向串口4发送数据
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
        // // 测试打印数据
        // printf("GPS Receive: ");
        // for (t = 0; t < rec_len; t++)
        // {
        //     printf("%c", gps_temp_rec[t]); // 向串口4发送数据
        // }
        // printf("\r\n");
        // 检测是否是完整的GPS数据包
        // $GNGGA或$$GNGGA
        if ((gps_temp_rec[3] == 0x47 && gps_temp_rec[4] == 0x47 && gps_temp_rec[5] == 0x41) ||
            (gps_temp_rec[0] == '$' && gps_temp_rec[4] == 0x48 && gps_temp_rec[5] == 0x47 && gps_temp_rec[6] == 0x41))
        {
            // 找到开始位置
            if(gps_temp_rec[3] == 0x47 && gps_temp_rec[4] == 0x47 && gps_temp_rec[5] == 0x41)
                start_idx = 0;
            else
                start_idx = 1;
            // 时
            time[0] = 10 * (gps_temp_rec[7+start_idx] - '0') + (gps_temp_rec[8+start_idx] - '0'); // UTC时间
            time[0] += 8;                                           // 北京时间
            if (time[0] >= 24)
                time[0] -= 24;
            // time[0] = (time[0] / 10) * 16 + time[0] % 10; // 转换为16进制
            // 分
            time[1] = 10 * (gps_temp_rec[9+start_idx] - '0') + (gps_temp_rec[10+start_idx] - '0');
            // 秒
            time[2] = 10 * (gps_temp_rec[11+start_idx] - '0') + (gps_temp_rec[12+start_idx] - '0');
            // GPS质量
            GPS_Quality = gps_temp_rec[45+start_idx]-'0';
            // 卫星数量
            GPS_Satellite_Num = 10 * (gps_temp_rec[45+start_idx] - '0') + (gps_temp_rec[46+start_idx] - '0');

            // 打印时间
            //printf("GPS Time: %d:%d:%d quality:%d, gps_num:%d\r\n", time[0], time[1], time[2],GPS_Quality,GPS_Satellite_Num);
            if (GPS_Quality == 0)
            {
                printf("GPS Quality is invalid\r\n");
                return 2;
            }
            return 1;
            // u8 flag;
            // printf("\r\n");
            // flag = GPS_Receive_Data_Analysis(gps_temp_rec, rec_len);
            // if (flag == 1)
            // {
            // 	printf("GPS Receive Data Analysis Success\r\n");
            // }
            // else
            // {
            // 	printf("GPS Receive Data Analysis Fail\r\n");
            // }
        }
        // printf("\r\n");//插入换行
        //  printf("Receive %d,%d, len=%d", gps_temp_rec[0],gps_temp_rec[1],rec_len);
        rec_len = 0;
        fail_count++;
        if (fail_count > 10)
        {
            return 0;
        }
        delay_ms(2000);
    }
}

// GPS获取数据，返回的是正确的时间,位置信息
u8 GPS_get_time_and_pos(u8 *time, float *_position, char *_lati_longi_str)
{
    u8 t;
    char temp_pos_char[9] = {'\0'}; // 临时位置信息缓存
    u8 rec_len = 0;
    u8 fail_count = 0;
    u8 start_idx=0;     // 开始位置，有时候会收到两个$，需要设置一下开始位置

    // GPS质量
    u8 GPS_Quality = 0; // 0:无效 1:GPS单点定位 2:DGPS差分定位 43位
    // 卫星数量
    u8 GPS_Satellite_Num = 0;   // 45,46位

    while (1)
    {
        printf("GPS_get_time\r\n");
        // 接收数据
        if(check_GPS_Receive())
        {
            printf("check_GPS_Receive\r\n");
            GPS_Receive(gps_temp_rec, &rec_len);
        }
        printf("rec_len: %i\r\n", rec_len);
        // 打印temp_rec
        for (t = 0; t < rec_len; t++)
        {
            printf("%c", gps_temp_rec[t]); // 向串口4发送数据
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
        // // 测试打印数据
        // printf("GPS Receive: ");
        // for (t = 0; t < rec_len; t++)
        // {
        //     printf("%c", gps_temp_rec[t]); // 向串口4发送数据
        // }
        // printf("\r\n");
        // 检测是否是完整的GPS数据包
        // $GNGGA或$$GNGGA
        if ((gps_temp_rec[3] == 0x47 && gps_temp_rec[4] == 0x47 && gps_temp_rec[5] == 0x41) ||
            (gps_temp_rec[0] == '$' && gps_temp_rec[4] == 0x48 && gps_temp_rec[5] == 0x47 && gps_temp_rec[6] == 0x41))
        {
            // 找到开始位置
            if(gps_temp_rec[3] == 0x47 && gps_temp_rec[4] == 0x47 && gps_temp_rec[5] == 0x41)
                start_idx = 0;
            else
                start_idx = 1;
            // 时
            time[0] = 10 * (gps_temp_rec[7+start_idx] - '0') + (gps_temp_rec[8+start_idx] - '0'); // UTC时间
            time[0] += 8;                                           // 北京时间
            if (time[0] >= 24)
                time[0] -= 24;
            // time[0] = (time[0] / 10) * 16 + time[0] % 10; // 转换为16进制
            // 分
            time[1] = 10 * (gps_temp_rec[9+start_idx] - '0') + (gps_temp_rec[10+start_idx] - '0');
            // 秒
            time[2] = 10 * (gps_temp_rec[11+start_idx] - '0') + (gps_temp_rec[12+start_idx] - '0');
            // GPS质量
            GPS_Quality = gps_temp_rec[45+start_idx]-'0';
            // 卫星数量
            GPS_Satellite_Num = 10 * (gps_temp_rec[47+start_idx] - '0') + (gps_temp_rec[48+start_idx] - '0');

            // 打印时间
            //printf("GPS Time: %d:%d:%d quality:%d, gps_num:%d\r\n", time[0], time[1], time[2],GPS_Quality,GPS_Satellite_Num);
            // 经纬度
            _position[0] = 10 * (gps_temp_rec[18+start_idx] - '0') + (gps_temp_rec[19+start_idx] - '0'); // 纬度
            strncpy(temp_pos_char, gps_temp_rec + 20+start_idx, 8); // 纬分8位，格式为mm.mmmmm
            _position[1] = atof(temp_pos_char); // 纬度
            _lati_longi_str[0] = gps_temp_rec[29+start_idx]; // 纬度标识
            _position[2] = 100 * (gps_temp_rec[31+start_idx] - '0') + 10*(gps_temp_rec[32+start_idx] - '0') + (gps_temp_rec[33+start_idx] - '0'); // 经度
            strncpy(temp_pos_char, gps_temp_rec + 34+start_idx, 8); // 经分8位，格式为mm.mmmmm
            _position[3] = atof(temp_pos_char); // 经度
            _lati_longi_str[1] = gps_temp_rec[43+start_idx]; // 经度标识

            
            if (GPS_Quality == 0)
            {
                printf("GPS Quality is invalid\r\n");
                return 2;
            }
            return 1;
            // u8 flag;
            // printf("\r\n");
            // flag = GPS_Receive_Data_Analysis(gps_temp_rec, rec_len);
            // if (flag == 1)
            // {
            // 	printf("GPS Receive Data Analysis Success\r\n");
            // }
            // else
            // {
            // 	printf("GPS Receive Data Analysis Fail\r\n");
            // }
        }
        // printf("\r\n");//插入换行
        //  printf("Receive %d,%d, len=%d", gps_temp_rec[0],gps_temp_rec[1],rec_len);
        rec_len = 0;
        fail_count++;
        if (fail_count > 10)
        {
            return 0;
        }
        delay_ms(2000);
    }
}
