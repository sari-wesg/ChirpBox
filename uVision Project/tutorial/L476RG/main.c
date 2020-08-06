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
#include "ds3231.h"
#endif

//**************************************************************************************************
//***** Local Defines and Consts *******************************************************************
#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

uint8_t test_round;

#if MX_PSEUDO_CONFIG
/* TODO: */
// static const uint32_t nodes[256] = {0x4B0023, 0x38001E, 0x1E0030, 0x210027, 0x1C0040, 0x440038, 0x260057, 0x520049, 0x360017, 0x550033};
// static const uint32_t nodes[256] = {0x4B0023, 0x38001E, 0x1E0030, 0x210027, 0x1C0040, 0x440038, 0x260057, 0x520049, 0x360017, 0x550033, 0x420020};
// static const uint32_t nodes[256] = {0x4B0023, 0x38001E, 0x1E0030, 0x210027, 0x1C0040, 0x440038, 0x260057, 0x520049, 0x360017, 0x420020};
// static const uint32_t nodes[256] = {0x4B0023, 0x420029, 0x38001E, 0x1E0030, 0x26003E, 0x350017, 0x4A002D, 0x1C0040, 0x440038};
static const uint32_t nodes[256] = {0x4B0023, 0x420029, 0x38001E, 0x1E0030, 0x26003E, 0x350017, 0x4A002D, 0x420020, 0x530045, 0X1D002B, 0x4B0027, 0x440038, 0x520049, 0x260057, 0X20003D, 0x360017, 0X30003C, 0x210027, 0X1C0040, 0x250031, 0x39005F, 0x550033};
// static const uint32_t nodes[256] = {0x4B0023, 0x550033};
// static const uint32_t nodes[256] = {0x440032};

#endif
const uint8_t VERSION_MAJOR = 0xd4, VERSION_NODE = 0x7f;
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

#if MX_DOUBLE_BITMAP
	uint32_t LORA_NODE_ID = 0;
#endif
uint8_t node_id_allocate;

#if MX_PSEUDO_CONFIG
uint8_t MX_NUM_NODES_CONF;
#endif
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

	menu_bank();

	gpi_int_enable();

// enable SysTick timer if needed
#if MX_VERBOSE_PROFILE
	SysTick->LOAD = -1u;
	SysTick->VAL = 0;
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
#endif

	// init RF transceiver
	#if (!MX_PSEUDO_CONFIG)
	gpi_radio_init();
	#endif

	node_id_restore();

#if MX_DOUBLE_BITMAP
	LORA_NODE_ID = TOS_NODE_ID;
#endif

	PRINTF("\tStarting node 0x%X \n", TOS_NODE_ID);

	// translate TOS_NODE_ID to logical node id used with mixer
	#if MX_PSEUDO_CONFIG
	for (node_id = 0; node_id < NUM_ELEMENTS(nodes); ++node_id)
	{
		PRINTF("node:%lu, 0x%x\n", node_id, nodes[node_id]);
		if (nodes[node_id] == 0)
			break;
	}
	MX_NUM_NODES_CONF = node_id;
	PRINTF("MX_NUM_NODES_CONF:%lu\n", MX_NUM_NODES_CONF);

	for (node_id = 0; node_id < MX_NUM_NODES_CONF; ++node_id)
	#else
	for (node_id = 0; node_id < NUM_ELEMENTS(nodes); ++node_id)
	#endif
	{
		if (nodes[node_id] == TOS_NODE_ID)
			break;
	}
	#if MX_PSEUDO_CONFIG
	if (node_id >= MX_NUM_NODES_CONF)
	#else
	if (node_id >= NUM_ELEMENTS(nodes))
	#endif
	{
		PRINTF("!!! PANIC: node mapping not found for node %x !!!\n", TOS_NODE_ID);
		while (1)
			;
	}

	// init RNG with randomized seed
	mixer_rand_seed(gpi_mulu_16x16(TOS_NODE_ID, gpi_tick_fast_native()));

