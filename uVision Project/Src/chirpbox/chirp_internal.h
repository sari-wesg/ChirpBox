
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
//**************************************************************************************************
//***** Global (Public) Defines and Consts *********************************************************

typedef enum Chirp_ISR_tag  /* For allocate isr functions */
{
	ISR_MIXER = 0,
	ISR_SNIFF = 4,
	ISR_TOPO  = 8,
	ISR_GPS   = 12,
} Chirp_ISR;

//**************************************************************************************************
typedef enum Sniff_Net_tag
{
	STATE_LORAWAN = 0, /* sniff LoRaWAN Network */
	STATE_LORA_FORM,   /* sniff LoRa Network with knowing the packet format in advance */
} Sniff_Net;

typedef enum Sniff_State_tag  /* State for timer */
{
	SNIFF_CAD_DETECT = 0,
	SNIFF_VALID_HEADER,
} Sniff_State;

typedef enum Sniff_Radio_tag  /* State of radio */
{
	SNIFF_RX = 0,
	SNIFF_TX,
	SNIFF_CAD,
} Sniff_Radio;

typedef enum Sniff_Trigger_tag  /* Trigger that jump out of sniff */
{
	SNIFF_TIME_TRIGGER = 0,
	SNIFF_RADIO_TRIGGER,
} Sniff_Trigger;
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
#define LP_TIMER_CNT_REG (LP_TIMER->CNT)  /* timer2 now count */

 /* TIMER3 */
#define DOG_TIMER htim5.Instance
#define DOG_TIMER_IRQ TIM5_IRQn
#define DOG_TIMER_ISR_NAME TIM5_IRQHandler

#define DOG_TIMER_CC_REG (DOG_TIMER->CCR1)  /* compare interrupt count */
#define DOG_TIMER_CNT_REG (DOG_TIMER->CNT)  /* timer3 now count */
//**************************************************************************************************
//***** Local (Private) Defines and Consts *********************************************************

//**************************************************************************************************
//***** Forward Class and Struct Declarations ******************************************************

//**************************************************************************************************
//***** Global Typedefs and Class Declarations *****************************************************

typedef enum Mixer_Task_tag
{
	/* copy packet: CHIRP_START, CHIRP_CONNECTIVITY, CHIRP_SNIFF, MX_ARRANGE */
	/* all to all: MX_COLLECT, CHIRP_TOPO, CHIRP_VERSION */
	/* one to all: MX_DISSEMINATE */
	CHIRP_START,
	MX_DISSEMINATE,
	MX_COLLECT,
	CHIRP_CONNECTIVITY,
	CHIRP_TOPO,
	CHIRP_SNIFF,
	CHIRP_VERSION,

	MX_ARRANGE,

	MX_TASK_FIRST = CHIRP_START,
	MX_TASK_LAST = MX_ARRANGE - 1
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

//sniff ********************************************************************************************
typedef struct Sniff_Time_tag
{
	uint8_t Hour;
	uint8_t Minute;
	uint8_t Second;
} Sniff_Time;

typedef struct Sniff_stat_tag
{
	uint32_t node_id;			 /* detected node_id */
	uint32_t radio_on_time;		 /* total radio on time (us) in 1 hour */
	Sniff_Time last_active_time; /* The last active time of radio on */
	Sniff_Time begin_stat_time;	 /* The begin time of calculating the radio on time */
	struct Sniff_stat_tag *next;
} Sniff_stat;

typedef struct __attribute__((packed)) Sniff_RF_tag
{
	uint8_t sf;
	uint8_t bw;
	uint8_t cr;
	uint8_t preamble;
	uint8_t tx_power;
	uint32_t frequency;
} Sniff_RF;

typedef struct Sniff_tag
{
	Sniff_Net net;
	Sniff_State state; /* the task/state in execution */
	Sniff_Radio radio; /* for choose the corresponding handler function */
	Sniff_RF rf;
	Gpi_Fast_Tick_Native sf_switch;
	Chirp_Time sniff_end;
} Sniff;

typedef struct __attribute__((packed)) Sniff_Config_tag
{
	uint8_t	sniff_id;
	uint32_t sniff_freq_khz;
} Sniff_Config;
//**************************************************************************************************
//***** Global Variables ***************************************************************************
/* main */
uint8_t MX_NUM_NODES_CONF;
extern uint8_t node_id_allocate;

/* ISR */
extern Chirpbox_ISR chirp_isr;

/* gps */
extern uint8_t node_id_allocate;

/* sniff */
extern Sniff_stat expired_node;
//**************************************************************************************************
//***** Prototypes of Global Functions *************************************************************
void SX1276OnDio0Irq();
void SX1276OnDio3Irq();

//**************************************************************************************************
/* GPS */
void GPS_Init();
void GPS_Uart_Irq();
Chirp_Time GPS_Get_Time();
time_t GPS_Conv(uint16_t year, uint8_t month, uint8_t date, uint8_t hour, uint8_t min, uint8_t sec);
time_t GPS_Diff(Chirp_Time *gps_time, uint16_t start_year, uint8_t start_month, uint8_t start_date, uint8_t start_hour, uint8_t start_min, uint8_t start_sec);
void GPS_Waiting(uint16_t start_year, uint8_t start_month, uint8_t start_date, uint8_t start_hour, uint8_t start_min, uint8_t start_sec);
void GPS_Wakeup(uint32_t interval_sec);
void GPS_Sleep(uint32_t interval_sec);
void gps_main_timer_isr(void);

/* Topology */
uint32_t topo_init(uint8_t nodes_num, uint8_t node_id, uint8_t sf);
Gpi_Fast_Tick_Extended topo_round_robin(uint8_t node_id, uint8_t nodes_num, uint8_t i, Gpi_Fast_Tick_Extended deadline);
void topo_result(uint8_t nodes_num);
void topo_dio0_isr();

/* Sniff */
void sniff_init(Sniff_Net LoRa_Net, uint32_t lora_frequency, uint16_t end_year, uint8_t end_month, uint8_t end_date, uint8_t end_hour, uint8_t end_min, uint8_t end_sec);
void sniff_cad();
void sniff_rx();
void sniff_tx(uint32_t node_id);
void chirp_dio0_isr();
void chirp_dio3_isr();

//**************************************************************************************************
/* Topology */

//**************************************************************************************************
//***** Implementations of Inline Functions ********************************************************

#endif // __CHIRPBOX_INTERNAL_H__
