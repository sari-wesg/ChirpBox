
//**************************************************************************************************
//***** Includes ***********************************************************************************
#include "ds3231.h"

//**************************************************************************************************
//***** Global Variables ****************************************************************************
DS3231_TypeDef DS3231;
I2C_HandleTypeDef hi2c2;

//**************************************************************************************************
//***** Local (Private) Defines and Consts *********************************************************
const DS3231_Reg DS3231_memaddr = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18};

//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************
uint8_t DS3231_Buff[DS3231_REG_LENGTH];

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

// stm32
// msp
/**
* @brief I2C MSP Initialization
* This function configures the hardware resources used in this example
* @param hi2c: I2C handle pointer
* @retval None
*/
void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(hi2c->Instance==I2C2)
  {
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**I2C2 GPIO Configuration
    PB10     ------> I2C2_SCL
    PB11     ------> I2C2_SDA
    */
    GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Peripheral clock enable */
    __HAL_RCC_I2C2_CLK_ENABLE();

    /* I2C2 interrupt Init */
    HAL_NVIC_SetPriority(I2C2_EV_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C2_EV_IRQn);
    HAL_NVIC_SetPriority(I2C2_ER_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C2_ER_IRQn);
  }
}

/**
* @brief I2C MSP De-Initialization
* This function freeze the hardware resources used in this example
* @param hi2c: I2C handle pointer
* @retval None
*/
void HAL_I2C_MspDeInit(I2C_HandleTypeDef* hi2c)
{
  if(hi2c->Instance==I2C2)
  {
    /* Peripheral clock disable */
    __HAL_RCC_I2C2_CLK_DISABLE();

    /**I2C2 GPIO Configuration
    PB10     ------> I2C2_SCL
    PB11     ------> I2C2_SDA
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10|GPIO_PIN_11);

    /* I2C2 interrupt DeInit */
    HAL_NVIC_DisableIRQ(I2C2_EV_IRQn);
    HAL_NVIC_DisableIRQ(I2C2_ER_IRQn);
  }
}

// it
/**
  * @brief This function handles I2C2 event interrupt.
  */
void I2C2_EV_IRQHandler(void)
{
  HAL_I2C_EV_IRQHandler(&hi2c2);
}

/**
  * @brief This function handles I2C2 error interrupt.
  */
void I2C2_ER_IRQHandler(void)
{
  HAL_I2C_ER_IRQHandler(&hi2c2);
}

// init
/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
void MX_I2C2_Init(void)
{
  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  // I2C speed frequency 100 KHz; Rise time 100 ns; Fall time 100 ns; Coefficient of digital filter 0;
  hi2c2.Init.Timing = 0x10D19CE4;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    while(1);
  }
  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    while(1);
  }
  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
  {
    while(1);
  }
}

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
  while (HAL_I2C_Mem_Write(&hi2c2, DS3231_ADD, 0, I2C_MEMADD_SIZE_8BIT, DS3231_Buff, DS3231_TIME_LENGTH, 0x01) != HAL_OK)
    ;
}

/**
  * @brief  Give a command to get RTC time
  * @param  None
  * @retval None
  */
void DS3231_GetTime(void)
{
  DS3231.flag = 0;
  HAL_I2C_Mem_Read_IT(&hi2c2, DS3231_ADD, 0, I2C_MEMADD_SIZE_8BIT, DS3231_Buff, DS3231_REG_LENGTH);
}

