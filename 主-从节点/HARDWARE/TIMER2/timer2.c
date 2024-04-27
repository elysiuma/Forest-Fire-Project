#include "sys.h"
#include "timer2.h"
#include "mq2.h"
#include "mq7.h"
#include "battery.h"
#include "wind.h"

void Timer_mq2_Init(u16 interval)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;

    NVIC_InitTypeDef NVIC_InitStructure;
	
	NVIC_InitStructure.NVIC_IRQChannel=TIM3_IRQn;   // 定时器3,16位
    NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x03;   //抢占优先级为3
    NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x03;          //响应优先级为3
    TIM_TimeBaseInitStructure.TIM_Period=(interval*2000-1);     //Tout=(ARR+1)(PSC+1)/Tclk   (2999999+1)(TIM_Prescaler+1)/84M
    TIM_TimeBaseInitStructure.TIM_Prescaler=41999;   // 84M/42000=2KHz的计数频率
    TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up;   // 向上计数
    TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1;
    //使能定时器3的外设时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);

    //初始化定时器3
    TIM_TimeBaseInit(TIM3,&TIM_TimeBaseInitStructure);

    //设置优先级分组为组2
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    //初始化优先级分组
    NVIC_Init(&NVIC_InitStructure);
    //配置定时器的中断的中断源
    TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE);
    //使能定时器2
    TIM_Cmd(TIM3,ENABLE);

}
//定时器3的中断函数
void TIM3_IRQHandler(void)
{
    if(TIM_GetITStatus(TIM3,TIM_IT_Update)!=RESET)
    {
        // 电源电压部分
        flag_battery_is_need_measure = 1;    // 电源电压需要测量，在main循环中执行，执行完复位0
        flag_wind_is_need_measure = 1;   // 风速风向需要测量，在main循环中执行，执行完复位0
        // MQ2和MQ7部分
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
        TIM_ClearITPendingBit(TIM3,TIM_IT_Update);
    }
}


