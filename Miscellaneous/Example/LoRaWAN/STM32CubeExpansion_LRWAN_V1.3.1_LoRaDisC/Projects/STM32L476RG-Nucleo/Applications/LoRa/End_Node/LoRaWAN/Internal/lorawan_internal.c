//**************************************************************************************************
//**** Includes ************************************************************************************

//**************************************************************************************************
#include "hw.h"
#include "lora.h"

//**************************************************************************************************
//***** Local Defines and Consts *******************************************************************

//**************************************************************************************************
//***** Local Typedefs and Class Declarations ******************************************************
typedef enum Slot_State_tag
{
	LORAWAN_RUNNING	    = 0,
	LORADISC_RUNNING	= 4,
} Slot_State;


//**************************************************************************************************
//***** Forward Declarations ***********************************************************************

//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************
static struct
{
	Slot_State				slot_state;

	Gpi_Slow_Tick_Extended	loradisc_gap_start;

	Gpi_Slow_Tick_Extended	next_grid_tick;
} s;

//**************************************************************************************************
//***** Global Variables ***************************************************************************
extern LoraFlagStatus AppProcessRequest;

//**************************************************************************************************
//***** Local Functions ****************************************************************************

// trigger grid timer (immediately)
static inline void trigger_lowpower_timer(int use_int_lock)
{
	register int	ie;

	if (use_int_lock)
		ie = gpi_int_lock();

	__HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_FLAG_CMPOK);
	LP_TIMER_CMP_REG = LP_TIMER_CNT_REG + 3;
	while (!(__HAL_LPTIM_GET_FLAG(&hlptim1, LPTIM_FLAG_CMPOK)));

	if (use_int_lock)
		gpi_int_unlock(ie);
}
//**************************************************************************************************

static inline void unmask_slow_timer(int clear_pending)
{
	if (clear_pending)
	{
		NVIC_ClearPendingIRQ(LP_TIMER_IRQ);
    }

    __HAL_LPTIM_ENABLE_IT(&hlptim1, LPTIM_IT_CMPM);
}

static inline void mask_slow_timer()
{
    __HAL_LPTIM_DISABLE_IT(&hlptim1, LPTIM_IT_CMPM);
}
//**************************************************************************************************

static void start_grid_timer()
{

}

//**************************************************************************************************
//***** Global Functions ***************************************************************************
void slow_tick_update()
{
    // disable lptim interrupt
    __HAL_LPTIM_DISABLE_IT(&hlptim1, LPTIM_IT_CMPM);
	__HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_FLAG_CMPM);

    // read the extended slow timer
    gpi_tick_slow_extended();

    __HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_FLAG_CMPOK);
    // set the interrupt time
    LP_TIMER_CMP_REG = LP_TIMER_CNT_REG + 0xF000ul;

    // enable lptim interrupt
    while (!(__HAL_LPTIM_GET_FLAG(&hlptim1, LPTIM_FLAG_CMPOK)));
    __HAL_LPTIM_ENABLE_IT(&hlptim1, LPTIM_IT_CMPM);
}

void slow_tick_end()
{
    // disable lptim interrupt
    __HAL_LPTIM_DISABLE_IT(&hlptim1, LPTIM_IT_CMPM);
	__HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_FLAG_CMPM);

    // read the extended slow timer
    gpi_tick_slow_extended();
}

// // LPTIM IRQ
// void lpwan_lp_timer_isr()
// {
//     slow_tick_update();
// }

// LPTIM IRQ
void lpwan_lp_timer_isr()
{
	mask_slow_timer();
	__HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_FLAG_CMPM);

    Gpi_Slow_Tick_Extended	r;
    Gpi_Slow_Tick_Extended	d;

    r = gpi_tick_slow_extended();

    d = s.next_grid_tick - r;

    __HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_FLAG_CMPOK);

    if ((int32_t)d < 0)
    {
        AppProcessRequest = LORA_SET;
        #if ENERGEST_CONF_ON
            ENERGEST_OFF(ENERGEST_TYPE_CPU);

            ENERGEST_OFF(ENERGEST_TYPE_LPM);
            ENERGEST_ON(ENERGEST_TYPE_CPU);
        #endif
    }
    else if (d > 0xF000ul)
    {
        LP_TIMER_CMP_REG = LP_TIMER_CNT_REG + 0xF000ul;
        while (!(__HAL_LPTIM_GET_FLAG(&hlptim1, LPTIM_FLAG_CMPOK)));
        unmask_slow_timer(0);
    }
    else
    {
        LP_TIMER_CMP_REG = LP_TIMER_CNT_REG + d;
        while (!(__HAL_LPTIM_GET_FLAG(&hlptim1, LPTIM_FLAG_CMPOK)));
        unmask_slow_timer(0);
    }
}


void lpwan_grid_timer_init(Gpi_Slow_Tick_Extended loradisc_gap_start)
{
	s.slot_state = LORAWAN_RUNNING;

    s.loradisc_gap_start = loradisc_gap_start;

    s.next_grid_tick = gpi_tick_slow_extended() + s.loradisc_gap_start;

    #if ENERGEST_CONF_ON
        ENERGEST_OFF(ENERGEST_TYPE_LPM);

        ENERGEST_ON(ENERGEST_TYPE_LPM);
        ENERGEST_OFF(ENERGEST_TYPE_CPU);
    #endif
    /* set timer */
	mask_slow_timer();
	__HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_FLAG_CMPM);
    __HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_FLAG_CMPOK);
    LP_TIMER_CMP_REG = LP_TIMER_CNT_REG + GPI_TICK_MS_TO_SLOW(1);
    while (!(__HAL_LPTIM_GET_FLAG(&hlptim1, LPTIM_FLAG_CMPOK)));

    REORDER_BARRIER();
    __DMB();

	// unmask grid timer interrupt
	unmask_slow_timer(1);
}

void lpwan_grid_timer_stop()
{
    mask_slow_timer();
    __HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_FLAG_CMPM);
    __HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_FLAG_CMPOK);
}