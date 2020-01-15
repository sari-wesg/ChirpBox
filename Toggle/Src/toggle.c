#include "toggle.h"
#include "ll_flash.h"

#define  JUMP_FLAG_ADDRESS             ((uint32_t)0x0807F800)
#define  JUMP_FLAG                     0x4A554D50 //"JUMP"
#define  JUMP_N_FLAG                   0xFFFFFFFF

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
		LL_Flash_PageErase(255);
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
