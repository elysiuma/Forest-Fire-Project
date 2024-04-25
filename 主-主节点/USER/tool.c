#include "sys.h"
#include "tool.h"

float u8_to_float(u8* data)
{
	union data{	
		float f;
		u8 ch[4];
	} data_1;		// 联合体变量
	float data_float;

	data_1.ch[0] = data[0];
	data_1.ch[1] = data[1];
	data_1.ch[2] = data[2];
	data_1.ch[3] = data[3];
	data_float = data_1.f;
	return data_float;
}

void float_to_u8(float data, u8* data_u8)
{
	union data{	
		float f;
		u8 ch[4];
	} data_1;		// 联合体变量

	data_1.f = data;
	data_u8[0] = data_1.ch[0];
	data_u8[1] = data_1.ch[1];
	data_u8[2] = data_1.ch[2];
	data_u8[3] = data_1.ch[3];
}