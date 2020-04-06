#ifndef __TOGGLE_H
#define __TOGGLE_H
#include "stdint.h"
#include "stm32l4xx.h"
#include "stm32l476xx.h"
#ifdef __cplusplus
    extern "C" {
#endif
/* Includes ------------------------------------------------------------------*/

#define FLAG_WRT_ERR            0x0F
#define FLAG_WRT_OK             0x00
#define BANK_TOGGLE_ERR         0x1F
#define BANK_TOGGLE_OK          0x10
#define USR_FLASH_START         0x0807F800
#define USR_FLASH_END           0x08080000
#define USR_PAGE                255
#define MAX_LEN            	    2048
#define FLASH_WT_OUT_OF_RANGE	0xFF


uint8_t TOGGLE_RESET_EXTI_CALLBACK(void) __attribute__((section(".ARM.__at_0x08000534")));
uint8_t USR_FLASH_PageErase(void);
uint8_t USR_FLASH_Program8(uint32_t faddr,uint8_t* pData, uint16_t DataLen);
uint8_t USR_FLASH_Read8(uint32_t faddr);
void DS3231_GPIO_Init(GPIO_TypeDef  *GPIOx, uint32_t GPIO_Pin);

#define __ALARM
#define GPIO_PIN_4                      ((uint16_t)0x0010)  /* Pin 4 selected    */
#define DS3231_INTCN_Pin                GPIO_PIN_4
#define DS3231_INTCN_Port               GPIOA
#define DS3231_INTCN_GPIO_INDEX         0uL                 /* GPIOA selected    */
#define DS3231_RCC_AHB2                 RCC_AHB2ENR_GPIOAEN
#define DS3231_EXTI_Handler             EXTI4_IRQHandler
#define DS3231_EXTI_IRQn                EXTI4_IRQn

#ifndef GPIO_MODE
    #define GPIO_MODE                   (0x00000003u)
#endif
#ifndef GPIO_MODE_IT
    #define GPIO_MODE_IT                (0x00010000u)
#endif
#ifndef GPIO_MODE_EVT
    #define GPIO_MODE_EVT               (0x00020000u)
#endif
#ifndef RISING_EDGE
    #define RISING_EDGE                 (0x00100000u)
#endif
#ifndef FALLING_EDGE
    #define FALLING_EDGE                (0x00200000u)
#endif
#ifndef GPIO_MODE_IT_FALLING
    #define  GPIO_MODE_IT_FALLING       (0x10210000u)   /*!< External Interrupt Mode with Falling edge trigger detection         */
#endif
#ifndef GPIO_PULLUP
    #define  GPIO_PULLUP                (0x00000001u)   /*!< Pull-up activation                  */
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __TOGGLE_H */
