#include "mq2.h"
#include "adc.h"
#include <math.h>

#define CAL_PPM_CO2 20  // У׼����������PPMֵ
#define MQ2_RL 2		// RL��ֵ(Mŷ)
static float R0 = 0; // Ԫ���ڽྻ�����е���ֵ

uint8_t flag_mq2 = 1;   //��ʼΪ����
u8 mq2_state_count = 0;
u8 flag_mq2_is_need_measure = 0;

//PA6
void MQ2_Init(void)
{    	 
	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//ʹ��GPIOAʱ��
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//��ͨ���ģʽ
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//�������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
	GPIO_Init(GPIOA, &GPIO_InitStructure);//��ʼ��
	GPIO_SetBits(GPIOA,GPIO_Pin_6);//PA6���øߣ�����ģ��

    Adc_Init_MQ2();         //��ʼ��ADC,��ӦMQ2��ģ�������ݶ˿�PC4

}

// flag=1:����MQ2  flag=0:�ر�MQ2
void MQ2_Switch(u8 flag)
{
    if(flag == 1)
    {
        if (!MQ2)   // ����MQ2�ر�ʱ�ٿ���
            MQ2 = 1;  
        flag_mq2 = 1;
    }
    else
    {
        if (MQ2)    // ����MQ2����ʱ�ٹر�
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


 // ������У׼����
void MQ2_PPM_Calibration(float RS)
{
    R0 = RS / pow(CAL_PPM_CO2 / 613.9f, 1 / -2.074f);
}

float MQ2_Scan(void)
{
    u16 adcx;
    u16 co2_ppm;
    float Vrl;  // ��·�����ѹ
    float RS;   // ��������Ч��ֵ
    // ��δ����MQ2���Զ�����
    if (!flag_mq2){
        MQ2_Switch(1);
    }

    adcx=Get_Adc_Average(ADC_Channel_14,20);//��ȡͨ��14��ת��ֵ��20��ȡƽ��
    Vrl = 2.5f * adcx / 4095.f;  //��ȡ�����Ĵ�С����ʵ�ʵ�ѹֵ	ADC�����ѹ��Χ0~2.5V�� 12λADC�� 2^12=4096��2.5v���ö������ѹ��ģ������
    Vrl = Vrl * 2;                          //���ݵ�·ͼ�õ�AO�˵�ѹֵ
    RS = (5 - Vrl) / Vrl * MQ2_RL;           //���㴫������Ч��ֵ
    if(R0 < 0.00001) // ��δУ׼���Զ�У׼(��ʼ����С��10ŷ)
    {
	    MQ2_PPM_Calibration(RS);
    }
    printf("Vrl=%f, RS=%f, R0=%f\r\n", Vrl, RS, R0);
    co2_ppm = 613.9f * pow(RS/R0, -2.074f);

    return co2_ppm;
}