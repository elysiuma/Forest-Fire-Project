/******************** IECAS********************************
 * file £ºBMP280.c
 * description: itialization BMP280 and with Calculate temperature and pressure.
 * writer    £ºzhaorongjian
 * Version update date: 2017-7-20
 * hardware connection:PB6-I2C1_SCL¡¢PB7-I2C1_SDA
 * Debug :STLink
**********************************************************************************/
/* Includes ------------------------------------------------------------------*/
#include <stm32f4xx_gpio.h>
#include <stdio.h>
#include <math.h>
#include "bmp280.h"
#include "usart4.h"
#include "i2c.h"
#include "delay.h"

/* --------------------------------correction coefficient----------------------------------*/
unsigned short dig_T1 ;
short dig_T2 ;
short dig_T3 ;
unsigned short dig_P1 ;
short dig_P2 ;
short dig_P3 ;
short dig_P4 ;
short dig_P5 ;
short dig_P6 ;
short dig_P7 ;
short dig_P8 ;
short dig_P9 ;
double t_fine; 
 /**
  * @file   bmp280_uint
  * @brief  Calculation  correction coefficient
  * @param  void
  * @retval void
  */
void bmp280_uint(void)
{
			uint8_t bmp280_id; 
			uint8_t buf[2];
			i2cRead(BMP280_ADDRESS,BMP280_CHIPID_REG,1,&bmp280_id);
			Delay (100);
			if(bmp280_id == 0x58)
			{
			i2cWrite(BMP280_ADDRESS, BMP280_RESET_REG,0xB6);       //RESET_REG
			i2cWrite(BMP280_ADDRESS,BMP280_CTRLMEAS_REG,0x4B);    //010,010,11
			i2cWrite(BMP280_ADDRESS,BMP280_CONFIG_REG,0xE4);      //111,001,00
			}
			Delay (100);
			i2cRead(BMP280_ADDRESS,BMP280_CHIPID_REG,1,&bmp280_id);
			Delay (100);
			i2cRead(BMP280_ADDRESS,BMP280_DIG_T1_LSB_REG,1, &buf[1]);
			i2cRead(BMP280_ADDRESS,BMP280_DIG_T1_MSB_REG,1, &buf[0]);
			dig_T1  = buf[0] << 8 |buf[1];
			//dig_T1=27136;
			Delay (100);
			i2cRead(BMP280_ADDRESS,BMP280_DIG_T2_LSB_REG,1, &buf[1]);
			i2cRead(BMP280_ADDRESS,BMP280_DIG_T2_MSB_REG,1, &buf[0]);
			dig_T2  = buf[0] << 8 |buf[1];
			Delay (100);
			i2cRead(BMP280_ADDRESS,BMP280_DIG_T3_LSB_REG,1, &buf[1]);
			i2cRead(BMP280_ADDRESS,BMP280_DIG_T3_MSB_REG,1, &buf[0]);
			dig_T3  = buf[0] << 8 |buf[1];
			Delay (100);
			i2cRead(BMP280_ADDRESS,BMP280_DIG_P1_LSB_REG,1, &buf[1]);
			i2cRead(BMP280_ADDRESS,BMP280_DIG_P1_MSB_REG,1, &buf[0]);
			dig_P1  = buf[0] << 8 |buf[1];
			Delay (100);
			i2cRead(BMP280_ADDRESS,BMP280_DIG_P2_LSB_REG,1, &buf[1]);
			i2cRead(BMP280_ADDRESS,BMP280_DIG_P2_MSB_REG,1, &buf[0]);
			dig_P2  = buf[0] << 8 |buf[1];
			Delay (100);
			i2cRead(BMP280_ADDRESS,BMP280_DIG_P3_LSB_REG,1, &buf[1]);
			i2cRead(BMP280_ADDRESS,BMP280_DIG_P3_MSB_REG,1, &buf[0]);
			dig_P3  = buf[0] << 8 |buf[1];
			Delay (100);	
			i2cRead(BMP280_ADDRESS,BMP280_DIG_P4_LSB_REG,1, &buf[1]);
			i2cRead(BMP280_ADDRESS,BMP280_DIG_P4_MSB_REG,1, &buf[0]);
			dig_P4  = buf[0] << 8 |buf[1];
			Delay (100);
			i2cRead(BMP280_ADDRESS,BMP280_DIG_P5_LSB_REG,1, &buf[1]);
			i2cRead(BMP280_ADDRESS,BMP280_DIG_P5_MSB_REG,1, &buf[0]);
			dig_P5  = buf[0] << 8 |buf[1];
			Delay (100);
			i2cRead(BMP280_ADDRESS,BMP280_DIG_P6_LSB_REG,1, &buf[1]);
			i2cRead(BMP280_ADDRESS,BMP280_DIG_P6_MSB_REG,1, &buf[0]);
			dig_P6  = buf[0] << 8 |buf[1];
			Delay (100);
			i2cRead(BMP280_ADDRESS,BMP280_DIG_P7_LSB_REG,1, &buf[1]);
			i2cRead(BMP280_ADDRESS,BMP280_DIG_P7_MSB_REG,1, &buf[0]);
			dig_P7  = buf[0] << 8 |buf[1];
			Delay (100);
			i2cRead(BMP280_ADDRESS,BMP280_DIG_P8_LSB_REG,1, &buf[1]);
			i2cRead(BMP280_ADDRESS,BMP280_DIG_P8_MSB_REG,1, &buf[0]);
			dig_P8  = buf[0] << 8 |buf[1];
			Delay (100);
			i2cRead(BMP280_ADDRESS,BMP280_DIG_P9_LSB_REG,1, &buf[1]);
			i2cRead(BMP280_ADDRESS,BMP280_DIG_P9_MSB_REG,1, &buf[0]);
			dig_P9  = buf[0] << 8 |buf[1];  
}
 /**
  * @file   bmp280_compensate_temperature_double
  * @brief  use  correction coefficient,Calculation adc_T to temperature
  * @param  adc_T
  * @retval Returns temperature in DegC, double precision. Output value of “51.23” equals 51.23 DegC.
  */
 double bmp280_compensate_temperature_double( int32_t bmp280_adc_T)
{  
    double var1, var2, bmp280_compensatetemperature;  
  
    var1 = (((double) bmp280_adc_T) / 16384.0 - ((double) dig_T1) / 1024.0)  
            * ((double) dig_T2);  
    var2 = ((((double) bmp280_adc_T) / 131072.0 - ((double) dig_T1) / 8192.0)  
            * (((double) bmp280_adc_T) / 131072.0 - ((double) dig_T1) / 8192.0))  
            * ((double) dig_T3);  
		//printf("bmp280_adc_T:%d, dig_T1:%d, dig_T2:%d, dig_T3:%d  \r\n", bmp280_adc_T, dig_T1, dig_T2, dig_T3);
    t_fine = (int32_t) (var1 + var2);  
    bmp280_compensatetemperature = (var1 + var2) / 5120.0;  
    return bmp280_compensatetemperature;  
}  
  /**
  * @file   bmp280_compensate_pressure_double
  * @brief  use  correction coefficient,Calculation adc_P to pressure
  * @param  adc_P
  * @retval Returns pressure in Pa as double. Output value of “96386.2” equals 96386.2 Pa = 963.862 hPa
  */  

 double bmp280_compensate_pressure_double( int32_t bmp280_adc_P) 
{  
    double var1, var2, bmp280_compensate_tepressure;  
  
    var1 = ((double) t_fine / 2.0) - 64000.0;
    var2 = var1 * var1 * ((double) dig_P6) / 32768.0;  
    var2 = var2 + var1 * ((double) dig_P5) * 2.0;  
    var2 = (var2 / 4.0) + (((double) dig_P4) * 65536.0);  
    var1 = (((double) dig_P3) * var1 * var1 / 524288.0  
            + ((double) dig_P2) * var1) / 524288.0;  
    var1 = (1.0 + var1 / 32768.0) * ((double) dig_P1);  
  
    if (var1 == 0.0) {  
        return 0; // avoid exception caused by division by zero  
    }
    bmp280_compensate_tepressure = 1048576.0 - (double) bmp280_adc_P;
    bmp280_compensate_tepressure = (bmp280_compensate_tepressure - (var2 / 4096.0)) * 6250.0 / var1;  
    var1 = ((double) dig_P9) * bmp280_compensate_tepressure * bmp280_compensate_tepressure / 2147483648.0;  
    var2 = bmp280_compensate_tepressure * ((double) dig_P8) / 32768.0;  
    bmp280_compensate_tepressure = bmp280_compensate_tepressure + (var1 + var2 + ((double) dig_P7)) / 16.0;  
  
    return bmp280_compensate_tepressure;  
}  
  /**
  * @file   bmp280_get_temperature
  * @brief  get temperature of bmp280
  * @param  void 
  * @retval double temperature
  */  
