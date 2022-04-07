
#ifndef __LORAWAN_INTERNAL_H__
#define __LORAWAN_INTERNAL_H__

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

//**************************************************************************************************
//***** Global (Public) Defines and Consts *********************************************************

typedef enum Chirp_ISR_tag  /* For allocate isr functions */
{
	ISR_MIXER = 0,
	ISR_LPWAN  = 4,
	ISR_LORAWAN  = 8,
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

//**************************************************************************************************
//***** Global Variables ***************************************************************************
/* main */
extern uint8_t node_id_allocate;

/* ISR */
extern Chirpbox_ISR chirp_isr;


//**************************************************************************************************
//***** Prototypes of Global Functions *************************************************************
void SX1276OnDio0Irq();
void SX1276OnDio3Irq();

//**************************************************************************************************
// for lpwan
// timer
void slow_tick_update();
void slow_tick_end();
void lpwan_lp_timer_isr();

// grid
void lpwan_grid_timer_init(uint32_t loradisc_gap_start_s);
void loradisc_grid_timer_init(uint8_t init, uint32_t loradisc_gap_start_s);

//**************************************************************************************************
//***** Implementations of Inline Functions ********************************************************

#endif // __LORAWAN_INTERNAL_H__
