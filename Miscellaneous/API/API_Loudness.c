//**************************************************************************************************
//***** Includes ***********************************************************************************
#include "API_ChirpBox.h"

//**************************************************************************************************
//***** Global Variables ****************************************************************************
ADC_HandleTypeDef hadc1;

//**************************************************************************************************
//***** Local (Private) Defines and Consts *********************************************************
#define VOLTAGE_MINIMAL 2000	 // voltage must above 2.0 V, if not, the adc channel is not connected to the battery output
#define VOLTAGE_LOWER_BOUND 2900 // voltage should above 2.7 V
#define VOLTAGE_INTERVAL 60		 // check the voltage per 60 seconds

//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************

//**************************************************************************************************
//***** Local Functions ****************************************************************************
/**
  * @brief  Get voltage of the battery. Battery input is connected to the ADC1 channel 6
  * @param  None
  * @retval voltage
  */
static void ADC_GetSound(uint32_t *voltage_list)
{
	gpi_watchdog_periodic();
	uint16_t vref = *(__IO uint16_t *)0x1FFF75AA;
	uint32_t adc_vref, adc_vch4, adc_vch6, adc_vch9, vdda;

	// start adc calibration
	HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, 0xffff);
	adc_vref = HAL_ADC_GetValue(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, 0xffff);
	adc_vch4 = HAL_ADC_GetValue(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, 0xffff);
	adc_vch6 = HAL_ADC_GetValue(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, 0xffff);
	adc_vch9 = HAL_ADC_GetValue(&hadc1);
	HAL_ADC_Stop(&hadc1);

	// calculate voltage
	vdda = vref * 1e3 / adc_vref * 3;
	// sound sensor
	voltage_list[0] = vdda * 1 * adc_vch4 / 4095;
	voltage_list[1] = adc_vch9;
}

//**************************************************************************************************
//***** Global Functions ***************************************************************************

/**
* @brief ADC MSP Initialization
* This function configures the hardware resources used in this example
* @param hadc: ADC handle pointer
* @retval None
*/
void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	if (hadc->Instance == ADC1)
	{
		/* USER CODE BEGIN ADC1_MspInit 0 */

		/* USER CODE END ADC1_MspInit 0 */
		/* Peripheral clock enable */
		__HAL_RCC_ADC_CLK_ENABLE();

		__HAL_RCC_GPIOC_CLK_ENABLE();
		__HAL_RCC_GPIOA_CLK_ENABLE();
		/**ADC1 GPIO Configuration
    PC3     ------> ADC1_IN4
    PA1     ------> ADC1_IN6
    PA4     ------> ADC1_IN9
    */
		GPIO_InitStruct.Pin = GPIO_PIN_3;
		GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_4;
		GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

		/* USER CODE BEGIN ADC1_MspInit 1 */

		/* USER CODE END ADC1_MspInit 1 */
	}
}

