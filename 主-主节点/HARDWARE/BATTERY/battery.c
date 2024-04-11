#include "battery.h"
#include "adc.h"

void BATTERY_Init(void)
{
    Adc_Init_Battery();
}

float BATTERY_Scan(void)
{
    u16 adcx;
    float R1=1;
    float R2=4;
    // 3.6V~4.2V
    float max_battery_quantity=12.6;
    float min_battery_quantity=10.8;
    float battery_quantity;
    float battery_quantity_percent;

    adcx=Get_Adc_Average(ADC_Channel_15,15);
    //printf("adcx=%d\r\n",adcx);
    battery_quantity=(float)adcx*(2.5/4096);        //接入的标准源电压2.5V
    //printf("adc_battery_quantity=%f\r\n",battery_quantity);
    battery_quantity = battery_quantity*(R1+R2)/R1;
    battery_quantity_percent = (battery_quantity-min_battery_quantity)/(max_battery_quantity-min_battery_quantity)*100;
    // 百分比取一位小数
    battery_quantity_percent = (int)(battery_quantity_percent*10+0.5)/10.0;
    // printf("battery_quantity=%f, battery_quantity_percent=%f\r\n",battery_quantity,battery_quantity_percent);
    if (battery_quantity_percent>100)
    {
        battery_quantity_percent=100;
        // printf("battery_quantity_percent>100, set to 100\r\n");
    }
    return battery_quantity_percent;
}
