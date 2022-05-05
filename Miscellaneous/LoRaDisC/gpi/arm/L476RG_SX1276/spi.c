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

    SPI_CLK_ENABLE();

//  hspi1.Init.CRCPolynomial = 7;
//  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
//  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;

  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }

  /*##-2- Configure the SPI GPIOs */
  HW_SPI_IoInit();
}

/*!
 * @brief De-initializes the SPI object and MCU peripheral
 *
 * @param [IN] none
 */
void HW_SPI_DeInit(void)
{

  HAL_SPI_DeInit(&hspi1);

  /*##-1- Reset peripherals ####*/
  __HAL_RCC_SPI1_FORCE_RESET();
  __HAL_RCC_SPI1_RELEASE_RESET();
  /*##-2- Configure the SPI GPIOs */
  HW_SPI_IoDeInit();
}

void HW_SPI_IoInit(void)
{
  GPIO_InitTypeDef initStruct = {0};


  initStruct.Mode = GPIO_MODE_AF_PP;
  initStruct.Pull = GPIO_NOPULL  ;
  initStruct.Speed = GPIO_SPEED_HIGH;
  initStruct.Alternate = SPI1_AF ;

  HW_GPIO_Init(RADIO_SCLK_PORT, RADIO_SCLK_PIN, &initStruct);
  HW_GPIO_Init(RADIO_MISO_PORT, RADIO_MISO_PIN, &initStruct);
  HW_GPIO_Init(RADIO_MOSI_PORT, RADIO_MOSI_PIN, &initStruct);

  initStruct.Mode = GPIO_MODE_OUTPUT_PP;
  initStruct.Pull = GPIO_NOPULL;

  HW_GPIO_Init(RADIO_NSS_PORT, RADIO_NSS_PIN, &initStruct);

  HW_GPIO_Write(RADIO_NSS_PORT, RADIO_NSS_PIN, 1);
}

void HW_SPI_IoDeInit(void)
{
  GPIO_InitTypeDef initStruct = {0};

  initStruct.Mode = GPIO_MODE_OUTPUT_PP;

  initStruct.Pull = GPIO_NOPULL  ;
  HW_GPIO_Init(RADIO_MOSI_PORT, RADIO_MOSI_PIN, &initStruct);
  HW_GPIO_Write(RADIO_MOSI_PORT, RADIO_MOSI_PIN, 0);

  initStruct.Pull = GPIO_PULLDOWN;
  HW_GPIO_Init(RADIO_MISO_PORT, RADIO_MISO_PIN, &initStruct);
  HW_GPIO_Write(RADIO_MISO_PORT, RADIO_MISO_PIN, 0);

  initStruct.Pull = GPIO_NOPULL  ;
  HW_GPIO_Init(RADIO_SCLK_PORT, RADIO_SCLK_PIN, &initStruct);
  HW_GPIO_Write(RADIO_SCLK_PORT, RADIO_SCLK_PIN, 0);

  initStruct.Pull = GPIO_NOPULL  ;
  HW_GPIO_Init(RADIO_NSS_PORT, RADIO_NSS_PIN, &initStruct);
  HW_GPIO_Write(RADIO_NSS_PORT, RADIO_NSS_PIN, 1);
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
