#include "spi.h"
/*---------------------------------------------------------------------------*/
SPI_HandleTypeDef hspi1;
/*---------------------------------------------------------------------------*/
void spi_init(void)
{
    hspi1.Instance = SPI1;

//	hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    hspi1.Init.BaudRatePrescaler = SpiFrequency( 10000000 );
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;


//  hspi1.Init.CRCPolynomial = 7;
//  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
//  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;

  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

uint16_t HW_SPI_InOut( uint16_t txData )
{
  uint16_t rxData ;

  HAL_SPI_TransmitReceive( &hspi1, ( uint8_t * ) &txData, ( uint8_t* ) &rxData, 1, HAL_MAX_DELAY);

  return rxData;
}

uint32_t SpiFrequency( uint32_t hz )
{
  uint32_t divisor = 0;
  uint32_t SysClkTmp = SystemCoreClock;
  uint32_t baudRate;

  while( SysClkTmp > hz)
  {
    divisor++;
    SysClkTmp= ( SysClkTmp >> 1);

    if (divisor >= 7)
      break;
  }

  baudRate =((( divisor & 0x4 ) == 0 )? 0x0 : SPI_CR1_BR_2  )|
            ((( divisor & 0x2 ) == 0 )? 0x0 : SPI_CR1_BR_1  )|
            ((( divisor & 0x1 ) == 0 )? 0x0 : SPI_CR1_BR_0  );

  return baudRate;
}
