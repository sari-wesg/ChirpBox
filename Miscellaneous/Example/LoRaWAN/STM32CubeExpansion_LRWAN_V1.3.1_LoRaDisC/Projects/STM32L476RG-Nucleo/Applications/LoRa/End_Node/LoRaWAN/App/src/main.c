/**
  ******************************************************************************
  * @file    main.c
  * @author  MCD Application Team
  * @brief   this is the main!
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "hw.h"
#include "timeServer.h"
#include "vcom.h"
#include <stdlib.h>
#include "ll_flash.h"
#include "lorawan.h"

// #include "Commissioning.h"
// volatile chirpbox_fut_config __attribute((section (".FUTSettingSection"))) fut_config ={2, 5, 0, DR_5, 0};
extern uint32_t __attribute__((section(".data"))) TOS_NODE_ID;

//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************


/* Private functions ---------------------------------------------------------*/
extern int32_t the_given_second;

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  /* STM32 HAL library initialization*/
  HAL_Init();
#if !CHIRPBOX_LORAWAN
  /* Configure the system clock*/
  SystemClock_Config();
#else
	gpi_platform_init();
#endif

  /* Configure the debug mode*/
  DBG_Init();

  /* Configure the hardware*/
  HW_Init();
#if !CHIRPBOX_LORAWAN
  MX_GPIO_Init();
#endif

  uint8_t node_id_buffer[8];
  node_id_restore(node_id_buffer);

  lorawan_start(TOS_NODE_ID);

  loradisc_start(TOS_NODE_ID);

  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */
}



/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
