  
#include "usart4.h"	
#include "led.h"
////////////////////////////////////////////////////////////////////////////////// 	 
//���ʹ��ucos,����������ͷ�ļ�����.
#if SYSTEM_SUPPORT_OS
#include "includes.h"					//ucos ʹ��	  
#endif
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32F4̽���߿�����
//����1��ʼ��		   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2014/6/10
//�汾��V1.5
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2009-2019
//All rights reserved
//********************************************************************************
//V1.3�޸�˵�� 
//֧����Ӧ��ͬƵ���µĴ��ڲ���������.
//�����˶�printf��֧��
//�����˴��ڽ��������.
//������printf��һ���ַ���ʧ��bug
//V1.4�޸�˵��
//1,�޸Ĵ��ڳ�ʼ��IO��bug
//2,�޸���USART_RX_STA,ʹ�ô����������ֽ���Ϊ2��14�η�
//3,������USART_REC_LEN,���ڶ��崮�����������յ��ֽ���(������2��14�η�)
//4,�޸���EN_USART1_RX��ʹ�ܷ�ʽ
//V1.5�޸�˵��
//1,�����˶�UCOSII��֧��
////////////////////////////////////////////////////////////////////////////////// 	  
 

//////////////////////////////////////////////////////////////////
//�������´���,֧��printf����,������Ҫѡ��use MicroLIB	  
#if 1
//#pragma import(__use_no_semihosting)             
//��׼����Ҫ��֧�ֺ���                 
struct __FILE 
{ 
	int handle; 
}; 

FILE __stdout;       
//����_sys_exit()�Ա���ʹ�ð�����ģʽ    
void _sys_exit(int8_t x) 
{ 
	x = x; 
} 
//�ض���fputc���� 
int fputc(int ch, FILE *f)
{ 	
	while((UART4->SR&0X40)==0);//ѭ������,ֱ���������   
	UART4->DR = (u8) ch;      
	return ch;
}
#endif
 
#if EN_UART4_RX   //���ʹ���˽���
//����4�жϷ������
//ע��,��ȡUSARTx->SR�ܱ���Ī������Ĵ���   	
u8 UART4_RX_BUF[USART4_REC_LEN];     //���ջ���,���USART_REC_LEN���ֽ�.
//����״̬
//bit15��	������ɱ�־
//bit14��	���յ�0x0d
//bit13~0��	���յ�����Ч�ֽ���Ŀ
u16 UART4_RX_STA=0;       //����״̬���	

//��ʼ��IO ����4
//bound:������
void uart4_init(u32 bound){
   //GPIO�˿�����
  GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC,ENABLE); //ʹ��GPIOCʱ��
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4,ENABLE);//ʹ��USART4ʱ��
 
	//����4��Ӧ���Ÿ���ӳ��
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource10,GPIO_AF_UART4); //GPIOC10����ΪUSART4
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource11,GPIO_AF_UART4); //GPIOC11����ΪUSART4
	
	//USART4�˿�����
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_10; //GPIOC11��GPIOC10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//���ù���
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//�ٶ�50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //���츴�����
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //����
	GPIO_Init(GPIOC,&GPIO_InitStructure); //��ʼ��PC11��PC10

   //USART4 ��ʼ������
	USART_InitStructure.USART_BaudRate = bound;//����������
	USART_InitStructure.USART_WordLength = USART_WordLength_9b;//�ֳ�Ϊ8λ���ݸ�ʽ
	// USART_InitStructure.USART_WordLength = USART_WordLength_9b;//�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
	// USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
	USART_InitStructure.USART_Parity = USART_Parity_Even;//żУ��λ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ
	USART_Init(UART4, &USART_InitStructure); //��ʼ������4
		
	USART_Cmd(UART4, ENABLE);  //ʹ�ܴ���4
	
	//USART_ClearFlag(UART4, USART_FLAG_TC);
	
#if EN_UART4_RX	
	USART_ITConfig(UART4, USART_IT_RXNE, ENABLE);//��������ж�

	//Usart4 NVIC ����
  	NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;//����4�ж�ͨ��
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =3;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ�����

#endif
	
}


void UART4_IRQHandler(void)                	//����4�жϷ������
{
	u8 Res;
	u8 Data_len;
	//LED=0;
#if SYSTEM_SUPPORT_OS 		//���SYSTEM_SUPPORT_OSΪ�棬����Ҫ֧��OS.
	OSIntEnter();    
#endif
	if(USART_GetITStatus(UART4, USART_IT_RXNE) != RESET)  //�����ж�(���յ������ݱ�����0x0d 0x0a��β)
	{
		//USART_ClearITPendingBit(UART4, USART_IT_RXNE);
		Res =USART_ReceiveData(UART4);//(UART4->DR);	//��ȡ���յ�������
		//DebugLed();
		if((UART4_RX_STA&0x8000)==0)//����δ���
		{
			
			// printf("UART4_RX_STA: %i, RES: %i", UART4_RX_STA, Res);
			// ָ����6C��ͷ��ΪLORAģ��ָ��
			if((UART4_RX_STA==0&&Res==0x6c)||UART4_RX_BUF[0]==0X6C)
			{
				UART4_RX_BUF[UART4_RX_STA&0X3FFF]=Res;
				UART4_RX_STA++;
				if((UART4_RX_STA&0X3FFF)>=2) 
				{
					Data_len = UART4_RX_BUF[1] + 5; 
					//printf("USART_RX_BUF[1]: %i\r\n", USART_RX_BUF[1]); //Data_len = Res + 5;
					//printf("UART4_RX_STA: %i, Data len: %i", UART4_RX_STA, Data_len);
					if(UART4_RX_STA>(USART4_REC_LEN-1)) UART4_RX_STA=0;//�������ݴ���(�����������ֽ���),���¿�ʼ����
					if((UART4_RX_STA&0X3FFF) == Data_len) UART4_RX_STA|=0x8000;	//���������(0x16Ϊloraͨ�Ź�Լ������)
				}
			}
			else{
				// ����Ϊ��ָͨ��
				if(UART4_RX_STA&0x4000)//���յ���0x0d
				{
					if(Res!=0x0a)UART4_RX_STA=0;//���մ���,���¿�ʼ
					else UART4_RX_STA|=0x8000;	//��������� 
				}
				else //��û�յ�0X0D
				{	
					if(Res==0x0d)UART4_RX_STA|=0x4000;
					else
					{
						UART4_RX_BUF[UART4_RX_STA&0X3FFF]=Res ;
						UART4_RX_STA++;
						if(UART4_RX_STA>(USART4_REC_LEN-1))UART4_RX_STA=0;//�������ݴ���,���¿�ʼ����	  
					}		 
				}
			}
		}   		 
  } 
#if SYSTEM_SUPPORT_OS 	//���SYSTEM_SUPPORT_OSΪ�棬����Ҫ֧��OS.
	OSIntExit();  											 
#endif
} 
#endif	

 



