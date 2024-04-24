/******************** IECAS********************************
 * file:SHT2X.c
 * description: itialization sht2X and use read temperature and humidity.
 * writer    £ºzhaorongjian
 * Version update date: 2017-7-20
 * hardware connection:PB6-I2C1_SCL¡¢PB7-I2C1_SDA
 * Debug :STLink
 **********************************************************************************/
/* Includes ------------------------------------------------------------------*/
#include "SHT2X.h"
#include "math.h"
#include "i2c.h"
#include "delay.h"

float temperatureC; // final temperature of SHT2X
float humidityRH;   // final humidity of SHT2X

uint8_t sndata1[8];
uint8_t sndata2[6];
uint32_t SN1;
uint32_t SN2;

/**
 * @file   SHT2X_IIC_WriteByte
 * @brief  write a series of data from sht2X.
 * @param
 *          - SendByte: the data need to be write.
 *          - WriteAddress: the address to be write
 * @retval return ENABLE(succeed ) and DISABLE(defeat)
 */
FunctionalState SHT2X_IIC_WriteByte(uint8_t WriteAddress, uint8_t SendByte)
{
  if (!IIC_Start())
    return DISABLE;
  IIC_Send_Byte(0x80); // Set the high start address and Device Address
  if (!IIC_Wait_Ack())
  {
    IIC_Stop();
    return DISABLE;
  }
  IIC_Send_Byte(WriteAddress); // Set the low start address
  IIC_Wait_Ack();
  IIC_Send_Byte(SendByte);
  IIC_Wait_Ack();
  IIC_Stop();
  delay_ms(10); //[?]EEPROM ?????????????????????
  return ENABLE;
}
/**
 * @file   SHT2X_IIC_ReadByte
 * @brief  read a series of data from sht2X.
 * @param
 *					- pBuffer: data Buffer.
 *     	  - length: datalength.
 *         - ReadAddress: data address.
 * @retval return ENABLE(succeed ) and DISABLE(defeat)
 */
FunctionalState SHT2X_IIC_ReadByte(uint8_t ReadAddress, uint16_t length, uint8_t *pBuffer)
{
  if (!IIC_Start())
    return DISABLE;
  IIC_Send_Byte(0x80); // Set the high start address and Device Address
  if (!IIC_Wait_Ack())
  {
    IIC_Stop();
    return DISABLE;
  }
  IIC_Send_Byte(ReadAddress); // Set the low start address
  IIC_Wait_Ack();
  IIC_Start();
  IIC_Send_Byte(0x81);
  IIC_Wait_Ack();
  while (length)
  {
    *pBuffer = IIC_Read_Byte();
    if (length == 1)
      IIC_NAck();
    else
      IIC_Ack();
    pBuffer++;
    length--;
  }
  IIC_Stop();
  return ENABLE;
}
/**
 * @file   SHT2x_CheckCrc
 * @brief  calculates checksum for n bytes of data and compares it with expected
 * @param
 *					- data[]: the data to be need to check.
 *     	  - startByte: start byte.
 *         - nbrOfBytes: check number.
 *         - checksum: check sum.
 * @retval return ENABLE(succeed ) and DISABLE(defeat)
 */
FunctionalState SHT2x_CheckCrc(u8 data[], u8 startBytes, u8 SHT2x_number, u8 SHT2x_checksum)
{
  u8 bit = 0;
  u8 crc = 0;
  u8 byteCtr;
  // calculates 8-Bit checksum with given polynomial
  for (byteCtr = startBytes; byteCtr < startBytes + SHT2x_number; ++byteCtr)
  {
    crc ^= (data[byteCtr]);
    for (bit = 8; bit > 0; --bit)
    {
      if (crc & 0x80)
        crc = (crc << 1) ^ 0x131;
      else
        crc = (crc << 1);
    }
  }
  if (crc == SHT2x_checksum)
    return ENABLE;
  else
    return DISABLE;
}
/**
 * @file   SHT2x_ReadUserRegister
 * @brief  reads the SHT2x user register
 * @param  no
 * @retval return data
 */
