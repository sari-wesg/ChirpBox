
#ifndef __PLATFORM_H__
#define __PLATFORM_H__

//**************************************************************************************************
//***** Includes ***********************************************************************************

#include "stm32l4xx_hal.h"

//**************************************************************************************************
//***** Global (Public) Defines and Consts *********************************************************

#define USART_TX_Pin GPIO_PIN_2
#define USART_TX_GPIO_Port GPIOA
#define USART_RX_Pin GPIO_PIN_3
#define USART_RX_GPIO_Port GPIOA


//**************************************************************************************************
//***** Prototypes of Global Functions *************************************************************

void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_USART2_UART_Init(void);
void MX_CRC_Init(void);
void Error_Handler(void);

#endif  /* __PLATFORM_H__ */
