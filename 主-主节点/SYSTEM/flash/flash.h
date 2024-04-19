#ifndef _flash_H
#define _flash_H

#include "stm32f4xx.h"

#define  TEST_ERROR    -1
#define  TEST_SUCCESS  0 

#define DATA_FLASH_SAVE_NUM 4   //存储数据个数（两个float）
 
#define FLASH_SAVE_ADDR  ADDR_FLASH_SECTOR_4    //扇区有64kb的大小 一般寸几个数据已经足够
//FLASH 扇区的起始地址
#define ADDR_FLASH_SECTOR_0     ((u32)0x08000000) 	//扇区0起始地址, 16 Kbytes  
#define ADDR_FLASH_SECTOR_1     ((u32)0x08004000) 	//扇区1起始地址, 16 Kbytes  
#define ADDR_FLASH_SECTOR_2     ((u32)0x08008000) 	//扇区2起始地址, 16 Kbytes  
#define ADDR_FLASH_SECTOR_3     ((u32)0x0800C000) 	//扇区3起始地址, 16 Kbytes  
#define ADDR_FLASH_SECTOR_4     ((u32)0x08010000) 	//扇区4起始地址, 64 Kbytes  
#define ADDR_FLASH_SECTOR_5     ((u32)0x08020000) 	//扇区5起始地址, 128 Kbytes  
#define ADDR_FLASH_SECTOR_6     ((u32)0x08040000) 	//扇区6起始地址, 128 Kbytes  
#define ADDR_FLASH_SECTOR_7     ((u32)0x08060000) 	//扇区7起始地址, 128 Kbytes  
#define ADDR_FLASH_SECTOR_8     ((u32)0x08080000) 	//扇区8起始地址, 128 Kbytes  
#define ADDR_FLASH_SECTOR_9     ((u32)0x080A0000) 	//扇区9起始地址, 128 Kbytes  
#define ADDR_FLASH_SECTOR_10    ((u32)0x080C0000) 	//扇区10起始地址,128 Kbytes  
#define ADDR_FLASH_SECTOR_11    ((u32)0x080E0000) 	//扇区11起始地址,128 Kbytes  

uint16_t STMFLASH_GetFlashSector(u32 addr);  

void write_to_flash(void);
void read_from_flash(float *FlashReadBuf, int *len);    // 从flash中读取数据

void read_flash(uint16_t *FlashReadBuf);
int write_flash(uint16_t *FlashWriteBuf);
void float_to_uint16(float f_number, uint16_t *u_list);    // 将float转换为两位uint16_t，整数和小数部分分别转换
float uint16_to_float(uint16_t *u_list);    // 将两位uint16_t转换为float，整数和小数部分分别转换

#endif

