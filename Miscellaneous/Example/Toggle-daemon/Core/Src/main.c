/***************************************************************************************************
 ***************************************************************************************************
 *
 *	Copyright (c) 2019, Networked Embedded Systems Lab, TU Dresden
 *	All rights reserved.
 *
 *	Redistribution and use in source and binary forms, with or without
 *	modification, are permitted provided that the following conditions are met:
 *		* Redistributions of source code must retain the above copyright
 *		  notice, this list of conditions and the following disclaimer.
 *		* Redistributions in binary form must reproduce the above copyright
 *		  notice, this list of conditions and the following disclaimer in the
 *		  documentation and/or other materials provided with the distribution.
 *		* Neither the name of the NES Lab or TU Dresden nor the
 *		  names of its contributors may be used to endorse or promote products
 *		  derived from this software without specific prior written permission.
 *
 *	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
 *	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ***********************************************************************************************/
/**
 *
 *	@file					main.c
 *
 *	@brief					main entry point
 *
 *	@version				$Id$
 *	@date					TODO
 *
 *	@author					Carsten Herrmann
 *
 ***************************************************************************************************

	@details

	TODO

 **************************************************************************************************/
//***** Trace Settings *****************************************************************************

#include "gpi/trace.h"

// message groups for TRACE messages (used in GPI_TRACE_MSG() calls)
// define groups appropriate for your needs, assign one bit per group
// values > GPI_TRACE_LOG_USER (i.e. upper bits) are reserved
#define TRACE_INFO GPI_TRACE_MSG_TYPE_INFO

// select active message groups, i.e., the messages to be printed (others will be dropped)
#ifndef GPI_TRACE_BASE_SELECTION
#define GPI_TRACE_BASE_SELECTION GPI_TRACE_LOG_STANDARD | GPI_TRACE_LOG_PROGRAM_FLOW
#endif
GPI_TRACE_CONFIG(main, GPI_TRACE_BASE_SELECTION | GPI_TRACE_LOG_USER);

//**************************************************************************************************
//**** Includes ************************************************************************************
#include "main.h"
#include "stm32l4xx_hal.h"

#include "mixer/mixer.h"

#include "gpi/tools.h"
#include "gpi/platform.h"
#include "gpi/interrupts.h"
#include "gpi/clocks.h"
#include "gpi/olf.h"
#include GPI_PLATFORM_PATH(radio.h)

#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "mixer/mixer_internal.h"

#if MX_FLASH_FILE
#include "menu.h"
#include "flash_if.h"
#include "stm32l4xx_hal_flash_ex.h"
#include "toggle.h"
#endif

#include "chirp_internal.h"
#if GPS_DATA
#include "chirpbox_ds3231.h"
#endif
#include "chirpbox_func.h"

//**************************************************************************************************
//***** Local Defines and Consts *******************************************************************
#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

const uint8_t VERSION_MAJOR = 0x2f, VERSION_NODE = 0x04;
//**************************************************************************************************
//***** Local Typedefs and Class Declarations ******************************************************

#define DEVICE_ID_REG0 (*((volatile uint32_t *)0x1FFF7590))
#define DEVICE_ID_REG1 (*((volatile uint32_t *)0x1FFF7594))
#define DEVICE_ID_REG2 (*((volatile uint32_t *)0x1FFF7598))

//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************

// uint32_t stm_node_id = 0;
unsigned char node_mac[8];
volatile uint32_t device_id[3];
uint32_t __attribute__((section(".data"))) TOS_NODE_ID = 0;
uint8_t mx_task = 0;

//**************************************************************************************************
//***** Global Variables ***************************************************************************
/* node id */
uint8_t node_id_allocate;
/* The total number of nodes */
uint8_t MX_NUM_NODES_CONF;
//**************************************************************************************************
//***** Local Functions ****************************************************************************

/**
 * @description: Read hardware id
 * @param None
 * @return: None
 */
