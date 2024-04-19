#include "flash.h"
#include "mq2.h"
#include "mq7.h"
#include "stm32f4xx_flash.h"

uint16_t write_data[DATA_FLASH_SAVE_NUM];
uint16_t read_data[DATA_FLASH_SAVE_NUM];

//通过地址获取扇区位置
uint16_t STMFLASH_GetFlashSector(u32 addr)
{
	if(addr<ADDR_FLASH_SECTOR_1)return FLASH_Sector_0;
	else if(addr<ADDR_FLASH_SECTOR_2)return FLASH_Sector_1;
	else if(addr<ADDR_FLASH_SECTOR_3)return FLASH_Sector_2;
	else if(addr<ADDR_FLASH_SECTOR_4)return FLASH_Sector_3;
	else if(addr<ADDR_FLASH_SECTOR_5)return FLASH_Sector_4;
	else if(addr<ADDR_FLASH_SECTOR_6)return FLASH_Sector_5;
	else if(addr<ADDR_FLASH_SECTOR_7)return FLASH_Sector_6;
	else if(addr<ADDR_FLASH_SECTOR_8)return FLASH_Sector_7;
	else if(addr<ADDR_FLASH_SECTOR_9)return FLASH_Sector_8;
	else if(addr<ADDR_FLASH_SECTOR_10)return FLASH_Sector_9;
	else if(addr<ADDR_FLASH_SECTOR_11)return FLASH_Sector_10; 
	return FLASH_Sector_11;	
}

//将数据写入内存 16位数据
int write_flash(uint16_t *FlashWriteBuf)
{
	int i;
	uint32_t StartAddr;
	StartAddr = FLASH_SAVE_ADDR;

	FLASH_Unlock();	//解锁
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
                  FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);

	if (FLASH_COMPLETE != FLASH_EraseSector(STMFLASH_GetFlashSector(StartAddr),VoltageRange_2)) //擦除扇区内容
    {		
		return TEST_ERROR;
	}
	
	for (i = 0; i < DATA_FLASH_SAVE_NUM; i++)
	{
		if (FLASH_COMPLETE != FLASH_ProgramHalfWord(StartAddr, FlashWriteBuf[i]))	//写入16位数据
		{			
			return TEST_ERROR;
		}
		StartAddr += 2;	//16位数据偏移两个位置
	}

	FLASH_Lock();	//上锁
     
	return TEST_SUCCESS;
}

//从内存读数据 16位数据
void read_flash(uint16_t *FlashReadBuf)
{
	int i;
	uint32_t StartAddr = FLASH_SAVE_ADDR;
	for (i = 0; i < DATA_FLASH_SAVE_NUM; i++)
	{
		FlashReadBuf[i] = *(__IO uint16_t*)StartAddr;
		StartAddr += 2;
	}
}

// 将float转换为两位uint16_t，整数和小数部分分别转换
void float_to_uint16(float f_number, uint16_t *u_list)
{
	int integer_part = (int)f_number; // 获取整数部分
	float decimal_part = f_number - integer_part; // 获取小数部分

	// 将整数部分转换为uint16_t
	u_list[0] = (uint16_t)integer_part;

	// 将小数部分映射到65535的u16范围中
	u_list[1] = (uint16_t)(decimal_part * 65535);
}

float uint16_to_float(uint16_t *u_list)
{
	float f_number;
	int integer_part = u_list[0];
	float decimal_part = (float)u_list[1] / 65535;

	f_number = integer_part + decimal_part;

	return f_number;
}

void write_to_flash(void)
{
	float mq2_R0, mq7_R0;
	uint16_t u16_buf[2];
	memset(write_data, 0, sizeof(write_data));
	// 获取float的R0
	mq2_R0 = MQ2_Get_R0();
	mq7_R0 = MQ7_Get_R0();
	// 将float转换为两个uint16_t
	float_to_uint16(mq2_R0, u16_buf);
	write_data[0] = u16_buf[0];
	write_data[1] = u16_buf[1];
	float_to_uint16(mq7_R0, u16_buf);
	write_data[2] = u16_buf[0];
	write_data[3] = u16_buf[1];
	if(TEST_SUCCESS!=write_flash(write_data))	
		return;  //写入错误
}


void read_from_flash(float *FlashReadBuf, int *len)
{
	memset(read_data, 0, sizeof(read_data));
	read_flash(read_data);
	FlashReadBuf[0] = uint16_to_float(&read_data[0]);	// MQ2 R0
	FlashReadBuf[1] = uint16_to_float(&read_data[2]);	// MQ7 R0
	*len = 2;
}
