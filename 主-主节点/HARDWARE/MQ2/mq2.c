#include "mq2.h"
#include "adc.h"

uint8_t flag_mq2 = 1;

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
    // ��δ����MQ2���Զ�����
    if (!flag_mq2){
        MQ2_Switch(1);
    }

    adcx=Get_Adc_Average(ADC_Channel_14,20);//��ȡͨ��14��ת��ֵ��20��ȡƽ��
    co2=(float)(adcx*(2.5/4096.0));          	//��ȡ�����Ĵ�С����ʵ�ʵ�ѹֵ������3.1111	ADC�����ѹ��Χ0~2.5V�� 12λADC�� 2^12=4096��2.5v���ö������ѹ��ģ������
    co2 = co2 * 2;                          //���ݵ�·ͼ�õ�AO�˵�ѹֵ
    // ��co2��ѹӳ��Ϊppm��0.4V��Ӧ200ppm��4.0V��Ӧ10000ppm
    co2_ppm = (9800.0/3.6) * co2 -(9800.0/9) + 200;
    if(co2_ppm<0)
        co2_ppm=0;

    return co2_ppm;
}