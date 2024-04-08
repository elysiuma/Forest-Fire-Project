#include "sys.h"
#include "timer2.h"
#include "mq2.h"
#include "lora.h"

static u8 timer_flag=1;
void Timer_mq2_Init(void)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;

    NVIC_InitTypeDef NVIC_InitStructure;
	
	NVIC_InitStructure.NVIC_IRQChannel=TIM5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x03;   //��ռ���ȼ�Ϊ3
    NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x03;          //��Ӧ���ȼ�Ϊ3
    TIM_TimeBaseInitStructure.TIM_Period=(5*10000-1);     //Tout=(ARR+1)(PSC+1)/Tclk   (2999999+1)(8399+1)/84M
    TIM_TimeBaseInitStructure.TIM_Prescaler=8399;
    TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up;   // ���ϼ���
    TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1;
    //ʹ�ܶ�ʱ��5������ʱ��
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5,ENABLE);

    //��ʼ����ʱ��5
    TIM_TimeBaseInit(TIM5,&TIM_TimeBaseInitStructure);

    //�������ȼ�����Ϊ��2
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    //��ʼ�����ȼ�����
    NVIC_Init(&NVIC_InitStructure);
    //���ö�ʱ�����жϵ��ж�Դ
    TIM_ITConfig(TIM5,TIM_IT_Update,ENABLE);
    //ʹ�ܶ�ʱ��2
    TIM_Cmd(TIM5,ENABLE);

}
//��ʱ��2���жϺ���
void TIM5_IRQHandler(void)
{
    if(TIM_GetITStatus(TIM5,TIM_IT_Update)!=RESET)
    {
		if(MQ2)
		{	
			MQ2 = !MQ2;
			timer_flag = 0;
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
        TIM_ClearITPendingBit(TIM5,TIM_IT_Update);
    }
}


