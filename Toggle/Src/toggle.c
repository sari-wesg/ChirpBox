#include "toggle.h"
#include "ll_flash.h"

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
					#if defined ( __ALARM )
						IMPORT  __main_after_scatterload
						BL      __main_after_scatterload
					#else
						IMPORT  __main
						LDR     R0, =__main
						BX      R0
					#endif
				}
				__ASM void __SCATTER(void)
				{
					PRESERVE8
					MOV 	 R12, LR
					IMPORT   __scatterload
					LDR      R0, =__scatterload
					LDR      R4,[R0,#27]
					LDR      R5,[R0,#31]
					B        __SCATTER + 28
					LDR      R0,[R4,#0xc]
					ORR      R3,R0,#1
					LDM      R4,{R0-R2}
					BLX      R3
					ADDS     R4,R4,#0x10
					CMP      R4,R5
					BCC      __SCATTER + 14
					BX		 R12
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
					#ifndef ( __ALARM )
						__ASM volatile
						(
							"LDR     R0, =__iar_program_start  \n"
							"BX      R0                        \n"
						)
					#else
						#error "Please define in the compiler options."
					#endif
				}
#endif

#if defined ( __ALARM )
void DS3231_EXTI_Handler( void )
{
	EXTI->PR1 = (GPIO_PIN_4);
	STMFLASH_BankSwitch();
}

static void SET_NVIC_SetPriority(IRQn_Type IRQn, uint32_t PreemptPriority, uint32_t SubPriority)
{
	uint32_t prioritygroup = 0x00;

	prioritygroup = NVIC_GetPriorityGrouping();

	NVIC_SetPriority(IRQn, NVIC_EncodePriority(prioritygroup, PreemptPriority, SubPriority));
}

void DS3231_GPIO_Init(GPIO_TypeDef  *GPIOx, uint32_t GPIO_Pin)
{
	SET_BIT(RCC->AHB2ENR, DS3231_RCC_AHB2);
	/* Delay after an RCC peripheral clock enabling */
	READ_BIT(RCC->AHB2ENR, DS3231_RCC_AHB2);

	uint32_t position = 0x00u;
	uint32_t iocurrent, temp;
	uint32_t GPIO_Mode = GPIO_MODE_IT_FALLING;
	uint32_t GPIO_Pull = GPIO_PULLUP;
    uint8_t  PinPos = 0;

    if ( ( GPIO_Pin & 0xFF00 ) != 0) { PinPos |= 0x8; }
    if ( ( GPIO_Pin & 0xF0F0 ) != 0) { PinPos |= 0x4; }
    if ( ( GPIO_Pin & 0xCCCC ) != 0) { PinPos |= 0x2; }
    if ( ( GPIO_Pin & 0xAAAA ) != 0) { PinPos |= 0x1; }

	position = PinPos;
	iocurrent = (GPIO_Pin) & (1uL << position);

	/* Configure IO Direction mode (Input, Output, Alternate or Analog) */
	temp = GPIOx->MODER;
	temp &= ~(GPIO_MODER_MODE0 << (position * 2u));
	temp |= ((GPIO_Mode & GPIO_MODE) << (position * 2u));
	GPIOx->MODER = temp;

	/* Activate the Pull-up or Pull down resistor for the current IO */
	temp = GPIOx->PUPDR;
	temp &= ~(GPIO_PUPDR_PUPD0 << (position * 2u));
	temp |= ((GPIO_Pull) << (position * 2u));
	GPIOx->PUPDR = temp;

	/*--------------------- EXTI Mode Configuration ------------------------*/
	/* Enable SYSCFG Clock */
	SET_BIT(RCC->APB2ENR, RCC_APB2ENR_SYSCFGEN);
	/* Delay after an RCC peripheral clock enabling */
	READ_BIT(RCC->APB2ENR, RCC_APB2ENR_SYSCFGEN);

	temp = SYSCFG->EXTICR[position >> 2u];
	temp &= ~(0x0FuL << (4u * (position & 0x03u)));
	temp |= (DS3231_INTCN_GPIO_INDEX << (4u * (position & 0x03u)));
	SYSCFG->EXTICR[position >> 2u] = temp;

	/* Clear EXTI line configuration */
	temp = EXTI->IMR1;
	temp &= ~(iocurrent);
	if((GPIO_Mode & GPIO_MODE_IT) == GPIO_MODE_IT)
	{
		temp |= iocurrent;
	}
	EXTI->IMR1 = temp;

	temp = EXTI->EMR1;
	temp &= ~(iocurrent);
	if((GPIO_Mode & GPIO_MODE_EVT) == GPIO_MODE_EVT)
	{
		temp |= iocurrent;
	}
	EXTI->EMR1 = temp;

	/* Clear Rising Falling edge configuration */
	temp = EXTI->RTSR1;
	temp &= ~(iocurrent);
	if((GPIO_Mode & RISING_EDGE) == RISING_EDGE)
	{
		temp |= iocurrent;
	}
	EXTI->RTSR1 = temp;

	temp = EXTI->FTSR1;
	temp &= ~(iocurrent);
	if((GPIO_Mode & FALLING_EDGE) == FALLING_EDGE)
	{
		temp |= iocurrent;
	}
	EXTI->FTSR1 = temp;

	SET_NVIC_SetPriority(DS3231_EXTI_IRQn, 0, 0);
	NVIC_EnableIRQ(DS3231_EXTI_IRQn);
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
	if (TOGGLE_RESET_EXTI_CALLBACK() == FLAG_WRT_OK)
	{
		#if defined ( __ALARM )
			#if defined ( __CC_ARM )
				__SCATTER();
			#elif defined ( __ICCARM__ )
				#error "Please define in the compiler options."
			#endif
			// Config the GPIO EXTI IRQ function of the INTCN_Pin that is connected to INT/SQW on DS3231
			DS3231_GPIO_Init(DS3231_INTCN_Port, DS3231_INTCN_Pin);
		#endif
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
