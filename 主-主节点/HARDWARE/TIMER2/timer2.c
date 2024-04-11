#include "sys.h"
#include "timer2.h"
#include "mq2.h"
#include "mq7.h"
#include "lora.h"


void Timer_mq2_Init(u16 interval)
{
    // interval为定时器中断时间间隔，单位为秒
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;

    NVIC_InitTypeDef NVIC_InitStructure;
	
	NVIC_InitStructure.NVIC_IRQChannel=TIM5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x03;   //抢占优先级为3
    NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x03;          //响应优先级为3
    TIM_TimeBaseInitStructure.TIM_Period=(interval*10000-1);     //Tout=(ARR+1)(PSC+1)/Tclk   (2999999+1)(8399+1)/84M
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
        mq2_state_count++;
        if (flag_mq2)
        {
            // MQ2启动的时候
            if (mq2_state_count>MQ2_ON_MAX)
            {
                // MQ2启动时间到，关闭MQ2
                MQ2_Switch(0);
                MQ7_Switch(0);
                mq2_state_count = 0;
            }
            else if (mq2_state_count==1)
            {
                MQ2_Switch(1); // 启动预热
                MQ7_Switch(1);
                flag_mq2_is_need_measure = 0;   // 即使有未进行的测量，预热期间也需关闭测量
            }
            else
                flag_mq2_is_need_measure = 1;    // MQ2需要测量，在main循环中执行，执行完复位0
        }
        else 
        {
            if (mq2_state_count>MQ2_OFF_MAX)
            {
                // MQ2关闭时间到
                MQ2_Switch(1);
                MQ7_Switch(1);
                mq2_state_count = 0;
            }
            else
                flag_mq2_is_need_measure = 0;   // 重新声明无需测量，避免问题
        }
        TIM_ClearITPendingBit(TIM5,TIM_IT_Update);
    }
}


