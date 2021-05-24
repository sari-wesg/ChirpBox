/**
  ******************************************************************************
  * @file    STM32L476G_EVAL/DualBank/inc/flash_if.h
  * @author  MCD Application Team
  * @version 1.0.0
  * @date    15-September-2016
  * @brief   This file provides all the headers of the flash_if functions.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2016 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

#ifndef __FLASH_IF_H__
#define __FLASH_IF_H__

//**************************************************************************************************
//***** Includes ***********************************************************************************

#include "stm32l4xx_hal.h"

//**************************************************************************************************
//***** Global Typedefs and Class Declarations *****************************************************

/* Error code */
enum
{
  FLASHIF_OK = 0,
  FLASHIF_ERASEKO,
  FLASHIF_WRITINGCTRL_ERROR,
  FLASHIF_WRITING_ERROR,
  FLASHIF_CRCKO,
  FLASHIF_RECORD_ERROR,
  FLASHIF_EMPTY,
  FLASHIF_PROTECTION_ERRROR
};

enum{
  FLASHIF_PROTECTION_NONE         = 0,
  FLASHIF_PROTECTION_PCROPENABLED = 0x1,
  FLASHIF_PROTECTION_WRPENABLED   = 0x2,
  FLASHIF_PROTECTION_RDPENABLED   = 0x4,
};

/* protection update */
enum {
  FLASHIF_WRP_ENABLE,
  FLASHIF_WRP_DISABLE
};

//**************************************************************************************************
//***** Global (Public) Defines and Consts *********************************************************

/* Notable Flash addresses */
#define FLASH_START_BANK1             ((uint32_t)0x08000000)
#define FLASH_START_BANK2             ((uint32_t)0x08080000)
#define USER_FLASH_END_ADDRESS        ((uint32_t)0x08100000)

#define USER_FLASH_ADDRESS            (USER_FLASH_END_ADDRESS - FLASH_PAGE) /* 80FF800 */

#define TOPO_FLASH_ADDRESS            (FLASH_START_BANK2 - FLASH_PAGE)     /* 807F800 */
#define DAEMON_FLASH_ADDRESS          (FLASH_START_BANK2 - 4 * FLASH_PAGE) /* 807E000 */
#define RESET_FLASH_ADDRESS           (FLASH_START_BANK2 - 5 * FLASH_PAGE) /* 807D800 */
#define TRACE_FLASH_ADDRESS           (FLASH_START_BANK2 - 6 * FLASH_PAGE) /* 807D000 */
#define DAEMON_DEBUG_FLASH_ADDRESS    (FLASH_START_BANK2 - 7 * FLASH_PAGE) /* 807C800 */
#define DAEMON_DEBUG_LBT_ADDRESS      (FLASH_START_BANK2 - 8 * FLASH_PAGE) /* 807C000 */

#define FIRMWARE_FLASH_ADDRESS_1      (FLASH_START_BANK2 - 3 * FLASH_PAGE) /* 0x0807 E800 */
#define FIRMWARE_FLASH_ADDRESS_2      (USER_FLASH_END_ADDRESS - 3 * FLASH_PAGE) /* 0x080F E800 */

#define FLASH_PAGE                    ((uint32_t)0x800)

#define TOPO_PAGE                     (255)
#define FIRMWARE_PAGE                 (253)
#define DAEMON_PAGE                   (252)
#define RESET_PAGE                    (251)
#define TRACE_PAGE                    (250)
#define DAEMON_DEBUG_PAGE             (249)
#define DAEMON_LBT_PAGE               (248)

#define DAEMON_DEBUG_ENERGY_LEN       (10 * 4) /* 40 bytes */
#define DAEMON_DEBUG_ENERGY_LEN_64    (16 * 4) /* 64 bytes */
#define DAEMON_DEBUG_ENERGY_LEN_192   (48 * 4) /* 192 bytes */

//**************************************************************************************************
//***** Prototypes of Global Functions *************************************************************

uint32_t FLASH_If_Erase(uint32_t StartSector);
uint32_t FLASH_If_Erase_Pages(uint32_t bank_active, uint32_t page);
uint32_t FLASH_If_Check_old(uint32_t start);
uint32_t FLASH_If_Write(uint32_t destination, uint32_t *p_source, uint32_t length);

#endif  /* __FLASH_IF_H__ */
