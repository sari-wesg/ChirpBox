#include "ll_flash.h"
uint8_t LL_Flash_Unlock(void);
uint32_t FLASH_If_Check(uint32_t start);
uint8_t LL_FLASH_OB_Unlock(void);

uint8_t LL_FLASH_PageErase(uint16_t PageNumber)
{
	uint32_t BankActive;
	BankActive = READ_BIT(SYSCFG->MEMRMP, SYSCFG_MEMRMP_FB_MODE);
	LL_Flash_Unlock();
	while (LL_FLASH_IsActiveFlag_BSY(FLASH))
	{
	}
	if(PageNumber>200)
	{
		SET_BIT(FLASH->CR, FLASH_CR_PER);        //enable flash earse
		if(BankActive == 0)                      //bank1 is active
		{
			CLEAR_BIT(FLASH->CR, FLASH_CR_BKER);   //earse bank1
		}
		else                                     //bank2 is active
		{
			SET_BIT(FLASH->CR, FLASH_CR_BKER);     //earse bank2
		}
	  SET_BIT(FLASH->CR, (PageNumber << 3));   //set page to earse
    SET_BIT(FLASH->CR, FLASH_CR_STRT);       //start earsing...
		while (LL_FLASH_IsActiveFlag_BSY(FLASH)) //wait for flash operation complete
	  {
	  }
		CLEAR_BIT(FLASH->CR, FLASH_CR_PNB);      //clear page to earse
		CLEAR_BIT(FLASH->CR, FLASH_CR_PER);      //disable flash earse
	}
	else
	{
		return LL_ERROR;
	}

	LL_FLASH_Lock(FLASH);
	return LL_OK;
}
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
uint8_t LL_FLASH_Program64(uint32_t faddr,uint32_t* pData)
{
	uint32_t prog_bit = 0;
	LL_Flash_Unlock();
	while (LL_FLASH_IsActiveFlag_BSY(FLASH))    //wait for flash operation complete
	{
	}
	LL_FLASH_EnableProgram(FLASH);              //flash program enable
	/* Program the double word */
  *(__IO uint32_t*)faddr = *pData;            //program 4 bytes, little endian
  *(__IO uint32_t*)(faddr+4) = *(pData+1);

	prog_bit = FLASH_CR_PG;
	while (LL_FLASH_IsActiveFlag_BSY(FLASH))    //wait for flash operation complete
	{
	}
	if(prog_bit!=0)
	{
		CLEAR_BIT(FLASH->CR,prog_bit);
	}
	LL_FLASH_DisenableProgram(FLASH);
	LL_FLASH_Lock(FLASH);
	return LL_OK;
}

uint8_t LL_FLASH_Program64s(uint32_t destination, uint32_t* pData,uint16_t DataLen)
{
	uint32_t prog_bit = 0;
	uint16_t i = 0;
	LL_Flash_Unlock();
	while (LL_FLASH_IsActiveFlag_BSY(FLASH))       //wait for flash operation complete
  {
	}
	for (i = 0; (i < DataLen / 2) && (destination <= (0x08080000 - 8)); i++)
	{
		LL_FLASH_EnableProgram(FLASH);              //flash program enable
		while (LL_FLASH_IsActiveFlag_BSY(FLASH))    //wait for flash operation complete
	  {
	  }
	  /* Program the double word */
    *(__IO uint32_t*)(destination) = *(pData+2*i);            //program 4 bytes, little endian
    *(__IO uint32_t*)(destination+4) = *(pData+2*i+1);
		if(*(uint64_t*)destination==*(uint64_t*)(pData+2*i))
		{
			destination += 8;
		  prog_bit = FLASH_CR_PG;
			CLEAR_BIT(FLASH->CR,prog_bit);
		}
		else
		  i = i-1;
		LL_FLASH_DisenableProgram(FLASH);
	}
	LL_FLASH_Lock(FLASH);
	return LL_OK;
}

uint32_t STMFLASH_Read32( uint32_t faddr )
{
  return *(__IO uint32_t*)faddr;
}

uint8_t STMFLASH_BankSwitch(void)
{
	uint8_t result;
	uint32_t BankActive = 0;

	LL_FLASH_Lock(FLASH);
	/* Clear OPTVERR bit set on virgin samples */
	if((FLASH_SR_OPTVERR) & (FLASH_ECCR_ECCC | FLASH_ECCR_ECCD))
  { SET_BIT(FLASH->ECCR, ((FLASH_SR_OPTVERR) & (FLASH_ECCR_ECCC | FLASH_ECCR_ECCD))); }

  if((FLASH_SR_OPTVERR) & ~(FLASH_ECCR_ECCC | FLASH_ECCR_ECCD))
  { WRITE_REG(FLASH->SR, ((FLASH_SR_OPTVERR) & ~(FLASH_ECCR_ECCC | FLASH_ECCR_ECCD))); }

	BankActive = READ_BIT(SYSCFG->MEMRMP, SYSCFG_MEMRMP_FB_MODE);
  result = LL_Flash_Unlock();

	if( result == LL_OK)
	{
		  result = LL_FLASH_OB_Unlock();
		  if((READ_BIT(FLASH->CR, FLASH_CR_OPTLOCK) == RESET))
			{
				  while (LL_FLASH_IsActiveFlag_BSY(FLASH))    //wait for flash operation complete
					{
					}
					if (BankActive != 0)
					{
						CLEAR_BIT(FLASH->OPTR, FLASH_OPTR_BFB2);
					}
					else
					{
						SET_BIT(FLASH->OPTR, FLASH_OPTR_BFB2);
					}
			}
			/* Set OPTSTRT Bit */
			SET_BIT(FLASH->CR, FLASH_CR_OPTSTRT);
			while (LL_FLASH_IsActiveFlag_BSY(FLASH))    //wait for flash operation complete
			{
			}
			/* If the option byte program operation is completed, disable the OPTSTRT Bit */
			CLEAR_BIT(FLASH->CR, FLASH_CR_OPTSTRT);

			/* Set the bit to force the option byte reloading */
			if (result == LL_OK)
			{
				LL_FLASH_SET_OBL_Launch(FLASH);
			}
			while (LL_FLASH_IsActiveFlag_BSY(FLASH))    //wait for flash operation complete
			{
			}
	}
	return result;
}

uint8_t LL_Flash_Unlock(void)
{
	while (LL_FLASH_IsActiveFlag_BSY(FLASH))
	{
	}
	if (LL_FLASH_LockState(FLASH)!=0)
	{
		LL_FLASh_SetKey(FLASH,FLASH_KEY1);
		LL_FLASh_SetKey(FLASH,FLASH_KEY2);
	}
	return LL_OK;
}
uint32_t FLASH_If_Check(uint32_t start)
{
  /* checking if the data could be code (first word is stack location) */
  if ((*(uint32_t*)start >> 24) != 0x20 ) return FLASH_CHK_EMPTY;
  return FLASH_CHK_OK;
}
uint8_t LL_FLASH_OB_Unlock(void)
{
  if(READ_BIT(FLASH->CR, FLASH_CR_OPTLOCK) != RESET)
  {
    /* Authorizes the Option Byte register programming */
    WRITE_REG(FLASH->OPTKEYR, FLASH_OPTKEY1);//allow programming
    WRITE_REG(FLASH->OPTKEYR, FLASH_OPTKEY2);//allow erasing
  }
  else
  {
    return LL_ERROR;
  }
  return LL_OK;
}