double bmp280_get_temperature(void) 
{  
    uint8_t lsb, msb, xlsb;  
    int32_t bmp280_adc_T;  
    double bmp280_temperature;   
 	 
	i2cRead(BMP280_ADDRESS, BMP280_TEMPERATURE_XLSB_REG,1,&xlsb);  
    i2cRead(BMP280_ADDRESS, BMP280_TEMPERATURE_LSB_REG,1,&lsb);  
    i2cRead(BMP280_ADDRESS, BMP280_TEMPERATURE_MSB_REG,1,&msb);    
    bmp280_adc_T = (msb << 12) | (lsb << 4) | (xlsb >> 4);  
    bmp280_temperature = bmp280_compensate_temperature_double(bmp280_adc_T);  
    return bmp280_temperature;  
}  
  /**
  * @file   bmp280_get_pressure
  * @brief  get pressure of bmp280
  * @param  void 
  * @retval double pressure
  */ 
double bmp280_get_pressure(void) 
{ 
    uint8_t lsb, msb, xlsb;  
    int32_t bmp280_adc_P;  
    double bmp280_pressure;	
    i2cRead(BMP280_ADDRESS, BMP280_PRESSURE_XLSB_REG,1,&xlsb);  
    i2cRead(BMP280_ADDRESS, BMP280_PRESSURE_LSB_REG,1,&lsb);  
    i2cRead(BMP280_ADDRESS, BMP280_PRESSURE_MSB_REG,1,&msb);   
    bmp280_adc_P = (msb << 12) | (lsb << 4) | (xlsb >> 4); 
    bmp280_pressure = bmp280_compensate_pressure_double( bmp280_adc_P);    
    return bmp280_pressure;  
}  
  /**
  * @file   bmp280_get_Altitude
  * @brief  get Altitude of bmp280
  * @param  void 
  * @retval double Altitude
  */ 
float bmp280_get_Altitude(float bmp280_fp) 
{  
		float Altitude;
		Altitude = 44330*(1-pow((bmp280_fp/101325.0),(1.0/5.255))); 
		return Altitude; 
}  
/*********************************************************************************************************
      END FILE
*********************************************************************************************************/





  