Chirp_Time DS3231_ShowTime()
{
  char buffer[50], buff[20];
  Chirp_Time RTC_Time;
  memset(&RTC_Time, 0, sizeof(RTC_Time));
  uint16_t count = 0;

  while (DS3231.flag == 0)
  {
    count++;
    assert_reset((count < 0xFFFF));
  }
  DS3231.flag = 0;
  switch (DS3231.Day)
  {
  case 1:
    sprintf(buff, "MONDAY");
    break;
  case 2:
    sprintf(buff, "TUESDAY");
    break;
  case 3:
    sprintf(buff, "WEDNESDAY");
    break;
  case 4:
    sprintf(buff, "THURSDAY");
    break;
  case 5:
    sprintf(buff, "FRIDAY");
    break;
  case 6:
    sprintf(buff, "SATURDAY");
    break;
  case 7:
    sprintf(buff, "SUNDAY");
    break;
  }
  sprintf(buffer, "%02d:%02d:%02d %s %02d:%02d:%02d\r\n", 2000 + DS3231.Year,
          DS3231.Month, DS3231.Date, buff, DS3231.Hour, DS3231.Minute, DS3231.Second);
  printf("%s", buffer);
  RTC_Time.chirp_year = 2000 + DS3231.Year;
  RTC_Time.chirp_month = DS3231.Month;
  RTC_Time.chirp_date = DS3231.Date;
  RTC_Time.chirp_day = DS3231.Day;
  RTC_Time.chirp_hour = DS3231.Hour;
  RTC_Time.chirp_min = DS3231.Minute;
  RTC_Time.chirp_sec = DS3231.Second;
  return RTC_Time;
}

