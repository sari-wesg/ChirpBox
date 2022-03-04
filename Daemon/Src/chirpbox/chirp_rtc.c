//**************************************************************************************************
//**** Includes ************************************************************************************
#include "mixer_internal.h"

#include "chirp_internal.h"
#include "stm32l4xx_hal.h"

#ifdef MX_CONFIG_FILE
#include STRINGIFY(MX_CONFIG_FILE)
#endif
//**************************************************************************************************
//***** Local Defines and Consts *******************************************************************
#if DEBUG_CHIRPBOX
#include <stdio.h>
#include <stdlib.h>

#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

//**************************************************************************************************
//***** Local Typedefs and Class Declarations ******************************************************

//**************************************************************************************************
//***** Forward Declarations ***********************************************************************

//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************
volatile uint32_t rtc_count = 0;

//**************************************************************************************************
//***** Global Variables ***************************************************************************

//**************************************************************************************************
//***** Local Functions ****************************************************************************

void RTC_Wakeup_Enable()
{
    /* Clear the EXTI's line Flag for RTC WakeUpTimer */
    __HAL_RTC_WAKEUPTIMER_EXTI_CLEAR_FLAG();
    __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&hrtc, RTC_FLAG_WUTF);

    /* Configure the Wakeup Timer counter */
    // 2048 = 32768 (LSE) / 16 (RTC_WAKEUPCLOCK_RTCCLK_DIV16)
    // set 1 second
    HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 2048, RTC_WAKEUPCLOCK_RTCCLK_DIV16);
}

void RTC_TIMER_ISR_NAME(void)
{
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

    /* Clear the EXTI's line Flag for RTC WakeUpTimer */
    __HAL_RTC_WAKEUPTIMER_EXTI_CLEAR_FLAG();
    __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&hrtc, RTC_FLAG_WUTF);

    gpi_watchdog_periodic();
    rtc_count++;
}

//**************************************************************************************************
//***** Global Functions ***************************************************************************

/**
  * @brief  Modify the time of timing register
  * @param  year: 00–99, the year added with 2000 is the actual year
  * @param  month: 01–12
  * @param  date: 01–31
  * @param  day: 1–7, the week serial number: Monday, Tuesday, Wednesday...
  * @param  hour: 00–23 (default)
  * @param  mintue: 00–59
  * @param  second: 00–59
  * @retval None
  */
void RTC_ModifyTime(uint8_t year, uint8_t month, uint8_t date, uint8_t day, uint8_t hour, uint8_t mintue, uint8_t second)
{
    RTC_TimeTypeDef nTime;
    RTC_DateTypeDef nDate;

    nTime.Hours = hour;
    nTime.Minutes = mintue;
    nTime.Seconds = second;
    nDate.WeekDay = day;
    nDate.Month = month;
    nDate.Date = date;
    nDate.Year = year;
    HAL_RTC_SetTime(&hrtc, &nTime, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&hrtc, &nDate, RTC_FORMAT_BIN);
}

Chirp_Time RTC_GetTime(void)
{
    Chirp_Time RTC_Time;
    RTC_TimeTypeDef nTime;
    RTC_DateTypeDef nDate;
    HAL_RTC_GetTime(&hrtc, &nTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &nDate, RTC_FORMAT_BIN);
    RTC_Time.chirp_year = 2000 + nDate.Year;
    RTC_Time.chirp_month = nDate.Month;
    RTC_Time.chirp_date = nDate.Date;
    RTC_Time.chirp_day = nDate.WeekDay;
    RTC_Time.chirp_hour = nTime.Hours;
    RTC_Time.chirp_min = nTime.Minutes;
    RTC_Time.chirp_sec = nTime.Seconds;
	PRINTF("RTC_GetTime: %d-%d-%d %d:%d:%d week: %d\n", RTC_Time.chirp_year, RTC_Time.chirp_month, RTC_Time.chirp_date, RTC_Time.chirp_hour, RTC_Time.chirp_min, RTC_Time.chirp_sec, RTC_Time.chirp_day);

    return RTC_Time;
}

void RTC_Waiting(uint16_t start_year, uint8_t start_month, uint8_t start_date, uint8_t start_hour, uint8_t start_min, uint8_t start_sec)
{
    Chirp_Time RTC_Wait_Time = RTC_GetTime();
    time_t diff = GPS_Diff(&RTC_Wait_Time, start_year, start_month, start_date, start_hour, start_min, start_sec);

    RTC_Wakeup_Enable();

    if (diff > 0)
    {
        rtc_count = 0;
        while (rtc_count <= diff)
        {
            PRINTF("rtc:%lu\n", rtc_count);
            // enter low-power mode
            gpi_int_disable();

            // gpi_sleep();
            HAL_PWREx_EnterSTOP1Mode(PWR_STOPENTRY_WFI);

            /* wake by the timer interrupt */
            gpi_int_enable();
            SystemClock_Config();
        }
    }
    rtc_count = 0;

    /* Disable the Wakeup Timer */
    __HAL_RTC_WAKEUPTIMER_DISABLE(&hrtc);

    /* In case of interrupt mode is used, the interrupt source must disabled */
    __HAL_RTC_WAKEUPTIMER_DISABLE_IT(&hrtc, RTC_IT_WUT);
}

// Stop mode:
void RTC_Waiting_Count_Stop(uint32_t Count_wait)
{
    PRINTF("Stop_Count:%lu\n", Count_wait);

    if (Count_wait > 1)
    {
        Count_wait = Count_wait;
        RTC_Wakeup_Enable();
        {
            rtc_count = 0;
            while (rtc_count <= Count_wait)
            {
                PRINTF("rtc:%lu\n", rtc_count);
                // enter low-power mode
                gpi_int_disable();

                // gpi_sleep();
                HAL_PWREx_EnterSTOP1Mode(PWR_STOPENTRY_WFI);

                /* wake by the timer interrupt */
                gpi_int_enable();
                SystemClock_Config();
            }
        }
        rtc_count = 0;

        /* Disable the Wakeup Timer */
        HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
    }
}

// Sleep mode:
void RTC_Waiting_Count_Sleep(uint32_t Count_wait)
{
    PRINTF("Sleep_Count:%lu\n", Count_wait);

    if (Count_wait > 1)
    {
        Count_wait = Count_wait;
        RTC_Wakeup_Enable();
        {
            rtc_count = 0;
            while (rtc_count <= Count_wait)
            {
                PRINTF("rtc:%lu\n", rtc_count);
                // enter low-power mode
                gpi_int_disable();

                #if ENERGEST_CONF_ON
                ENERGEST_OFF(ENERGEST_TYPE_CPU);
                ENERGEST_ON(ENERGEST_TYPE_LPM);
                #endif

                gpi_sleep();
                // HAL_PWREx_EnterSTOP1Mode(PWR_STOPENTRY_WFI);

                #if ENERGEST_CONF_ON
                ENERGEST_ON(ENERGEST_TYPE_CPU);
                ENERGEST_OFF(ENERGEST_TYPE_LPM);
                #endif

                /* wake by the timer interrupt */
                gpi_int_enable();
                // SystemClock_Config();
            }
        }
        rtc_count = 0;

        /* Disable the Wakeup Timer */
        HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
    }
}

//**************************************************************************************************
