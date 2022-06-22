

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
#ifndef LORADISC //loradisc mode
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
#define REGION_CN470							   	// Frequency by country
#define USE_MODEM_LORA							   	// Radio modem
#define CHANNEL_MAX                                	9
#define CHANNEL_MIN                                	0
#define CHANNEL_STEP                               	200000
#define CN470_FREQUENCY							   	486300000


#define LORADISC_LATENCY						   	177320

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

/******************************* discover config ******************************/
#define LORAWAN_MAX_NUM         8
#define DISCOVER_SLOT_DEFAULT   40
#define LORAWAN_TIME_LENGTH		5 //TODO:
#define LORAWAN_DURATION_S		5
#define LORAWAN_GUARDTIME_S		0
#define LPM_TIMER_UPDATE_S		1000 /* Used only to keep lptimer interrupted at regular intervals */

/******************************* collect config ********************************/
#define LORADISC_COLLECT_LENGTH 9

/******************************* dissemination config **************************/



#define MAX(a,b) \
({ __typeof__ (a) _a = (a); \
	__typeof__ (b) _b = (b); \
	_a > _b ? _a : _b; })

#define MIN(a,b) \
({ __typeof__ (a) _a = (a); \
	__typeof__ (b) _b = (b); \
	_a <= _b ? _a : _b; })

//**************************************************************************************************
//***** Global Typedefs and Class Declarations *****************************************************
typedef enum Disc_Primitive_tag
{
	FLOODING = 1,
	DISSEMINATION = 2,
	COLLECTION = 3
} Disc_Primitive;

/* local config */
typedef struct __attribute__((packed)) LoRaDisC_Discover_tag
{
	/* defined infomation */
	Gpi_Slow_Tick_Extended 	discover_duration_slow;
	Gpi_Slow_Tick_Extended 	loradisc_duration_slow;
	Gpi_Slow_Tick_Extended 	lorawan_duration_slow;
	uint8_t					lorawan_num;	// total number of lorawan nodes
	uint32_t 				lorawan_bitmap; // node for lorawan
	Gpi_Slow_Tick_Extended 	lorawan_duration;
	uint16_t				loradisc_collect_id;

	/* local infomation */
	uint8_t					lorawan_on;
	uint8_t					collect_on;
	uint8_t					loradisc_lorawan_on;
	uint8_t					dissem_on;
	uint8_t					loradisc_on;
	uint8_t 				discover_on; // time for discover
	uint32_t 				node_id_bitmap; // discovered node id
	uint8_t					lorawan_relay_id;

	int32_t					lorawan_diff; // self compared to node 0
	Gpi_Slow_Tick_Extended	lorawan_begin[LORAWAN_MAX_NUM]; // the begin time of all nodes
	Gpi_Slow_Tick_Extended 	next_loradisc_gap;

	uint8_t					discover_slot;

	/* global infomation */
	uint16_t				lorawan_interval_s[LORAWAN_MAX_NUM];
} LoRaDisC_Discover_Config;


//**************************************************************************************************
//***** Global Variables ***************************************************************************

//**************************************************************************************************
//***** Prototypes of Global Functions *************************************************************
// loradisc
void loradisc_node_id();
void loradisc_init();
void loradisc_reconfig(uint8_t nodes_num, uint8_t data_length, Disc_Primitive primitive, uint8_t sf, uint8_t tp, uint32_t lora_frequency);
void loradisc_write(uint8_t i, uint8_t *data);
void loradisc_read(uint8_t *data);
void loradisc_start(Disc_Primitive primitive);
// Primitive
uint8_t loradisc_collect();
void loradisc_dissem();

// loradisc preparation
void loradisc_packet_write(uint8_t node_id, uint8_t *data);

// lorawan
void lorawan_listen_init(uint8_t node_id);
void lorawan_listen();
void lorawan_transmission();

void lorawan_main_timer_isr();
void lorawan_dio0_isr();
void lorawan_dio3_isr();


/* LBT */
void lbt_init();
void lbt_update();
uint8_t lbt_pesudo_channel(uint8_t channel_total, uint8_t last_channel, uint16_t pesudo_value, uint32_t lbt_available);
uint32_t lbt_update_channel(uint32_t tx_us, uint8_t tx_channel);
void lbt_check_time();

/* RTC */
void RTC_ModifyTime(uint8_t year, uint8_t month, uint8_t date, uint8_t day, uint8_t hour, uint8_t mintue, uint8_t second);
Chirp_Time RTC_GetTime(void);
void RTC_Waiting(uint16_t start_year, uint8_t start_month, uint8_t start_date, uint8_t start_hour, uint8_t start_min, uint8_t start_sec);
void RTC_Waiting_Count_Stop(uint32_t Count_wait);
void RTC_Waiting_Count_Sleep(uint32_t Count_wait);


/* Discovery */
void loradisc_discover_init();
void loradisc_discover_update_initiator();
void loradisc_discover(uint16_t lorawan_interval_s);
Gpi_Slow_Tick_Extended calculate_next_loradisc();
void calculate_next_lorawan();
uint8_t compare_discover_initiator_expired();
uint8_t reset_discover_slot_num();

/* Calculate time for LoRaDisC */
int check_combination_groups(int a[], int b[], const int N, const int M);
void generate_combination(int n, int m, int a[], int b[], const int N, const int M, Gpi_Slow_Tick_Extended loradisc_start_time[], Gpi_Slow_Tick_Extended loradisc_end_time[], Gpi_Slow_Tick_Extended loradisc_duration);
void select_LoRaDisC_time(uint8_t node_number, uint8_t time_length, Gpi_Slow_Tick_Extended loradisc_start_time[], Gpi_Slow_Tick_Extended loradisc_end_time[], Gpi_Slow_Tick_Extended loradisc_duration);

/* relay the loradisc packets with lorawan */
uint32_t lorawan_relay_node_id_allocate(uint8_t relay_id);
uint8_t lorawan_relay_collect(uint8_t relay_id);
uint8_t lorawan_relay_max_packetnum();

int lorawan_loradisc_send(uint8_t *data, uint8_t data_length);

#endif  /* __LORADISC_H__ */
