#ifndef __MIXER_CONFIG_H__
#define __MIXER_CONFIG_H__
// mixer configuration file
// Adapt the settings to the needs of your application.

#include "gpi/platform_spec.h"		// GPI_ARCH_IS_...
#include "gpi/tools.h"				// NUM_ELEMENTS()

#ifndef PSEUDO_CONF
	#define PSEUDO_CONF						1
#endif

#define DEBUG_CHIRPBOX 1
#if DEBUG_CHIRPBOX

#define PRINTF_CHIRP(...) printf(__VA_ARGS__)
#else
#define PRINTF_CHIRP(...)
#endif

/*radio---------------------------------------------------------------------------*/
#define REGION_CN470				// Frequency by country
#define USE_MODEM_LORA				// Radio modem
#define CHANNEL_MAX                                7
#define CHANNEL_MIN                                0
#define CHANNEL_STEP                               200000

#if defined( USE_MODEM_LORA )
#if (!PSEUDO_CONF)
#define LORA_SPREADING_FACTOR                       7         // [SF7..SF12]
#define LORA_BANDWIDTH                              7         // [0: 7.8 kHz,
                                                              //  1: 10.4 kHz,
                                                              //  2:15.6 kHz,
                                                              //  3: 20.8 kHz,
                                                              //  4: 31.25 kHz,
                                                              //  5: 41.7 kHz,
                                                              //  6: 62.5 kHz,
                                                              //  7: 125 kHz,
                                                              //  8: 250 kHz,
                                                              //  9: 500 kHz]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define TX_OUTPUT_POWER                             14        // dBm
#define LORA_CHANNEL                                CHANNEL_MIN
#endif

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
// Payload size
#if (!PSEUDO_CONF)
#define MX_PAYLOAD_CONF_SIZE	        40
#endif

#if (!PSEUDO_CONF)
// Round size
#define MIXER_PERIOD_TIME_S                 5     // (s) mixer round time in seconds
#define MX_ROUND_CONF_LENGTH                25    // needed slot number
#endif

#if 1
// config 24: node_id in the network, here, each node in the network has one packet
    #if PSEUDO_CONF
    uint8_t *payload_distribution;
    #else
    static const uint32_t nodes[] = {0x4B0023, 0x530045, 0x210027, 0x260057};
    static const uint8_t payload_distribution[] = {0, 0, 0, 0, 0, 0};
    #endif
#endif

// basic settings
#if (!PSEUDO_CONF)

#ifdef MX_ROUND_CONF_LENGTH
#define MX_ROUND_LENGTH		MX_ROUND_CONF_LENGTH
#else
#define MX_ROUND_LENGTH		200		// in #slots
#endif

#define MX_NUM_NODES			NUM_ELEMENTS(nodes)
#define MX_GENERATION_SIZE		NUM_ELEMENTS(payload_distribution)
#ifdef MX_PAYLOAD_CONF_SIZE
#define MX_PAYLOAD_SIZE			MX_PAYLOAD_CONF_SIZE
#else
#define MX_PAYLOAD_SIZE			8
#endif
#endif

// choose a slot length according to your settings
// NOTE: Measurement unit is hybrid ticks.
// For the tutorial project we use quiet long slots to be able to trace all messages
// on the UART-based console.
// slot length
#if (!PSEUDO_CONF)
#define MX_SLOT_LENGTH_IN_US	150000
#define MX_SLOT_LENGTH			GPI_TICK_US_TO_HYBRID2(MX_SLOT_LENGTH_IN_US)
#endif

// turn verbose log messages on or off
#define MX_VERBOSE_CONFIG		0
#define MX_VERBOSE_STATISTICS	1
#define MX_VERBOSE_PACKETS		1

#if (!PSEUDO_CONF)
#define MIXER_INTERVAL  		((MIXER_PERIOD_TIME_S * 1000) / 1) - MX_ROUND_LENGTH * (MX_SLOT_LENGTH_IN_US / 1000)
#define MIXER_PERIOD			((MIXER_PERIOD_TIME_S * 1000) / 1)
#define UPDATE_PERIOD			GPI_TICK_MS_TO_HYBRID2(MIXER_INTERVAL)
#endif

/*********************************************************/
#define ENERGEST_CONF_ON        1

/*********************************************************/
#define ROUND_HEADER_LENGTH 	        4   /* Data section to represent the round number and node id */
#define DATA_HEADER_LENGTH     	        8   /* Reservation section to transmit commands, real data is written behind that section */
#define HASH_HEADER            	        4
#define HASH_TAIL            	        2
#define HASH_TAIL_CODE            	    2

#define CHIRP_HEADER            	    0x1234
#define GLOSSY_HEADER            	    0x5678
/*********************************************************/
#define MX_GENERATION_SIZE_MAX  0xFF /* 255 packets */
#define MX_NUM_NODES_MAX        0xFF /* 255 nodes */

#define DOG_PERIOD            	20

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