u8 SHT2x_ReadUserRegister(void)
{
  u8 data[1] = {0};
  SHT2X_IIC_ReadByte(USER_REG_R, 1, data);
  return data[0];
}
/**
 * @file   SHT2x_WriteUserRegister
 * @brief  writes the SHT2x user register (8bit)
 * @param  userdata£ºthe parameter to be need write
 * @retval return ENABLE(succeed ) and DISABLE(defeat)
 */
FunctionalState SHT2x_WriteUserRegister(u8 userdata)
{
  SHT2X_IIC_WriteByte(USER_REG_W, userdata);
  if (userdata == SHT2x_ReadUserRegister())
    return ENABLE;
  else
    return DISABLE;
}

/**
 * @file   SHT2x_Calc_T
 * @brief  calculating temperature
 * @param  NO
 * @retval return ENABLE(succeed ) and DISABLE(defeat)
 */
FunctionalState SHT2x_Calc_T(void)
{
  u8 length = 0;
  u8 Tdata[3] = {0};
  if (!IIC_Start())
    return DISABLE;
  IIC_Send_Byte(I2C_ADR_W);
  if (!IIC_Wait_Ack())
  {
    IIC_Stop();
    return DISABLE;
  }
  IIC_Send_Byte(TRIG_T_MEASUREMENT_POLL);
  IIC_Wait_Ack();
  delay_us(20);
  IIC_Stop();
  do
  {
    IIC_Start();
    IIC_Send_Byte(I2C_ADR_R);
  } while (!IIC_Wait_Ack());
  for (length = 0; length <= 3; length++)
  {
    Tdata[length] = IIC_Read_Byte();
    IIC_Ack();
  };
  IIC_NAck();
  IIC_Stop();
  if (((Tdata[0] + Tdata[1] + Tdata[2]) > 0) && SHT2x_CheckCrc(Tdata, 0, 2, Tdata[2]))
  {
    temperatureC = (-46.85 + (175.72 / 65536) * ((float)((((uint16_t)Tdata[0] << 8) + (uint16_t)Tdata[1]) & 0xfffc)));
    return ENABLE;
  }
  else
    return DISABLE;
}

/**
 * @file   SHT2x_Calc_RH
 * @brief  calculating humidity
 * @param  NO
 * @retval return ENABLE(succeed ) and DISABLE(defeat)
 */
FunctionalState SHT2x_Calc_RH(void)
{
  u8 length = 0;
  u8 RHdata[3] = {0};
  if (!IIC_Start())
    return DISABLE;
  IIC_Send_Byte(I2C_ADR_W);
  if (!IIC_Wait_Ack())
  {
    IIC_Stop();
    return DISABLE;
  }
  IIC_Send_Byte(TRIG_RH_MEASUREMENT_POLL);
  IIC_Wait_Ack();
  delay_us(20);
  IIC_Stop();
  do
  {
    IIC_Start();
    IIC_Send_Byte(I2C_ADR_R);
  } while (!IIC_Wait_Ack());
  for (length = 0; length <= 3; length++)
  {
    RHdata[length] = IIC_Read_Byte();
    IIC_Ack();
  };
  IIC_NAck();
  IIC_Stop();
  if (((RHdata[0] + RHdata[1] + RHdata[2]) > 0) && SHT2x_CheckCrc(RHdata, 0, 2, RHdata[2]))
  {
    humidityRH = -6.0 + 125.0 / 65536 * ((float)((((uint16_t)RHdata[0] << 8) + (uint16_t)RHdata[1]) & 0xfff0));
    return ENABLE;
  }
  else
    return DISABLE;
}
/**
 * @file   SHT2x_SoftReset
 * @brief  softreset
 * @param  NO
 * @retval NO
 */
FunctionalState SHT2x_SoftReset(void)
{
  if (!IIC_Start())
    return DISABLE;
  IIC_Send_Byte(0x80);
  if (!IIC_Wait_Ack())
  {
    IIC_Stop();
    return DISABLE;
  }
  IIC_Send_Byte(SOFT_RESET);
  IIC_Wait_Ack();
  IIC_Stop();
  delay_ms(1500);
  return ENABLE;
}

