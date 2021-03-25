//**************************************************************************************************
//***** Includes ***********************************************************************************
#include "chirpbox_func.h"
#include "chirp_internal.h"

//**************************************************************************************************
//***** Global Variables ****************************************************************************

//**************************************************************************************************
//***** Local (Private) Defines and Consts *********************************************************
#define VOLTAGE_MINIMAL     2000    // voltage must above 2.0 V, if not, the adc channel is not connected to the battery output
#define VOLTAGE_LOWER_BOUND 2900    // voltage should above 2.7 V
#define VOLTAGE_INTERVAL    60     // check the voltage per 60 seconds

//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************

//**************************************************************************************************
//***** Local Functions ****************************************************************************

//**************************************************************************************************
//***** Global Functions ***************************************************************************

/**
  * @brief  Get voltage of the battery. Battery input is connected to the ADC1 channel 6
  * @param  None
  * @retval voltage
  */
uint16_t ADC_GetVoltage(void)
{
    uint16_t vref = *(__IO uint16_t *)0x1FFF75AA;
    uint32_t adc_vref, adc_vbat, adc_vch6, vbat, vdda, vch6;

    // start adc calibration
    HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 0xffff);
    adc_vref = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 0xffff);
    adc_vbat = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 0xffff);
    adc_vch6 = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    // calculate voltage
    vdda = vref * 1e3 / adc_vref * 3;
    vbat = vdda * 3 * adc_vbat / 4095;
    vch6 = vdda * 2 * adc_vch6 / 4095;

    PRINTF("vref:%u.%3u, vbat:%lu.%3lu, vch6:%lu.%3lu\n", vref / 1000, vref % 1000, vbat / 1000, vbat % 1000, vch6 / 1000, vch6 % 1000);
    return vch6;
}

/**
  * @brief  Make sure the voltage of the battery is over the minimal voltage
  * @param  None
  * @retval None
  */
void ADC_CheckVoltage(void)
{
    uint16_t voltage = VOLTAGE_LOWER_BOUND;
    while (1)
    {
        voltage = ADC_GetVoltage();
        if (!((voltage <= VOLTAGE_LOWER_BOUND) && (voltage > VOLTAGE_MINIMAL)))
            break;
        RTC_Waiting_Count(VOLTAGE_INTERVAL);
    }
}
