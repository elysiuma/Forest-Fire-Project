#include "wind.h"
#include "usart6.h"
#include <string.h>
#include <stdlib.h>

// 风速风向模块
u8 flag_wind_is_need_measure = 0; // 是否需要测量风速风向

u8 query_windsensor[11] = {0x24, 0x41, 0x44, 0x2C, 0x30, 0x34, 0x2A, 0x36, 0x33, 0x0D, 0x0A}; // 向风速传感器请求数据
u8 cab_windsensor[11] = {0x24, 0x41, 0x5A, 0x2C, 0x30, 0x34, 0x2A, 0x37, 0x39, 0x0D, 0x0A};   // 风速风向校准

void get_data(char *data_str, float *data);

void Wind_Init(void) // 初始化
{
    // 初始化风速风向模块
    uart6_init(9600);
}

void Wind_query(void)
{
    USART6_DATA(query_windsensor, 11);
}

void Wind_analysis(float *temp, float *pres, float *humi, float *wind_sp, float *wind_dir)
{
    u8 i;
    u8 temp_rec[150]; // 用于存储接收到的数据
    u8 rec_len = 0;
    float data[5] = {0}; // 用于存储数据

    printf("get windsensor data!\r\n");
    USART6_Receive_Data(temp_rec, &rec_len);
    // puts(temp_rec);
    printf("data analyzing...\r\n");
    get_data(temp_rec, data);

    printf("windsensor query finished...\r\n");
    for (i = 0; i < 5; i++)
        printf("%f ", data[i]);
    printf("\r\n");

    *temp = data[0];
    *pres = data[1];
    *humi = data[2];
    *wind_sp = data[3];
    *wind_dir = data[4];
}

void get_data(char *data_str, float *data)
{
    u8 i = 0, flag = 0, j = 0, k = 0;
    float data_float;
    char float_str[15];
    char *substr;
    substr = strstr(data_str, "\"T\"");

    for (k = 0; k < 5; k++)
    {
        while (1)
        {
            if (substr[i] == ',' || substr[i] == '}')
            {
                i++;
                break;
            }
            if (flag)
            {
                float_str[j] = substr[i];
                j++;
            }
            if (substr[i] == ':')
                flag = 1;
            i++;
        }
        float_str[j] = '\0';
        flag = 0;
        j = 0;
        data_float = atof(float_str);
        printf("data_float: %f, float_str:", data_float);
        puts(float_str);
        printf("\r\n");
        data[k] = data_float;
    }
}

u8 check_Wind_receive(void) // 检查是否接收到风速风向数据
{
    if (USART6_RX_STA & 0x8000)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
