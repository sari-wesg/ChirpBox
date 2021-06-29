#ifndef __RTC_H__
#define __RTC_H__

//**************************************************************************************************
//***** Includes ***********************************************************************************
#include "stm32l4xx.h"
#include "API_ChirpBox.h"

//**************************************************************************************************
//***** Global Typedefs and Class Declarations *****************************************************

typedef struct DS3231_TypeDef_tag
{
	uint8_t Year;
	uint8_t Month;
	uint8_t Day;
	uint8_t Date;
	uint8_t Hour;
	uint8_t Minute;
	uint8_t Second;
	uint8_t TempMSB, TempLSB;
	uint8_t Control;
	uint8_t Status;
	float Temp;
	volatile uint8_t flag;
	volatile uint8_t alarm1_flag;
} DS3231_TypeDef;

typedef struct DS3231_Reg_tag
{
	uint8_t sec;
	uint8_t min;
	uint8_t hour;
	uint8_t day;
	uint8_t date;
	uint8_t month;
	uint8_t year;
	uint8_t alarm1_sec;
	uint8_t alarm1_min;
	uint8_t alarm1_hour;
	uint8_t alarm1_dydt;
	uint8_t alarm2_min;
	uint8_t alarm2_hour;
	uint8_t alarm2_dydt;
	uint8_t control;
	uint8_t status;
	uint8_t offset;
	uint8_t temp_m;
	uint8_t temp_l;
} DS3231_Reg;

//**************************************************************************************************
//***** Global (Public) Defines and Consts *********************************************************
#define DS3231_TIME_LENGTH 0x07
#define DS3231_ALARM1_LENGTH 0x04
#define DS3231_ALARM2_LENGTH 0x03
#define DS3231_REG_LENGTH 0x13
#define DS3231_ADD 0xD0

//**************************************************************************************************
//***** Global Variables ***************************************************************************
extern I2C_HandleTypeDef hi2c2;
extern DS3231_TypeDef DS3231;

//**************************************************************************************************
//***** Prototypes of Global Functions *************************************************************
void DS3231_ModifyTime(uint8_t year, uint8_t month, uint8_t date,
					   uint8_t day, uint8_t hour, uint8_t mintue, uint8_t second);
void DS3231_GetTime(void);
Chirp_Time DS3231_ShowTime();
void DS3231_ClearAlarm1_Time();
void DS3231_SetAlarm1_Time(uint8_t date, uint8_t hour, uint8_t mintue, uint8_t second);
void DS3231_GetAlarm1_Time();
void MX_I2C2_Init(void);

#endif /* __RTC_H__ */
