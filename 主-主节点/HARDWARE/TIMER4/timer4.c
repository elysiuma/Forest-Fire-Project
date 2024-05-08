#include "sys.h"
#include "timer4.h"
#include "lora.h"
#include "biglora.h"
#include "mqtt4g.h"

#define snode_times 1   // 每2次中断更新一次从节点查询
#define max_wait_snode_times 14  // 最大等待从节点查询中断数, 14*5 = 70s
#define msnode_times 12  // 每24次中断更新一次主从节点查询

static u8 _count_interrupt_snode = 0;     // 计数中断次数,每snode_times次中断更新一次从节点查询
static u8 _count_interrupt_msnode = 0;     // 计数中断次数,每msnode_times次中断更新一次主从节点查询

u8 update_SNode_query_idx(u8 _count_interrupt);
void update_MSNode_query_idx(void);

void Timer_QueryDelay_Init(u16 interval)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;   // Change to TIM4
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x03;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x03;
    TIM_TimeBaseInitStructure.TIM_Period = (interval * 2000 - 1);
    TIM_TimeBaseInitStructure.TIM_Prescaler = 41999;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    // Enable the clock for TIM4
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);   // Change to RCC_APB1Periph_TIM4

    // Initialize TIM4
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseInitStructure);   // Change to TIM4

    // Set the priority group to group 2
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    // Initialize the NVIC
    NVIC_Init(&NVIC_InitStructure);
    // Configure the interrupt source for TIM4
    TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);   // Change to TIM4
    // Enable TIM4
    TIM_Cmd(TIM4, ENABLE);   // Change to TIM4
}

// TIM4 interrupt handler function
void TIM4_IRQHandler(void)   // Change the function name to TIM4_IRQHandler
{
    u8 flag = 0;
    if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)
    {
        if (is_need_query_data) // 需要查询子节点数据
        {
            if (_count_interrupt_snode++ >= snode_times)   // 每2次中断更新一次从节点查询
            {
                flag = update_SNode_query_idx(_count_interrupt_snode);   // Update the query index of the subnode
                if (flag)   // 如果已收到消息或超时
                {
                    _count_interrupt_snode = 0;
                }
            }
        }
        if (is_need_query_MSnode)   // 需要查询主从节点数据
        {
            if (_count_interrupt_msnode++ >= msnode_times)   // 每24次中断更新一次主从节点查询
            {
                _count_interrupt_msnode = 0;
                update_MSNode_query_idx();   // Update the query index of the MSnode
            }
        }

        TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
    }
}

u8 update_SNode_query_idx(u8 _count_interrupt)
{

    if(is_need_query_data && current_query_node_idx < SubNodeSet.nNode)
    {
        if(SubNodeSet.SubNode_list[current_query_node_idx].SubNodeStatus == 2 &&
            _count_interrupt < max_wait_snode_times)   // 如果当前节点状态为已发送查询命令且未超时
        {
            return 0;       // 继续等待，不更新查询节点
        }
    }

    if (current_query_node_idx == SubNodeSet.nNode)     // 如果已经查询完所有子节点
    {
        is_need_query_data = 0;
        current_query_node_idx = 200;
        is_need_send_4g = 1;   // 查询完所有子节点数据后，需要查询4G数据
    }
    else if (current_query_node_idx < SubNodeSet.nNode)    // 如果还没有查询完所有子节点
    {
        current_query_node_idx++;
    }
    else
    {
        LORA_Update_All_SubNode_Status();   // 更新所有子节点状态   
        current_query_node_idx = 0;     // 重新开始查询
    }
    return 1;
}

void update_MSNode_query_idx(void)
{
    if (current_query_MSnode_idx == MSNodeSet.nNode)     // 如果已经查询完所有主从节点
    {
        is_need_query_MSnode = 0;
        current_query_MSnode_idx = 200;
    }
    else if (current_query_MSnode_idx < MSNodeSet.nNode)    // 如果还没有查询完所有主从节点
    {
        current_query_MSnode_idx++;
    }
    else
    {
        BIGLORA_Update_All_MSNode_Status();   // 更新所有主从节点状态   
        current_query_MSnode_idx = 0;     // 重新开始查询
    }
}
