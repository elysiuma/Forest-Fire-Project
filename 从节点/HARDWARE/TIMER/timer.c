#include "sys.h"
#include "timer.h"
#include "mq2.h"
#include "lora.h"

static u8 timer_flag=1;
void Timer_Init(void)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;

    NVIC_InitTypeDef NVIC_InitStructure;
	
	NVIC_InitStructure.NVIC_IRQChannel=TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x03;   //��ռ���ȼ�Ϊ3
    NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x03;          //��Ӧ���ȼ�Ϊ3
    TIM_TimeBaseInitStructure.TIM_Period=(60*10000-1);     //Tout=(ARR+1)(PSC+1)/Tclk   (2999999+1)(8399+1)/84M
    TIM_TimeBaseInitStructure.TIM_Prescaler=8399;
    TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1;
    //ʹ�ܶ�ʱ��2������ʱ��
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);

    //��ʼ����ʱ��2
    TIM_TimeBaseInit(TIM2,&TIM_TimeBaseInitStructure);

    //�������ȼ�����  Ϊ��2
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    //��ʼ�����ȼ�����
    NVIC_Init(&NVIC_InitStructure);
    //���ö�ʱ�����жϵ��ж�Դ
    TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE);
    //ʹ�ܶ�ʱ��2
    TIM_Cmd(TIM2,ENABLE);
	/*
    while(1)
    {
       //����ѭ���ȴ��ж�
    }*/

}
//��ʱ��2���жϺ���
void TIM2_IRQHandler(void)
{
    if(TIM_GetITStatus(TIM2,TIM_IT_Update)!=RESET)
    {
		if(MQ2)
		{	
			MQ2 = !MQ2;
			timer_flag = 0;
            //is_need_send_lora_data = 1;
		}
		else
		{	
			if(timer_flag==0)
			timer_flag = 1;
			else if(timer_flag==1)
			{
				MQ2 = !MQ2;
				timer_flag = 0;
			}
		}
        TIM_ClearITPendingBit(TIM2,TIM_IT_Update);
    }
}
