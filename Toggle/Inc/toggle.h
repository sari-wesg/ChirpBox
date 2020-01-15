#ifndef __TOGGLE_H
#define __TOGGLE_H
#include "stdint.h"
#ifdef __cplusplus
 extern "C" {
#endif
/* Includes ------------------------------------------------------------------*/

#define FLAG_WRT_ERR        0x0F
#define FLAG_WRT_OK         0x00
#define BANK_TOGGLE_ERR     0x1F
#define BANK_TOGGLE_OK      0x10
	 


uint8_t TOGGLE_RESET_EXTI_CALLBACK(void);//uint16_t
	
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __TOGGLE_H */
