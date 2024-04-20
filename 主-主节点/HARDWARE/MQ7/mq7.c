#include "mq7.h"
#include "adc.h"
#include <math.h>

#define CAL_PPM_CO 10  // 校准环境中CO PPM值
#define MQ7_RL 2		// RL阻值(M欧)
static float R0 = 0; // 元件在洁净空气中的阻值

uint8_t flag_mq7 = 1;   //初始为启动
// u8 mq7_state_count = 0;
// u8 flag_mq7_is_need_measure = 0;

//PC1
void MQ7_Init(void)
{    	 
	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);//使能GPIOC时钟
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_Init(GPIOC, &GPIO_InitStructure);//初始化
	GPIO_SetBits(GPIOC,GPIO_Pin_1);//PC1设置高，启动模块

    Adc_Init_MQ7();         //初始化ADC,对应MQ7的模拟量数据端口PC0

}

// flag=1:开启MQ2  flag=0:关闭MQ2
void MQ7_Switch(u8 flag)
{
    if(flag == 1)
    {
        if (!MQ7)   // 仅在MQ7关闭时再开启
            MQ7 = 1;  
        flag_mq7 = 1;
    }
    else
    {
        if (MQ7)    // 仅在MQ7开启时再关闭
            MQ7 = 0; 
        flag_mq7 = 0;
    }
    
}


 
/****************************************
 *  RS/R0        ppm		            *
 *  1.6		     50                     *
 *  1	         100                    *
 *  0.6		     200                    *
 *  0.46	     300                    *
 *  0.39	     400                    *
 *  0.28	     600                    *
 *  0.21	     1000                   *
 * ppm = 98.322f * pow(RS/R0, -1.458f)  
 * **************************************/



 // 传感器校准函数
void MQ7_PPM_Calibration(float RS)
{
    R0 = RS / pow(CAL_PPM_CO / 98.322, 1 / -1.458f);
    printf("CAL_RESet MQ7 R0=%f\r\n", R0);
}

float MQ7_Scan(void)
{
    u16 adcx;
    u16 co_ppm;
    float Vrl;  // 电路输出电压
    float RS;   // 传感器等效阻值
    float R0_temp;  // 从flash中读取的R0
    // 若未开启MQ7则自动启动
    if (!flag_mq7){
        MQ7_Switch(1);
    }

    adcx=Get_Adc_Average(ADC_Channel_10,20);//获取通道10的转换值，20次取平均
    Vrl = 2.5f * adcx / 4095.f;  //获取计算后的带小数的实际电压值	ADC输入电压范围0~2.5V， 12位ADC， 2^12=4096，2.5v是用额外的稳压器模块输入
    Vrl = Vrl * 2;                          //根据电路图得到AO端电压值
    RS = (5 - Vrl) / Vrl * MQ7_RL;           //计算传感器等效阻值
    if(!MQ7_is_R0_valid(R0)) // 若未校准则自动校准(初始电阻小于10欧)
    {
        R0_temp = MQ7_Get_R0_from_flash();
        if (MQ7_is_R0_valid(R0_temp))   
        {
            // 从flash中获取R0，并替换
            R0 = R0_temp;
            printf("Get MQ2 R0 from flash=%f\r\n", R0);
        }
        else
        {
            // flash里面的R0无效，重新校准
            MQ7_PPM_Calibration(RS);
            write_to_flash();   // 写入flash
            printf("write to flash %f\r\n", R0);
        }
    }
    co_ppm = 98.322f * pow(RS/R0, -1.458f);
    // printf("Vrl=%f, RS=%f, R0=%f\r\n", Vrl, RS, R0);

    return co_ppm;
}

float MQ7_Get_R0(void)
{
    return R0;
}

float MQ7_Get_R0_from_flash(void)
{
    //从flash中获取R0
    float read_buf[2]={0};
    int len = 0;
    read_from_flash(read_buf, &len);
    return read_buf[1];  
}

u8 MQ7_is_R0_valid(float _R0)
{
    //判断_R0是否有效(单位为M欧)
    if (_R0 > 0.00001)  // 判断非0为有效
    {
        return 1;
    }
    return 0;
}