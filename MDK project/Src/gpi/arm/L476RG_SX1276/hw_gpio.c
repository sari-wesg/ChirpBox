#include "hw_gpio.h"
/*---------------------------------------------------------------------------*/
void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  // ---------------config all gpio as reset---------------
  /* except PA14 (SWCLK) and PA13 (SWDIO) */
  HAL_GPIO_WritePin(GPIOA, 0x9FFF, GPIO_PIN_RESET);
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  HW_GPIO_Init(GPIOA, 0x9FFF, &GPIO_InitStruct );
  HAL_GPIO_WritePin(GPIOA, 0x9FFF, GPIO_PIN_RESET);

  HAL_GPIO_WritePin(GPIOB, 0xffff, GPIO_PIN_RESET);
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  HW_GPIO_Init(GPIOB, 0xffff, &GPIO_InitStruct );
  HAL_GPIO_WritePin(GPIOB, 0xffff, GPIO_PIN_RESET);

  HAL_GPIO_WritePin(GPIOC, 0xffff, GPIO_PIN_RESET);
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  HW_GPIO_Init(GPIOC, 0xffff, &GPIO_InitStruct );
  HAL_GPIO_WritePin(GPIOC, 0xffff, GPIO_PIN_RESET);

  /*Configure LED pin Output Level */
  HAL_GPIO_WritePin(LED_GPIO_Port, LED1|LED2|LED3|LED4|LED5|LED6, GPIO_PIN_RESET);
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  HW_GPIO_Init(LED_GPIO_Port, LED1|LED2|LED3|LED4|LED5|LED6, &GPIO_InitStruct );

  // TODO: no need to config?
  /*Configure RADIO_DIO_3_PIN pin Output Level */
  HAL_GPIO_WritePin(RADIO_DIO_3_PORT, RADIO_DIO_3_PIN, GPIO_PIN_RESET);
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  HW_GPIO_Init(RADIO_DIO_3_PORT, RADIO_DIO_3_PIN, &GPIO_InitStruct );
}

static GpioIrqHandler *GpioIrq[16] = { NULL };

IRQn_Type MSP_GetIRQn( uint16_t GPIO_Pin)
{
  switch( GPIO_Pin )
  {
    case GPIO_PIN_0:  return EXTI0_IRQn;
    case GPIO_PIN_1:  return EXTI1_IRQn;
    case GPIO_PIN_2:  return EXTI2_IRQn;
    case GPIO_PIN_3:  return EXTI3_IRQn;
    case GPIO_PIN_4:  return EXTI4_IRQn;
    case GPIO_PIN_5:
    case GPIO_PIN_6:
    case GPIO_PIN_7:
    case GPIO_PIN_8:
    case GPIO_PIN_9:  return EXTI9_5_IRQn;
    case GPIO_PIN_10:
    case GPIO_PIN_11:
    case GPIO_PIN_12:
    case GPIO_PIN_13:
    case GPIO_PIN_14:
    case GPIO_PIN_15:
	default: return EXTI15_10_IRQn;
  }
}

void HW_GPIO_Init( GPIO_TypeDef* port, uint16_t GPIO_Pin, GPIO_InitTypeDef* initStruct)
{
  RCC_GPIO_CLK_ENABLE(  (uint32_t) port);

  initStruct->Pin = GPIO_Pin ;

  HAL_GPIO_Init( port, initStruct );
}

void HW_GPIO_SetIrq( GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, uint32_t prio, GpioIrqHandler *irqHandler )
{
  IRQn_Type IRQnb;

  uint32_t BitPos = HW_GPIO_GetBitPos( GPIO_Pin ) ;

  if ( irqHandler != NULL)
  {
    GpioIrq[ BitPos ] = irqHandler;

    IRQnb = MSP_GetIRQn( GPIO_Pin );

    HAL_NVIC_SetPriority( IRQnb , prio, 0);

    HAL_NVIC_EnableIRQ( IRQnb );
  }
  else
  {
    GpioIrq[ BitPos ] = NULL;
  }
}

void HW_GPIO_IrqHandler( uint16_t GPIO_Pin )
{
  uint32_t BitPos = HW_GPIO_GetBitPos( GPIO_Pin );

  if ( GpioIrq[ BitPos ]  != NULL)
  {
    GpioIrq[ BitPos ] ( NULL );
  }
}

void HW_GPIO_Write( GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin,  uint32_t value )
{
  HAL_GPIO_WritePin( GPIOx, GPIO_Pin , (GPIO_PinState) value );
}

uint32_t HW_GPIO_Read( GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin )
{
  return HAL_GPIO_ReadPin( GPIOx, GPIO_Pin);
}

uint8_t HW_GPIO_GetBitPos(uint16_t GPIO_Pin)
{
  uint8_t PinPos=0;

  if ( ( GPIO_Pin & 0xFF00 ) != 0) { PinPos |= 0x8; }
  if ( ( GPIO_Pin & 0xF0F0 ) != 0) { PinPos |= 0x4; }
  if ( ( GPIO_Pin & 0xCCCC ) != 0) { PinPos |= 0x2; }
  if ( ( GPIO_Pin & 0xAAAA ) != 0) { PinPos |= 0x1; }

  return PinPos;
}
/*LPTIM---------------------------------------------------------------------------*/
extern LPTIM_HandleTypeDef hlptim1;
#define LPTIM_second            32768

uint32_t LPTIM_ms2Tick( uint32_t ms )
{
  return (ms * LPTIM_second / 1000);
}

uint32_t Get_Current_LPTIM_Tick( void )
{
  return HAL_LPTIM_ReadCounter(&hlptim1);
}

uint32_t ElapsedTick( uint32_t past )
{
  uint32_t now = Get_Current_LPTIM_Tick();
  if (now < past)
    return (now - past + LPTIM_second);
  else
    return (now - past);
}
