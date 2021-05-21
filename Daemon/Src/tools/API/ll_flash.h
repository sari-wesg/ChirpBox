#ifndef __LL_FLASH_H
#define __LL_FLASH_H
#include "stdint.h"
#include "stm32l4xx.h"
#include "stm32l476xx.h"
#ifdef __cplusplus
  extern "C" {
#endif

#define LL_OK                      0x00
#define LL_ERROR                   0x01
#ifndef FLASH_KEY1
  #define FLASH_KEY1                 ((uint32_t)0x45670123)               /*!< Flash key1 */
#endif
#ifndef FLASH_KEY2
  #define FLASH_KEY2                 ((uint32_t)0xCDEF89AB)               /*!< Flash key2: used with FLASH_KEY1\
                                                                            to unlock the FLASH registers access */
#endif
#ifndef FLASH_OPTKEY1
  #define FLASH_OPTKEY1              ((uint32_t)0x08192A3BU)               /*!< Flash option byte key1 */
#endif
#ifndef FLASH_OPTKEY2
  #define FLASH_OPTKEY2              ((uint32_t)0x4C5D6E7FU)               /*!< Flash option byte key2: used with FLASH_OPTKEY1
                                                                            to allow option bytes operations */
#endif

enum
{
  FLASH_CHK_OK = 0,
  FLASH_CHK_EMPTY
};

__STATIC_INLINE uint32_t LL_FLASH_IsActiveFlag_BSY(FLASH_TypeDef *FLASHx)
{
  return (READ_BIT(FLASHx->SR, FLASH_SR_BSY) == (FLASH_SR_BSY));
}
  /* Set the OBL_Launch bit to launch the option byte loading */
__STATIC_INLINE void LL_FLASH_SET_OBL_Launch(FLASH_TypeDef *FLASHx)
{
  SET_BIT(FLASHx->CR, FLASH_CR_OBL_LAUNCH);
}
__STATIC_INLINE void LL_FLASH_Lock(FLASH_TypeDef *FLASHx)
{
  SET_BIT(FLASHx->CR, FLASH_CR_LOCK);
}
/*read flash's states of lock or unlock*/
__STATIC_INLINE uint32_t LL_FLASH_LockState(FLASH_TypeDef *FLASHx)
{
	return READ_BIT(FLASHx->CR,FLASH_CR_LOCK);
}
/*set key for flash*/
__STATIC_INLINE void LL_FLASh_SetKey(FLASH_TypeDef *FLASHx,uint32_t key)
{
	WRITE_REG(FLASHx->KEYR,key);
}

/*EnableProgram*/
__STATIC_INLINE void LL_FLASH_EnableProgram(FLASH_TypeDef *FLASHx)
{
  SET_BIT(FLASHx->CR,FLASH_CR_PG);
}
/*DisenableProgram*/
__STATIC_INLINE void LL_FLASH_DisenableProgram(FLASH_TypeDef *FLASHx)
{
  CLEAR_BIT(FLASHx->CR,FLASH_CR_PG);
}

uint8_t LL_FLASH_PageErase(uint16_t Npages);
uint32_t STMFLASH_Read32( uint32_t faddr );
uint8_t LL_FLASH_Program64s(uint32_t destination, uint32_t* pData,uint16_t DataLen);
uint8_t LL_FLASH_Program64(uint32_t faddr,uint32_t* pData);
uint8_t STMFLASH_BankSwitch(void);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __LL_FLASH_H */
