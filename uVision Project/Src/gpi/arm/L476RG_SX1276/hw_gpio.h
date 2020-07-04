#ifndef __HW_GPIO_H__
#define __HW_GPIO_H__
#include "stm32l4xx_hal.h"
#include "gpi/platform.h"
/*---------------------------------------------------------------------------*/
#define RCC_GPIO_CLK_ENABLE( __GPIO_PORT__ )              \
do {                                                    \
    switch( __GPIO_PORT__)                                \
    {                                                     \
      case GPIOA_BASE: __HAL_RCC_GPIOA_CLK_ENABLE(); break;    \
      case GPIOB_BASE: __HAL_RCC_GPIOB_CLK_ENABLE(); break;    \
      case GPIOC_BASE: __HAL_RCC_GPIOC_CLK_ENABLE(); break;    \
      case GPIOD_BASE: __HAL_RCC_GPIOD_CLK_ENABLE(); break;    \
      case GPIOH_BASE: default:  __HAL_RCC_GPIOH_CLK_ENABLE(); \
    }                                                    \
  } while(0)  

typedef void( GpioIrqHandler )();
/*functions---------------------------------------------------------------------------*/
void MX_GPIO_Init(void);
IRQn_Type MSP_GetIRQn( uint16_t GPIO_Pin);
void HW_GPIO_Init( GPIO_TypeDef* port, uint16_t GPIO_Pin, GPIO_InitTypeDef* initStruct);
void HW_GPIO_SetIrq( GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, uint32_t prio,  GpioIrqHandler *irqHandler );
void HW_GPIO_IrqHandler( uint16_t GPIO_Pin );
void HW_GPIO_Write( GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin,  uint32_t value );
uint32_t HW_GPIO_Read( GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin );
uint8_t HW_GPIO_GetBitPos(uint16_t GPIO_Pin);
/*LPTIM---------------------------------------------------------------------------*/
uint32_t LPTIM_ms2Tick( uint32_t ms );
uint32_t Get_Current_LPTIM_Tick( void );
uint32_t ElapsedTick( uint32_t past );


#endif /* __HW_GPIO_H__ */