static void node_id_restore(void)
{
	device_id[0] = DEVICE_ID_REG0;
	device_id[1] = DEVICE_ID_REG1;
	device_id[2] = DEVICE_ID_REG2;

	(*(uint32_t *)node_mac) = DEVICE_ID_REG1;
	(*(((uint32_t *)node_mac) + 1)) = DEVICE_ID_REG2 + DEVICE_ID_REG0;
	// stm_node_id = (uint32_t)(DEVICE_ID_REG0);
	TOS_NODE_ID = (uint32_t)(DEVICE_ID_REG0);
}

/**
 * @description: Initialization with hardware based on HAL library, peripherals, GPS, interrupt, System clock, radio, node id allocation and rand seed.
 * @param None
 * @return: node_id
 */
void hardware_init()
{
	uint8_t task[1];
	HAL_StatusTypeDef status;

	HAL_Init();
	gpi_platform_init();

#if BANK_1_RUN
	/* Only when the board is stable (eg, after a long time of getting GPS signal), the flash option bytes can be changed. Otherwise, readout protection will be triggered, when the voltage of the external power supply falls below the power down threshold.
	*/
	SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
	HAL_Delay(5000);
	Bank_WRT_Check();
#endif

	/* Disable SysTick Interrupt */
	HAL_SuspendTick();

	// menu_bank();
	PRINTF("\nDaemon for testing switch bank\n");

	gpi_int_enable();

	/* init RF transceiver */
	node_id_restore();

	PRINTF("\tStarting node 0x%X \n", TOS_NODE_ID);

	DS3231_ClearAlarm1_Time();

	Chirp_Time gps_time;
	memset(&gps_time, 0, sizeof(gps_time));

	gps_time.chirp_year = 2021;
	gps_time.chirp_month = 1;
	gps_time.chirp_date = 1;
	gps_time.chirp_day = 5;
	gps_time.chirp_hour = 12;
	gps_time.chirp_min = 0;
	gps_time.chirp_sec = 0;


	status = HAL_TIMEOUT;
	while (status != HAL_OK)
	{
		PRINTF("Input initiator task: \n1: w WRT\t2: wo WRT\n");
		status = HAL_UART_Receive(&UART_Handle, &task, sizeof(task), DOWNLOAD_TIMEOUT);
		while (UART_Handle.RxState == HAL_UART_STATE_BUSY_RX)
			;
	}
	if ((task[0] - '0') == 1)
	{
		printf("Switch bank with WRT!\n");
		DS3231_ModifyTime(gps_time.chirp_year - 2000, gps_time.chirp_month, gps_time.chirp_date, gps_time.chirp_day, gps_time.chirp_hour, gps_time.chirp_min, gps_time.chirp_sec);
		// set alarm after 5 seconds
		DS3231_SetAlarm1_Time(gps_time.chirp_date, gps_time.chirp_hour, gps_time.chirp_min, gps_time.chirp_sec + 5);
		Bank1_WRP(0, 255);

		/* switch to bank2 */
		STMFLASH_BankSwitch();
	}
	if ((task[0] - '0') == 2)
	{
		printf("Switch bank without WRT!\n");
		DS3231_ModifyTime(gps_time.chirp_year - 2000, gps_time.chirp_month, gps_time.chirp_date, gps_time.chirp_day, gps_time.chirp_hour, gps_time.chirp_min, gps_time.chirp_sec);
		// set alarm after 5 seconds
		DS3231_SetAlarm1_Time(gps_time.chirp_date, gps_time.chirp_hour, gps_time.chirp_min, gps_time.chirp_sec + 5);
		Bank1_nWRP();

		/* switch to bank2 */
		STMFLASH_BankSwitch();
	}
}

//**************************************************************************************************
//***** Global Functions ***************************************************************************

int main(void)
{
	/****************************** HARDWARE INITIALIZATION ***************************/
	hardware_init();

	return 0;
}

//**************************************************************************************************
void _Error_Handler(char *file, int line)
{
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	while (1)
	{
	}
	/* USER CODE END Error_Handler_Debug */
}

//**************************************************************************************************
