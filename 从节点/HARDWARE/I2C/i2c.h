#ifndef __I2C_H
#define __I2C_H			 

/* Includes ------------------------------------------------------------------*/
#include "sys.h"
#include <stm32f4xx.h>

//IO 方向设置
#define SDA_IN() {GPIOB->MODER&=~(3<<(9*2));GPIOB->MODER|=0<<9*2;}
//PB9 输入模式
#define SDA_OUT() {GPIOB->MODER&=~(3<<(9*2));GPIOB->MODER|=1<<9*2;} 
//PB9 输出模式
//IO 操作函数
#define IIC_SCL PBout(8) //SCL
#define IIC_SDA PBout(9) //SDA
#define READ_SDA PBin(9) //输入 SDA

/* Private function prototypes -----------------------------------------------*/
void IIC_Init(void); // 初始化 IIC
FunctionalState IIC_Start(void);
void IIC_Stop(void);
u8 IIC_Wait_Ack(void);
void IIC_Ack(void);
void IIC_NAck(void);
void IIC_Send_Byte(u8 txd);
u8 IIC_Read_Byte(void);
u8 i2cWrite(uint8_t addr_, uint8_t reg_, uint8_t data);
u8 i2cRead(uint8_t addr_, uint8_t reg_, uint8_t len, uint8_t* buf);


#endif 
/*********************************************************************************************************
      END FILE
*********************************************************************************************************/
