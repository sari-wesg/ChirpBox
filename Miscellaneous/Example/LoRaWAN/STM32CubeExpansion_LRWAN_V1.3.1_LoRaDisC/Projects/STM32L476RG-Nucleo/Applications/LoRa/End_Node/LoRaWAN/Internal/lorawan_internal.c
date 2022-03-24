
#include "hw.h"

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

// LPTIM IRQ
void lpwan_lp_timer_isr()
{
    slow_tick_update();
}
