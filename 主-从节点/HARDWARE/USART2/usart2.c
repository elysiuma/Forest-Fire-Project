#include "sys.h"
#include "usart2.h"	
#include "delay.h"


#if EN_USART2_RX   		//���ʹ��USART2�˽���
u8 USART2_RX_BUF[USART2_REC_LEN];
u8 USART2_TX_BUF[USART2_REC_LEN];
u16 USART2_RX_CNT=0;   		//����״̬���

//����2�жϷ������
void USART2_IRQHandler(void)
{
	u8 res;	    
	u8 Data_len;
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
	{	 	
	 	res =USART_ReceiveData(USART2);
		if((USART2_RX_CNT&0x8000)==0)//����δ���
		{
			//printf("USART2_RX_CNT: %i, RES: %i", USART2_RX_CNT, res);
			USART2_RX_BUF[USART2_RX_CNT&0X3FFF]=res;
			USART2_RX_CNT++;
			// printf("UART4_RX_STA: %i, RES: %i", UART4_RX_STA, Res);
			if((USART2_RX_CNT&0X3FFF)>=2) 
			{
				Data_len = USART2_RX_BUF[1] + 5; 
				if(USART2_RX_CNT>(USART2_REC_LEN-1)) USART2_RX_CNT=0;//�������ݴ���(�����������ֽ���),���¿�ʼ����
				if((USART2_RX_CNT&0X3FFF) == Data_len) USART2_RX_CNT|=0x8000;	//���������(0x16Ϊloraͨ�Ź�Լ������)
			}
		}
	}  											 
} 

#endif										 


void uart2_init(u32 bound)
{  	
	//GPIO�˿�����
	//����ͨ�ſ�
    //TXDO ģ�鷢�� TTL PD5
    //RXDI ģ����� TTL PD6
  	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD,ENABLE); 
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE);	//ʹ��USART2ʱ��
	
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource5,GPIO_AF_USART2); //PD5����ΪUSART2
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource6,GPIO_AF_USART2); //PD6����ΪUSART2
	
  	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF; //���ù��� 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//�ٶ� 50MHz 
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; 	//���츴�����
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; 	//���� 
	GPIO_Init(GPIOD,&GPIO_InitStructure); 	//��ʼ�� PD5��PD6
		

	USART_InitStructure.USART_BaudRate = bound;//����������
	USART_InitStructure.USART_WordLength = USART_WordLength_9b;//�ֳ�Ϊ9λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
	// USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
	USART_InitStructure.USART_Parity = USART_Parity_Even;//żУ��λ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ
  	USART_Init(USART2, &USART_InitStructure); //��ʼ������2
	
 	USART_Cmd(USART2, ENABLE);  //ʹ�ܴ��� 2
	
	USART_ClearFlag(USART2, USART_FLAG_TC);
	
#if EN_USART2_RX	
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//���������ж�

	//Usart2 NVIC ����
 	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =3;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ�����

#endif	
}


void USART2_DATA(u8 *buf,u8 len)
{
	u8 t;
	//printf("USART2_DATA: %i", buf[1]);
  	for(t=0;t<len;t++)		
	{
		USART_SendData(USART2,buf[t]); 
	  	while(USART_GetFlagStatus(USART2,USART_FLAG_TC)!=SET);
		//printf("SEND_DATA: %x", buf[t]);
    	    
	}	 
	//while(USART_GetFlagStatus(USART2,USART_FLAG_TC)==RESET);	
	//USART2_RX_CNT=0;	  
	
}


void USART2_CMD(unsigned char *lb)
{
    while(*lb)
    {
        USART_SendData(USART2,*lb);
        while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)
        {
        }
        lb ++;
    }
}


void USART2_Receive_Data(u8 *buf,u8 *len)
{
	u8 i=0;		
	u8 data_len=USART2_RX_CNT&0x3fff;
	delay_ms(10);	
	// ��ӡ���յ�������
	//printf("data_len: %i, len: %i", data_len, *len);
	if(data_len>0)
	{
		for(i=0;i<data_len;i++)
		{
			buf[(*len)+i]=USART2_RX_BUF[i];	
		}	
		*len+=data_len;	
		USART2_RX_CNT=0;	
	}
}


void USART2_Receive_Data_NoClear(u8 *buf,u8 *len)
{
	u8 i=0;
	*len=0;				
	delay_ms(10);		
	if(USART2_RX_CNT>0)
	{
		for(i=0;i<USART2_RX_CNT;i++)
		{
			buf[i]=USART2_RX_BUF[i];	
		}		
		*len=USART2_RX_CNT;
	}
}

void Usart2CommandAnalysis(void)
{
	// u8 rxlen=0;
	// memset(USART2_TX_BUF, 0, sizeof(USART2_TX_BUF));
	// USART2_Receive_Data(USART2_TX_BUF,&rxlen);
	
}


