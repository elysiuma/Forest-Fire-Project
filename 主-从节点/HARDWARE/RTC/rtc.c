#include "sys.h"
#include "rtc.h"
#include "delay.h"
#include "gps.h"

u16 last_time_gps=999; 
u8 is_need_update_time=0;
u8 flag_get_time = 0; // 0:获取时间失败 1:获取时间成功 2:未找到卫星时间不准

u8 customRTC_Init(void){
    RTC_InitTypeDef RTC_InitStructure;
    u16 retry=0X1FFF; 

    last_time_gps = 999;
    is_need_update_time = 0;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);//使能 PWR 时钟
    PWR_BackupAccessCmd(ENABLE); //使能后备寄存器访问
    if(RTC_ReadBackupRegister(RTC_BKP_DR0)!=0x5050)//是否第一次配置?
    {
        RCC_LSEConfig(RCC_LSE_ON);//LSE 开启 
        while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
        //检查指定的 RCC 标志位设置与否,等待低速晶振就绪
        { retry++;
        delay_ms(10);
        }
        if(retry==0)
            return 0; //LSE 开启失败. 
        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE); //选择 LSE 作为 RTC 时钟 
        RCC_RTCCLKCmd(ENABLE); //使能 RTC 时钟
        RTC_InitStructure.RTC_AsynchPrediv = 0x7F;//RTC 异步分频系数(1~0X7F)
        RTC_InitStructure.RTC_SynchPrediv = 0xFF;//RTC 同步分频系数(0~7FFF)
        RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24;//24 小时格式
        RTC_Init(&RTC_InitStructure);//初始化 RTC 参数
        RTC_Set_Time(00,00,00); //设置时间
        RTC_Set_Date(23,4,10,1); //设置日期
        RTC_WriteBackupRegister(RTC_BKP_DR0,0x5050);//标记已经初始化过了
    }
    return 1; //初始化成功

}

void RTC_Set_Time(u8 hour,u8 min,u8 sec){
    RTC_TimeTypeDef RTC_TimeStructure;
    RTC_TimeStructure.RTC_H12 = RTC_H12_AM;
    RTC_TimeStructure.RTC_Hours = hour;
    RTC_TimeStructure.RTC_Minutes = min;
    RTC_TimeStructure.RTC_Seconds = sec;
    RTC_SetTime(RTC_Format_BIN, &RTC_TimeStructure);
}

void RTC_Set_Date(u8 year,u8 month,u8 date,u8 week){
    RTC_DateTypeDef RTC_DateStructure;
    RTC_DateStructure.RTC_WeekDay = week;
    RTC_DateStructure.RTC_Month = month;
    RTC_DateStructure.RTC_Date = date;
    RTC_DateStructure.RTC_Year = year;
    RTC_SetDate(RTC_Format_BIN, &RTC_DateStructure);
}

void RTC_Get_Time(u8* time){
    RTC_TimeTypeDef RTC_TimeStructure;
    RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);
    time[0] = RTC_TimeStructure.RTC_Hours;
    time[1] = RTC_TimeStructure.RTC_Minutes;
    time[2] = RTC_TimeStructure.RTC_Seconds;
}


u8 RTC_update_device_time(void){

    u8 time[3];
    printf("last_time_gps=%d\r\n",last_time_gps);
    last_time_gps++;
    if(last_time_gps > 999)
        // 防止last_time_gps溢出,999即为从未完成过校时
        last_time_gps = 999;
    // 如果距离上次gps同步时间超过了12个周期，就重新同步一次时间
    if(last_time_gps >= 12)
    {
        printf("Try to get time from GPS!\r\n");
        // 0.获取时间
        flag_get_time = GPS_get_time(time);
        printf("flag_get_time=%d\r\n",flag_get_time);
        if (flag_get_time == 1)
        {
            printf("Get time success!\r\n");
        }
        else if (flag_get_time == 2)
        {
            printf("Get time fail, no satellite!\r\n");
            if(last_time_gps<999)
                // 如果没有卫星，且在设备运行过程中有过一次成功的时间同步，就不再同步时间
                return 0;
        }
        else
        {
            printf("Get time fail!\r\n");
            return 0;
        }
        // 1.设置时间
        RTC_Set_Time(time[0],time[1],time[2]);
        printf("Set time success!\r\n");
        if(flag_get_time == 1)
            last_time_gps = 0;
    }
    return 1;
}

// 检测是否曾经利用正确的GPS时间同步过时间
u8 RTC_check_device_time(void){
    if(last_time_gps == 999)
        return 0;
    else
        return 1;
}


