#include "sys.h"
#include "usart3.h"	
#include "delay.h"


#if EN_USART3_RX   		//如果使能USART3了接收
u8 USART3_RX_BUF[USART3_REC_LEN];
u8 USART3_TX_BUF[USART3_REC_LEN];
u16 USART3_RX_STA=0;   		//接收状态标记

//串口3中断服务程序
void USART3_IRQHandler(void)
{
	u8 Res;	    
	u16 Data_len;
	if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
	{	 	
		// 指令以＄开头的为GPS模块发来的信息 默认接受GGA协议信息 包含时间经纬度等等，0x0d 0x0a结尾
		//if((USART3_RX_STA==0&&Res==0x24)||USART3_RX_BUF[0]==0X24)
		Res = USART_ReceiveData(USART3);
		if((USART3_RX_STA&0x8000)==0)//接收未完成
		{
			//GPS模块返回确认帧
			if((USART3_RX_STA==0&&Res==0xA0)||USART3_RX_BUF[0]==0XA0)
			{
				USART3_RX_BUF[USART3_RX_STA&0X3FFF]=Res;
				USART3_RX_STA++;
				if((USART3_RX_STA&0X3FFF)>=4) 
				{
					Data_len = 256 * USART3_RX_BUF[2] + USART3_RX_BUF[3] + 7; 	// 包含0d0a的长度
					//printf("USART_RX_BUF[1]: %i\r\n", USART_RX_BUF[1]); //Data_len = Res + 5;
					//printf("UART4_RX_STA: %i, Data len: %i", UART4_RX_STA, Data_len);
					if(USART3_RX_STA>(USART3_REC_LEN-1)) 
						USART3_RX_STA=0;//接收数据错误(超过最大接受字节数),重新开始接收
					if((USART3_RX_STA&0X3FFF) == Data_len) 
						USART3_RX_STA|=0x8000;	//接收完成了
				}
			}
			
			else
			{	
				if(USART3_RX_STA&0x4000)//接收到了0x0d
				{
					if(Res!=0x0a) USART3_RX_STA=0;//接收错误,重新开始
					else 
					{
						USART3_RX_BUF[USART3_RX_STA&0X3FFF]=Res;
						USART3_RX_STA|=0x8000;	//接收完成了 
					}
				}
				else //还没收到0X0D
				{	
					if(Res==0x0d) 
					{
						USART3_RX_STA|=0x4000;
						USART3_RX_BUF[USART3_RX_STA&0X3FFF]=Res;
						USART3_RX_STA++;
					}
					else
					{
						USART3_RX_BUF[USART3_RX_STA&0X3FFF]=Res;
						USART3_RX_STA++;
						if(USART3_RX_STA>(USART3_REC_LEN-1)) USART3_RX_STA=0;//接收数据错误,重新开始接收	  
					}		 
				}
			}
		}
		else
		{
			USART3_RX_STA=0;
			USART3_RX_BUF[0]=Res;
			USART3_RX_STA++;
		} 
	} 											 
} 

#endif								 

void uart3_init(u32 bound)
{  	
	//GPIO端口设置
	//串口通信口
    //TXDO 模块发送 TTL PD5
    //RXDI 模块接收 TTL PD6
  	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD,ENABLE); 
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,ENABLE);	//使能USART3时钟
	
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource8,GPIO_AF_USART3); //PD8复用为USART3
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource9,GPIO_AF_USART3); //PD9复用为USART3
	
  	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF; //复用功能 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//速度 50MHz 
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; 	//推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; 	//上拉 
	GPIO_Init(GPIOD,&GPIO_InitStructure); 	//初始化 PD8，PD9
		

	USART_InitStructure.USART_BaudRate = bound;//波特率设置
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为9位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	//USART_InitStructure.USART_Parity = USART_Parity_Even;//偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
  	USART_Init(USART3, &USART_InitStructure); //初始化串口3
	
 	USART_Cmd(USART3, ENABLE);  //使能串口 3
	
	USART_ClearFlag(USART3, USART_FLAG_TC);
	
#if EN_USART3_RX	
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);//开启接受中断

	//Usart2 NVIC 配置
 	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器、

#endif	
}


void USART3_DATA(u8 *buf,u8 len)
{
	u8 t;
	//printf("USART3_DATA: %i", buf[1]);
  	for(t=0;t<len;t++)		
	{
		USART_SendData(USART3,buf[t]); 
	  	while(USART_GetFlagStatus(USART3,USART_FLAG_TC)!=SET);
		//printf("SEND_DATA: %x", buf[t]);
    	    
	}	 
	//while(USART_GetFlagStatus(USART2,USART_FLAG_TC)==RESET);	
	//USART2_RX_CNT=0;	  
	
}


void USART3_CMD(unsigned char *lb)
{
    while(*lb)
    {
        USART_SendData(USART3,*lb);
        while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET)
        {
        }
        lb ++;
    }
}


void USART3_Receive_Data(u8 *buf,u8 *len)
{
	u8 i=0;		
	u8 data_len=USART3_RX_STA&0x3fff;
	delay_ms(10);	
	// 打印接收到的数据
	//printf("data_len: %i, len: %i", data_len, *len);
	if(data_len>0)
	{
		for(i=0;i<data_len;i++)
		{
			buf[(*len)+i]=USART3_RX_BUF[i];	
		}	
		*len+=data_len;	
		USART3_RX_STA=0;	
	}
}


void USART3_Receive_Data_NoClear(u8 *buf,u8 *len)
{
	u8 i=0;
	*len=0;				
	delay_ms(10);		
	if(USART3_RX_STA>0)
	{
		for(i=0;i<USART3_RX_STA;i++)
		{
			buf[i]=USART3_RX_BUF[i];	
		}		
		*len=USART3_RX_STA;
	}
}

void Usart3CommandAnalysis(void)
{
	// u8 rxlen=0;
	// memset(USART2_TX_BUF, 0, sizeof(USART2_TX_BUF));
	// USART2_Receive_Data(USART2_TX_BUF,&rxlen);
	
}