void DS3231_ClearAlarm1_Time()
{
  uint8_t alarm_flag = 0;
  uint8_t count = 0;
  while (!alarm_flag)
  {
    count++;
    assert_reset((count < 10));
    printf("clear alarm\n");
    /* read control and status */
    while (HAL_I2C_Mem_Read(&hi2c2, DS3231_ADD, DS3231_memaddr.control, I2C_MEMADD_SIZE_8BIT,
                            &(DS3231.Control), 2, 0xffff) != HAL_OK);
    // Clear the AF1 and AF2 in Status (0Fh)
    DS3231.Control &= 0xFC;
    DS3231.Status &= 0xFC;
    while (HAL_I2C_Mem_Write(&hi2c2, DS3231_ADD, DS3231_memaddr.control, I2C_MEMADD_SIZE_8BIT,
                            &(DS3231.Control), 2, 0xffff) != HAL_OK)
      ;
    /* read alarm Status */
    while (HAL_I2C_Mem_Read(&hi2c2, DS3231_ADD, DS3231_memaddr.control, I2C_MEMADD_SIZE_8BIT,
                            &(DS3231.Control), 2, 0xffff) != HAL_OK);
    if ((!(DS3231.Control & 0x03)) && (!(DS3231.Status & 0x03)))
      alarm_flag = 1;
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
  uint8_t alarm_flag = 0;
  uint8_t count = 0;
  while (!alarm_flag)
  {
    count++;
    assert_reset((count < 10));
    printf("set alarm\n");
    /* write alarm time */
    DS3231_Buff[DS3231_memaddr.alarm1_dydt] = DEC2BCD(date);
    DS3231_Buff[DS3231_memaddr.alarm1_hour] = DEC2BCD(hour);
    DS3231_Buff[DS3231_memaddr.alarm1_min] = DEC2BCD(mintue);
    DS3231_Buff[DS3231_memaddr.alarm1_sec] = DEC2BCD(second);
    while (HAL_I2C_Mem_Write(&hi2c2, DS3231_ADD, DS3231_memaddr.alarm1_sec, I2C_MEMADD_SIZE_8BIT,
                            &(DS3231_Buff[DS3231_memaddr.alarm1_sec]), DS3231_ALARM1_LENGTH, 0xffff) != HAL_OK)
      ;
    /* read alarm time */
    while (HAL_I2C_Mem_Read(&hi2c2, DS3231_ADD, DS3231_memaddr.alarm1_sec, I2C_MEMADD_SIZE_8BIT,
                            &(DS3231_Buff[DS3231_memaddr.alarm1_sec]), DS3231_ALARM1_LENGTH, 0xffff) != HAL_OK);
    /* if alarm time set right, flag as 1 */
    if ((BCD2DEC(DS3231_Buff[DS3231_memaddr.alarm1_dydt]) == date) && (BCD2DEC(DS3231_Buff[DS3231_memaddr.alarm1_hour]) == hour) && (BCD2DEC(DS3231_Buff[DS3231_memaddr.alarm1_min]) == mintue) && (BCD2DEC(DS3231_Buff[DS3231_memaddr.alarm1_sec]) == second))
      alarm_flag = 1;
  }
  alarm_flag = 0;
  while (!alarm_flag)
  {
    printf("Enable alarm\n");
    // Enable the A1IE and INTCN in Control (0Eh)
    DS3231.Control |= 0x05;
    // Clear the AF1 and AF2 in Status (0Fh)
    DS3231.Status &= 0xFC;
    while (HAL_I2C_Mem_Write(&hi2c2, DS3231_ADD, DS3231_memaddr.control, I2C_MEMADD_SIZE_8BIT,
                            &(DS3231.Control), 2, 0xffff) != HAL_OK)
      ;
    /* read alarm enable */
    while (HAL_I2C_Mem_Read(&hi2c2, DS3231_ADD, DS3231_memaddr.control, I2C_MEMADD_SIZE_8BIT,
                            &(DS3231.Control), 2, 0xffff) != HAL_OK);
    if ((DS3231.Control & 0x05) && (!(DS3231.Status & 0x03)))
      alarm_flag = 1;
  }
}

void DS3231_GetAlarm1_Time()
{
  uint8_t date = 0;
  uint8_t hour = 0;
  uint8_t mintue = 0;
  uint8_t second = 0;
  uint8_t alarm_flag = 0;
  uint8_t count = 0;
  while (!alarm_flag)
  {
    count++;
    assert_reset((count < 10));
    printf("get alarm\n");
    /* read alarm time */
    while (HAL_I2C_Mem_Read(&hi2c2, DS3231_ADD, DS3231_memaddr.alarm1_sec, I2C_MEMADD_SIZE_8BIT,
                            &(DS3231_Buff[DS3231_memaddr.alarm1_sec]), DS3231_ALARM1_LENGTH, 0xffff) != HAL_OK);
    date = BCD2DEC(DS3231_Buff[DS3231_memaddr.alarm1_dydt]);
    hour = BCD2DEC(DS3231_Buff[DS3231_memaddr.alarm1_hour]);
    mintue = BCD2DEC(DS3231_Buff[DS3231_memaddr.alarm1_min]);
    second = BCD2DEC(DS3231_Buff[DS3231_memaddr.alarm1_sec]);
    alarm_flag = 1;
  }
  char buffer[50];
  sprintf(buffer, "%02d %02d:%02d:%02d\r\n", date, hour, mintue, second);
  printf("%s", buffer);
  while (HAL_I2C_Mem_Read(&hi2c2, DS3231_ADD, DS3231_memaddr.control, I2C_MEMADD_SIZE_8BIT,
                          &(DS3231.Control), 2, 0xffff) != HAL_OK);
  uint8_t i = ((DS3231.Control & 0x05) && (!(DS3231.Status & 0x03)));
  printf("true:%d\n", i);
}

Chirp_Time api_obtain_rtc_time()
{
    DS3231_GetTime();
    Chirp_Time time = DS3231_ShowTime();
    return time;
}

//**************************************************************************************************

/**
  * @brief  IRQ callback of received I2C via HAL_I2C_Mem_Read_IT
  * @param  hi2c
  * @retval None
  */
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
  if (hi2c->Instance == hi2c2.Instance)
  {
    DS3231.flag = 1;
    DS3231.Second = BCD2DEC(DS3231_Buff[DS3231_memaddr.sec]);
    DS3231.Minute = BCD2DEC(DS3231_Buff[DS3231_memaddr.min]);
    DS3231.Hour = BCD2DEC(DS3231_Buff[DS3231_memaddr.hour]);
    DS3231.Day = BCD2DEC(DS3231_Buff[DS3231_memaddr.day]);
    DS3231.Date = BCD2DEC(DS3231_Buff[DS3231_memaddr.date]);
    DS3231.Month = BCD2DEC(DS3231_Buff[DS3231_memaddr.month]);
    DS3231.Year = BCD2DEC(DS3231_Buff[DS3231_memaddr.year]);
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
  if (hi2c->Instance == hi2c2.Instance)
  {
  }
}
