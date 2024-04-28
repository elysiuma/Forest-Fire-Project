#include "sys.h"
#include "timer.h"
#include "rtc.h"
#include "lora.h"

// extern u8 data_u8[24];
//u8 data_test[3] = {0x01, 0x02, 0x03};
void Timer_Init(u16 interval)
{
    // interval为定时器中断时间间隔，单位为秒
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;

    NVIC_InitTypeDef NVIC_InitStructure;
	
	NVIC_InitStructure.NVIC_IRQChannel=TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x03;   //抢占优先级为3
    NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x03;          //响应优先级为3
    TIM_TimeBaseInitStructure.TIM_Period=(interval*10000-1);     //Tout=(ARR+1)(PSC+1)/Tclk   (2999999+1)(8399+1)/84M
    TIM_TimeBaseInitStructure.TIM_Prescaler=8399;
    TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up;   // 向上计数
    TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1;
    //使能定时器2的外设时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);

    //初始化定时器2
    TIM_TimeBaseInit(TIM2,&TIM_TimeBaseInitStructure);

    //设置优先级分组为组2
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    //初始化优先级分组
    NVIC_Init(&NVIC_InitStructure);
    //配置定时器的中断的中断源
    TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE);
    //使能定时器2
    TIM_Cmd(TIM2,ENABLE);

}
//定时器2的中断函数
void TIM2_IRQHandler(void)
{
    if(TIM_GetITStatus(TIM2,TIM_IT_Update)!=RESET)
    {
        //printf("tim2\r\n");
        is_need_update_time = 1;
    }
    TIM_ClearITPendingBit(TIM2,TIM_IT_Update);
}


