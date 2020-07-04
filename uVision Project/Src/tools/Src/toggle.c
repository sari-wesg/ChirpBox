#include "toggle.h"
#include "ll_flash.h"
#include "flash_if.h"
#include "stm32l4xx_hal_def.h"

#define  JUMP_FLAG_ADDRESS             ((uint32_t)0x0807F7F8)    //page 254
#define  JUMP_FLAG                     0x4A554D50 //"JUMP"
#define  JUMP_N_FLAG                   0xFFFFFFFF
#define  USER_DATA_PATCH               0xFF

uint8_t TOGGLE_RESET_EXTI_CALLBACK(void)//uint16_t
{
	unsigned short state=FLAG_WRT_ERR;
	unsigned int JumpFlag[2] = {0};
	if(STMFLASH_Read32(JUMP_FLAG_ADDRESS) == JUMP_N_FLAG)
	{
		JumpFlag[0] = JUMP_FLAG;
		JumpFlag[1] = JUMP_FLAG;
		if(LL_FLASH_Program64(JUMP_FLAG_ADDRESS, (uint32_t *)&JumpFlag)!=LL_OK)
		{
			state = FLAG_WRT_ERR;//Write Error
		}
		else
		{
			state = FLAG_WRT_OK;//Write Ok
		}
	}
	else if(STMFLASH_Read32(JUMP_FLAG_ADDRESS) == JUMP_FLAG)
	{
		LL_FLASH_PageErase(254);
		if (STMFLASH_BankSwitch() != LL_OK)
		{
			state = BANK_TOGGLE_ERR;//Jump Error
		}
		else
		{
			state = BANK_TOGGLE_OK;//Jump OK
		}
	}
	return state;
}

#if   defined ( __CC_ARM )
				__ASM void INTO_MAIN(void)
				{
						IMPORT  __main
						LDR     R0, =__main
						BX      R0
				}
#elif defined ( __GNUC__ )
				static inline __attribute__((always_inline)) void BEFORE_MAIN(void)
				{
					__ASM volatile
					(
						"ldr   sp, =_estack	\n"
						"movs	r1, #0			  \n"
						"b	LoopCopyDataInit\n"
						"CopyDataInit:			\n"
						"ldr	r3, =_sidata	\n"
						"ldr	r3, [r3, r1]	\n"
						"str	r3, [r0, r1]	\n"
						"adds	r1, r1, #4		\n"
						"LoopCopyDataInit:"
						"ldr	r0, =_sdata		\n"
						"ldr	r3, =_edata		\n"
						"adds	r2, r0, r1		\n"
						"cmp	r2, r3			  \n"
						"bcc	CopyDataInit	\n"
						"ldr	r2, =_sbss		\n"
						"b	LoopFillZerobss	\n"
						"FillZerobss:			  \n"
						"movs	r3, #0			  \n"
						"str	r3, [r2], #4	\n"
						"LoopFillZerobss:		\n"
						"ldr	r3, = _ebss		\n"
						"cmp	r2, r3			  \n"
						"bcc	FillZerobss		\n"
					);
				}

				static inline __attribute__((always_inline)) void INTO_MAIN(void)
				{
					__ASM volatile
					(
						"bl	main				  \n"
						"LoopForever:			\n"
						"b LoopForever		\n"
					);
				}
#elif defined ( __ICCARM__ )
				static void INTO_MAIN(void)
				{
					__ASM volatile
					(
						"LDR     R0, =__iar_program_start  \n"
						"BX      R0                        \n"
					)
				}
#endif

void Reset_Handler(void)
{
	#if defined ( __GNUC__ )
		BEFORE_MAIN();
		SystemInit();
	#else
		SystemInit();
	#endif

	/* Only examine JUMP when in bank 2 */
	uint32_t BankActive = READ_BIT(SYSCFG->MEMRMP, SYSCFG_MEMRMP_FB_MODE);
	if (((BankActive != 0) && (TOGGLE_RESET_EXTI_CALLBACK() == FLAG_WRT_OK)) || (BankActive == 0))
	{
		INTO_MAIN();
	}
}

uint8_t USR_FLASH_PageErase(void)
{
	return LL_FLASH_PageErase(255);
}

uint8_t USR_FLASH_Program8(uint32_t faddr,uint8_t* pData, uint16_t DataLen)
{
	uint8_t data_add=0;
	uint16_t data32_len=DataLen;
	uint8_t i=0;
	if(DataLen > MAX_LEN)
		return FLASH_WT_OUT_OF_RANGE;
	else
	{
		if(data32_len%8!=0)
		{
			data_add = 8 - (data32_len%8);
			for(i=0;i < data_add; i++)
			{
			  *(uint8_t*)(pData+DataLen+i) = (uint8_t)(USER_DATA_PATCH+i);
        data32_len += 1;
			}
		}
		data32_len = data32_len/4;
		return LL_FLASH_Program64s(faddr, (uint32_t*)pData, data32_len);
	}
}

uint8_t USR_FLASH_Read8( uint32_t faddr )
{
  return *(__IO uint8_t*)faddr;
}


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
