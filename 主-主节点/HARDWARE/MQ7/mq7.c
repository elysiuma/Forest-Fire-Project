#include "mq7.h"
#include "adc.h"
#include <math.h>

#define CAL_PPM_CO 10  // У׼������CO PPMֵ
#define MQ7_RL 2		// RL��ֵ(Mŷ)
static float R0 = 0; // Ԫ���ڽྻ�����е���ֵ

uint8_t flag_mq7 = 1;   //��ʼΪ����
// u8 mq7_state_count = 0;
// u8 flag_mq7_is_need_measure = 0;

//PC1
void MQ7_Init(void)
{    	 
	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);//ʹ��GPIOCʱ��
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//��ͨ���ģʽ
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//�������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
	GPIO_Init(GPIOC, &GPIO_InitStructure);//��ʼ��
	GPIO_SetBits(GPIOC,GPIO_Pin_1);//PC1���øߣ�����ģ��

    Adc_Init_MQ7();         //��ʼ��ADC,��ӦMQ7��ģ�������ݶ˿�PC0

}

// flag=1:����MQ2  flag=0:�ر�MQ2
void MQ7_Switch(u8 flag)
{
    if(flag == 1)
    {
        if (!MQ7)   // ����MQ7�ر�ʱ�ٿ���
            MQ7 = 1;  
        flag_mq7 = 1;
    }
    else
    {
        if (MQ7)    // ����MQ7����ʱ�ٹر�
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



 // ������У׼����
void MQ7_PPM_Calibration(float RS)
{
    R0 = RS / pow(CAL_PPM_CO / 98.322, 1 / -1.458f);
    printf("CAL_RESet MQ7 R0=%f\r\n", R0);
}

float MQ7_Scan(void)
{
    u16 adcx;
    u16 co_ppm;
    float Vrl;  // ��·�����ѹ
    float RS;   // ��������Ч��ֵ
    float R0_temp;  // ��flash�ж�ȡ��R0
    // ��δ����MQ7���Զ�����
    if (!flag_mq7){
        MQ7_Switch(1);
    }

    adcx=Get_Adc_Average(ADC_Channel_10,20);//��ȡͨ��10��ת��ֵ��20��ȡƽ��
    Vrl = 2.5f * adcx / 4095.f;  //��ȡ�����Ĵ�С����ʵ�ʵ�ѹֵ	ADC�����ѹ��Χ0~2.5V�� 12λADC�� 2^12=4096��2.5v���ö������ѹ��ģ������
    Vrl = Vrl * 2;                          //���ݵ�·ͼ�õ�AO�˵�ѹֵ
    RS = (5 - Vrl) / Vrl * MQ7_RL;           //���㴫������Ч��ֵ
    if(!MQ7_is_R0_valid(R0)) // ��δУ׼���Զ�У׼(��ʼ����С��10ŷ)
    {
        R0_temp = MQ7_Get_R0_from_flash();
        if (MQ7_is_R0_valid(R0_temp))   
        {
            // ��flash�л�ȡR0�����滻
            R0 = R0_temp;
            printf("Get MQ2 R0 from flash=%f\r\n", R0);
        }
        else
        {
            // flash�����R0��Ч������У׼
            MQ7_PPM_Calibration(RS);
            write_to_flash();   // д��flash
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
    //��flash�л�ȡR0
    float read_buf[2]={0};
    int len = 0;
    read_from_flash(read_buf, &len);
    return read_buf[1];  
}

u8 MQ7_is_R0_valid(float _R0)
{
    //�ж�_R0�Ƿ���Ч(��λΪMŷ)
    if (_R0 > 0.00001)  // �жϷ�0Ϊ��Ч
    {
        return 1;
    }
    return 0;
}