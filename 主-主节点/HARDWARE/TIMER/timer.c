#include "sys.h"
#include "timer.h"
#include "rtc.h"
#include "lora.h"

// extern u8 data_u8[24];
//u8 data_test[3] = {0x01, 0x02, 0x03};
void Timer_Init(u16 interval)
{
    // intervalΪ��ʱ���ж�ʱ��������λΪ��
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;

    NVIC_InitTypeDef NVIC_InitStructure;
	
	NVIC_InitStructure.NVIC_IRQChannel=TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x03;   //��ռ���ȼ�Ϊ3
    NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x03;          //��Ӧ���ȼ�Ϊ3
    TIM_TimeBaseInitStructure.TIM_Period=(interval*10000-1);     //Tout=(ARR+1)(PSC+1)/Tclk   (2999999+1)(8399+1)/84M
    TIM_TimeBaseInitStructure.TIM_Prescaler=8399;
    TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up;   // ���ϼ���
    TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1;
    //ʹ�ܶ�ʱ��2������ʱ��
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);

    //��ʼ����ʱ��2
    TIM_TimeBaseInit(TIM2,&TIM_TimeBaseInitStructure);

    //�������ȼ�����Ϊ��2
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    //��ʼ�����ȼ�����
    NVIC_Init(&NVIC_InitStructure);
    //���ö�ʱ�����жϵ��ж�Դ
    TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE);
    //ʹ�ܶ�ʱ��2
    TIM_Cmd(TIM2,ENABLE);

}
//��ʱ��2���жϺ���
void TIM2_IRQHandler(void)
{
    if(TIM_GetITStatus(TIM2,TIM_IT_Update)!=RESET)
    {
        //printf("tim2\r\n");
        is_need_update_time = 1;
		get_data_flag++;
    }
    TIM_ClearITPendingBit(TIM2,TIM_IT_Update);
}


