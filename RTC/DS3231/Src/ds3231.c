
//**************************************************************************************************
//***** Includes ***********************************************************************************
#include "ds3231.h"
#include "iic.h"

//**************************************************************************************************
//***** Global Variables ****************************************************************************
extern I2C_HandleTypeDef hi2c2;

//**************************************************************************************************
//***** Local (Private) Defines and Consts *********************************************************
const DS3231_Reg DS3231_memaddr = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18};


//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************
uint8_t DS3231_Buff[DS3231_REG_LENGTH];
DS3231_TypeDef DS3231;

//**************************************************************************************************
//***** Local Functions ****************************************************************************
/**
  * @brief  Convert a BCD to a DEC
  * @param  temp: The BCD to be converted
  * @retval The converted DEC
  */
uint8_t BCD2DEC(uint8_t temp)
{
	return (temp - 6 * (temp >> 4));
}

/**
  * @brief  Convert a DEC to a BCD
  * @param  temp: The DEC to be converted
  * @retval The converted BCD
  */
uint8_t DEC2BCD(uint8_t temp)
{
	return (temp + 6 * (temp / 10));
}

//**************************************************************************************************
//***** Global Functions ***************************************************************************

/**
  * @brief  Modify the time of timing register
  * @param  year: 00–99, the year added with 2000 is the actual year
  * @param  month: 01–12
  * @param  date: 01–31
  * @param  day: 1–7, the week serial number: Monday, Tuesday, Wednesday...
  * @param  hour: 00–23 (default)
  * @param  mintue: 00–59
  * @param  second: 00–59
  * @retval None
  */
void DS3231_ModifyTime(uint8_t year, uint8_t month, uint8_t date,
                        uint8_t day, uint8_t hour, uint8_t mintue, uint8_t second)
{
    uint8_t DS3231_Databuff[DS3231_TIME_LENGTH];
    uint8_t i;
    DS3231_Databuff[DS3231_memaddr.year] = year;
    DS3231_Databuff[DS3231_memaddr.month] = month;
    DS3231_Databuff[DS3231_memaddr.date] = date;
    DS3231_Databuff[DS3231_memaddr.day] = day;
    DS3231_Databuff[DS3231_memaddr.hour] = hour; //default modem is 24 hour
    DS3231_Databuff[DS3231_memaddr.min] = mintue;
    DS3231_Databuff[DS3231_memaddr.sec] = second;
    for (i = 0; i < DS3231_TIME_LENGTH; i++)
        DS3231_Buff[i] = DEC2BCD(DS3231_Databuff[i]);
    while(HAL_I2C_Mem_Write(&hi2c2, DS3231_ADD, 0, I2C_MEMADD_SIZE_8BIT, DS3231_Buff, DS3231_TIME_LENGTH, 0x01) != HAL_OK);
}

/**
  * @brief  Give a command to get RTC time
  * @param  None
  * @retval None
  */
void DS3231_GetTime(void)
{
    HAL_I2C_Mem_Read_IT(&hi2c2, DS3231_ADD, 0, I2C_MEMADD_SIZE_8BIT, DS3231_Buff, DS3231_REG_LENGTH);
}

void DS3231_ShowTime(void)
{
	char buffer[50], buff[20];

	if(DS3231.flag == 1)
	{
		DS3231.flag = 0;
		switch(DS3231.Day)
		{
			case 1: sprintf(buff,"MONDAY");break;
			case 2: sprintf(buff,"TUESDAY");break;
			case 3: sprintf(buff,"WEDNESDAY");break;
			case 4: sprintf(buff,"THURSDAY");break;
			case 5: sprintf(buff,"FRIDAY");break;
			case 6: sprintf(buff,"SATURDAY");break;
			case 7: sprintf(buff,"SUNDAY");break;
		}
		sprintf(buffer, "%02d:%02d:%02d %s %02d:%02d:%02d\r\n",2000 + DS3231.Year,\
                DS3231.Month, DS3231.Date, buff, DS3231.Hour, DS3231.Minute, DS3231.Second);
		printf("%s", buffer);
  }
}

/**
  * @brief  Set the alarm1 time and enable the alarm
  * @param  date: 01–31
  * @param  hour: 00–23 (default)
  * @param  mintue: 00–59
  * @param  second: 00–59
  * @retval None
  */
