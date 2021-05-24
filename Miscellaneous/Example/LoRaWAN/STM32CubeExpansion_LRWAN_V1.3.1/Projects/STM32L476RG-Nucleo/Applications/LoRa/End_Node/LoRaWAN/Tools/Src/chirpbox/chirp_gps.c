//**************************************************************************************************
//**** Includes ************************************************************************************

#include "chirp_internal.h"
#include "stm32l4xx_hal.h"
//#include "menu.h"

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

#define UTC_BASE_YEAR 1970
#define MONTH_PER_YEAR 12
#define DAY_PER_YEAR 365
#define SEC_PER_DAY 86400
#define SEC_PER_HOUR 3600
#define SEC_PER_MIN 60

#define UNIX_TIME_LENGTH 10

//**************************************************************************************************
//***** Local Typedefs and Class Declarations ******************************************************
typedef enum GPS_State_tag  /* State of radio */
{
    GPS_GET_TIME = 0,
    GPS_WAKEUP,
} GPS_State;

//**************************************************************************************************
//***** Forward Declarations ***********************************************************************

//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************
/* time conversion */
const unsigned char g_day_per_mon[MONTH_PER_YEAR] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

/* to receive data from uart */
char aRxBuffer[UNIX_TIME_LENGTH];

volatile uint32_t pps_count = 0;
volatile uint8_t gps_done = 0;

static Chirp_Time chirp_time;
GPS_State gps_state;
//**************************************************************************************************
//***** Global Variables ***************************************************************************
extern UART_HandleTypeDef huart3;

//**************************************************************************************************
//***** Local Functions ****************************************************************************

static unsigned char applib_dt_is_leap_year(unsigned short year)
{
    if ((year % 400) == 0)
        return 1;
    else if ((year % 100) == 0)
        return 0;
    else if ((year % 4) == 0)
        return 1;
    else
        return 0;
}

static unsigned char applib_dt_last_day_of_mon(unsigned char month, unsigned short year)
{
    if ((month == 0) || (month > 12))
        return g_day_per_mon[1] + applib_dt_is_leap_year(year);

    if (month != 2)
        return g_day_per_mon[month - 1];
    else
        return g_day_per_mon[1] + applib_dt_is_leap_year(year);
}

static void change_unix(long ts, Chirp_Time *gps_time)
{
    int year = 0;
    int month = 0;
    int date = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;

    int dates = ts / SEC_PER_DAY;
    int yearTmp = 0;
    int dayTmp = 0;
    for (yearTmp = UTC_BASE_YEAR; dates > 0; yearTmp++)
    {
        dayTmp = (DAY_PER_YEAR + applib_dt_is_leap_year(yearTmp));
        if (dates >= dayTmp)
            dates -= dayTmp;
        else
            break;
    }
    year = yearTmp;

    int monthTmp = 0;
    for (monthTmp = 1; monthTmp < MONTH_PER_YEAR; monthTmp++)
    {
        dayTmp = applib_dt_last_day_of_mon(monthTmp, year);
        if (dates >= dayTmp)
            dates -= dayTmp;
        else
            break;
    }
    month = monthTmp;

    date = dates + 1;

    int secs = ts % SEC_PER_DAY;
    hour = secs / SEC_PER_HOUR;
    hour += 8;
    if (hour >= 24)
    {
        hour -= 24;
        date++;
        dayTmp = applib_dt_last_day_of_mon(monthTmp, year);
        if (date > dayTmp)
        {
            date -= dayTmp;
            if (month == 12)
                yearTmp = yearTmp + 1;

            monthTmp = (monthTmp + 1) % MONTH_PER_YEAR;
        }
    }
    year = yearTmp;
    month = monthTmp;

    secs %= SEC_PER_HOUR;
    minute = secs / SEC_PER_MIN;
    second = secs % SEC_PER_MIN;

    if (monthTmp == 1 || monthTmp == 2)
    {
        monthTmp += 12;
        yearTmp--;
    }
    day =  (date + 2 * monthTmp + 3 * (monthTmp + 1) / 5 + yearTmp + yearTmp / 4 - yearTmp / 100 + yearTmp / 400) % 7 + 1;

    gps_time->chirp_year = (uint16_t)year;
    gps_time->chirp_month = (uint8_t)month;
    gps_time->chirp_date = (uint8_t)date;
    gps_time->chirp_day = (uint8_t)day;
    gps_time->chirp_hour = (uint8_t)hour;
    gps_time->chirp_min = (uint8_t)minute;
    gps_time->chirp_sec = (uint8_t)second;
    PRINTF("%d-%d-%d %d:%d:%d week: %d\n", gps_time->chirp_year, gps_time->chirp_month, gps_time->chirp_date, gps_time->chirp_hour, gps_time->chirp_min, gps_time->chirp_sec, gps_time->chirp_day);
}

