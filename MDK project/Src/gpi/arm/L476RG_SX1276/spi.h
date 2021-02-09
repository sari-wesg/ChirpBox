#ifndef __SPI_H__
#define __SPI_H__

#include "stm32l4xx_hal.h"

void spi_init(void);
uint16_t HW_SPI_InOut( uint16_t txData );
uint32_t SpiFrequency( uint32_t hz );

#endif /* __SPI_H__ */
