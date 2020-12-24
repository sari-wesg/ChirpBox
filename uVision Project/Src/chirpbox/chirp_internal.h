
#ifndef __CHIRPBOX_INTERNAL_H__
#define __CHIRPBOX_INTERNAL_H__

//**************************************************************************************************
//***** Includes ***********************************************************************************
#include "stdint.h"
#include <time.h>

// #include "mixer_internal.h"
#include "gpi/interrupts.h"

#include "gpi/clocks.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "stm32l4xx_hal.h"
#include "trace_flash.h"
//**************************************************************************************************
//***** Global (Public) Defines and Consts *********************************************************

typedef enum Chirp_ISR_tag  /* For allocate isr functions */
{
	ISR_MIXER = 0,
	ISR_TOPO  = 4,
	ISR_GPS   = 8,
} Chirp_ISR;

//**************************************************************************************************

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

 /* TIMER3 */
#define DOG_TIMER htim5.Instance
#define DOG_TIMER_IRQ TIM5_IRQn
#define DOG_TIMER_ISR_NAME TIM5_IRQHandler

#define DOG_TIMER_CC_REG (DOG_TIMER->CCR1)  /* compare interrupt count */
#define DOG_TIMER_CNT_REG (DOG_TIMER->CNT)  /* timer3 now count */

 /* RTC */
#define RTC_TIMER hrtc.Instance
#define RTC_TIMER_ISR_NAME RTC_WKUP_IRQHandler
//**************************************************************************************************
//***** Local (Private) Defines and Consts *********************************************************

//**************************************************************************************************
//***** Forward Class and Struct Declarations ******************************************************

//**************************************************************************************************
//***** Global Typedefs and Class Declarations *****************************************************

typedef enum Mixer_Task_tag
{
	/* copy packet: CHIRP_START, CHIRP_CONNECTIVITY, MX_ARRANGE */
	/* all to all: MX_COLLECT, CHIRP_TOPO, CHIRP_VERSION */
	/* one to all: MX_DISSEMINATE */
	CHIRP_START,
	MX_DISSEMINATE,
	MX_COLLECT,
	CHIRP_CONNECTIVITY,
	CHIRP_TOPO,
	CHIRP_VERSION,

	MX_ARRANGE,

	MX_TASK_FIRST = CHIRP_START,
	MX_TASK_LAST = MX_ARRANGE - 1,
	MX_GLOSSY = MX_TASK_LAST + 1 + 1
} Mixer_Task;

typedef struct Chirpbox_tag
{
	Chirp_ISR state; /* the task/state in execution */
} Chirpbox_ISR;

//GPS / RTC ********************************************************************************************

typedef struct Chirp_Time_tag
{
	uint16_t		chirp_year;
	uint8_t			chirp_month;
	uint8_t			chirp_date;
	uint8_t			chirp_day;
	uint8_t			chirp_hour;
	uint8_t			chirp_min;
	uint8_t			chirp_sec;
} Chirp_Time;

//topology ********************************************************************************************

typedef struct Topology_result_tag
{
	uint8_t			rx_num;
	uint16_t		reliability;
} Topology_result;

typedef struct Topology_result_link_tag
{
	int16_t			snr_link;
	int16_t			rssi_link;
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
//***** Global Variables ***************************************************************************
/* main */
uint8_t MX_NUM_NODES_CONF;
extern uint8_t node_id_allocate;

/* ISR */
extern Chirpbox_ISR chirp_isr;

/* gps */
extern uint8_t node_id_allocate;

/* stats */
extern Chirp_Stats_All chirp_stats_all;
extern Chirp_Energy chirp_stats_all_debug;

//**************************************************************************************************
//***** Prototypes of Global Functions *************************************************************
void SX1276OnDio0Irq();
void SX1276OnDio3Irq();

//**************************************************************************************************
/* GPS */
void GPS_Init();
void GPS_On();
void GPS_Off();
void GPS_Uart_Irq();
Chirp_Time GPS_Get_Time();
time_t GPS_Conv(uint16_t year, uint8_t month, uint8_t date, uint8_t hour, uint8_t min, uint8_t sec);
time_t GPS_Diff(Chirp_Time *gps_time, uint16_t start_year, uint8_t start_month, uint8_t start_date, uint8_t start_hour, uint8_t start_min, uint8_t start_sec);
void GPS_Waiting(uint16_t start_year, uint8_t start_month, uint8_t start_date, uint8_t start_hour, uint8_t start_min, uint8_t start_sec);
void GPS_Waiting_PPS(uint32_t PPS_wait);
void GPS_Wakeup(uint32_t interval_sec);
void GPS_Sleep(uint32_t interval_sec);
void gps_main_timer_isr(void);

/* Topology */
uint32_t topo_init(uint8_t nodes_num, uint8_t node_id, uint8_t sf, uint8_t payload_len);
Gpi_Fast_Tick_Extended topo_round_robin(uint8_t node_id, uint8_t nodes_num, uint8_t i, Gpi_Fast_Tick_Extended deadline);
void topo_result(uint8_t nodes_num);
void topo_dio0_isr();
void topo_main_timer_isr();

/* Stats */
void Stats_value(uint8_t stats_type, uint32_t value);
void Stats_value_debug(uint8_t energy_type, uint32_t value);
void Stats_to_Flash(Mixer_Task task);

/* LBT */
uint8_t lbt_pesudo_channel(uint8_t channel_total, uint8_t last_channel, uint16_t pesudo_value, uint32_t lbt_available);
uint32_t lbt_update_channel(uint32_t tx_us, uint8_t tx_channel);
void lbt_check_time();

/* RTC */
void RTC_ModifyTime(uint8_t year, uint8_t month, uint8_t date, uint8_t day, uint8_t hour, uint8_t mintue, uint8_t second);
Chirp_Time RTC_GetTime(void);
void RTC_Waiting(uint16_t start_year, uint8_t start_month, uint8_t start_date, uint8_t start_hour, uint8_t start_min, uint8_t start_sec);
void RTC_Waiting_Count(uint32_t Count_wait);
void RTC_Waiting_Count_Sleep(uint32_t Count_wait);

//**************************************************************************************************
/* Topology */

//**************************************************************************************************
//***** Implementations of Inline Functions ********************************************************

#endif // __CHIRPBOX_INTERNAL_H__