void gps_pps_IRQ()
{
    gpi_watchdog_periodic();
    pps_count++;
    PRINTF("pps:%lu\n", pps_count);
}

//**************************************************************************************************
//***** Global Functions ***************************************************************************

void GPS_Init()
{
    GPIO_InitTypeDef GPIO_InitStruct;

    // config gps
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HW_GPIO_Init(GPIOA, GPIO_PIN_12|GPIO_PIN_11, &GPIO_InitStruct );
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12|GPIO_PIN_11, GPIO_PIN_RESET);

    // config gps
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HW_GPIO_Init(GPIOB, GPIO_PIN_12, &GPIO_InitStruct );
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);

    /*Configure trigger */
    HAL_GPIO_WritePin(GPS_TRIGGER_Port, GPS_TRIGGER_Pin, GPIO_PIN_RESET);
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    HW_GPIO_Init(GPS_TRIGGER_Port, GPS_TRIGGER_Pin, &GPIO_InitStruct);

    /*Configure pps */
    GPIO_InitStruct.Pin = GPS_PPS_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPS_PPS_Port, &GPIO_InitStruct);

    HW_GPIO_SetIrq(GPS_PPS_Port, GPS_PPS_Pin, 0, gps_pps_IRQ);
    HAL_NVIC_DisableIRQ( EXTI0_IRQn );
}

void GPS_On()
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12|GPIO_PIN_11, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
}

void GPS_Off()
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12|GPIO_PIN_11, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
}

void GPS_Uart_Irq()
{
    // change_unix(strtol(aRxBuffer, NULL, 10) - 1, &chirp_time);
    gps_done = 2;

    HAL_UART_Abort(&huart3);
    /* Disable usart, stop receiving data */
    __HAL_UART_DISABLE_IT(&huart3, UART_IT_RXNE);
    /* Disable Main Timer, since we have received GPS time */
    __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_CC1);
}

Chirp_Time GPS_Get_Time()
{
	chirp_isr.state = ISR_GPS;
    gps_state = GPS_GET_TIME;

    __HAL_UART_DISABLE(&huart3);
    __HAL_UART_ENABLE(&huart3);
    memset(aRxBuffer, 0, sizeof(aRxBuffer));
    memset(&chirp_time, 0, sizeof(chirp_time));

    HAL_UART_Receive_IT(&huart3, (uint8_t *)aRxBuffer, sizeof(aRxBuffer));

    HAL_GPIO_WritePin(GPS_TRIGGER_Port, GPS_TRIGGER_Pin, GPIO_PIN_SET);
    __HAL_TIM_CLEAR_IT(&htim2, TIM_IT_CC1);
    MAIN_TIMER_CC_REG = MAIN_TIMER_CNT_REG + GPI_TICK_US_TO_FAST(1000000);
    __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_CC1);
    HAL_GPIO_WritePin(GPS_TRIGGER_Port, GPS_TRIGGER_Pin, GPIO_PIN_RESET);
    gps_done = 0;
    while (gps_done == 0)
        ;
    if (gps_done == 2)
    {
        PRINTF("GPS timestamp:%s\n", aRxBuffer);
        change_unix(strtol(aRxBuffer, NULL, 10) - 1, &chirp_time);
    }
    return chirp_time;
}