/**
* @brief ADC MSP De-Initialization
* This function freeze the hardware resources used in this example
* @param hadc: ADC handle pointer
* @retval None
*/
void HAL_ADC_MspDeInit(ADC_HandleTypeDef *hadc)
{
	if (hadc->Instance == ADC1)
	{
		/* USER CODE BEGIN ADC1_MspDeInit 0 */

		/* USER CODE END ADC1_MspDeInit 0 */
		/* Peripheral clock disable */
		__HAL_RCC_ADC_CLK_DISABLE();

		/**ADC1 GPIO Configuration
    PC3     ------> ADC1_IN4
    PA1     ------> ADC1_IN6
    PA4     ------> ADC1_IN9
    */
		HAL_GPIO_DeInit(GPIOC, GPIO_PIN_3);

		HAL_GPIO_DeInit(GPIOA, GPIO_PIN_1 | GPIO_PIN_4);

		/* USER CODE BEGIN ADC1_MspDeInit 1 */

		/* USER CODE END ADC1_MspDeInit 1 */
	}
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
void MX_ADC1_Init(void)
{

	/* USER CODE BEGIN ADC1_Init 0 */

	/* USER CODE END ADC1_Init 0 */

	ADC_MultiModeTypeDef multimode = {0};
	ADC_ChannelConfTypeDef sConfig = {0};

	/* USER CODE BEGIN ADC1_Init 1 */

	/* USER CODE END ADC1_Init 1 */
	/** Common config
  */
	hadc1.Instance = ADC1;
	hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
	hadc1.Init.Resolution = ADC_RESOLUTION_12B;
	hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
	hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
	hadc1.Init.LowPowerAutoWait = DISABLE;
	hadc1.Init.ContinuousConvMode = DISABLE;
	hadc1.Init.NbrOfConversion = 4;
	hadc1.Init.DiscontinuousConvMode = DISABLE;
	hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	hadc1.Init.DMAContinuousRequests = DISABLE;
	hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
	hadc1.Init.OversamplingMode = DISABLE;
	if (HAL_ADC_Init(&hadc1) != HAL_OK)
	{
		Error_Handler();
	}
	/** Configure the ADC multi-mode
  */
	multimode.Mode = ADC_MODE_INDEPENDENT;
	if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
	{
		Error_Handler();
	}
	/** Configure Regular Channel
  */
	sConfig.Channel = ADC_CHANNEL_VREFINT;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_640CYCLES_5;
	sConfig.SingleDiff = ADC_SINGLE_ENDED;
	sConfig.OffsetNumber = ADC_OFFSET_NONE;
	sConfig.Offset = 0;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
	{
		Error_Handler();
	}
	/** Configure Regular Channel
  */
	sConfig.Channel = ADC_CHANNEL_4;
	sConfig.Rank = ADC_REGULAR_RANK_2;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
	{
		Error_Handler();
	}
	/** Configure Regular Channel
  */
	sConfig.Channel = ADC_CHANNEL_6;
	sConfig.Rank = ADC_REGULAR_RANK_3;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
	{
		Error_Handler();
	}
	/** Configure Regular Channel
  */
	sConfig.Channel = ADC_CHANNEL_9;
	sConfig.Rank = ADC_REGULAR_RANK_4;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN ADC1_Init 2 */

	/* USER CODE END ADC1_Init 2 */
}

/**
  * @brief  Get voltage of the battery. Battery input is connected to the ADC1 channel 6
  * @param  None
  * @retval voltage
  */
uint16_t ADC_GetVoltage(void)
{
	gpi_watchdog_periodic();
	uint16_t vref = *(__IO uint16_t *)0x1FFF75AA;
	uint32_t adc_vref, adc_vch4, adc_vch6, adc_vch9, vdda;

	// start adc calibration
	HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, 0xffff);
	adc_vref = HAL_ADC_GetValue(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, 0xffff);
	adc_vch4 = HAL_ADC_GetValue(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, 0xffff);
	adc_vch6 = HAL_ADC_GetValue(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, 0xffff);
	adc_vch9 = HAL_ADC_GetValue(&hadc1);
	HAL_ADC_Stop(&hadc1);

	// calculate voltage
	vdda = vref * 1e3 / adc_vref * 3;
	// battery level
	uint16_t voltage = vdda * 2 * adc_vch6 / 4095;
	printf("voltage:%lu.%03lu\n", voltage / 1000, voltage % 1000);

	return voltage;
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
static uint8_t read_gate()
{
	uint8_t gate = HW_GPIO_Read(GPIOC, GPIO_PIN_11);
	return gate;
}

void ADC_GetLoud(void)
{
	// Init gate pin
	GPIO_InitTypeDef GPIO_InitStruct;
	__HAL_RCC_GPIOC_CLK_ENABLE();

	/*Configure GPIO pin : PC11 */
	GPIO_InitStruct.Pin = GPIO_PIN_11;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
	uint32_t *voltage_list;
	voltage_list = (uint32_t *)malloc(2 * sizeof(uint32_t));
	while (1)
	{
		ADC_GetSound(voltage_list);
		printf("audio:%lu.%03lu,envelope:%lu,gate:%lu\n", voltage_list[0] / 1000, voltage_list[0] % 1000, voltage_list[1], read_gate());
		HAL_Delay(50);
	}
	free(voltage_list);

	/* Disable SysTick Interrupt */
	HAL_SuspendTick();
}
