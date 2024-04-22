#include "sys.h"
#include "usart2.h"
#include "delay.h"
#include "led.h"

#if EN_USART2_RX // 如果使能USART2了接收
u8 USART2_RX_BUF[USART2_REC_LEN];
u8 USART2_TX_BUF[USART2_REC_LEN];
u16 USART2_RX_CNT = 0; // 接收状态标记

// 串口2中断服务程序
void USART2_IRQHandler(void)
{
	u8 res;
	u8 Data_len;
	if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
	{
		res = USART_ReceiveData(USART2);
		// printf("USART2_RX_CNT: %i, RES: %02x\r\n", USART2_RX_CNT, res);
		// 不要在中断中打印，会出现问题，只能打印两个字节
		if ((USART2_RX_CNT & 0x8000) == 0) // 接收未完成
		{
			// 指令以6C开头则为LORA模块指令
			if ((USART2_RX_CNT == 0 && res == 0x6c) || USART2_RX_BUF[0] == 0X6C)
			{
				USART2_RX_BUF[USART2_RX_CNT & 0X3FFF] = res;
				USART2_RX_CNT++;
				// printf("UART4_RX_STA: %i, RES: %i", UART4_RX_STA, Res);
				if ((USART2_RX_CNT & 0X3FFF) >= 2)
				{
					Data_len = USART2_RX_BUF[1] + 5;
					if (USART2_RX_CNT > (USART2_REC_LEN - 1))
						USART2_RX_CNT = 0; // 接收数据错误(超过最大接受字节数),重新开始接收
					if ((USART2_RX_CNT & 0X3FFF) == Data_len)
						USART2_RX_CNT |= 0x8000; // 接收完成了(0x16为lora通信规约结束符)
				}
			}
			else{
				// 否则为普通透传数据， 0d0a结尾
				if(USART2_RX_CNT&0x4000)//接收到了0x0d
				{
					if(res!=0x0a)USART2_RX_CNT=0;//接收错误,重新开始
					else USART2_RX_CNT|=0x8000;	//接收完成了 
				}
				else //还没收到0X0D
				{	
					if(res==0x0d)USART2_RX_CNT|=0x4000;
					else
					{
						USART2_RX_BUF[USART2_RX_CNT&0X3FFF]=res ;
						USART2_RX_CNT++;
						if(USART2_RX_CNT>(USART2_REC_LEN-1))USART2_RX_CNT=0;//接收数据错误,重新开始接收	  
					}		 
				}
			}
		}
		else{
			USART2_RX_CNT = 0;
			USART2_RX_BUF[0] = res;
			USART2_RX_CNT++;
		}

		

	}
}

#endif

void uart2_init(u32 bound)
{
	// GPIO端口设置
	// 串口通信口
	// TXDO 模块发送 TTL PD5
	// RXDI 模块接收 TTL PD6
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE); // 使能USART2时钟

	GPIO_PinAFConfig(GPIOD, GPIO_PinSource5, GPIO_AF_USART2); // PD5复用为USART2
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource6, GPIO_AF_USART2); // PD6复用为USART2

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;	  // 复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; // 速度 50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;	  // 推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;	  // 上拉
	GPIO_Init(GPIOD, &GPIO_InitStructure);			  // 初始化 PD5，PD6

	USART_InitStructure.USART_BaudRate = bound;					// 波特率设置
	USART_InitStructure.USART_WordLength = USART_WordLength_8b; // 字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;		// 一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;			// 无奇偶校验位
	// USART_InitStructure.USART_Parity = USART_Parity_Even;//偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;					// 收发模式
	USART_Init(USART2, &USART_InitStructure);										// 初始化串口2

	USART_Cmd(USART2, ENABLE); // 使能串口 2

	USART_ClearFlag(USART2, USART_FLAG_TC);

#if EN_USART2_RX
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE); // 开启接受中断

	// Usart2 NVIC 配置
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3; // 抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		  // 子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			  // IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);							  // 根据指定的参数初始化VIC寄存器、

#endif
}

void USART2_DATA(u8 *buf, u8 len)
{
	u8 t;
	for (t = 0; t < len; t++)
	{
		USART_SendData(USART2, buf[t]);
		while (USART_GetFlagStatus(USART2, USART_FLAG_TC) != SET)
			;
	}
	// while(USART_GetFlagStatus(USART2,USART_FLAG_TC)==RESET);
	// USART2_RX_CNT=0;
}

void USART2_CMD(unsigned char *lb)
{
	while (*lb)
	{
		USART_SendData(USART2, *lb);
		while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)
		{
		}
		lb++;
	}
}

void USART2_Receive_Data(u8 *buf, u8 *len)
{
	u8 i = 0;
	u8 data_len = USART2_RX_CNT & 0x3fff;
	delay_ms(10);
	// 打印接收到的数据
	// printf("data_len: %i, len: %i", data_len, *len);
	if (data_len > 0)
	{
		for (i = 0; i < data_len; i++)
		{
			buf[(*len) + i] = USART2_RX_BUF[i];
		}
		*len += data_len;
		USART2_RX_CNT = 0;
	}
}

void USART2_Receive_Data_NoClear(u8 *buf, u8 *len)
{
	u8 rxlen = USART2_RX_CNT;
	u8 i = 0;
	*len = 0;
	delay_ms(10);
	if (rxlen == USART2_RX_CNT && rxlen)
	{
		for (i = 0; i < rxlen; i++)
		{
			buf[i] = USART2_RX_BUF[i];
		}
		*len = USART2_RX_CNT;
	}
}

void Usart2CommandAnalysis(void)
{
	// u8 rxlen=0;
	// memset(USART2_TX_BUF, 0, sizeof(USART2_TX_BUF));
	// USART2_Receive_Data(USART2_TX_BUF,&rxlen);
}
