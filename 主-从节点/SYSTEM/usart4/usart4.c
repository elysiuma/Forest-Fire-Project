  
#include "usart4.h"	
#include "led.h"
////////////////////////////////////////////////////////////////////////////////// 	 
//如果使用ucos,则包括下面的头文件即可.
#if SYSTEM_SUPPORT_OS
#include "includes.h"					//ucos 使用	  
#endif
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32F4探索者开发板
//串口1初始化		   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2014/6/10
//版本：V1.5
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved
//********************************************************************************
//V1.3修改说明 
//支持适应不同频率下的串口波特率设置.
//加入了对printf的支持
//增加了串口接收命令功能.
//修正了printf第一个字符丢失的bug
//V1.4修改说明
//1,修改串口初始化IO的bug
//2,修改了USART_RX_STA,使得串口最大接收字节数为2的14次方
//3,增加了USART_REC_LEN,用于定义串口最大允许接收的字节数(不大于2的14次方)
//4,修改了EN_USART1_RX的使能方式
//V1.5修改说明
//1,增加了对UCOSII的支持
////////////////////////////////////////////////////////////////////////////////// 	  
 

//////////////////////////////////////////////////////////////////
//加入以下代码,支持printf函数,而不需要选择use MicroLIB	  
#if 1
//#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 
}; 

FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
void _sys_exit(int8_t x) 
{ 
	x = x; 
} 
//重定义fputc函数 
int fputc(int ch, FILE *f)
{ 	
	while((UART4->SR&0X40)==0);//循环发送,直到发送完毕   
	UART4->DR = (u8) ch;      
	return ch;
}
#endif
 
#if EN_UART4_RX   //如果使能了接收
//串口4中断服务程序
//注意,读取USARTx->SR能避免莫名其妙的错误   	
u8 UART4_RX_BUF[USART4_REC_LEN];     //接收缓冲,最大USART_REC_LEN个字节.
//接收状态
//bit15，	接收完成标志
//bit14，	接收到0x0d
//bit13~0，	接收到的有效字节数目
u16 UART4_RX_STA=0;       //接收状态标记	

//初始化IO 串口4
//bound:波特率
void uart4_init(u32 bound){
   //GPIO端口设置
  GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC,ENABLE); //使能GPIOC时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4,ENABLE);//使能USART4时钟
 
	//串口4对应引脚复用映射
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource10,GPIO_AF_UART4); //GPIOC10复用为USART4
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource11,GPIO_AF_UART4); //GPIOC11复用为USART4
	
	//USART4端口配置
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_10; //GPIOC11与GPIOC10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//速度50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //上拉
	GPIO_Init(GPIOC,&GPIO_InitStructure); //初始化PC11，PC10

   //USART4 初始化设置
	USART_InitStructure.USART_BaudRate = bound;//波特率设置
	USART_InitStructure.USART_WordLength = USART_WordLength_9b;//字长为8位数据格式
	// USART_InitStructure.USART_WordLength = USART_WordLength_9b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	// USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_Parity = USART_Parity_Even;//偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
	USART_Init(UART4, &USART_InitStructure); //初始化串口4
		
	USART_Cmd(UART4, ENABLE);  //使能串口4
	
	//USART_ClearFlag(UART4, USART_FLAG_TC);
	
#if EN_UART4_RX	
	USART_ITConfig(UART4, USART_IT_RXNE, ENABLE);//开启相关中断

	//Usart4 NVIC 配置
  	NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;//串口4中断通道
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器、

#endif
	
}


void UART4_IRQHandler(void)                	//串口4中断服务程序
{
	u8 Res;
	u8 Data_len;
	//LED=0;
#if SYSTEM_SUPPORT_OS 		//如果SYSTEM_SUPPORT_OS为真，则需要支持OS.
	OSIntEnter();    
#endif
	if(USART_GetITStatus(UART4, USART_IT_RXNE) != RESET)  //接收中断(接收到的数据必须是0x0d 0x0a结尾)
	{
		//USART_ClearITPendingBit(UART4, USART_IT_RXNE);
		Res =USART_ReceiveData(UART4);//(UART4->DR);	//读取接收到的数据
		//DebugLed();
		if((UART4_RX_STA&0x8000)==0)//接收未完成
		{
			
			// printf("UART4_RX_STA: %i, RES: %i", UART4_RX_STA, Res);
			// 指令以6C开头则为LORA模块指令
			if((UART4_RX_STA==0&&Res==0x6c)||UART4_RX_BUF[0]==0X6C)
			{
				UART4_RX_BUF[UART4_RX_STA&0X3FFF]=Res;
				UART4_RX_STA++;
				if((UART4_RX_STA&0X3FFF)>=2) 
				{
					Data_len = UART4_RX_BUF[1] + 5; 
					//printf("USART_RX_BUF[1]: %i\r\n", USART_RX_BUF[1]); //Data_len = Res + 5;
					//printf("UART4_RX_STA: %i, Data len: %i", UART4_RX_STA, Data_len);
					if(UART4_RX_STA>(USART4_REC_LEN-1)) UART4_RX_STA=0;//接收数据错误(超过最大接受字节数),重新开始接收
					if((UART4_RX_STA&0X3FFF) == Data_len) UART4_RX_STA|=0x8000;	//接收完成了(0x16为lora通信规约结束符)
				}
			}
			else{
				// 否则为普通指令
				if(UART4_RX_STA&0x4000)//接收到了0x0d
				{
					if(Res!=0x0a)UART4_RX_STA=0;//接收错误,重新开始
					else UART4_RX_STA|=0x8000;	//接收完成了 
				}
				else //还没收到0X0D
				{	
					if(Res==0x0d)UART4_RX_STA|=0x4000;
					else
					{
						UART4_RX_BUF[UART4_RX_STA&0X3FFF]=Res ;
						UART4_RX_STA++;
						if(UART4_RX_STA>(USART4_REC_LEN-1))UART4_RX_STA=0;//接收数据错误,重新开始接收	  
					}		 
				}
			}
		}   		 
  } 
#if SYSTEM_SUPPORT_OS 	//如果SYSTEM_SUPPORT_OS为真，则需要支持OS.
	OSIntExit();  											 
#endif
} 
#endif	

 



