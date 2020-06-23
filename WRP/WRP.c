//#include "stm32l4xx_hal.h"
#include "flash_if.h"
#include "stm32l4xx_hal_def.h"
/**
  * @brief  Set the FLASH_WRP2xR status of user flash area.
  * @param  none
  * @retval HAL_StatusTypeDef HAL_OK if change is applied.
  */

uint32_t Bank1_WRP(uint32_t strtA_offset, uint32_t endA_offset)
{
  FLASH_OBProgramInitTypeDef OptionsBytesStruct1;
  HAL_StatusTypeDef retr;
    
	/* Check the parameters */
  assert_param(IS_FLASH_PAGE(strtA_offset));
  assert_param(IS_FLASH_PAGE(endA_offset));
  /* Unlock the Flash to enable the flash control register access *************/
  retr = HAL_FLASH_Unlock();

  /* Unlock the Options Bytes *************************************************/
  retr |= HAL_FLASH_OB_Unlock();

  OptionsBytesStruct1.RDPLevel = OB_RDP_LEVEL_0;
  OptionsBytesStruct1.OptionType = OPTIONBYTE_WRP;
  OptionsBytesStruct1.WRPArea = OB_WRPAREA_BANK1_AREAA;
  OptionsBytesStruct1.WRPEndOffset = endA_offset;
  OptionsBytesStruct1.WRPStartOffset = strtA_offset;
  retr |= HAL_FLASHEx_OBProgram(&OptionsBytesStruct1);

  return (retr == HAL_OK ? FLASHIF_OK : FLASHIF_PROTECTION_ERRROR);
}


/**
  * @brief  Reset the FLASH_WRP2xR status of user flash area.
  * @param  none
  * @retval HAL_StatusTypeDef HAL_OK if change is applied.
  */
uint32_t Bank1_nWRP( void )
{
  FLASH_OBProgramInitTypeDef OptionsBytesStruct1;
  HAL_StatusTypeDef retr;

  /* Unlock the Flash to enable the flash control register access *************/
  retr = HAL_FLASH_Unlock();

  /* Unlock the Options Bytes *************************************************/
  retr |= HAL_FLASH_OB_Unlock();

  OptionsBytesStruct1.RDPLevel = OB_RDP_LEVEL_0;
  OptionsBytesStruct1.OptionType = OPTIONBYTE_WRP;
  OptionsBytesStruct1.WRPArea = OB_WRPAREA_BANK1_AREAA;
  OptionsBytesStruct1.WRPEndOffset = 0x00;
  OptionsBytesStruct1.WRPStartOffset = 0xFF;
  retr |= HAL_FLASHEx_OBProgram(&OptionsBytesStruct1);

  OptionsBytesStruct1.WRPArea = OB_WRPAREA_BANK1_AREAB;
  retr |= HAL_FLASHEx_OBProgram(&OptionsBytesStruct1);

  return (retr == HAL_OK ? FLASHIF_OK : FLASHIF_PROTECTION_ERRROR);
}
