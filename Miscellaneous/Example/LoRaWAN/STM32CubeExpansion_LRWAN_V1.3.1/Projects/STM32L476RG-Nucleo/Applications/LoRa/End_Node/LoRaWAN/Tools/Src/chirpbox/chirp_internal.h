
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
#include "API_trace_flash.h"
//**************************************************************************************************
//***** Global (Public) Defines and Consts *********************************************************

typedef enum Chirp_ISR_tag  /* For allocate isr functions */
{
	ISR_MIXER = 0,
	ISR_TOPO  = 4,
	ISR_GPS   = 8,
} Chirp_ISR;

#ifndef TOPO_DEFAULT_SF
	#define TOPO_DEFAULT_SF			7	// The LoRa default under test spreading factor is 7
#endif
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

 /* RTC */
#define RTC_TIMER hrtc.Instance
#define RTC_TIMER_ISR_NAME RTC_WKUP_IRQHandler
//**************************************************************************************************
//***** Local (Private) Defines and Consts *********************************************************

//**************************************************************************************************
//***** Forward Class and Struct Declarations ******************************************************

//**************************************************************************************************
//***** Global Typedefs and Class Declarations *****************************************************

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

//***** Global Variables ***************************************************************************
/* main */
uint8_t MX_NUM_NODES_CONF;
extern uint8_t node_id_allocate;

/* ISR */
extern Chirpbox_ISR chirp_isr;

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

//**************************************************************************************************
//***** Implementations of Inline Functions ********************************************************

#endif // __CHIRPBOX_INTERNAL_H__
