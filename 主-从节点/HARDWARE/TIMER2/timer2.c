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
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x03;   //抢占优先级为3
    NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x03;          //响应优先级为3
    TIM_TimeBaseInitStructure.TIM_Period=(5*10000-1);     //Tout=(ARR+1)(PSC+1)/Tclk   (2999999+1)(8399+1)/84M
    TIM_TimeBaseInitStructure.TIM_Prescaler=8399;
    TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up;   // 向上计数
    TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1;
    //使能定时器5的外设时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5,ENABLE);

    //初始化定时器5
    TIM_TimeBaseInit(TIM5,&TIM_TimeBaseInitStructure);

    //设置优先级分组为组2
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    //初始化优先级分组
    NVIC_Init(&NVIC_InitStructure);
    //配置定时器的中断的中断源
    TIM_ITConfig(TIM5,TIM_IT_Update,ENABLE);
    //使能定时器2
    TIM_Cmd(TIM5,ENABLE);

}
//定时器2的中断函数
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