void DS3231_SetAlarm1_Time(uint8_t date, uint8_t hour, uint8_t mintue, uint8_t second)
{
    DS3231_Buff[DS3231_memaddr.alarm1_dydt] = DEC2BCD(date);
    DS3231_Buff[DS3231_memaddr.alarm1_hour] = DEC2BCD(hour);
    DS3231_Buff[DS3231_memaddr.alarm1_min] = DEC2BCD(mintue);
    DS3231_Buff[DS3231_memaddr.alarm1_sec] = DEC2BCD(second);
    while(HAL_I2C_Mem_Write(&hi2c2, DS3231_ADD, DS3231_memaddr.alarm1_sec, I2C_MEMADD_SIZE_8BIT,
                    &(DS3231_Buff[DS3231_memaddr.alarm1_sec]), DS3231_ALARM1_LENGTH, 0xffff) != HAL_OK);
    // Enable the A1IE and INTCN in Control (0Eh)
    DS3231.Control |= 0x05;
    // Clear the AF1 and AF2 in Status (0Fh)
    DS3231.Status &= 0xFC;
    while(HAL_I2C_Mem_Write(&hi2c2, DS3231_ADD, DS3231_memaddr.control, I2C_MEMADD_SIZE_8BIT,
                      &(DS3231.Control), 2, 0xffff) != HAL_OK);
}

/**
  * @brief  Set the alarm2 time and enable the alarm
  * @param  date: 01–31
  * @param  hour: 00–23 (default)
  * @param  mintue: 00–59
  * @retval None
  */
void DS3231_SetAlarm2_Time(uint8_t date, uint8_t hour, uint8_t mintue)
{
    DS3231_Buff[DS3231_memaddr.alarm2_dydt] = DEC2BCD(date);
    DS3231_Buff[DS3231_memaddr.alarm2_hour] = DEC2BCD(hour);
    DS3231_Buff[DS3231_memaddr.alarm2_min] = DEC2BCD(mintue);
    while(HAL_I2C_Mem_Write(&hi2c2, DS3231_ADD, DS3231_memaddr.alarm2_min, I2C_MEMADD_SIZE_8BIT,
                    &(DS3231_Buff[DS3231_memaddr.alarm2_min]), DS3231_ALARM2_LENGTH, 0xffff) != HAL_OK);
    // Enable the A1IE and INTCN in Control (0Eh)
    DS3231.Control |= 0x06;
    // Clear the AF1 and AF2 in Status (0Fh)
    DS3231.Status &= 0xFC;
    while(HAL_I2C_Mem_Write(&hi2c2, DS3231_ADD, DS3231_memaddr.control, I2C_MEMADD_SIZE_8BIT,
                      &(DS3231.Control), 2, 0xffff) != HAL_OK);
}
//**************************************************************************************************

/**
  * @brief  IRQ callback of received I2C via HAL_I2C_Mem_Read_IT
  * @param  hi2c
  * @retval None
  */
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	if(hi2c->Instance == hi2c2.Instance)
	{
		DS3231.flag    = 1;
		DS3231.Second  = BCD2DEC(DS3231_Buff[DS3231_memaddr.sec]);
		DS3231.Minute  = BCD2DEC(DS3231_Buff[DS3231_memaddr.min]);
		DS3231.Hour    = BCD2DEC(DS3231_Buff[DS3231_memaddr.hour]);
		DS3231.Day     = BCD2DEC(DS3231_Buff[DS3231_memaddr.day]);
		DS3231.Date    = BCD2DEC(DS3231_Buff[DS3231_memaddr.date]);
		DS3231.Month   = BCD2DEC(DS3231_Buff[DS3231_memaddr.month]);
		DS3231.Year    = BCD2DEC(DS3231_Buff[DS3231_memaddr.year]);
		DS3231.Control = DS3231_Buff[DS3231_memaddr.control];
		DS3231.Status = DS3231_Buff[DS3231_memaddr.status];
	}
}

/**
  * @brief  IRQ callback of transmitted I2C via HAL_I2C_Mem_Write_IT
  * @param  hi2c
  * @retval None
  */
void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	if(hi2c->Instance == hi2c2.Instance)
	{
  }
}
