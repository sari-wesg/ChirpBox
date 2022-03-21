#ifndef __SPI_H__
#define __SPI_H__

#include "stm32l4xx_hal.h"
#include "stm32l4xx_hw_conf.h"


void spi_init(void);
/*!
 * @brief De-initializes the SPI object and MCU peripheral
 *
 * @param [IN] none
 */
void HW_SPI_DeInit(void);

/*!
 * @brief Initializes the SPI IOs
 *
 * @param [IN] none
 */
void HW_SPI_IoInit(void);

/*!
 * @brief De-initializes the SPI IOs
 *
 * @param [IN] none
 */
void HW_SPI_IoDeInit(void);

/*!
 * @brief Sends outData and receives inData
 *
 * @param [IN] outData Byte to be sent
 * @retval inData      Received byte.
 */
uint16_t HW_SPI_InOut( uint16_t txData );
uint32_t SpiFrequency( uint32_t hz );

#endif /* __SPI_H__ */