#if GPS_DATA
	#if BANK_1_RUN
	DS3231_ClearAlarm1_Time();
	#endif
	GPS_Init();
	GPS_Waiting_PPS(3);
	Chirp_Time gps_time;
    memset(&gps_time, 0, sizeof(gps_time));
	while(!gps_time.chirp_year)
	{
		gps_time = GPS_Get_Time();
	}
	#if BANK_1_RUN
	time_t rtc_diff = 0x05;
	uint8_t count = 0;
	/* if is in bank1, daemon erase jump1 to ensure keep in bank1 */
	while((rtc_diff < 0) || (rtc_diff >= 0x05))
	{
		count++;
		assert_reset(count < 10);
		DS3231_ModifyTime(gps_time.chirp_year - 2000, gps_time.chirp_month, gps_time.chirp_date, gps_time.chirp_day, gps_time.chirp_hour, gps_time.chirp_min, gps_time.chirp_sec);
		DS3231_GetTime();
		Chirp_Time RTC_Time = DS3231_ShowTime();
		rtc_diff = GPS_Diff(&gps_time, RTC_Time.chirp_year, RTC_Time.chirp_month, RTC_Time.chirp_date, RTC_Time.chirp_hour, RTC_Time.chirp_min, RTC_Time.chirp_sec);
	}
	#endif
	#if BANK_1_RUN
	FLASH_If_WriteProtectionClear();
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
	uint32_t round;
	Gpi_Fast_Tick_Native deadline;
	unsigned int i;
	uint8_t failed_round = 0;

	uint8_t node_id = hardware_init();

	node_id_allocate = node_id;

	/************************************ Sniff ************************************/
	// chirp_mx_radio_config(7, 7, 1, 8, 14, 470000);

	// if (!node_id)
	// {
	// 	sniff_init(STATE_LORA_FORM, 470000, 2020, 6, 16, 13, 37, 40);
	// }
	// else
	// {
	// 	sniff_tx(node_id);
	// }
	// PRINTF("mixer\n");
	// while (1)
	// 	;

	/************************************ Chirpbox ************************************/
	#if ((MX_PSEUDO_CONFIG) && (CHIRP_OUTLINE))
		chirp_start(node_id, MX_NUM_NODES_CONF);
	#endif

	/************************************ Mixer ***************************************/
	#if MX_PSEUDO_CONFIG
		chirp_mx_packet_config(MX_NUM_NODES_CONF, MX_NUM_NODES_CONF, 8);
		chirp_mx_radio_config(7, 7, 1, 8, 14, 470000);
		uint32_t packet_time = SX1276GetPacketTime(chirp_config.lora_sf, chirp_config.lora_bw, 1, 0, 8, chirp_config.phy_payload_size);
		printf("packet_time:%lu\n", packet_time);
		chirp_mx_slot_config(packet_time + 100000, MX_NUM_NODES_CONF * 6, 1500000);
		chirp_mx_payload_distribution(MX_COLLECT);
	#endif

	/* test for 3 times */
	for (test_round = 0; test_round < 3; test_round++)
	{
		// deadline for first round is now (-> start as soon as possible)
		// deadline = gpi_tick_hybrid();
		// deadline = gpi_tick_hybrid() + UPDATE_PERIOD;
		// deadline = gpi_tick_fast_native() + UPDATE_PERIOD;
		// if (node_id_allocate < SENSOR_NUM * GROUP_NUM)
		// 	deadline += 1 * MX_SLOT_LENGTH;
		deadline = gpi_tick_fast_native();
		// clear:
		clear_data();
#if MX_DOUBLE_BITMAP
		clear_start_up_flag();
#endif
		round = 1;

		while (1)
		{
			gpi_radio_init();

			// init mixer
			mixer_init(node_id);

			startup_message(round, node_id, mx_task);

			// arm mixer, node 0 = initiator
			// start first round with infinite scan
			// -> nodes join next available round, does not require simultaneous boot-up
			// mixer_arm(((!node_id) ? MX_ARM_INITIATOR : 0) | ((1 == round) ? MX_ARM_INFINITE_SCAN : 0));
            mixer_arm(((!node_id) ? MX_ARM_INITIATOR : 0) | ((1 == 0) ? MX_ARM_INFINITE_SCAN : 0));

			// delay initiator a bit
			// -> increase probability that all nodes are ready when initiator starts the round
			// -> avoid problems in view of limited deadline accuracy
			if (!node_id)
			{
				#if MX_PSEUDO_CONFIG
				deadline += 2 * chirp_config.mx_slot_length;
				#else
				deadline += 2 * MX_SLOT_LENGTH;
				#endif
			}

			// start when deadline reached
			// ATTENTION: don't delay after the polling loop (-> print before)
			// while (gpi_tick_compare_hybrid(gpi_tick_hybrid(), deadline) < 0);
			while (gpi_tick_compare_fast_native(gpi_tick_fast_native(), deadline) < 0);

			deadline = mixer_start();

			if (!read_message(&round, &mx_task))
			{
				failed_round++;
				printf("failed_round:%lu\n", failed_round);
				if (failed_round > 3)
				{
					__disable_fault_irq();
					NVIC_SystemReset();
				}
			}
            while (gpi_tick_compare_fast_native(gpi_tick_fast_native(), deadline) < 0);

			printf("round:%lu\n", round);

#if MX_DOUBLE_BITMAP
			if (test_round >= ECHO_INTERVAL_NUM)
				set_start_up_flag();
			if (mx.start_up_flag)
			{
				gpi_radio_init();
				break;
			}
#endif
			#if MX_PSEUDO_CONFIG
			Gpi_Fast_Tick_Native update_period = GPI_TICK_MS_TO_HYBRID2(((chirp_config.mx_period_time_s * 1000) / 1) - chirp_config.mx_round_length * (chirp_config.mx_slot_length_in_us / 1000));
			deadline += update_period;
			#else
			deadline += UPDATE_PERIOD;
			#endif

			round++;
		}

		PRINTF("----------Echo started----------\n");
// go into the next step: save the former message
#if MX_DOUBLE_BITMAP
		mixer_init(node_id);
		for (i = 0; i < MX_GENERATION_SIZE; i++)
		{
			startup_message(0, i, 0);
		}
#if GPS_DATA
		// read_GPS();
		read_GPS_num(3);
		wait_til_gps_time(GPS_ECHO_HOUR, GPS_ECHO_MINUTE, GPS_ECHO_SECOND);
#endif
		mixer_start();
#if MX_PACKET_TABLE
		read_packet_table();
		get_real_packet_group();
		if (node_id_allocate >= SENSOR_NUM * GROUP_NUM)
			calculate_action_time();
		evaluation_results();
#endif
#endif
	}

	// send_packet in all to all
	statistics_results();

#if SEND_RESULT
	//******************************************************************
	// clear:
	PRINTF("----------Sensor results----------\n");
	clear_data_result();
#if MX_DOUBLE_BITMAP
	clear_start_up_flag();
#endif
	PRINTF("----------READ_RESULTS----------\n");
	sensor_send_results_in_mixer(READ_RESULTS);
	PRINTF("----------READ_TOPOLOGY----------\n");
	sensor_send_results_in_mixer(READ_TOPOLOGY);
	// re-assign the node_id for actuators:
	if (node_id < SENSOR_NUM * GROUP_NUM)
		node_id += ACTUATOR_NUM * GROUP_NUM;
	else
		node_id -= SENSOR_NUM * GROUP_NUM;
	node_id_allocate = node_id;
	PRINTF("----------Actuator results----------\n");
	PRINTF("----------READ_RESULTS----------\n");
	sensor_send_results_in_mixer(READ_RESULTS);
	PRINTF("----------READ_TOPOLOGY----------\n");
	sensor_send_results_in_mixer(READ_TOPOLOGY);
#endif

	return 0;
	// GPI_TRACE_RETURN(0);
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

#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
//**************************************************************************************************
//**************************************************************************************************
