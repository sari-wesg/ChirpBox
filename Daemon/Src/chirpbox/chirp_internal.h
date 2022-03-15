
#ifndef __CHIRPBOX_INTERNAL_H__
#define __CHIRPBOX_INTERNAL_H__

//**************************************************************************************************
//***** Includes ***********************************************************************************
#include "stdint.h"

#include "gpi/interrupts.h"

#include "gpi/clocks.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "stm32l4xx_hal.h"
#include "API_ChirpBox.h"
#include "mixer_config.h"

//**************************************************************************************************
//***** Global (Public) Defines and Consts *********************************************************

typedef enum Chirp_ISR_tag  /* For allocate isr functions */
{
	ISR_MIXER = 0,
	ISR_TOPO  = 4,
	ISR_LORAWAN  = 8,
} Chirp_ISR;

//**************************************************************************************************
/* chirpbox config */
/* Only allowed to run in the daemon bank (bank1), e.g. alarm clearing and flash write protection removal. */
#ifndef DAEMON_BANK
	#define DAEMON_BANK								1
#endif

#ifndef GPS_DATA
	#define GPS_DATA								1
	#define DS3231_ON								1
#endif

/* DATA config */
#define DISSEM_MAX              32
#define DISSEM_BITMAP_32        ((DISSEM_MAX + 32 - 1) / 32)

/* Round config */
#define ROUND_SETUP             1

/* TOPO */
#ifndef TOPO_DEFAULT_SF
	#define TOPO_DEFAULT_SF			7	// The LoRa default under test spreading factor is 7
#endif

/* TIMER2 */
#define MAIN_TIMER htim2.Instance
#define MAIN_TIMER_IRQ TIM2_IRQn
#define MAIN_TIMER_ISR_NAME TIM2_IRQHandler

#define MAIN_TIMER_CC_REG (MAIN_TIMER->CCR1)  /* compare interrupt count */
#define MAIN_TIMER_CNT_REG (MAIN_TIMER->CNT)  /* timer2 now count */

/* LPTIM1 */
#define LP_TIMER hlptim1.Instance
#define LP_TIMER_IRQ LPTIM1_IRQn
#define LP_TIMER_ISR_NAME LPTIM1_IRQHandler

#define LP_TIMER_CMP_REG (LP_TIMER->CMP)  /* compare interrupt count */
#define LP_TIMER_CNT_REG (LP_TIMER->CNT)  /* lptim1 now count */

/* RTC */
#define RTC_TIMER hrtc.Instance
#define RTC_TIMER_ISR_NAME RTC_WKUP_IRQHandler
//**************************************************************************************************
//***** Local (Private) Defines and Consts *********************************************************

//**************************************************************************************************
//***** Forward Class and Struct Declarations ******************************************************

//**************************************************************************************************
//***** Global Typedefs and Class Declarations *****************************************************

typedef enum ChirpBox_Task_tag
{
	/* copy packet: CB_START, CB_CONNECTIVITY, CB_GLOSSY_ARRANGE */
	/* all to all: CB_COLLECT, CB_VERSION */
	/* one to all: CB_DISSEMINATE */
	CB_START,
	CB_DISSEMINATE,
	CB_COLLECT,
	CB_CONNECTIVITY,
	CB_VERSION,

	/* Note: do not change the sequence! */
	CB_GLOSSY_ARRANGE,
	CB_GLOSSY_SYNCHRONIZED,
	CB_GLOSSY,

	CB_TASK_FIRST = CB_START,
	CB_TASK_LAST = CB_GLOSSY_ARRANGE - 1,
} ChirpBox_Task;

typedef enum ChirpBox_ArrangeDataLength_tag
{
	/* copy packet: CB_START, CB_CONNECTIVITY, CB_GLOSSY_ARRANGE */
	/* all to all: CB_COLLECT, CB_VERSION */
	/* one to all: CB_DISSEMINATE */
	CB_START_LENGTH = 0x04,
	CB_DISSEMINATE_LENGTH = 0x08,
	CB_COLLECT_LENGTH = 0x0D,
	CB_CONNECTIVITY_LENGTH = 0xFF,
	CB_VERSION_LENGTH = 0xFF,
	CB_GLOSSY_LENGTH = 1
} ChirpBox_ArrangeDataLength;

typedef struct Chirpbox_tag
{
	Chirp_ISR state; /* the task/state in execution */
} Chirpbox_ISR;

//topology ********************************************************************************************
typedef struct Topology_result_tag
{
	uint8_t			rx_num;
} Topology_result;

typedef struct Topology_result_link_tag
{
	uint16_t		reliability;
	int8_t			snr_link_min;
	int8_t			snr_link_max;
	int16_t			rssi_link_min;
	int16_t			rssi_link_max;
	int16_t			snr_total;
	int16_t			rssi_total;
} Topology_result_link;

//Stats ********************************************************************************************

typedef enum Chirp_Stats_Type_tag
{
	SLOT_STATS,
	RX_STATS,
	TX_STATS
} Chirp_Stats_Type;

typedef struct Chirp_Stats_tag
{
	uint32_t		stats_sum;
	uint32_t		stats_count;
	uint32_t		stats_min;
	uint32_t		stats_max;
	uint32_t		stats_none;
} Chirp_Stats;