time_t GPS_Conv(uint16_t year, uint8_t month, uint8_t date, uint8_t hour, uint8_t min, uint8_t sec)
{
    time_t retval = 0;
    struct tm tm;
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = date;
    tm.tm_hour = hour;
    tm.tm_min = min;
    tm.tm_sec = sec;
    tm.tm_isdst = -1;
    retval = mktime(&tm);
    return retval;
}

time_t GPS_Diff(Chirp_Time *gps_time, uint16_t start_year, uint8_t start_month, uint8_t start_date, uint8_t start_hour, uint8_t start_min, uint8_t start_sec)
{
    time_t now = 0;
    time_t start = 0;
    time_t diff = 0;

    now = GPS_Conv(gps_time->chirp_year, gps_time->chirp_month, gps_time->chirp_date, gps_time->chirp_hour, gps_time->chirp_min, gps_time->chirp_sec);
    start = GPS_Conv(start_year, start_month, start_date, start_hour, start_min, start_sec);
    diff = start - now;
    return diff;
}

void GPS_Waiting(uint16_t start_year, uint8_t start_month, uint8_t start_date, uint8_t start_hour, uint8_t start_min, uint8_t start_sec)
{
    __HAL_GPIO_EXTI_CLEAR_FLAG(GPS_PPS_Pin);

    GPS_Get_Time();
    time_t diff = GPS_Diff(&chirp_time, start_year, start_month, start_date, start_hour, start_min, start_sec);
    HAL_NVIC_EnableIRQ( EXTI0_IRQn );
    if (diff > 0)
    {
        pps_count = 0;
        while (pps_count <= diff)
            ;
    }
    pps_count = 0;
    HAL_NVIC_DisableIRQ( EXTI0_IRQn );
}

/**
  * @brief  wait until PPS is normal, then we can read the gps time
  * @param  none
  * @retval none
  */
void GPS_Waiting_PPS(uint32_t PPS_wait)
{
    __HAL_GPIO_EXTI_CLEAR_FLAG(GPS_PPS_Pin);
    HAL_NVIC_EnableIRQ( EXTI0_IRQn );
    {
        pps_count = 0;
        while (pps_count <= PPS_wait)
        {
            gpi_watchdog_periodic();
        }
    }
    pps_count = 0;
    HAL_NVIC_DisableIRQ( EXTI0_IRQn );
}

/**
  * @brief  obtain current gps time to calculate the length of main timer interrupt compare time
  * @param  interval_sec: interval time of interrupt
  * @retval none
  */
void GPS_Wakeup(uint32_t interval_sec)
{
    GPS_Get_Time();
    time_t diff = GPS_Diff(&chirp_time, 1970, 1, 1, 0, 0, 0);
    time_t sleep_sec = interval_sec - (time_t)(0 - diff) % interval_sec;
    PRINTF("sleep_sec:%ld\n", (int32_t)sleep_sec);
    gps_state = GPS_WAKEUP;
    __HAL_TIM_CLEAR_IT(&htim2, TIM_IT_CC1);

    HAL_NVIC_EnableIRQ( EXTI0_IRQn );
    // sleep_sec += 1;
    if (sleep_sec > 0)
    {
        pps_count = 0;
        while (pps_count <= sleep_sec)
        {
            // enter low-power mode
            gpi_int_disable();

            gpi_sleep();

            /* wake by the timer interrupt */
            gpi_int_enable();
        }
    }
    pps_count = 0;
    HAL_NVIC_DisableIRQ( EXTI0_IRQn );
}

/**
  * @brief  call this to go into sleep mode, and wake up at whole time point according to GPS time. Should not exceed 263 s, which is the longest duration of main timer
  * @param  interval_sec: interval time of interrupt
  * @retval none
  */
void GPS_Sleep(uint32_t interval_sec)
{
    GPS_Wakeup(interval_sec);
}

void gps_main_timer_isr(void)
{
    gpi_watchdog_periodic();
    #if GPS_DATA
    if (gps_state == GPS_GET_TIME)
    {
        __HAL_TIM_CLEAR_IT(&htim2, TIM_IT_CC1);

        __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_CC1);
        gps_done = 1;
        PRINTF("gps timeout!\n");
    }
    #endif
}
//**************************************************************************************************
