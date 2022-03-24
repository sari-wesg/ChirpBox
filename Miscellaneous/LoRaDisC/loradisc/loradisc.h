

#ifndef __LORADISC_H__
#define __LORADISC_H__

//**************************************************************************************************
//***** Includes ***********************************************************************************
// Radio
#include "gpi/platform.h"
#include GPI_PLATFORM_PATH(sx1276Regs_Fsk.h)
#include GPI_PLATFORM_PATH(sx1276Regs_LoRa.h)
// Integers
#include <stdint.h>
// ChirpBox's API
#include "API_ChirpBox.h"


#ifndef DEBUG_DISC
	#define DEBUG_DISC								1
#endif

#if DEBUG_DISC
#include <stdio.h>
#include <stdlib.h>

#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/* Whether to print logs */
#define DEBUG_DISC 1
#if DEBUG_DISC
#define PRINTF_DISC(...) printf(__VA_ARGS__)
#else
#define PRINTF_DISC(...)
#endif
//**************************************************************************************************
//***** Global (Public) Defines and Consts *********************************************************
/***************************** function config ****************************/
#ifndef LORADISC
	#define LORADISC								1
#endif

#ifndef MX_HEADER_CHECK
	#define MX_HEADER_CHECK							1
#endif

#ifndef MX_LBT_ACCESS
	#define MX_LBT_ACCESS							1
#endif

#ifndef NODE_LENGTH
    #define NODE_LENGTH 							0xFF
#endif

/***************************** radio config ****************************/
#define REGION_CN470							   // Frequency by country
#define USE_MODEM_LORA							   // Radio modem
#define CHANNEL_MAX                                9
#define CHANNEL_MIN                                0
#define CHANNEL_STEP                               200000
#define CN470_FREQUENCY							   486300000

#if defined( USE_MODEM_LORA )

#define LORA_SYMBOL_TIMEOUT                         500       // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false     // [false: Explicit Header mode
                                                              //  true: Implicit Header mode]
#define LORA_IQ_INVERSION_ON                        false
#define TX_TIMEOUT_VALUE                            3000
#define RX_TIMEOUT_VALUE                            3000


#elif defined( USE_MODEM_FSK )

#define FSK_FDEV                                    25000     // Hz
#define FSK_DATARATE                                50000     // bps
#define FSK_BANDWIDTH                               50000     // Hz
#define FSK_AFC_BANDWIDTH                           83333     // Hz
#define FSK_PREAMBLE_LENGTH                         5         // Same for Tx and Rx
#define FSK_FIX_LENGTH_PAYLOAD_ON                   false

#else
    #error "Please define a modem in the compiler options."
#endif
/***************************** physical config ****************************/
#define LoRaDisC_DEFAULT_BW				7   /* 7: 125 kHz, 8: 250 kHz, 9: 500kHz */
#define LoRaDisC_PREAMBLE_LENGTH		8   /* 8: 8 symbols */

/************************ packet format config **************************/
#define LORADISC_HEADER_LEN				8

#define FLOODING_SURPLUS_LENGTH			3
#define FLOODING_LENGTH					(255-LORADISC_HEADER_LEN)


#define ROUND_HEADER_LENGTH 	        4   /* Data section to represent the round number and node id */
#define DATA_HEADER_LENGTH     	        8   /* Reservation section to transmit commands, real data is written behind that section */
#define HASH_HEADER            	        2
#define HASH_TAIL            	        2

#define DISC_HEADER            	        0x1234
#define FLOODING_HEADER            	    0x5678

/******************************* LBT config ******************************/
#define LBT_CHANNEL_NUM         10

#define LBT_DELAY_IN_US         10000
#define CHANNEL_ALTER           2
#define LBT_CCA_TIME            5000
#define LBT_CCA_STEP            500
#define LBT_CCA_STEP_NUM        10

#define LBT_TX_TIME_S           3600

//**************************************************************************************************
//***** Global Typedefs and Class Declarations *****************************************************
typedef enum Disc_Primitive_tag
{
	FLOODING = 1,
	DISSEMINATION = 2,
	COLLECTION = 3
} Disc_Primitive;


//**************************************************************************************************
//***** Global Variables ***************************************************************************

//**************************************************************************************************
//***** Prototypes of Global Functions *************************************************************
// loradisc
void loradisc_write(uint8_t i, uint8_t *data);
void loradisc_read(uint8_t *data);
void loradisc_start(uint32_t dev_id);

// loradisc preparation
void loradisc_packet_write(uint8_t node_id, uint8_t *the_data);

// lorawan
void lorawan_listen_init(uint8_t node_id);
void lorawan_listen();
void lorawan_transmission();

void lorawan_main_timer_isr();
void lorawan_dio0_isr();
void lorawan_dio3_isr();


/* LBT */
uint8_t lbt_pesudo_channel(uint8_t channel_total, uint8_t last_channel, uint16_t pesudo_value, uint32_t lbt_available);
uint32_t lbt_update_channel(uint32_t tx_us, uint8_t tx_channel);
void lbt_check_time();

/* RTC */
void RTC_ModifyTime(uint8_t year, uint8_t month, uint8_t date, uint8_t day, uint8_t hour, uint8_t mintue, uint8_t second);
Chirp_Time RTC_GetTime(void);
void RTC_Waiting(uint16_t start_year, uint8_t start_month, uint8_t start_date, uint8_t start_hour, uint8_t start_min, uint8_t start_sec);
void RTC_Waiting_Count_Stop(uint32_t Count_wait);
void RTC_Waiting_Count_Sleep(uint32_t Count_wait);

#endif  /* __LORADISC_H__ */