typedef struct Chirp_Stats_All_tag
{
	Chirp_Stats		slot;
	Chirp_Stats		rx_on;
	Chirp_Stats		tx_on;
} Chirp_Stats_All;


typedef struct __attribute__((packed)) Chirp_Energy_tag
{
	uint32_t CPU;
	uint32_t LPM;
	uint32_t STOP;

	uint32_t FLASH_WRITE_BANK1;
	uint32_t FLASH_WRITE_BANK2;
	uint32_t FLASH_ERASE;
	uint32_t FLASH_VERIFY;

	uint32_t TRANSMIT;
	uint32_t LISTEN;

	uint32_t GPS;
} Chirp_Energy;

//**************************************************************************************************
typedef struct __attribute__((packed)) Chirp_Outline_tag
{
	ChirpBox_Task 		task;
	ChirpBox_Task 		arrange_task;

	uint16_t			round; 				/* current round num */
	uint16_t			round_max; 			/* desired round num to carriage task */
	uint8_t				round_setup_to_delete; 		/* setup round for all nodes synchronization */

	uint32_t			packet_time;
	uint16_t			default_slot_num;
	uint32_t			default_sf;
	uint32_t			default_freq;
	int8_t				default_tp;
	uint8_t				default_payload_len;
	uint8_t				default_generate_size;
	uint32_t			firmware_bitmap[DISSEM_BITMAP_32];
	uint32_t			task_bitmap[DISSEM_BITMAP_32];

	// send back the results in dissem
	uint8_t				dissem_back_sf;
	uint8_t				dissem_back_slot_num;

	uint32_t			hash_header;

	uint8_t				glossy_resync;
	uint8_t				glossy_gps_on;

	/* CB_START: mixer config */
	uint16_t			start_year;
	uint8_t				start_month;
	uint8_t				start_date;
	uint8_t				start_hour;
	uint8_t				start_min;
	uint8_t				start_sec;

	uint16_t			end_year;
	uint8_t				end_month;
	uint8_t				end_date;
	uint8_t				end_hour;
	uint8_t				end_min;
	uint8_t				end_sec;

	uint8_t				flash_protection;

	/* CB_DISSEMINATE / CB_COLLECT : mixer config */
	uint8_t				num_nodes;
	uint8_t				generation_size;
	uint8_t				payload_len; /* Payload length other than the header of the packet, used for flooding/dissemination/collection, range 0~255-8(header)-2(hash code) */
	uint16_t 			file_chunk_len;

	/* CB_DISSEMINATE */
	uint32_t			firmware_size;
	uint8_t				firmware_md5[16];
	uint16_t			version_hash;

	uint32_t			file_compression;

	uint8_t				patch_update;
	uint8_t 			patch_bank;

	uint8_t				patch_page;
	uint32_t			old_firmware_size;

	/* CB_COLLECT */
	uint32_t			collect_addr_start;
	uint32_t			collect_addr_end;

	/* CB_CONNECTIVITY */
	uint8_t				sf_bitmap;
	uint32_t			freq;
	int8_t				tx_power;
	uint8_t				topo_payload_len;

	/* debug energy (address must be 32-bit) */
	Chirp_Energy		chirp_energy[3];
	// idle1, arrange1, start, idle2, arrange2, disfut, idle3, arrange3, collre, idle4, arrange4, connect, idle5, arrange5, colltopo
} Chirp_Outl;

//**************************************************************************************************
//***** Global Variables ***************************************************************************
/* main */
uint8_t MX_NUM_NODES_CONF;
extern uint8_t node_id_allocate;

/* ISR */
extern Chirpbox_ISR chirp_isr;

/* stats */
extern Chirp_Stats_All chirp_stats_all;
extern Chirp_Energy chirp_stats_all_debug;

//**************************************************************************************************
//***** Prototypes of Global Functions *************************************************************
void SX1276OnDio0Irq();
void SX1276OnDio3Irq();

//**************************************************************************************************
/* chirpbox */
void 		chirp_write(uint8_t node_id, Chirp_Outl *chirp_outl);
uint8_t 	chirp_recv(uint8_t node_id, Chirp_Outl *chirp_outl);
uint8_t		chirp_round(uint8_t node_id, Chirp_Outl *chirp_outl);

/* Topology */
uint32_t topo_init(uint8_t nodes_num, uint8_t node_id, uint8_t sf, uint8_t payload_len);
void topo_round_robin(uint8_t node_id, uint8_t nodes_num, uint8_t i);
void topo_result(uint8_t nodes_num, uint8_t topo_test_id);
void topo_dio0_isr();
void topo_main_timer_isr();
void topo_manager(uint8_t nodes_num, uint8_t node_id, uint8_t sf_bitmap, uint8_t payload_len);

/* Stats */
void Stats_value(uint8_t stats_type, uint32_t value);
void Stats_value_debug(uint8_t energy_type, uint32_t value);
void Stats_to_Flash(ChirpBox_Task task);

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

//**************************************************************************************************
/* Topology */

//**************************************************************************************************
//***** Implementations of Inline Functions ********************************************************

#endif // __CHIRPBOX_INTERNAL_H__