/**
 * @file   SHT2x_GetSerialNumber
 * @brief  Get SerialNumber of sht2X
 * @param
 * @retval
 */
u8 SHT2x_GetSerialNumber(u8 *pBuffer1, u8 *pBuffer2)
{
  u8 length = 8;
  if (!IIC_Start())
    return DISABLE;
  IIC_Send_Byte(0x80);
  if (!IIC_Wait_Ack())
  {
    IIC_Stop();
    return DISABLE;
  }
  IIC_Send_Byte(0xfa);
  IIC_Wait_Ack();
  IIC_Send_Byte(0x0f);
  IIC_Wait_Ack();
  if (!IIC_Start())
    return DISABLE;
  IIC_Send_Byte(0x81);
  if (!IIC_Wait_Ack())
  {
    IIC_Stop();
    return DISABLE;
  }
  while (length)
  {
    *pBuffer1 = IIC_Read_Byte();
    if (length == 1)
      IIC_NAck();
    else
      IIC_Ack();
    pBuffer1++;
    length--;
  }
  IIC_Stop();
  length = 6;
  if (!IIC_Start())
    return DISABLE;
  IIC_Send_Byte(0x80);
  if (!IIC_Wait_Ack())
  {
    IIC_Stop();
    return DISABLE;
  }
  IIC_Send_Byte(0xfc);
  IIC_Wait_Ack();
  IIC_Send_Byte(0xc9);
  IIC_Wait_Ack();
  if (!IIC_Start())
    return DISABLE;
  IIC_Send_Byte(0x81);
  if (!IIC_Wait_Ack())
  {
    IIC_Stop();
    return DISABLE;
  }
  while (length)
  {
    *pBuffer2 = IIC_Read_Byte();
    if (length == 1)
      IIC_NAck();
    else
      IIC_Ack();
    pBuffer2++;
    length--;
  }
  IIC_Stop();
  return ENABLE;
}
/**
 * @file   SHT2X_Init
 * @brief  Init SHT2X
 * @param  NO
 * @retval int flag
 */
int SHT2X_Init(void)
{
  SHT2x_GetSerialNumber(sndata1, sndata2);
  if ((sndata1[0] + sndata1[1] + sndata1[3] + sndata1[4] + sndata1[5] + sndata1[6] + sndata1[7]) > 0)
  {
    if (
        SHT2x_CheckCrc(sndata1, 0, 1, sndata1[1]) &&
        SHT2x_CheckCrc(sndata1, 2, 1, sndata1[3]) &&
        SHT2x_CheckCrc(sndata1, 4, 1, sndata1[5]) &&
        SHT2x_CheckCrc(sndata1, 6, 1, sndata1[7]) &&
        SHT2x_CheckCrc(sndata2, 0, 2, sndata2[2]) &&
        SHT2x_CheckCrc(sndata2, 3, 2, sndata2[5]))
    {

      SN1 = ((sndata2[3] << 24) + (sndata2[4] << 16) + (sndata1[0] << 8) + sndata1[2]);
      SN2 = ((sndata1[4] << 24) + (sndata1[6] << 16) + (sndata2[0] << 8) + sndata2[1]);
    }
    else
      return 4;
    ;
  }
  else
  {
    SHT2x_GetSerialNumber(sndata1, sndata2);
    return 0;
  };

  if (SHT2x_WriteUserRegister(0x3a))
    return 1;
  else
    return 2;
  ;
}
/**
 * @file   SHT2X_TEST_T
 * @brief  get temperature of SHT20
 * @param  NO
 * @retval float temperature
 */
float SHT2X_TEST_T(void)
{
  /*calculate humidity of SHT2X*/
  if (SHT2x_Calc_T())
    return temperatureC;
  else
    return 0;
}
/**
 * @file   SHT2X_TEST_RH
 * @brief  get humidity of SHT20
 * @param  NO
 * @retval float humidity
 */
float SHT2X_TEST_RH(void)
{

  /*calculate humidity of SHT2X*/
  if (SHT2x_Calc_RH())
    return humidityRH;
  else
    return 0;
}

/*********************************************************************************************************
      END FILE
*********************************************************************************************************/
