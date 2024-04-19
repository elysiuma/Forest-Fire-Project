#include "sys.h"
#include "timer2.h"
#include "mq2.h"
#include "mq7.h"
#include "lora.h"


void Timer_mq2_Init(u16 interval)
{
    // intervalΪ��ʱ���ж�ʱ��������λΪ��
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;

    NVIC_InitTypeDef NVIC_InitStructure;
	
	NVIC_InitStructure.NVIC_IRQChannel=TIM5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x03;   //��ռ���ȼ�Ϊ3
    NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x03;          //��Ӧ���ȼ�Ϊ3
    TIM_TimeBaseInitStructure.TIM_Period=(interval*10000-1);     //Tout=(ARR+1)(PSC+1)/Tclk   (2999999+1)(8399+1)/84M
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
        mq2_state_count++;
        if (flag_mq2)
        {
            // MQ2������ʱ��
            if (mq2_state_count>MQ2_ON_MAX)
            {
                // MQ2����ʱ�䵽���ر�MQ2
                MQ2_Switch(0);
                MQ7_Switch(0);
                mq2_state_count = 0;
            }
            else if (mq2_state_count==1)
            {
                MQ2_Switch(1); // ����Ԥ��
                MQ7_Switch(1);
                flag_mq2_is_need_measure = 0;   // ��ʹ��δ���еĲ�����Ԥ���ڼ�Ҳ��رղ���
            }
            else
                flag_mq2_is_need_measure = 1;    // MQ2��Ҫ��������mainѭ����ִ�У�ִ���긴λ0
        }
        else 
        {
            if (mq2_state_count>MQ2_OFF_MAX)
            {
                // MQ2�ر�ʱ�䵽
                MQ2_Switch(1);
                MQ7_Switch(1);
                mq2_state_count = 0;
            }
            else
                flag_mq2_is_need_measure = 0;   // �������������������������
        }
        TIM_ClearITPendingBit(TIM5,TIM_IT_Update);
    }
}


