

#ifndef __LORAWAN_H__
#define __LORAWAN_H__

//**************************************************************************************************
//***** Includes ***********************************************************************************
#include "hw.h"

//**************************************************************************************************
//***** Global (Public) Defines and Consts *********************************************************
/***************************** function config ****************************/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define LORAWAN_MAX_BAT   254

/*!
 * LoRaWAN Adaptive Data Rate
 * @note Please note that when ADR is enabled the end-device should be static
 */
// #define LORAWAN_ADR_STATE LORAWAN_ADR_ON
#define LORAWAN_ADR_STATE LORAWAN_ADR_OFF

/*!
 * LoRaWAN Default data Rate Data Rate
 * @note Please note that LORAWAN_DEFAULT_DATA_RATE is used only when ADR is disabled
 */
#define LORAWAN_DEFAULT_DATA_RATE                   DR_0

/*!
 * LoRaWAN application port
 * @note do not use 224. It is reserved for certification
 */
#define LORAWAN_APP_PORT                            2

/*!
 * LoRaWAN default endNode class port
 */
#define LORAWAN_DEFAULT_CLASS                       CLASS_A
// #define LORAWAN_DEFAULT_CLASS                       CLASS_C

/*!
 * LoRaWAN default confirm state
 */
#define LORAWAN_DEFAULT_CONFIRM_MSG_STATE           LORAWAN_UNCONFIRMED_MSG

/***************************** radio config ****************************/

/***************************** physical config ****************************/

/************************ packet format config **************************/

/******************************* LBT config ******************************/

//**************************************************************************************************
//***** Global Typedefs and Class Declarations *****************************************************

//**************************************************************************************************
//***** Global Variables ***************************************************************************

//**************************************************************************************************
//***** Prototypes of Global Functions *************************************************************
void lorawan_start();
void node_id_restore(uint8_t *id);


#endif  /* __LORAWAN_H__ */
