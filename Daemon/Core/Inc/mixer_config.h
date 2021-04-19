#ifndef __MIXER_CONFIG_H__
#define __MIXER_CONFIG_H__
// mixer configuration file
// Adapt the settings to the needs of your application.

#include "gpi/platform_spec.h"		// GPI_ARCH_IS_...
#include "gpi/tools.h"				// NUM_ELEMENTS()

#define DEBUG_CHIRPBOX 1
#if DEBUG_CHIRPBOX

#define PRINTF_CHIRP(...) printf(__VA_ARGS__)
#else
#define PRINTF_CHIRP(...)
#endif

/*radio---------------------------------------------------------------------------*/
#define REGION_CN470				// Frequency by country
#define USE_MODEM_LORA				// Radio modem
#define CHANNEL_MAX                                9
#define CHANNEL_MIN                                0
#define CHANNEL_STEP                               200000

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
/*config---------------------------------------------------------------------------*/
uint8_t *payload_distribution;

// turn verbose log messages on or off
#define MX_VERBOSE_CONFIG		0
#define MX_VERBOSE_STATISTICS	1
#define MX_VERBOSE_PACKETS		1

/*********************************************************/
#define ENERGEST_CONF_ON        1

/*********************************************************/
#define ROUND_HEADER_LENGTH 	        4   /* Data section to represent the round number and node id */
#define DATA_HEADER_LENGTH     	        8   /* Reservation section to transmit commands, real data is written behind that section */
#define HASH_HEADER            	        4
#define HASH_TAIL            	        2
#define HASH_TAIL_CODE            	    2

#define DISC_HEADER            	        0x1234
#define FLOODING_HEADER            	    0x5678
/*********************************************************/
#define MX_GENERATION_SIZE_MAX  0xFF /* 255 packets */
#define MX_NUM_NODES_MAX        0xFF /* 255 nodes */

#define LBT_CHANNEL_NUM         10

#define LBT_DELAY_IN_US         10000
#define CHANNEL_ALTER           2
#define LBT_CCA_TIME            5000
#define LBT_CCA_STEP            500
#define LBT_CCA_STEP_NUM        10

#define LBT_TX_TIME_S           3600

#define DISSEM_MAX              32
#define DISSEM_BITMAP_32        ((DISSEM_MAX + 32 - 1) / 32)

#endif /* __MIXER_CONFIG_H__ */
