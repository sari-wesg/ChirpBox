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
#include "chirpbox-setting.h"

volatile chirpbox_daemon_config __attribute((section (".ChirpBoxSettingSection"))) daemon_config ={0};
// volatile chirpbox_daemon_config __attribute((section (".ChirpBoxSettingSection"))) daemon_config ={{0x00350045, 0x00420029, 0x003C0044, 0x001E0030, 0x0026003E, 0x00350017, 0x004A002D, 0x00420020, 0x00530045, 0x001D002B, 0x004B0027, 0x00440038, 0x00520049, 0x004B0023, 0x0020003D, 0x00360017, 0x0030003C, 0x00210027, 0x001C0040, 0x00250031, 0x0039005F}, 0x2B06, 460000};

const uint8_t VERSION_MAJOR = 0x2f, VERSION_NODE = 0x04;
//**************************************************************************************************
//***** Local Typedefs and Class Declarations ******************************************************

#define DEVICE_ID_REG0 (*((volatile uint32_t *)0x1FFF7590))
#define DEVICE_ID_REG1 (*((volatile uint32_t *)0x1FFF7594))
#define DEVICE_ID_REG2 (*((volatile uint32_t *)0x1FFF7598))

//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************

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
static uint8_t hardware_init()
{
	uint8_t node_id;

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

	menu_bank();

	gpi_int_enable();

	/* init RF transceiver */
	gpi_radio_init();
	node_id_restore();

	PRINTF("\tStarting node 0x%08x \n", TOS_NODE_ID);
    print_chirpbox_daemon_config((chirpbox_daemon_config*)&daemon_config);

	/* translate TOS_NODE_ID to logical node id used with mixer */
	for (node_id = 0; node_id < NUM_ELEMENTS(daemon_config.UID_list); ++node_id)
	{
		if (daemon_config.UID_list[node_id] == 0)
			break;
	}
	MX_NUM_NODES_CONF = node_id;
	PRINTF("MX_NUM_NODES_CONF:%d\n", MX_NUM_NODES_CONF);

	for (node_id = 0; node_id < MX_NUM_NODES_CONF; ++node_id)
	{
		if (daemon_config.UID_list[node_id] == TOS_NODE_ID)
			break;
	}

	if (node_id >= MX_NUM_NODES_CONF)
	{
		PRINTF("Warning: node mapping not found for node 0x%x !!!\n", TOS_NODE_ID);
		while (1)
			;
	}
	PRINTF("Running with node ID: %d\n", node_id);

	/* init RNG with randomized seed */
	mixer_rand_seed(gpi_mulu_16x16(TOS_NODE_ID, gpi_tick_fast_native()));

	DS3231_ClearAlarm1_Time();
	GPS_Init();
	GPS_On();
#if GPS_DATA
	GPS_Waiting_PPS(10);
	Chirp_Time gps_time;
    memset(&gps_time, 0, sizeof(gps_time));
	while(!gps_time.chirp_year)
	{
		gps_time = GPS_Get_Time();
	}
	RTC_ModifyTime(gps_time.chirp_year - 2000, gps_time.chirp_month, gps_time.chirp_date, gps_time.chirp_day, gps_time.chirp_hour, gps_time.chirp_min, gps_time.chirp_sec);
	#if BANK_1_RUN
	time_t rtc_diff = 0x05;
	uint8_t count = 0;
	/* if is in bank1, daemon erase jump1 to ensure keep in bank1 */
	while((rtc_diff < 0) || (rtc_diff >= 0x05))
	{
		count++;
		assert_reset((count < 10));
		DS3231_ModifyTime(gps_time.chirp_year - 2000, gps_time.chirp_month, gps_time.chirp_date, gps_time.chirp_day, gps_time.chirp_hour, gps_time.chirp_min, gps_time.chirp_sec);
		DS3231_GetTime();
		Chirp_Time RTC_Time = DS3231_ShowTime();
		rtc_diff = GPS_Diff(&gps_time, RTC_Time.chirp_year, RTC_Time.chirp_month, RTC_Time.chirp_date, RTC_Time.chirp_hour, RTC_Time.chirp_min, RTC_Time.chirp_sec);
	}
	#endif
    uint32_t reset_time_flash[sizeof(Chirp_Time) / sizeof(uint32_t)];
	memcpy(reset_time_flash, (uint32_t *)&gps_time, sizeof(reset_time_flash));
	FLASH_If_Erase_Pages(1, RESET_PAGE);
	FLASH_If_Write(RESET_FLASH_ADDRESS, (uint32_t *)reset_time_flash, sizeof(reset_time_flash) / sizeof(uint32_t));
#endif

	return node_id;
}

//**************************************************************************************************
//***** Global Functions ***************************************************************************

int main(void)
{
	/****************************** HARDWARE INITIALIZATION ***************************/
	uint8_t node_id = hardware_init();
	node_id_allocate = node_id;

	/************************************ Chirpbox ************************************/
	chirp_start(node_id, MX_NUM_NODES_CONF);

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
