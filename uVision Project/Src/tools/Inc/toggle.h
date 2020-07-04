#ifndef __TOGGLE_H
#define __TOGGLE_H
#include "stdint.h"
#ifdef __cplusplus
    extern "C" {
#endif
/* Includes ------------------------------------------------------------------*/

#define FLAG_WRT_ERR            0x0F
#define FLAG_WRT_OK             0x00
#define BANK_TOGGLE_ERR         0x1F
#define BANK_TOGGLE_OK          0x10
#define USR_FLASH_START         0x0807F800
#define USR_FLASH_END           0x08080000
#define USR_PAGE                255
#define MAX_LEN            	    2048
#define FLASH_WT_OUT_OF_RANGE	0xFF

uint8_t TOGGLE_RESET_EXTI_CALLBACK(void) __attribute__((section(".ARM.__at_0x08000534")));
uint8_t USR_FLASH_PageErase(void);
uint8_t USR_FLASH_Program8(uint32_t faddr,uint8_t* pData, uint16_t DataLen);
uint8_t USR_FLASH_Read8(uint32_t faddr);

uint32_t Bank1_WRP(uint32_t strtA_offset, uint32_t endA_offset);
uint32_t Bank1_nWRP(void);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __TOGGLE_H */
