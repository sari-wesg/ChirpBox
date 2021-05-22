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

	if ((TOGGLE_RESET_EXTI_CALLBACK() == FLAG_WRT_OK))
	{
		INTO_MAIN();
	}
}
