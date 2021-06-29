#ifndef __GPS_H__
#define __GPS_H__

//**************************************************************************************************
//***** Includes ***********************************************************************************
#include "stm32l4xx.h"
#include "API_ChirpBox.h"
#include <time.h>

//**************************************************************************************************
//***** Global Typedefs and Class Declarations *****************************************************

//**************************************************************************************************
//***** Global (Public) Defines and Consts *********************************************************
#define GPS_TRIGGER_Pin GPIO_PIN_2
#define GPS_TRIGGER_Port GPIOC
#define GPS_PPS_Pin GPIO_PIN_0
#define GPS_PPS_Port GPIOC

//**************************************************************************************************
//***** Global Variables ***************************************************************************


//**************************************************************************************************
//***** Prototypes of Global Functions *************************************************************
void GPS_Init();
void GPS_On();
void GPS_Off();
void GPS_Uart_Irq();
Chirp_Time GPS_Get_Time();
time_t GPS_Diff(Chirp_Time *gps_time, uint16_t start_year, uint8_t start_month, uint8_t start_date, uint8_t start_hour, uint8_t start_min, uint8_t start_sec);
void GPS_Waiting(uint16_t start_year, uint8_t start_month, uint8_t start_date, uint8_t start_hour, uint8_t start_min, uint8_t start_sec);
void GPS_Waiting_PPS(uint32_t PPS_wait);
void GPS_Wakeup(uint32_t interval_sec);
void MX_USART3_UART_Init(void);

#endif /* __GPS_H__ */
