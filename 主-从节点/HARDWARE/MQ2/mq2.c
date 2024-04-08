#include "mq2.h"
#include "adc.h"

uint8_t flag_mq2 = 1;

//PA6
void MQ2_Init(void)
{    	 
	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//使能GPIOA时钟
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化
	GPIO_SetBits(GPIOA,GPIO_Pin_6);//PA6设置高，启动模块

    Adc_Init_MQ2();         //初始化ADC,对应MQ2的模拟量数据端口PC4

}

void MQ2_Switch(u8 flag)
{
    if(flag == 1)
    {
        MQ2 = 1;
        flag_mq2 = 1;
    }
    else
    {
        MQ2 = 0;
        flag_mq2 = 0;
    }
    
}

float MQ2_Scan(void)
{
    u16 adcx;
    u16 co2_ppm;
    float co2;
    // 若未开启MQ2则自动启动
    if (!flag_mq2){
        MQ2_Switch(1);
    }

    adcx=Get_Adc_Average(ADC_Channel_14,20);//获取通道14的转换值，20次取平均
    co2=(float)(adcx*(2.5/4096.0));          	//获取计算后的带小数的实际电压值，比如3.1111	ADC输入电压范围0~2.5V， 12位ADC， 2^12=4096，2.5v是用额外的稳压器模块输入
    co2 = co2 * 2;                          //根据电路图得到AO端电压值
    // 将co2电压映射为ppm，0.4V对应200ppm，4.0V对应10000ppm
    co2_ppm = (9800.0/3.6) * co2 -(9800.0/9) + 200;
    if(co2_ppm<0)
        co2_ppm=0;

    return co2_ppm;
}