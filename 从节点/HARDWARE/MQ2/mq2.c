#include "mq2.h"
#include "adc.h"
#include "flash.h"
#include <math.h>

#define CAL_PPM_CO2 20  // 校准环境中烟雾PPM值
#define MQ2_RL 2		// RL阻值(M欧)
static float R0 = 0; // 元件在洁净空气中的阻值
static u8 is_force_calibrate = 0; // 是否强制校准

uint8_t flag_mq2 = 1;   //初始为启动
u8 mq2_state_count = 0;
u8 flag_mq2_is_need_measure = 0;

//PA7
void MQ2_Init(void)
{    	 
	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//使能GPIOA时钟
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化
	GPIO_SetBits(GPIOA,GPIO_Pin_7);//PA6设置高，启动模块

    Adc_Init_MQ2();         //初始化ADC,对应MQ2的模拟量数据端口PC4

}

// flag=1:开启MQ2  flag=0:关闭MQ2
void MQ2_Switch(u8 flag)
{
    if(flag == 1)
    {
        if (!MQ2)   // 仅在MQ2关闭时再开启
            MQ2 = 1;  
        flag_mq2 = 1;
    }
    else
    {
        if (MQ2)    // 仅在MQ2开启时再关闭
            MQ2 = 0; 
        flag_mq2 = 0;
    }
    
}

/****************************************
 * 1.651428	          200               *
 * 1.437143	          300               *
 * 1.257143	          400               *
 * 1.137143	          500               *
 * 1		          600               *
 * 0.928704	          700               *
 * 0.871296	          800               *
 * 0.816667	          900               *
 * 0.785714	          1000              *
 * 0.574393	          2000              *
 * 0.466047	          3000              *
 * 0.415581	          4000              *
 * 0.370478	          5000              *
 * 0.337031	          6000              *
 * 0.305119	          7000              *
 * 0.288169	          8000              *
 * 0.272727	          9000              *
 * 0.254795	          10000             *
 *                                      *
 * ppm = 613.9f * pow(RS/RL, -2.074f)   *
 ***************************************/


 // 传感器校准函数
void MQ2_PPM_Calibration(float RS)
{
    R0 = RS / pow(CAL_PPM_CO2 / 613.9f, 1 / -2.074f);
    printf("CAL_RESet MQ2 R0=%f\r\n", R0);
}

float MQ2_Get_R0_from_flash(void)
{
    //从flash中获取R0
    float read_buf[2]={0};
    int len = 0;
    read_from_flash(read_buf, &len);
    return read_buf[0];  
}

u8 MQ2_is_R0_valid(float _R0)
{
    //判断_R0是否有效(单位为M欧)
    if (_R0 > 0.00001)  // 判断非0为有效
    {
        return 1;
    }
    return 0;
}

float MQ2_Scan(void)
{
    u16 adcx;
    u16 co2_ppm;
    float Vrl;  // 电路输出电压
    float RS;   // 传感器等效阻值
    float R0_temp = 0;  // 临时存储R0,来源于flash
    // 若未开启MQ2则自动启动
    if (!flag_mq2){
        MQ2_Switch(1);
    }

    adcx=Get_Adc_Average(ADC_Channel_14,20);//获取通道14的转换值，20次取平均
    Vrl = 2.5f * adcx / 4095.f;  //获取计算后的带小数的实际电压值	ADC输入电压范围0~2.5V， 12位ADC， 2^12=4096，2.5v是用额外的稳压器模块输入
    Vrl = Vrl * 2;                          //根据电路图得到AO端电压值
    RS = (5 - Vrl) / Vrl * MQ2_RL;           //计算传感器等效阻值
    if(!MQ2_is_R0_valid(R0)) // 若未校准则自动校准(初始电阻小于10欧)
    {
        R0_temp = MQ2_Get_R0_from_flash();
        if (!is_force_calibrate & MQ2_is_R0_valid(R0_temp))   
        {
            // 从flash中获取R0，并替换
            R0 = R0_temp;
            printf("Get MQ2 R0 from flash=%f\r\n", R0);
        }
        else
        {
            // flash里面的R0无效，重新校准
            MQ2_PPM_Calibration(RS);
            write_to_flash();   // 写入flash
            printf("write to flash %f\r\n", R0);
            if(is_force_calibrate)
            {
                is_force_calibrate = 0;
                printf("finish force calibration\r\n");
            }
        }
    }
    co2_ppm = 613.9f * pow(RS/R0, -2.074f);
    // printf("Vrl=%f, RS=%f, R0=%f\r\n", Vrl, RS, R0);

    return co2_ppm;
}

float MQ2_Get_R0(void)
{
    //获取R0
    return R0;  
}

 
