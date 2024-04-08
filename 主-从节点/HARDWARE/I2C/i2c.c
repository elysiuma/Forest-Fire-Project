#include "i2c.h"
#include "delay.h"

// 初始化 IIC
void IIC_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE); // 使能 GPIOB 时钟
  // GPIOB8,B9 初始化设置
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;      // 普通输出模式
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;     // 推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz; // 100MHz
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;       // 上拉
  GPIO_Init(GPIOB, &GPIO_InitStructure);             // 初始化
  IIC_SCL = 1;
  IIC_SDA = 1;
}


// 产生 IIC 起始信号
FunctionalState IIC_Start(void)
{
  SDA_OUT(); // sda 线输出
  IIC_SDA = 1;
  IIC_SCL = 1;
  delay_us(4);
  IIC_SDA = 0; // START:when CLK is high,DATA change form high to low
  delay_us(4);
  IIC_SCL = 0; // 钳住 I2C 总线，准备发送或接收数据
  return ENABLE;
}


// 产生 IIC 停止信号
void IIC_Stop(void)
{
  SDA_OUT(); // sda 线输出
  IIC_SCL = 0;
  IIC_SDA = 0; // STOP:when CLK is high DATA change form low to high
  delay_us(4);
  IIC_SCL = 1;
  IIC_SDA = 1; // 发送 I2C 总线结束信号
  delay_us(4);
}


// 等待应答信号到来
// 返回值：10，接收应答失败
//  1，接收应答成功
u8 IIC_Wait_Ack(void)
{
  u8 ucErrTime = 0;
  SDA_IN(); // SDA 设置为输入
  IIC_SDA = 1;
  delay_us(1);
  IIC_SCL = 1;
  delay_us(1);
  while (READ_SDA)
  {
    ucErrTime++;
    if (ucErrTime > 250)
    {
      IIC_Stop();
      return 0;
    }
  }
  IIC_SCL = 0; // 时钟输出 0
  return 1;
}


// 产生 ACK 应答
void IIC_Ack(void)
{
  IIC_SCL = 0;
  SDA_OUT();
  IIC_SDA = 0;
  delay_us(2);
  IIC_SCL = 1;
  delay_us(2);
  IIC_SCL = 0;
}


// 不产生 ACK 应答
void IIC_NAck(void)
{
  IIC_SCL = 0;
  SDA_OUT();
  IIC_SDA = 1;
  delay_us(2);
  IIC_SCL = 1;
  delay_us(2);
  IIC_SCL = 0;
}


// IIC 发送一个字节
// 返回从机有无应答
// 1，有应答
// 0，无应答
void IIC_Send_Byte(u8 txd)
{
  u8 t;
  SDA_OUT();
  IIC_SCL = 0; // 拉低时钟开始数据传输
  for (t = 0; t < 8; t++)
  {
    IIC_SDA = (txd & 0x80) >> 7;
    txd <<= 1;
    delay_us(2); // 对 TEA5767 这三个延时都是必须的
    IIC_SCL = 1;
    delay_us(2);
    IIC_SCL = 0;
    delay_us(2);
  }
}


// 读 1 个字节，ack=1 时，发送 ACK，ack=0，发送 nACK
u8 IIC_Read_Byte()
{
  unsigned char i, receive = 0;
  SDA_IN(); // SDA 设置为输入
  for (i = 0; i < 8; i++)
  {
    IIC_SCL = 0;
    delay_us(2);
    IIC_SCL = 1;
    receive <<= 1;
    if (READ_SDA)
      receive++;
    delay_us(1);
  }
  // 這部分在SHT中完成
  // if (!ack)
  //   IIC_NAck(); // 发送 nACK
  // else
  //   IIC_Ack(); // 发送 ACK
  return receive;
}

u8 i2cWrite(uint8_t addr, uint8_t reg, uint8_t data)
{
    if (!IIC_Start())
        return 0;
    IIC_Send_Byte(addr << 1 | I2C_Direction_Transmitter);
    if (!IIC_Wait_Ack()) 
    {
        IIC_Stop();
        return 0;
    }
    IIC_Send_Byte(reg);
    IIC_Wait_Ack();
    IIC_Send_Byte(data);
    IIC_Wait_Ack();
    IIC_Stop();
    return 1;
}


u8 i2cRead(uint8_t addr, uint8_t reg, uint8_t len, uint8_t *buf)
{
    if (!IIC_Start())
        return 0;
    IIC_Send_Byte(addr << 1 | I2C_Direction_Transmitter);
    if (!IIC_Wait_Ack()) 
    {
        IIC_Stop();
        return 0;
    }
    IIC_Send_Byte(reg);
    IIC_Wait_Ack();
    IIC_Start();
    IIC_Send_Byte(addr << 1 | I2C_Direction_Receiver);
    IIC_Wait_Ack();
    while (len) 
    {
        *buf = IIC_Read_Byte();
        if (len == 1)
            IIC_NAck();
        else
            IIC_Ack();
        buf++;
        len--;
    }
    IIC_Stop();
    return 1;
}
