#include "sys.h"
#include "usart3.h"	
#include "delay.h"


#if EN_USART3_RX   		//���ʹ��USART3�˽���
u8 USART3_RX_BUF[USART3_REC_LEN];
u8 USART3_TX_BUF[USART3_REC_LEN];
u16 USART3_RX_STA=0;   		//����״̬���

//����3�жϷ������
void USART3_IRQHandler(void)
{
	u8 Res;	    
	u16 Data_len;
	if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
	{	 	
		// ָ���ԡ翪ͷ��ΪGPSģ�鷢������Ϣ Ĭ�Ͻ���GGAЭ����Ϣ ����ʱ�侭γ�ȵȵȣ�0x0d 0x0a��β
		//if((USART3_RX_STA==0&&Res==0x24)||USART3_RX_BUF[0]==0X24)
		Res = USART_ReceiveData(USART3);
		if((USART3_RX_STA&0x8000)==0)//����δ���
		{
			//GPSģ�鷵��ȷ��֡
			if((USART3_RX_STA==0&&Res==0xA0)||USART3_RX_BUF[0]==0XA0)
			{
				USART3_RX_BUF[USART3_RX_STA&0X3FFF]=Res;
				USART3_RX_STA++;
				if((USART3_RX_STA&0X3FFF)>=4) 
				{
					Data_len = 256 * USART3_RX_BUF[2] + USART3_RX_BUF[3] + 7; 	// ����0d0a�ĳ���
					//printf("USART_RX_BUF[1]: %i\r\n", USART_RX_BUF[1]); //Data_len = Res + 5;
					//printf("UART4_RX_STA: %i, Data len: %i", UART4_RX_STA, Data_len);
					if(USART3_RX_STA>(USART3_REC_LEN-1)) 
						USART3_RX_STA=0;//�������ݴ���(�����������ֽ���),���¿�ʼ����
					if((USART3_RX_STA&0X3FFF) == Data_len) 
						USART3_RX_STA|=0x8000;	//���������
				}
			}
			
			else
			{	
				if(USART3_RX_STA&0x4000)//���յ���0x0d
				{
					if(Res!=0x0a) USART3_RX_STA=0;//���մ���,���¿�ʼ
					else 
					{
						USART3_RX_BUF[USART3_RX_STA&0X3FFF]=Res;
						USART3_RX_STA|=0x8000;	//��������� 
					}
				}
				else //��û�յ�0X0D
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
						if(USART3_RX_STA>(USART3_REC_LEN-1)) USART3_RX_STA=0;//�������ݴ���,���¿�ʼ����	  
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
	//GPIO�˿�����
	//����ͨ�ſ�
    //TXDO ģ�鷢�� TTL PD5
    //RXDI ģ����� TTL PD6
  	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD,ENABLE); 
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,ENABLE);	//ʹ��USART3ʱ��
	
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource8,GPIO_AF_USART3); //PD8����ΪUSART3
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource9,GPIO_AF_USART3); //PD9����ΪUSART3
	
  	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF; //���ù��� 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//�ٶ� 50MHz 
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; 	//���츴�����
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; 	//���� 
	GPIO_Init(GPIOD,&GPIO_InitStructure); 	//��ʼ�� PD8��PD9
		

	USART_InitStructure.USART_BaudRate = bound;//����������
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ9λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
	//USART_InitStructure.USART_Parity = USART_Parity_Even;//żУ��λ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ
  	USART_Init(USART3, &USART_InitStructure); //��ʼ������3
	
 	USART_Cmd(USART3, ENABLE);  //ʹ�ܴ��� 3
	
	USART_ClearFlag(USART3, USART_FLAG_TC);
	
#if EN_USART3_RX	
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);//���������ж�

	//Usart2 NVIC ����
 	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =3;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ�����

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
	// ��ӡ���յ�������
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


