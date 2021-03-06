/**
  ******************************************************************************
  * @file    STM32L476G_EVAL/DualBank/src/flash_if.c
  * @author  MCD Application Team
  * @version 1.0.0
  * @date    15-September-2016
  * @brief   This file provides all the memory related operation functions.
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

/** @addtogroup STM32L4xx_DB
  * @{
  */

/** @addtogroup Flash_IF
  * @brief Internal flash memory manipulating functions.
  * @{
  */


//**************************************************************************************************
//***** Includes ***********************************************************************************

#include<string.h>
#include<stdlib.h>
#include "flash_if.h"
#include "API_ChirpBox.h"

//**************************************************************************************************
//***** Local Defines and Consts *******************************************************************

//**************************************************************************************************
//***** Global Functions ***************************************************************************

/**
  * @brief  This function does an erase of all user flash area
  * @param  bank_active: start of user flash area
  * @retval FLASHIF_OK : user flash area successfully erased
  *         FLASHIF_ERASEKO : error occurred
  */
uint32_t FLASH_If_Erase(uint32_t bank_active)
{
  gpi_watchdog_periodic();
  uint32_t bank_to_erase, error = 0;
  FLASH_EraseInitTypeDef pEraseInit;
  HAL_StatusTypeDef status = HAL_OK;

  if (bank_active == 0)
  {
    bank_to_erase = FLASH_BANK_2;
  }
  else
  {
    bank_to_erase = FLASH_BANK_1;
  }
  #if ENERGEST_CONF_ON
    ENERGEST_ON(ENERGEST_TYPE_FLASH_ERASE);
    ENERGEST_OFF(ENERGEST_TYPE_CPU);
  #endif
  /* Unlock the Flash to enable the flash control register access *************/
  HAL_FLASH_Unlock();

  pEraseInit.Banks = bank_to_erase;
  pEraseInit.NbPages = 255;
  pEraseInit.Page = 0;
  pEraseInit.TypeErase = FLASH_TYPEERASE_MASSERASE;

  status = HAL_FLASHEx_Erase(&pEraseInit, &error);

  /* Lock the Flash to disable the flash control register access (recommended
     to protect the FLASH memory against possible unwanted operation) *********/
  HAL_FLASH_Lock();
  #if ENERGEST_CONF_ON
    ENERGEST_OFF(ENERGEST_TYPE_FLASH_ERASE);
    ENERGEST_ON(ENERGEST_TYPE_CPU);
  #endif
  if (status != HAL_OK)
  {
    /* Error occurred while page erase */
    return FLASHIF_ERASEKO;
  }

  return FLASHIF_OK;
}

//**************************************************************************************************

uint32_t FLASH_If_Erase_Pages(uint32_t bank_active, uint32_t page)
{
  gpi_watchdog_periodic();
  uint32_t bank_to_erase, error = 0;
  FLASH_EraseInitTypeDef pEraseInit;
  HAL_StatusTypeDef status = HAL_OK;

  if (bank_active == 0)
  {
    bank_to_erase = FLASH_BANK_2;
  }
  else
  {
    bank_to_erase = FLASH_BANK_1;
  }

  #if ENERGEST_CONF_ON
    ENERGEST_ON(ENERGEST_TYPE_FLASH_ERASE);
    ENERGEST_OFF(ENERGEST_TYPE_CPU);
  #endif
  /* Unlock the Flash to enable the flash control register access *************/
  HAL_FLASH_Unlock();

  pEraseInit.Banks = bank_to_erase;
  pEraseInit.NbPages = 1;
  pEraseInit.Page = page;
  pEraseInit.TypeErase = FLASH_TYPEERASE_PAGES;

  status = HAL_FLASHEx_Erase(&pEraseInit, &error);

  /* Lock the Flash to disable the flash control register access (recommended
     to protect the FLASH memory against possible unwanted operation) *********/
  HAL_FLASH_Lock();
  #if ENERGEST_CONF_ON
    ENERGEST_OFF(ENERGEST_TYPE_FLASH_ERASE);
    ENERGEST_ON(ENERGEST_TYPE_CPU);
  #endif
  if (status != HAL_OK)
  {
    /* Error occurred while page erase */
    return FLASHIF_ERASEKO;
  }

  return FLASHIF_OK;
}

//**************************************************************************************************

/**
  * @brief  This function does an CRC check of an application loaded in a memory bank.
  * @param  start: start of user flash area
  * @retval FLASHIF_OK: user flash area successfully erased
  *         other: error occurred
  */
uint32_t FLASH_If_Check_old(uint32_t start)
{
  /* checking if the data could be code (first word is stack location) */
  if ((*(uint32_t*)start >> 24) != 0x20 ) return FLASHIF_EMPTY;

  return FLASHIF_OK;
}

//**************************************************************************************************

/**
  * @brief  This function writes a data buffer in flash (data are 32-bit aligned).
  * @note   After writing data buffer, the flash content is checked.
  * @param  destination: start address for target location
  * @param  p_source: pointer on buffer with data to write
  * @param  length: length of data buffer (unit is 32-bit word)
  * @retval uint32_t 0: Data successfully written to Flash memory
  *         1: Error occurred while writing data in Flash memory
  *         2: Written Data in flash memory is different from expected one
  */
uint32_t FLASH_If_Write(uint32_t destination, uint32_t *p_source, uint32_t length)
{
  gpi_watchdog_periodic();
  uint32_t status = FLASHIF_OK;
  uint32_t i = 0;

  #if ENERGEST_CONF_ON
    if (destination >= FLASH_START_BANK2)
      ENERGEST_ON(ENERGEST_TYPE_FLASH_WRITE_BANK2);
    else
      ENERGEST_ON(ENERGEST_TYPE_FLASH_WRITE_BANK1);
    ENERGEST_OFF(ENERGEST_TYPE_CPU);
  #endif
  /* Unlock the Flash to enable the flash control register access *************/
  HAL_FLASH_Unlock();

  /* DataLength must be a multiple of 64 bit */
  for (i = 0; (i < length / 2) && (destination <= (USER_FLASH_END_ADDRESS - 8)); i++)
  {
    /* Device voltage range supposed to be [2.7V to 3.6V], the operation will
       be done by word */
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, destination, *((uint64_t *)(p_source + 2*i))) == HAL_OK)
    {
      /* Check the written value */
      if (*(uint64_t*)destination != *(uint64_t *)(p_source + 2*i))
      {
        /* Flash content doesn't match SRAM content */
        status = FLASHIF_WRITINGCTRL_ERROR;
        break;
      }
      /* Increment FLASH destination address */
      destination += 8;
    }
    else
    {
      /* Error occurred while writing data in Flash memory */
      status = FLASHIF_WRITING_ERROR;
      break;
    }
  }

  /* Lock the Flash to disable the flash control register access (recommended
     to protect the FLASH memory against possible unwanted operation) *********/
  HAL_FLASH_Lock();
  #if ENERGEST_CONF_ON
    if (destination >= FLASH_START_BANK2)
    {
      ENERGEST_OFF(ENERGEST_TYPE_FLASH_WRITE_BANK2);
    }
    else
      ENERGEST_OFF(ENERGEST_TYPE_FLASH_WRITE_BANK1);
    ENERGEST_ON(ENERGEST_TYPE_CPU);
  #endif
  return status;
}

//**************************************************************************************************
