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

#include "flash_if.h"
#include<string.h>
#include<stdlib.h>
#define JANPATCH_STREAM     Flash_FILE        // use POSIX FILE
#include "janpatch.h"

//**************************************************************************************************
//***** Local Defines and Consts *******************************************************************
#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


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
  uint32_t status = FLASHIF_OK;
  uint32_t i = 0;

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

  return status;
}

//**************************************************************************************************

/**
  * @brief  Configure the write protection status of user flash area.
  * @retval uint32_t FLASHIF_OK if change is applied.
  */
uint32_t FLASH_If_WriteProtectionClear( void )
{
  FLASH_OBProgramInitTypeDef OptionsBytesStruct1;
  HAL_StatusTypeDef retr;

  /* Unlock the Flash to enable the flash control register access *************/
  retr = HAL_FLASH_Unlock();

  /* Unlock the Options Bytes *************************************************/
  retr |= HAL_FLASH_OB_Unlock();

  OptionsBytesStruct1.RDPLevel = OB_RDP_LEVEL_0;
  OptionsBytesStruct1.OptionType = OPTIONBYTE_WRP;
  OptionsBytesStruct1.WRPArea = OB_WRPAREA_BANK2_AREAA;
  OptionsBytesStruct1.WRPEndOffset = 0x00;
  OptionsBytesStruct1.WRPStartOffset = 0xFF;
  retr |= HAL_FLASHEx_OBProgram(&OptionsBytesStruct1);

  OptionsBytesStruct1.WRPArea = OB_WRPAREA_BANK2_AREAB;
  retr |= HAL_FLASHEx_OBProgram(&OptionsBytesStruct1);

  return (retr == HAL_OK ? FLASHIF_OK : FLASHIF_PROTECTION_ERRROR);
}

//**************************************************************************************************

/**
  * @brief  Modify the BFB2 status of user flash area.
  * @param  none
  * @retval HAL_StatusTypeDef HAL_OK if change is applied.
  */
HAL_StatusTypeDef FLASH_If_BankSwitch(void)
{
  FLASH_OBProgramInitTypeDef ob_config;
  HAL_StatusTypeDef result;

  HAL_FLASH_Lock();

  /* Clear OPTVERR bit set on virgin samples */
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);//cleared by writing 1 bin15

  /* Get the current configuration */
  HAL_FLASHEx_OBGetConfig( &ob_config );

  ob_config.OptionType = OPTIONBYTE_USER;
  ob_config.USERType = OB_USER_BFB2;
  if ((ob_config.USERConfig) & (OB_BFB2_ENABLE)) /* BFB bit set 1,BANK1 active for boot */
  {
    ob_config.USERConfig = OB_BFB2_DISABLE;
  }
  else
  {
    ob_config.USERConfig = OB_BFB2_ENABLE;
  }

  /* Initiating the modifications */
  result = HAL_FLASH_Unlock();

  /* program if unlock is successful */
  if ( result == HAL_OK )
  {
    result = HAL_FLASH_OB_Unlock();

    /* program if unlock is successful*/
    if ((READ_BIT(FLASH->CR, FLASH_CR_OPTLOCK) == RESET))
    {
      result = HAL_FLASHEx_OBProgram(&ob_config);
    }
    if (result == HAL_OK)
    {
      HAL_FLASH_OB_Launch();
    }
  }
  return result;
}

//**************************************************************************************************
//**************************************************************************************************
int the_fseek(Flash_FILE *file, long int offset, int origin)
{
  if (origin == SEEK_SET) {
    file->now_page = file->origin_page + offset / FLASH_PAGE;
  }
  else if (origin == SEEK_CUR) {
    file->now_page += offset / FLASH_PAGE;
  }
  else {
      return -1;
  }
  return 0;
}

size_t the_fwrite(const void *ptr, size_t size, size_t count, Flash_FILE *file)
{
  uint32_t BankActive = READ_BIT(SYSCFG->MEMRMP, SYSCFG_MEMRMP_FB_MODE);
  FLASH_If_Erase_Pages(BankActive, file->now_page);
  uint32_t flash_bank_address = (BankActive == 1)? FLASH_START_BANK1 : FLASH_START_BANK2;
  // The flash write below is in the double-word form,
  // so we need to ensure the count is multiple of sizeof(uint64_t)
  if (count % 8)
    count += 8 - (count % 8);
  FLASH_If_Write(flash_bank_address + file->now_page * FLASH_PAGE, (uint32_t *)ptr, count / sizeof(uint32_t));
  return count;
}

size_t the_fread(void *ptr, size_t size, size_t count, Flash_FILE* file)
{
  uint32_t BankActive = READ_BIT(SYSCFG->MEMRMP, SYSCFG_MEMRMP_FB_MODE);
  uint32_t flash_bank_address = (BankActive == 1)? FLASH_START_BANK1 : FLASH_START_BANK2;

  memcpy((uint32_t *)ptr, (uint32_t *)(flash_bank_address + file->now_page * FLASH_PAGE), count);
  uint32_t left_count = file->file_size - (file->now_page - file->origin_page) * FLASH_PAGE;
  // Ensure the real useful bytes are no larger than left bytes
  if ((count > left_count) && (file->file_size))
    count = left_count;
  return count;
}

static void progress(uint8_t percentage) {
    printf("Patch progress: %d%%\n", percentage);
}

int Filepatch(uint32_t originalPage, uint32_t originalSize,
              uint32_t patchPage, uint32_t patchSize, uint32_t newPage)
{
  janpatch_ctx ctx = {
      // fread/fwrite buffers for every file, minimum size is 1 byte
      // when you run on an embedded system with block size flash, set it to the size of a block for best performance
      { (unsigned char*)malloc(FLASH_PAGE), FLASH_PAGE },
      { (unsigned char*)malloc(FLASH_PAGE), FLASH_PAGE },
      { (unsigned char*)malloc(FLASH_PAGE), FLASH_PAGE },

      &the_fread,
      &the_fwrite,
      &the_fseek,
      &progress
  };

  Flash_FILE originalFile = {originalPage, 0, originalSize};
  Flash_FILE patchFile = {patchPage, 0, patchSize};
  Flash_FILE newFile = {newPage};

  int jpr = janpatch(ctx, &originalFile, &patchFile, &newFile);
  if (!jpr)
    return 0;
  else return -1;
}
//**************************************************************************************************
//**************************************************************************************************
