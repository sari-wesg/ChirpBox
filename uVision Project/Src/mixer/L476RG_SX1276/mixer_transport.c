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
 ***********************************************************************************************//**
 *
 *	@file					nrf52840/mixer_transport.c
 *
 *	@brief					Mixer transport layer for Nordic nRF52840
 *
 *	@version				$Id$
 *	@date					TODO
 *
 *	@author					Carsten Herrmann
 *
 ***************************************************************************************************

	@details

	TODO

	ATTENTION: Product Specification (4413_417 v1.0) is inconsistent: In Fig. 123 FRAMESTART
	is triggered after reception of PHR while the text says that it is generated after SFD.
	We assume that it gets triggered after PHR, not SFD, for the following reasons:
	- On page 296 it says "Frames with zero length will be discarded, and the FRAMESTART
	  event will not be generated in this case." This behavior would be impossible with
	  FRAMESTART triggered before PHR.
	- Bitcounter connected to FRAMESTART (with shortcut) and BCC set to
	  (1 (PHR) + payload size + 2 (CRC) ) * 8 does not trigger BCCMATCH at the end of a valid
	  frame, without much doubt because Bitcounter starts to count after PHR, not before.

 **************************************************************************************************/
//***** Trace Settings *****************************************************************************

#include "gpi/trace.h"
#include GPI_PLATFORM_PATH(sx1276Regs_Fsk.h)
#include GPI_PLATFORM_PATH(sx1276Regs_LoRa.h)
// message groups for TRACE messages (used in GPI_TRACE_MSG() calls)
#define TRACE_INFO			GPI_TRACE_MSG_TYPE_INFO
#define TRACE_WARNING		GPI_TRACE_MSG_TYPE_WARNING
#define TRACE_VERBOSE		GPI_TRACE_MSG_TYPE_VERBOSE

// select active message groups, i.e., the messages to be printed (others will be dropped)
#ifndef GPI_TRACE_BASE_SELECTION
	#define GPI_TRACE_BASE_SELECTION	GPI_TRACE_LOG_STANDARD | GPI_TRACE_LOG_PROGRAM_FLOW
#endif
GPI_TRACE_CONFIG(mixer_transport, GPI_TRACE_BASE_SELECTION | GPI_TRACE_LOG_USER);

//**************************************************************************************************
//**** Includes ************************************************************************************

#include "../mixer_internal.h"
#include "memxor.h"

#include "gpi/tools.h"
#include "gpi/platform.h"
#include "gpi/clocks.h"
#include "gpi/interrupts.h"
#include "gpi/olf.h"

#if ENERGEST_CONF_ON
#include GPI_PLATFORM_PATH(energest.h)
#endif

#include "mixer_transport.h"

#include GPI_PLATFORM_PATH(radio.h)
#include GPI_PLATFORM_PATH(spi.h)
#include GPI_PLATFORM_PATH(hw_gpio.h)

#include <stdio.h>
#include <inttypes.h>

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

//**************************************************************************************************
//***** Local Defines and Consts *******************************************************************

#if MX_VERBOSE_PROFILE
	GPI_PROFILE_SETUP("mixer_transport.c", 1700, 4);
#endif

#define PROFILE_ISR(...)	PROFILE_P(0, ## __VA_ARGS__)

// //SX1276*******************************************************************************************
// #define SYMBOL_BANDWIDTH 		( LORA_BANDWIDTH > 8) ? 500000 : ( ( LORA_BANDWIDTH - 6) * 125000 )
// #define SYMBOL_RATE				( ( SYMBOL_BANDWIDTH ) / ( 1 << LORA_SPREADING_FACTOR ) )
// #define SYMBOL_TIME				(uint32_t)1e6 / ( SYMBOL_RATE )					//us
// #define PREAMBLE_TIME			(( LORA_PREAMBLE_LENGTH + 4 ) * ( SYMBOL_TIME ) + ( SYMBOL_TIME ) / 4)

// // payload with explict header
// #define PAYLOAD_TMP			    (uint32_t)(ceil( (int32_t)( 8 * PHY_PAYLOAD_SIZE - 4 * LORA_SPREADING_FACTOR + 28 + 16 ) / \
//                                 (double)( 4 * LORA_SPREADING_FACTOR ) ) * ( LORA_CODINGRATE + 4 ))
// #define PAYLOAD_NUM         	( 8 + ( ( (PAYLOAD_TMP) > 0 ) ? (PAYLOAD_TMP) : 0 ) )
// #define PAYLOAD_TIME        	(PAYLOAD_NUM) * (SYMBOL_TIME)
// #define PAYLOAD_AIR_TIME 		( ( PREAMBLE_TIME ) + ( PAYLOAD_TIME ) )

// // explict header mode
// #define HEADER_LEN          	2												//explict header length
// #define HEADER_TMP			    (uint32_t)(ceil( (int32_t)( 8 * HEADER_LEN - 4 * LORA_SPREADING_FACTOR + 28 - 20) / \
//                                 (double)( 4 * LORA_SPREADING_FACTOR ) ) * ( 4 + 4 ))
// #define HEADER_NUM          	8 + ( ( (HEADER_TMP) > 0 ) ? (HEADER_TMP) : 0 )
// #define HEADER_TIME         	( ( HEADER_NUM ) * ( SYMBOL_TIME ) + PREAMBLE_TIME )
// #define HEADER_TIME_SHORT       ( ( HEADER_NUM ) * ( SYMBOL_TIME ) )

// #define AFTER_HEADER_TIME   	( ( PAYLOAD_TIME ) - ( HEADER_TIME_SHORT ) )			//expected rxdone time after a valid header detection

// #if MX_DOUBLE_BITMAP
// 	// #define BITMAP_BYTE   			( (MX_GENERATION_SIZE + 7) / 8 ) * 2
// 	#define BITMAP_BYTE   			(offsetof(Packet, payload) - offsetof(Packet, phy_payload_begin))
// 	#define BITMAP_TMP			    (uint32_t)(ceil( (int32_t)( 8 * BITMAP_BYTE - 4 * LORA_SPREADING_FACTOR + 28) / \
// 									(double)( 4 * LORA_SPREADING_FACTOR ) ) * ( 1 + 4 ))
// 	#define BITMAP_NUM          	8 + ( ( (BITMAP_TMP) > 0 ) ? (BITMAP_TMP) : 0 )
// 	#define BITMAP_TIME         	( ( BITMAP_NUM ) * ( SYMBOL_TIME ) + PREAMBLE_TIME )
// 	#define AFTER_HEADER_BITMAP   	( ( BITMAP_TIME ) - ( HEADER_TIME ) )				//expected rxdone time after a valid header detection
// #endif

// //**************************************************************************************************
// ASSERT_CT_STATIC(IS_POWER_OF_2(FAST_HYBRID_RATIO), unefficient_FAST_HYBRID_RATIO);

// // timing parameters

// // NOTE: a drift tolerance of 300 ppm (150 ppm on each side) should be a comfortable choice
// // (typical clock crystals have < 20...50 ppm at 25�C and temperature coefficient < 0.04 ppm/K)
// // note: MIN(2500, ...) is effective in case of very long slots (up to seconds). It reduces the
// // tolerance to avoid that RX_WINDOW overflows too fast (or even immediately).
// #define DRIFT_TOLERANCE			MIN(2500, MAX((MX_SLOT_LENGTH + 999) / 1000, 1))	// +/- 1000 ppm

// #define MAX_PROPAGATION_DELAY	GPI_TICK_US_TO_HYBRID(2)

// #define PACKET_AIR_TIME			GPI_TICK_US_TO_HYBRID2(PAYLOAD_AIR_TIME + 712)
// #define GRID_TO_EVENT_OFFSET	    GPI_TICK_US_TO_HYBRID2(HEADER_TIME)	// preamble + explict header + event signaling latency
// #define RX_TO_GRID_OFFSET		(0 + GPI_TICK_US_TO_HYBRID(37))		// software latency + RX ramp up time
// #define TX_TO_GRID_OFFSET		(0 + GPI_TICK_US_TO_HYBRID(130))		// software latency + TX ramp up time

// #define RX_WINDOW_INCREMENT		(2 * DRIFT_TOLERANCE)			// times 2 is important to widen the window in next slot (times 1 would follow only)
// #define RX_WINDOW_MAX			MIN(0x7FFFFFFF, MIN(15 * RX_WINDOW_INCREMENT, (MX_SLOT_LENGTH - PACKET_AIR_TIME - RX_TO_GRID_OFFSET) / 2))
// #define RX_WINDOW_MIN			MIN(RX_WINDOW_MAX / 2, MAX(2 * RX_WINDOW_INCREMENT, GPI_TICK_US_TO_HYBRID(1)))		// minimum must cover variations in execution time from timer polling to RX on

// #define GRID_DRIFT_FILTER_DIV	4
// #define GRID_TICK_UPDATE_DIV	2
// #define GRID_DRIFT_MAX			MIN(3 * DRIFT_TOLERANCE * GRID_TICK_UPDATE_DIV * GRID_DRIFT_FILTER_DIV, 0x7FFFFF)

// #define TX_OFFSET_FILTER_DIV	2
// #define TX_OFFSET_MAX			(2 * MAX_PROPAGATION_DELAY + GPI_TICK_US_TO_HYBRID(2))

// #define ISR_LATENCY_BUFFER		122		// in microseconds
// #define ISR_LATENCY_SLOW		4 * HYBRID_SLOW_RATIO		// in fast tick

// #define GRID_DRIFT_OFFSET		GPI_TICK_US_TO_HYBRID2(20 * SYMBOL_TIME)

// #define SLOT_INTERVAL			((((PACKET_AIR_TIME + GPI_TICK_US_TO_HYBRID2(4 * SYMBOL_TIME)) * MX_DUTY_CYCLE_PERCENT) / MX_SLOT_LENGTH) + 1)
//stm32l476RG**************************************************************************************

// // TIMER2
// #define MAIN_TIMER					htim2.Instance
// #define MAIN_TIMER_IRQ				TIM2_IRQn
// #define MAIN_TIMER_ISR_NAME			TIM2_IRQHandler

// #define MAIN_TIMER_CC_REG			(MAIN_TIMER->CCR1)				// compare interrupt count
// #define MAIN_TIMER_CNT_REG			(MAIN_TIMER->CNT)				// timer2 now count

// // LPTIM1
// #define LP_TIMER					hlptim1.Instance
// #define LP_TIMER_IRQ				LPTIM1_IRQn
// #define LP_TIMER_ISR_NAME			LPTIM1_IRQHandler

// #define LP_TIMER_CMP_REG			(LP_TIMER->CMP)					// compare interrupt count
// #define LP_TIMER_CNT_REG			(LP_TIMER->CNT)					// timer2 now count

#define MAX_NON_RECEIVE				3
//**************************************************************************************************

#if MX_VERBOSE_STATISTICS
	#define LED_ISR(name, led)									\
		name ## _ ();											\
		void name() {											\
			if (gpi_wakeup_event) {								\
				mx.wake_up_timestamp = gpi_tick_fast_native();	\
				gpi_wakeup_event = 0;							\
			}													\
			gpi_led_toggle(led);								\
			name ## _ ();										\
			gpi_led_toggle(led);								\
		}														\
		void name ## _ ()
#else
	#define LED_ISR(name, led)									\
		name ## _ ();											\
		void name() {											\
			gpi_led_toggle(led);								\
			name ## _ ();										\
			gpi_led_toggle(led);								\
		}														\
		void name ## _ ()
#endif

//**************************************************************************************************
//***** Local Typedefs and Class Declarations ******************************************************

typedef enum Slot_State_tag
{
	RESYNC		= 0,
	RX_RUNNING	= 16,
	TX_RUNNING	= 12,
	IDLE		= 12,	// ATTENTION: TX and IDLE are not distinguished here

} Slot_State;

//**************************************************************************************************
//***** Forward Declarations ***********************************************************************

void	timeout_isr();
void	grid_timer_isr();

//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************

static struct
{
	uint32_t header_time;
	uint32_t after_header_time;

	uint32_t max_propagation_delay;
	Gpi_Hybrid_Tick packet_air_time;
	uint32_t rx_to_grid_offset;
	uint32_t tx_to_grid_offset;
	uint32_t rx_window_increment;
	uint32_t rx_window_max;
	uint32_t rx_window_min;
	int32_t grid_drift_filter_div;
	int32_t grid_tick_update_div;
	uint32_t grid_drift_max;
	int32_t tx_offset_filter_div;
	uint32_t tx_offset_max;
	uint32_t isr_latency_buffer;
	uint32_t isr_latency_slow;
	uint32_t grid_drift_offset;

	Gpi_Hybrid_Tick max_tb_interval;

	#if MX_HEADER_CHECK
	Gpi_Hybrid_Tick after_header_hybrid;
	#endif
} radio; /* definition */

static struct
{
	Gpi_Hybrid_Tick			event_tick_nominal;
	Gpi_Hybrid_Tick			next_grid_tick;
	Gpi_Hybrid_Tick			next_trigger_tick;
	Gpi_Hybrid_Tick			slow_trigger;
	Gpi_Hybrid_Tick			hybrid_trigger;
	uint8_t					grid_timer_flag;

	Gpi_Slow_Tick_Native 	event_tick_nominal_slow;
	Gpi_Slow_Tick_Native 	next_grid_tick_slow;
	Gpi_Slow_Tick_Native 	next_trigger_tick_slow;

	int32_t					grid_drift;
	int32_t					grid_drift_cumulative;
	uint32_t				rx_trigger_offset;
	uint32_t				tx_trigger_offset;

	Packet					tx_fifo;

	Slot_State				slot_state;
	Slot_Activity			next_slot_task;

#if MX_VERBOSE_STATISTICS
	Gpi_Fast_Tick_Native	radio_start_timestamp;
#endif

#if MX_PREAMBLE_UPDATE
	uint8_t					valid_preamble;
	uint8_t					non_receive;
#endif

#if MX_LBT_AFA
	uint8_t					tx_channel;
	uint8_t					rx_channel;
	uint8_t					rx_wait_flag;
	uint8_t					rx_time;
	uint8_t					tx_time;
	uint8_t					tx_guard_flag;
	uint8_t					tx_dead_flag;
	uint8_t					lbt_valid_preamble;
#endif

#if MX_HEADER_CHECK
	uint8_t					valid_header;
#endif
} s;

// TODO: NJTRST
// uint8_t Dio3Irq = 0;

#if MX_DOUBLE_BITMAP
	static uint8_t BITMAP_FIFO[BITMAP_BYTE];
#endif

#if MX_LBT_AFA
	static uint8_t CODING_VECTOR_FIFO[BITMAP_BYTE_INFO];
#endif

#if MX_HEADER_CHECK
	static uint8_t APP_HEADER_FIFO[HASH_HEADER];
#endif
//**************************************************************************************************
//***** Global Variables ***************************************************************************

//**************************************************************************************************
//***** Local Functions ****************************************************************************

static void mixer_transport_initiate_radio()
{
	#if MX_PSEUDO_CONFIG
	uint32_t symbol_bandwidth = ( chirp_config.lora_bw > 8) ? 500000 : ( ( chirp_config.lora_bw - 6) * 125000 );
	uint32_t symbol_rate = ( ( symbol_bandwidth ) / ( 1 << chirp_config.lora_sf ) );
	#else
	uint32_t symbol_bandwidth = ( LORA_BANDWIDTH > 8) ? 500000 : ( ( LORA_BANDWIDTH - 6) * 125000 );
	uint32_t symbol_rate = ( ( symbol_bandwidth ) / ( 1 << LORA_SPREADING_FACTOR ) );
	#endif
	uint32_t symbol_time = (uint32_t)1e6 / symbol_rate;
	#if MX_PSEUDO_CONFIG
	uint32_t payload_air_time = SX1276GetPacketTime(chirp_config.lora_sf, chirp_config.lora_bw, chirp_config.lora_cr, 0, chirp_config.lora_plen, chirp_config.phy_payload_size + HASH_TAIL_CODE);
	uint32_t drift_tolerance = MIN(2500, MAX((chirp_config.mx_slot_length + 999) / 1000, 1));
	#else
	uint32_t payload_air_time = SX1276GetPacketTime(LORA_SPREADING_FACTOR, LORA_BANDWIDTH, LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH, PHY_PAYLOAD_SIZE);
	uint32_t drift_tolerance = MIN(2500, MAX((MX_SLOT_LENGTH + 999) / 1000, 1));
	#endif
	#if MX_PSEUDO_CONFIG
	radio.header_time = SX1276GetPacketTime(chirp_config.lora_sf, chirp_config.lora_bw, chirp_config.lora_cr, 1, chirp_config.lora_plen, 2);
	#else
	radio.header_time = SX1276GetPacketTime(LORA_SPREADING_FACTOR, LORA_BANDWIDTH, LORA_CODINGRATE, 1, LORA_PREAMBLE_LENGTH, 2);
	#endif
	radio.after_header_time = payload_air_time - radio.header_time;

	radio.max_propagation_delay = GPI_TICK_US_TO_HYBRID(2);

	radio.packet_air_time = GPI_TICK_US_TO_HYBRID2(payload_air_time + 712);
	radio.rx_to_grid_offset = (0 + GPI_TICK_US_TO_HYBRID(37));
	radio.tx_to_grid_offset = (0 + GPI_TICK_US_TO_HYBRID(130));

	radio.rx_window_increment = 2 * drift_tolerance;
	#if MX_PSEUDO_CONFIG
	radio.rx_window_max = MIN(0x7FFFFFFF, MIN(15 * radio.rx_window_increment, (chirp_config.mx_slot_length - radio.packet_air_time - radio.rx_to_grid_offset) / 2));
	#else
	radio.rx_window_max = MIN(0x7FFFFFFF, MIN(15 * radio.rx_window_increment, (MX_SLOT_LENGTH - radio.packet_air_time - radio.rx_to_grid_offset) / 2));
	#endif
	radio.rx_window_min = MIN(radio.rx_window_max / 2, MAX(2 * radio.rx_window_increment, GPI_TICK_US_TO_HYBRID(1)));

	radio.grid_drift_filter_div = 4;
	radio.grid_tick_update_div = 2;
	radio.grid_drift_max = MIN(3 * drift_tolerance * radio.grid_tick_update_div * radio.grid_drift_filter_div, 0x7FFFFF);

	radio.tx_offset_filter_div = 2;
	radio.tx_offset_max = (2 * radio.max_propagation_delay + GPI_TICK_US_TO_HYBRID(2));

	radio.isr_latency_buffer = 122;
	radio.isr_latency_slow = 4 * HYBRID_SLOW_RATIO;

	radio.grid_drift_offset = GPI_TICK_US_TO_HYBRID2(5 * symbol_time);

	radio.max_tb_interval = GPI_TICK_US_TO_HYBRID(2000);

	#if MX_HEADER_CHECK
	uint32_t after_header_us = SX1276GetPacketTime(chirp_config.lora_sf, chirp_config.lora_bw, chirp_config.lora_cr, 0, chirp_config.lora_plen, HASH_HEADER) - radio.header_time + 2 * symbol_time;
	// printf("after_header_us:%lu\n", after_header_us);
	radio.after_header_hybrid = GPI_TICK_US_TO_HYBRID2(after_header_us);
	#endif
}

//**************************************************************************************************

// trigger grid/timeout timer (immediately)
static inline void trigger_main_timer(int use_int_lock)
{
	register int	ie;

	if (use_int_lock)
		ie = gpi_int_lock();

	MAIN_TIMER_CC_REG = MAIN_TIMER_CNT_REG + 10;

	if (use_int_lock)
		gpi_int_unlock(ie);

}

static inline void trigger_lowpower_timer(int use_int_lock)
{
	register int	ie;

	if (use_int_lock)
		ie = gpi_int_lock();

	LP_TIMER_CMP_REG = LP_TIMER_CNT_REG + 3;

	if (use_int_lock)
		gpi_int_unlock(ie);
}
//**************************************************************************************************

static inline void unmask_main_timer(int clear_pending)
{
	if (clear_pending)
	{
		NVIC_ClearPendingIRQ(MAIN_TIMER_IRQ);
    }

    __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_CC1);
}

static inline void mask_main_timer()
{
    __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_CC1);
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

// ATTENTION: to be called from ISRs only
static inline __attribute__((always_inline)) void set_event(Event event)
{
	// ATTENTION: use API function to ensure that load/store exclusive works right (if used)
	gpi_atomic_set(&(mx.events), BV(event));
//	mx.events |= BV(event);
}

//**************************************************************************************************

static uint8_t write_tx_fifo(uint8_t *buffer, uint8_t *p2, uint8_t size)
{
    uint8_t 		i, tmp_data;
	uint8_t			or_data = 0;

	if(p2 == NULL)
	{
		or_data = 1;

		//NSS = 0;
		HW_GPIO_Write( RADIO_NSS_PORT, RADIO_NSS_PIN, 0 );

		HW_SPI_InOut( REG_LR_FIFO | 0x80 );
		for( i = 0; i < size; i++ )
		{
			HW_SPI_InOut( buffer[i] );
		}

		//NSS = 1;
		HW_GPIO_Write( RADIO_NSS_PORT, RADIO_NSS_PIN, 1 );
	}
	else
	{
		//NSS = 0;
		HW_GPIO_Write( RADIO_NSS_PORT, RADIO_NSS_PIN, 0 );

		HW_SPI_InOut( REG_LR_FIFO | 0x80 );
		for( i = 0; i < size; i++ )
		{
			tmp_data = ( buffer[i] ^ p2[i] );
			HW_SPI_InOut( tmp_data );
			or_data = (tmp_data)? ++or_data: or_data;
		}

		//NSS = 1;
		HW_GPIO_Write( RADIO_NSS_PORT, RADIO_NSS_PIN, 1 );
	}

	return or_data;
}

//**************************************************************************************************

static void start_grid_timer()
{
	Gpi_Hybrid_Reference	r;
	Gpi_Hybrid_Tick			d, t;
	// Gpi_Slow_Tick_Native	t_slow;
	// gpi_led_on(GPI_LED_6);
	r = gpi_tick_hybrid_reference();
	// gpi_led_off(GPI_LED_6);

	d = s.next_trigger_tick - r.hybrid_tick;
	s.hybrid_trigger = s.next_trigger_tick;

	t = s.next_trigger_tick - GPI_TICK_US_TO_HYBRID(radio.isr_latency_buffer);
	s.slow_trigger = s.next_trigger_tick;

	// t_slow = s.next_trigger_tick_slow - ((Gpi_Slow_Tick_Native)ISR_LATENCY_SLOW / HYBRID_SLOW_RATIO);
	// t = r.hybrid_tick + (Gpi_Slow_Tick_Native)(t_slow - LP_TIMER_CNT_REG) * HYBRID_SLOW_RATIO;
	mask_main_timer();
	__HAL_TIM_CLEAR_IT(&htim2, TIM_IT_CC1);
	mask_slow_timer();
	__HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_FLAG_CMPM);
	// printf("s8:%lu, %lu\n", s.next_grid_tick - r.hybrid_tick, s.next_grid_tick);


	// if we are late
	s.grid_timer_flag = 0;
	// note: signed comparison is important
	// if (gpi_tick_compare_hybrid(r.hybrid_tick, t) >= 0)
	if ((int32_t)(d) < (int32_t)(GPI_TICK_US_TO_HYBRID(1000000 / GPI_SLOW_CLOCK_RATE + 50) + GPI_TICK_US_TO_HYBRID(radio.isr_latency_buffer)))
	{
		// printf("s6\n");
		// trigger grid timer immediately
		// NOTE: capture register is updated implicitly
		// mask_main_timer();
		// __HAL_TIM_CLEAR_IT(&htim2, TIM_IT_CC1);
		trigger_main_timer(0);
		unmask_main_timer(0);

		#if MX_VERBOSE_STATISTICS
			mx.stat_counter.num_grid_late++;
		#endif
    }
	else if (d <= radio.max_tb_interval)
	{
		MAIN_TIMER_CC_REG = r.fast_capture + d * FAST_HYBRID_RATIO - GPI_TICK_US_TO_FAST(radio.isr_latency_buffer);
		unmask_main_timer(0);
	}
	// else if trigger tick is in reach for fast timer
	else
	{
		ASSERT_CT(HYBRID_SLOW_RATIO <= 0x10000);
		if (d > 0xF000ul * HYBRID_SLOW_RATIO)
			d = r.hybrid_tick + 0xE000ul * HYBRID_SLOW_RATIO;
		else d = s.next_trigger_tick - radio.max_tb_interval / 2;

		LP_TIMER_CMP_REG = LP_TIMER_CNT_REG + (d - r.hybrid_tick) / HYBRID_SLOW_RATIO;

		// LP_TIMER_CMP_REG += MAX_TB_INTERVAL / HYBRID_SLOW_RATIO;

		s.grid_timer_flag = 1;
		// printf("s3:%lu, %lu\n", s.next_grid_tick - r.hybrid_tick, s.next_grid_tick);
		unmask_slow_timer(1);
    }

	// else if trigger tick is far away
	s.slot_state = (RESYNC == s.slot_state) ? RESYNC : IDLE;
}

//**************************************************************************************************

// mode: 0 = RESYNC quick, 1 = RESYNC normal, 2 = start Tx (for initiator)
static inline __attribute__((always_inline)) void enter_resync(int mode)
{
	#if MX_LBT_AFA
		s.rx_wait_flag = 0;
		s.rx_time = 0;
	#endif

	if (2 == mode)
	{
		// start Tx
		s.slot_state = IDLE;
		s.next_slot_task = TX;
	}
	else if (1 == mode)
	{
		// start Rx by activating RESYNC
		s.slot_state = RESYNC;
		s.next_slot_task = RX;
    }
	else if (STOP == s.next_slot_task)
	{
		// call grid timer ISR (and not timeout timer ISR)
		s.slot_state = IDLE;
    }
	else
	{
		// enter RESYNC
		s.slot_state = RESYNC;
		s.next_slot_task = RX;
    }

	// trigger grid timer immediately (with interrupt masked)
	// NOTE: MAIN_TIMER_CC_REG is updated implicitly
	mask_main_timer();
	__HAL_TIM_CLEAR_IT(&htim2, TIM_IT_CC1);
	mask_slow_timer();
	__HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_FLAG_CMPM);
	trigger_main_timer(1);
	trigger_lowpower_timer(1);

	// if quick (and dirty): don't update s.next_grid_tick to save time
	// ATTENTION: this expects s.next_grid_tick to be initialized and causes the first RESYNC timeout
	// interval to be up to +/- MX_SLOT_LENGTH longer. Furthermore it saves a REORDER_BARRIER(), so
	// don't use quick with interrupts unlocked.
	if (0 != mode)
	{
		// init s.next_grid_tick to precise value
		// ATTENTION: this is important to align the initiator exactly at the grid
		// NOTE: MAIN_TIMER_CC_REG + buffer (see below) is the trigger tick. Grid timer ISR will
		// assume + ISR_LATENCY_BUFFER as polling interval. Hence,
		// grid tick = MAIN_TIMER_CC_REG + buffer + ISR_LATENCY_BUFFER + TX_TO_GRID_OFFSET.
		// NOTE: if mode == 1, the timing is not critical since we enter RESYNC mode
		s.next_grid_tick =
			gpi_tick_fast_to_hybrid(MAIN_TIMER_CC_REG) +
			GPI_TICK_US_TO_HYBRID(20) +
			GPI_TICK_US_TO_HYBRID(radio.isr_latency_buffer) +
			radio.tx_to_grid_offset;
		// s.next_grid_tick_slow = LP_TIMER_CMP_REG + (uint16_t)((uint32_t)(ISR_LATENCY_SLOW + TX_TO_GRID_OFFSET) / (uint32_t)HYBRID_SLOW_RATIO);

		// add some time buffer covering the remaining execution time from here until interrupt
		// gets unlocked
		// ATTENTION: don't do that before calling gpi_tick_fast_to_hybrid() because then
		// gpi_tick_fast_to_hybrid() would work on a future value.
		// MAIN_TIMER_CC_REG += GPI_TICK_US_TO_FAST(20);
		mask_main_timer();
		__HAL_TIM_CLEAR_IT(&htim2, TIM_IT_CC1);
		MAIN_TIMER_CC_REG += GPI_TICK_US_TO_FAST(20);

		REORDER_BARRIER();
		__DMB();
    }

	// unmask grid timer interrupt
	unmask_main_timer(0);
}

//**************************************************************************************************
#define USE_MODEM_LORA
void LED_ISR(mixer_dio0_isr, LED_DIO0_ISR)
{
#if	ENERGEST_CONF_ON
	ENERGEST_ON(ENERGEST_TYPE_IRQ);
#endif
#if defined( USE_MODEM_LORA )
	// Gpi_Slow_Tick_Native dio0_event_tick_slow = LP_TIMER_CNT_REG;
	// Gpi_Hybrid_Tick dio0_event_tick_slow = gpi_tick_hybrid();
	Gpi_Hybrid_Reference r = gpi_tick_hybrid_reference();

	Gpi_Hybrid_Tick dio0_event_tick_slow = r.hybrid_tick;
	// printf("s4:%lu, %lu\n", s.next_grid_tick - r.hybrid_tick, s.next_grid_tick);

    // if Rx
	// NOTE: s.slot_state = RX_RUNNING or RESYNC
	if (TX_RUNNING != s.slot_state)
	{
		#if MX_LBT_AFA
			s.rx_time = 0;
			s.rx_wait_flag = 0;
		#endif
		// Clear Irq
		SX1276Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXDONE );
		// situation at this point: Rx done, radio entering DISABLED state
		gpi_led_off(LED_RX);

		// stop timeout timer
		// -> not needed because this is done implicitely below
		mask_main_timer();
		__HAL_TIM_CLEAR_IT(&htim2, TIM_IT_CC1);
		mask_slow_timer();
		__HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_FLAG_CMPM);

		#if MX_VERBOSE_STATISTICS
		{
			mx.stat_counter.radio_on_time += gpi_tick_fast_native() - s.radio_start_timestamp;
			s.radio_start_timestamp = 0;
		}
		#endif

		#if ENERGEST_CONF_ON
			ENERGEST_OFF(ENERGEST_TYPE_LISTEN);
		#endif

		// if LEN or CRC not ok: regard packet as invisible
		volatile uint8_t packet_len = (uint8_t)SX1276Read( REG_LR_RXNBBYTES );
		volatile uint8_t irqFlags = SX1276Read( REG_LR_IRQFLAGS );
		if( ( ( irqFlags & RFLR_IRQFLAGS_PAYLOADCRCERROR_MASK ) == RFLR_IRQFLAGS_PAYLOADCRCERROR )
		#if MX_PSEUDO_CONFIG
		|| ( packet_len != chirp_config.phy_payload_size + HASH_TAIL_CODE )
		#else
		|| ( packet_len != PHY_PAYLOAD_SIZE )
		#endif
		)
		{
			// printf("wrong:%lu, %lu, %lu\n", packet_len, irqFlags, chirp_config.phy_payload_size);
			GPI_TRACE_MSG_FAST(TRACE_INFO, "broken packet received, LEN: %d, CRC:error", (int)(packet_len));

			#if MX_VERBOSE_STATISTICS
				mx.stat_counter.num_rx_broken++;
			#endif
			SX1276SetOpMode( RFLR_OPMODE_SLEEP );

			// trigger timeout timer (immediately) -> do error handling there
			// NOTE: don't need to unmask timer here because it already is
			trigger_main_timer(0);
			unmask_main_timer(1);
        }
		// if packet ok: process packet
		else
		{
			// printf("ok\n");
			#if MX_PREAMBLE_UPDATE
				if (s.non_receive >= MAX_NON_RECEIVE)
					s.non_receive = 0;
			#endif

			uint8_t RxPacketBuffer[packet_len];
			memset( RxPacketBuffer, 0, ( size_t )packet_len );
			// read rx packet from start address (in data buffer) of last packet received
			SX1276Write( REG_LR_FIFOADDRPTR, SX1276Read( REG_LR_FIFORXCURRENTADDR ) );
			SX1276ReadFifo( RxPacketBuffer, packet_len );
			SX1276SetOpMode( RFLR_OPMODE_SLEEP );
			int j;
			PRINTF("rxpacket:\n");
			for(j = 0; j < 8; j++){
				PRINTF("%d ", RxPacketBuffer[j]);
			}
			PRINTF("\ncoding:\n");

			uint16_t code_tail_hash_rx = Chirp_RSHash((uint8_t *)RxPacketBuffer, chirp_config.phy_payload_size);
			uint16_t hash_code_rx = RxPacketBuffer[packet_len - 2] << 8 | RxPacketBuffer[packet_len - 1];
			if ((hash_code_rx != code_tail_hash_rx) || (!hash_code_rx))
			{
				// trigger timeout timer (immediately) -> do error handling there
				// NOTE: don't need to unmask timer here because it already is
				trigger_main_timer(0);
				unmask_main_timer(1);
			}
			else
			{
				#if MX_DOUBLE_BITMAP
				for(j = 0; j < BITMAP_BYTE ; j++){
					PRINTF("%d ", BITMAP_FIFO[j]);
				}
				PRINTF("\n");
				#endif
				// allocate rx queue destination slot
				Packet	*packet;
				#if MX_PSEUDO_CONFIG
				gpi_memcpy_dma_aligned(&(mx.rx_queue[mx.rx_queue_num_written % NUM_ELEMENTS(mx.rx_queue)]->phy_payload_begin), RxPacketBuffer, chirp_config.phy_payload_size);
				packet = mx.rx_queue[mx.rx_queue_num_written % NUM_ELEMENTS(mx.rx_queue)];
					#if INFO_VECTOR_QUEUE
					gpi_memcpy_dma_inline((uint8_t *)&(mx.code_queue[mx.rx_queue_num_written % NUM_ELEMENTS(mx.code_queue)]->vector[0]), (uint8_t *)(RxPacketBuffer + offsetof(Packet, packet_chunk) + chirp_config.coding_vector.pos), chirp_config.coding_vector.len);
					gpi_memcpy_dma_inline((uint8_t *)&(mx.info_queue[mx.rx_queue_num_written % NUM_ELEMENTS(mx.info_queue)]->vector[0]), (uint8_t *)(RxPacketBuffer + offsetof(Packet, packet_chunk) + chirp_config.info_vector.pos), chirp_config.info_vector.len);
					#endif
				#else
				gpi_memcpy_dma_aligned(&mx.rx_queue[mx.rx_queue_num_written % NUM_ELEMENTS(mx.rx_queue)].phy_payload_begin, RxPacketBuffer, PHY_PAYLOAD_SIZE);
				packet = &mx.rx_queue[mx.rx_queue_num_written % NUM_ELEMENTS(mx.rx_queue)];
					#if INFO_VECTOR_QUEUE
					gpi_memcpy_dma_inline(mx.code_queue[mx.rx_queue_num_written % NUM_ELEMENTS(mx.code_queue)].vector, (uint8_t *)(RxPacketBuffer + offsetof(Packet, coding_vector)), sizeof_member(Packet, coding_vector));
					gpi_memcpy_dma_inline(mx.info_queue[mx.rx_queue_num_written % NUM_ELEMENTS(mx.info_queue)].vector, (uint8_t *)(RxPacketBuffer + offsetof(Packet, info_vector)), sizeof_member(Packet, info_vector));
					#endif
				#endif

				PROFILE_ISR("radio ISR process Rx packet begin");

				int	strobe_resync = 0;

				GPI_TRACE_MSG_FAST(TRACE_VERBOSE, "CRC ok");

				#if MX_VERBOSE_STATISTICS
					mx.stat_counter.num_rx_success++;
				#endif

				// update slot timing control values
				{
					// Gpi_Hybrid_Tick	event_tick;
					// Gpi_Slow_Tick_Native event_tick_slow = dio0_event_tick_slow;
					// Gpi_Hybrid_Tick event_tick = dio0_event_tick_slow;
					/* see "Longshot", ipsn 2019 */
					uint32_t rx_processing_time[6] = {682, 1372, 2850, 5970, 12800, 27000};
					Gpi_Hybrid_Tick event_tick = dio0_event_tick_slow - GPI_TICK_US_TO_HYBRID2(rx_processing_time[chirp_config.lora_sf - 7]);
					// printf("l1:%lu\n", event_tick);

					ASSERT_CT(sizeof(Gpi_Slow_Tick_Native) >= sizeof(uint16_t));

					// if RESYNC requested: realign slot grid based on capture value
					if (RESYNC == s.slot_state)
					{
						// s.next_grid_tick_slow = event_tick_slow - (Gpi_Slow_Tick_Native)(PACKET_AIR_TIME / HYBRID_SLOW_RATIO) + (Gpi_Slow_Tick_Native)MX_SLOT_LENGTH_SLOW;
						// s.next_grid_tick = MAIN_TIMER_CNT_REG + (Gpi_Slow_Tick_Native)(s.next_grid_tick_slow - LP_TIMER_CNT_REG) * HYBRID_SLOW_RATIO;
						#if MX_PSEUDO_CONFIG
						s.next_grid_tick = event_tick - radio.packet_air_time + chirp_config.mx_slot_length;
						#else
						s.next_grid_tick = event_tick - radio.packet_air_time + MX_SLOT_LENGTH;
						#endif

						#if MX_LBT_AFA
							// s.next_grid_tick_slow -= (CHANNEL_DURATION - BITMAP_TIME_SLOW);
							// s.next_grid_tick -= (Gpi_Hybrid_Tick)((CHANNEL_DURATION - BITMAP_TIME_SLOW) * HYBRID_SLOW_RATIO);
							s.next_grid_tick -= (Gpi_Hybrid_Tick)((CHANNEL_DURATION - BITMAP_TIME_SLOW) * HYBRID_SLOW_RATIO);
						#endif

						s.grid_drift = 0;
						s.grid_drift_cumulative = 0;
						s.tx_trigger_offset = radio.tx_to_grid_offset;

						// don't set Rx window to tight after resync because we don't have
						// any information on grid drift yet
						// TODO:
						s.rx_trigger_offset = radio.rx_to_grid_offset + radio.rx_window_max / 2;

						s.slot_state = RX_RUNNING;

						mx.slot_number = packet->slot_number;
						GPI_TRACE_MSG_FAST(TRACE_INFO, "(re)synchronized to slot %u", mx.slot_number);
					}
					// else use phase-lock control loop to track grid
					else
					{
						int32_t	gd;

						if (mx.slot_number != packet->slot_number)
						{
							#if MX_VERBOSE_STATISTICS
								mx.stat_counter.num_rx_slot_mismatch++;
							#endif
							GPI_TRACE_MSG_FAST(TRACE_WARNING, "!!! slot_number mismatch: expected: %u, received: %u !!!",
								mx.slot_number, packet->slot_number);

							mx.slot_number = packet->slot_number;
							// TODO: should not happen -> start RESYNC?
						}
						// update grid drift and next grid tick
						// NOTE: s.grid_drift uses fix point format with ld(GRID_DRIFT_FILTER_DIV) fractional digits

						// compute SFD event deviation
						// NOTE: result is bounded by Rx window size
						// event_tick_slow = (int16_t)event_tick_slow - (int16_t)s.event_tick_nominal_slow;
						// event_tick = (int16_t)(event_tick_slow) * HYBRID_SLOW_RATIO;
						event_tick -= s.event_tick_nominal;

						// keep Rx window in this range (with some margin)
						// TODO:
						s.rx_trigger_offset = radio.rx_window_min;
						gd = (uint32_t)MAX(ABS(s.grid_drift / radio.grid_drift_filter_div), ABS((int32_t)event_tick));
						if (s.rx_trigger_offset < (uint32_t)gd + radio.rx_window_increment)
							s.rx_trigger_offset = (uint32_t)gd + radio.rx_window_increment;
						s.rx_trigger_offset += radio.rx_to_grid_offset;

						// restore nominal grid tick (i.e. remove previously added control value)
						// TODO:
						// s.next_grid_tick_slow -= (int16_t)((s.grid_drift) / (int32_t)(HYBRID_SLOW_RATIO) / (int32_t)(GRID_DRIFT_FILTER_DIV * GRID_TICK_UPDATE_DIV));
						s.next_grid_tick -= s.grid_drift / (radio.grid_drift_filter_div * radio.grid_tick_update_div);

						// update grid drift:
						// new = 1/c * measurement + (c-1)/c * old = old - 1/c * old + 1/c * measurement
						// + GRID_DRIFT_FILTER_DIV / 2 leads to rounding
						s.grid_drift -= (s.grid_drift + radio.grid_drift_filter_div / 2) / radio.grid_drift_filter_div;
						gd = s.grid_drift;
						gd += (int32_t)event_tick;
						s.grid_drift = gd;
						// printf("gd:%ld, %lu\n", gd, 5 * GRID_DRIFT_MAX);

						// if drift exceeds limit: start RESYNC
						// NOTE: saturation could also help since obviously we are still able to receive
						// something (at the moment). Nevertheless it seems that we are in a critical
						// situation, so resync appears adequate as well.
						if (ABS(gd) > 500 * radio.grid_drift_max)
						{
							#if MX_VERBOSE_STATISTICS
								mx.stat_counter.num_grid_drift_overflow++;
								mx.stat_counter.num_resync++;
							#endif
							GPI_TRACE_MSG_FAST(TRACE_INFO, "grid drift overflow: %d > %d -> enter RESYNC",
								ABS(gd), radio.grid_drift_max);

							strobe_resync = 1;
						}

						else
						{
							// update grid tick
							// NOTE: this realizes the proportional term of a PID controller
							// TODO:
							// s.next_grid_tick_slow += (int16_t)((s.grid_drift) / (int32_t)(HYBRID_SLOW_RATIO) / (int32_t)(GRID_DRIFT_FILTER_DIV * GRID_TICK_UPDATE_DIV));
							// s.next_grid_tick = MAIN_TIMER_CNT_REG + (Gpi_Slow_Tick_Native)((s.next_grid_tick_slow - LP_TIMER_CNT_REG) * HYBRID_SLOW_RATIO);
							s.next_grid_tick += s.grid_drift / (radio.grid_drift_filter_div * radio.grid_tick_update_div);

							// keep cumulative grid drift
							// NOTE: this is the base for the integral component of a PID controller
							gd = s.grid_drift_cumulative;
							gd += s.grid_drift;
							if (gd > 0x7FFF)
								gd = 0x7FFF;
							else if (gd < -0x8000l)
								gd = -0x8000l;
							s.grid_drift_cumulative = gd;

							// update tx trigger offset
							// NOTE: this realizes the integral term of a PID controller in an indirect way
							// (through a loopback with potentially high uncertainty on its reaction)
							// TODO:
							s.tx_trigger_offset = s.grid_drift_cumulative / (radio.grid_drift_filter_div * radio.tx_offset_filter_div);

							if ((int32_t)s.tx_trigger_offset < 0)
								s.tx_trigger_offset = 0;
							else if (s.tx_trigger_offset > radio.tx_offset_max)
								s.tx_trigger_offset = radio.tx_offset_max;
							s.tx_trigger_offset += radio.tx_to_grid_offset;

							GPI_TRACE_MSG_FAST(TRACE_VERBOSE, "grid_drift_cum: %d, tx_offset: %u",
								s.grid_drift_cumulative, s.tx_trigger_offset - radio.tx_to_grid_offset);
						}
					}


					// special handling during start-up phase, see tx decision for details
					#if (MX_COORDINATED_TX && !MX_BENCHMARK_NO_COORDINATED_STARTUP)
						#if MX_PSEUDO_CONFIG
						if (!strobe_resync && (mx.slot_number < chirp_config.mx_generation_size) && packet->flags.has_next_payload)
						#else
						if (!strobe_resync && (mx.slot_number < MX_GENERATION_SIZE) && packet->flags.has_next_payload)
						#endif
						{
							// ATTENTION: don't rely on mx.tx_sideload or mx.tx_reserve at this point
							// (mx.tx_sideload may change between here and next trigger tick, mx.tx_reserve
							// may point to an incosistent row since it is not guarded w.r.t. ISR level).
							// Instead, there is a very high probability that mx.tx_packet is ready since
							// we did not TX in current slot (otherwise we wouldn't be here).
							#if MX_PSEUDO_CONFIG
							if (((mx.tx_packet->packet_chunk[chirp_config.rand.pos] & PACKET_IS_READY) >> PACKET_IS_READY_POS) && (STOP != s.next_slot_task))
							#else
							if (mx.tx_packet.is_ready && (STOP != s.next_slot_task))
							#endif
							{
								GPI_TRACE_MSG_FAST(TRACE_VERBOSE, "tx decision: has_next_payload set");
								s.next_slot_task = TX;
							}

							#if MX_DOUBLE_BITMAP
								if ((mx.start_up_flag) & (mx.non_update > 2))
									s.next_slot_task = RX;
							#endif
						}
					#endif

					// s.next_trigger_tick = s.next_grid_tick -
					// 	((s.next_slot_task == TX) ? s.tx_trigger_offset : s.rx_trigger_offset);
					// s.next_trigger_tick_slow = s.next_grid_tick_slow -
					// 	(Gpi_Slow_Tick_Native)(((s.next_slot_task == TX) ? s.tx_trigger_offset : s.rx_trigger_offset)) / HYBRID_SLOW_RATIO;

					s.next_trigger_tick = s.next_grid_tick -
						((s.next_slot_task == TX) ? s.tx_trigger_offset : s.rx_trigger_offset);

					GPI_TRACE_MSG_FAST(TRACE_VERBOSE, "next_grid: %lu, grid_drift: %+d (%+dus)",
						(long)gpi_tick_hybrid_to_us(s.next_grid_tick), s.grid_drift,
						(s.grid_drift >= 0) ?
							(int)gpi_tick_hybrid_to_us(s.grid_drift / radio.grid_drift_filter_div) :
							-(int)gpi_tick_hybrid_to_us(-s.grid_drift / radio.grid_drift_filter_div)
					);
				}


				// check potential queue overflow, if ok: keep packet
				if (mx.rx_queue_num_writing - mx.rx_queue_num_read < NUM_ELEMENTS(mx.rx_queue))
				{
					mx.rx_queue_num_written++;

					#if MX_LBT_AFA
						mx.lbt_coding_check_abort_rx = 0;
					#endif

					// use packet as next Tx sideload (-> fast tx update)
					#if MX_PSEUDO_CONFIG
					if (chirp_config.mx_generation_size != mx.rank)
					#else
					if (MX_GENERATION_SIZE != mx.rank)
					#endif
					{
						#if MX_PSEUDO_CONFIG
						mx.tx_sideload = &(packet->packet_chunk[chirp_config.coding_vector.pos]);
						#else
						mx.tx_sideload = &(packet->coding_vector[0]);
						#endif
					}

					set_event(RX_READY);

					#if MX_VERBOSE_STATISTICS
						mx.stat_counter.num_received++;
					#endif
				}
				#if MX_VERBOSE_STATISTICS
				else
				{
					GPI_TRACE_MSG_FAST(TRACE_INFO, "Rx queue overflow, NW: %u, NR: %u", mx.rx_queue_num_writing, mx.rx_queue_num_read);

					#if MX_PSEUDO_CONFIG
					if (mx.rank < chirp_config.mx_generation_size)
					#else
					if (mx.rank < MX_GENERATION_SIZE)
					#endif
					{
						mx.stat_counter.num_rx_queue_overflow++;
					}
					else mx.stat_counter.num_rx_queue_overflow_full_rank++;
				}
				#endif
				ASSERT_CT(NUM_ELEMENTS(mx.rx_queue) >= 2, single_entry_rx_queue_will_not_work);

				// start RESYNC if requested
				if (strobe_resync)
				{
					// printf("strobe_resync\n");
					#if MX_VERBOSE_STATISTICS
						mx.stat_counter.num_resync++;
					#endif
					enter_resync(0);
				}
				// handover to grid timer (if not already done by enter_resync())
				else
				{
					// printf("start_grid_timer\n");
					start_grid_timer();
				}

				PROFILE_ISR("radio ISR process Rx packet end");
			}
		}
    }

	// if Tx
	else
	{
        // Clear Irq
        SX1276Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_TXDONE );

		// situation at this point: transmission completed, radio entering DISABLED state
		SX1276SetOpMode( RFLR_OPMODE_SLEEP );
		gpi_led_off(LED_TX);


		#if MX_VERBOSE_STATISTICS
		// if (s.radio_start_timestamp & 1)
		{
			mx.stat_counter.radio_on_time += gpi_tick_fast_native() - s.radio_start_timestamp;
			s.radio_start_timestamp = 0;
        }
		#endif

		#if ENERGEST_CONF_ON
			ENERGEST_OFF(ENERGEST_TYPE_TRANSMIT);
		#endif

		GPI_TRACE_MSG_FAST(TRACE_INFO, "Tx done");
	}

	_return_:

	PROFILE_ISR("radio ISR return");

#if	ENERGEST_CONF_ON
	ENERGEST_OFF(ENERGEST_TYPE_IRQ);
#endif

	GPI_TRACE_RETURN_FAST();

#elif defined( USE_MODEM_FSK )
    #error "Please define FSK parameters."
#endif
}


void LED_ISR(mixer_dio3_isr, LED_DIO3_ISR)
{
#if	ENERGEST_CONF_ON
	ENERGEST_ON(ENERGEST_TYPE_IRQ);
#endif

	// TODO:
	// if (Dio3Irq)
	{
		#if defined( USE_MODEM_LORA )

			mask_main_timer();
			__HAL_TIM_CLEAR_IT(&htim2, TIM_IT_CC1);
			mask_slow_timer();
			__HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_FLAG_CMPM);

			// if Rx
			// NOTE: s.slot_state = RX_RUNNING or RESYNC
			if (TX_RUNNING != s.slot_state)
			{
				// Clear Irq
				SX1276Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_VALIDHEADER );
				// if frame detected
				GPI_TRACE_MSG_FAST(TRACE_VERBOSE, "header detected");

				// REG_LR_RXNBBYTES cannot be read before a valid packet is received, so we do not check the payload length here
				ASSERT_CT(~(MX_PREAMBLE_UPDATE & MX_LBT_AFA), inconsistent_program);

				#if MX_PREAMBLE_UPDATE
					if ((mx.start_up_flag) && (mx.slot_number > 1) && ( RESYNC != s.slot_state ))
					{
						s.valid_preamble = 1;
						MAIN_TIMER_CC_REG = MAIN_TIMER_CNT_REG + GPI_TICK_US_TO_FAST2(AFTER_HEADER_BITMAP);
					}
					else
				#elif MX_LBT_AFA
					if (RESYNC != s.slot_state )
					{
						s.lbt_valid_preamble = 1;
						MAIN_TIMER_CC_REG = MAIN_TIMER_CNT_REG + GPI_TICK_US_TO_FAST2(AFTER_HEADER_BITMAP_INFO);
					}
					else
				#endif
					{
						// MAIN_TIMER_CC_REG = MAIN_TIMER_CNT_REG + GPI_TICK_US_TO_FAST2(PAYLOAD_TIME);
						Gpi_Hybrid_Reference r = gpi_tick_hybrid_reference();
						// s.slow_trigger = r.hybrid_tick + GPI_TICK_US_TO_HYBRID2(AFTER_HEADER_TIME);
						s.slow_trigger = r.hybrid_tick + radio.packet_air_time;
						MAIN_TIMER_CC_REG = r.fast_capture + (s.slow_trigger - r.hybrid_tick) * FAST_HYBRID_RATIO;
						LP_TIMER_CMP_REG = LP_TIMER_CNT_REG + 5 * radio.max_tb_interval;
						s.grid_timer_flag = 0;

						#if MX_HEADER_CHECK
							s.valid_header = 1;
							MAIN_TIMER_CC_REG = r.fast_capture + radio.after_header_hybrid * FAST_HYBRID_RATIO;
						#endif
					}
				unmask_main_timer(1);
				unmask_slow_timer(1);
			}

			PROFILE_ISR("radio ISR return");
			GPI_TRACE_RETURN_FAST();

		#elif defined( USE_MODEM_FSK )
			#error "Please define FSK parameters."
		#endif
	}
	// else
	// 	Dio3Irq = 1;

#if	ENERGEST_CONF_ON
	ENERGEST_OFF(ENERGEST_TYPE_IRQ);
#endif
}

//**************************************************************************************************
// helper ISR for grid timer, see start_grid_timer() for details
void LED_ISR(LP_TIMER_ISR_NAME, GPI_LED_5)
{
	// start_grid_timer();
	mask_slow_timer();
	__HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_FLAG_CMPM);
	if (s.grid_timer_flag)
	{
		// printf("s7:%lu, %lu\n", s.next_grid_tick - r.hybrid_tick, s.next_grid_tick);
		start_grid_timer();
	}
	else
	{
		Gpi_Hybrid_Reference	r;
		Gpi_Hybrid_Tick			d;

		r = gpi_tick_hybrid_reference();

		d = s.slow_trigger - r.hybrid_tick;
		if (d > 0xF000ul * HYBRID_SLOW_RATIO)
		{
			d = r.hybrid_tick + 0xE000ul * HYBRID_SLOW_RATIO;
			LP_TIMER_CMP_REG = LP_TIMER_CNT_REG + (d - r.hybrid_tick) / HYBRID_SLOW_RATIO;
			unmask_slow_timer(0);
		}
		else if (d > radio.max_tb_interval)
		{
			d = s.slow_trigger - radio.max_tb_interval / 2;
			LP_TIMER_CMP_REG = LP_TIMER_CNT_REG + (d - r.hybrid_tick) / HYBRID_SLOW_RATIO;
			unmask_slow_timer(0);
		}
	}
}

//**************************************************************************************************

//grid/timeout timer IRQ dispatcher
//ATTENTION: must be subordinated to gpi fast clock ISR if same timer is used and gpi ISR present
// void __attribute__((naked)) MAIN_TIMER_ISR_NAME()
void __attribute__((naked)) mixer_main_timer_isr()
{
	// asm block implements optimized version of the following behavior:
	// switch (s.slot_state)
	// {
	// 	case RESYNC:			timeout_isr(); grid_timer_isr();	break;
	// 	case RX_RUNNING:		timeout_isr();						break;
	// 	case TX_RUNNING, IDLE:	grid_timer_isr();					break;
	// 	default:				undefined behavior
	// }
	__asm__ volatile
	(
		"ldr	r0, 1f				\n"		// r0 = s.slot_state
		"ldrb	r0, [r0]			\n"
		"add	pc, r0				\n"		// jump into vector table (see ARM DUI 0553A for details)
		".align 2					\n"		// ensure alignment and correct offset
		"push.w	{lr}				\n"		//  0: resync: call both ISRs (-> return to here)
		"bl.w	timeout_isr			\n"
		"pop.w	{lr}				\n"
		"b.w	grid_timer_isr		\n"		// 12: grid timer (don't return to here)
		"b.w	timeout_isr			\n"		// 16: timeout (don't return to here)
		"1:							\n"
		".word	%c0					\n"
		: : "i"(&s.slot_state)
	);
}

//**************************************************************************************************

// timeout ISR
// triggered if there was no successful packet transfer in a specific time interval
void LED_ISR(timeout_isr, LED_TIMEOUT_ISR)
{
#if	ENERGEST_CONF_ON
	ENERGEST_ON(ENERGEST_TYPE_IRQ);
#endif

	GPI_TRACE_FUNCTION_FAST();
	PROFILE_ISR("timeout ISR entry");

	mask_main_timer();

	//clear IRQ
	__HAL_TIM_CLEAR_IT(&htim2, TIM_IT_CC1);

	mask_slow_timer();
	__HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_FLAG_CMPM);

	ASSERT_CT(~(MX_PREAMBLE_UPDATE & MX_LBT_AFA), inconsistent_program);

	#if MX_HEADER_CHECK
	if (s.valid_header)
	{
		s.valid_header = 0;
		memset( APP_HEADER_FIFO, 0, HASH_HEADER );
		SX1276Write( REG_LR_FIFOADDRPTR, SX1276Read( REG_LR_FIFORXCURRENTADDR ));
		SX1276ReadFifo( APP_HEADER_FIFO, HASH_HEADER );
		uint32_t app_header = APP_HEADER_FIFO[3] << 24 | APP_HEADER_FIFO[2] << 16 | APP_HEADER_FIFO[1] << 8 | APP_HEADER_FIFO[0];
		// printf("app_header:%x, %x\n", app_header, chirp_config.packet_hash);
		if (app_header == chirp_config.packet_hash)
		{
			Gpi_Hybrid_Reference r = gpi_tick_hybrid_reference();
			s.slow_trigger = r.hybrid_tick + radio.packet_air_time;
			MAIN_TIMER_CC_REG = r.fast_capture + (s.slow_trigger - r.hybrid_tick) * FAST_HYBRID_RATIO;
			LP_TIMER_CMP_REG = LP_TIMER_CNT_REG + 5 * radio.max_tb_interval;
			s.grid_timer_flag = 0;
			__HAL_TIM_CLEAR_IT(&htim2, TIM_IT_CC1);

			unmask_main_timer(1);
			unmask_slow_timer(1);

#if	ENERGEST_CONF_ON
	ENERGEST_OFF(ENERGEST_TYPE_IRQ);
#endif
			GPI_TRACE_RETURN_FAST();
		}
		else
		{
			// printf("header:%x, %x\n", app_header, chirp_config.packet_hash);
			// turn radio off
			SX1276SetOpMode( RFLR_OPMODE_SLEEP );
			gpi_led_off(LED_RX);
		}
	}
	#endif
	// NOTE: being here implies that Rx is active (state = RESYNC or RX_RUNNING)
	#if MX_PREAMBLE_UPDATE
		if ((s.valid_preamble) && (mx.start_up_flag))
		{
			s.valid_preamble = 0;
			memset( BITMAP_FIFO, 0, BITMAP_BYTE );
			// SX1276Write( REG_LR_FIFOADDRPTR, 0 );
			SX1276Write( REG_LR_FIFOADDRPTR, SX1276Read( REG_LR_FIFORXCURRENTADDR ));
			SX1276ReadFifo( BITMAP_FIFO, BITMAP_BYTE );
			// printf("----id:%d, %d, %d, %d, %d, %d\n", BITMAP_FIFO[0], BITMAP_FIFO[1], BITMAP_FIFO[2], BITMAP_FIFO[3],
			// BITMAP_FIFO[4], BITMAP_FIFO[5]);
			memcpy(&mx.packet_header.phy_payload_begin, BITMAP_FIFO, BITMAP_BYTE);
			// printf("slot:%d, %d\n", mx.packet_header.slot_number, mx.packet_header.sender_id);

			// if (1)
			if (mx.packet_header.sender_id < MX_NUM_NODES)
			{
				uint8_t update = 0;
				update = bitmap_update_check_header(&(BITMAP_FIFO[offsetof(Packet, coding_vector)]), mx.tx_packet.sender_id);
				// PRINTF("update:%d\n", update);
				// if ((!update) && (s.non_receive < MAX_NON_RECEIVE))
				if (((!update) || (update == OWN_UPDATE)) && (s.non_receive < MAX_NON_RECEIVE))
				{
					if (update == OWN_UPDATE)
					{
						gpi_memcpy_dma_aligned(mx.tx_packet.coding_vector_2, mx.matrix[mx.tx_packet.sender_id].coding_vector, sizeof(mx.matrix[0].coding_vector) + sizeof(mx.matrix[0].payload));
						unwrap_coding_vector(mx.tx_packet.coding_vector, mx.tx_packet.coding_vector_2, 1);
						mx.tx_packet.is_ready = 1;
						mx.tx_sideload = NULL;

						mx.next_task_own_update = 1;
					}
					mx.preamble_update_abort_rx = 1;

					s.non_receive ++;

					mx.non_update += 2;

					// mask interrupts (radio)
					// NOTE: stopping timeout timer is not needed since this is done implicitely below
					SX1276Write( REG_LR_IRQFLAGSMASK, 0xFFFF );

					// turn radio off
					SX1276SetOpMode( RFLR_OPMODE_SLEEP );
					gpi_led_off(LED_RX | LED_TX);

					#if MX_VERBOSE_STATISTICS
					if (s.radio_start_timestamp & 1)
					{
						mx.stat_counter.radio_on_time += gpi_tick_fast_native() - s.radio_start_timestamp;
						s.radio_start_timestamp = 0;
					}
					#endif

					#if ENERGEST_CONF_ON
						ENERGEST_OFF(ENERGEST_TYPE_LISTEN);
					#endif

					// update next trigger tick
					s.next_trigger_tick = s.next_grid_tick -
						((s.next_slot_task == TX) ? s.tx_trigger_offset : s.rx_trigger_offset);

					s.next_trigger_tick_slow = s.next_grid_tick_slow -
						(Gpi_Slow_Tick_Native)((s.next_slot_task == TX) ? s.tx_trigger_offset : s.rx_trigger_offset) / HYBRID_SLOW_RATIO;

					// handover to grid timer
					// NOTE: this is done automatically while RESYNC is running
					// (more precisely: if RESYNC was active before entering current function)
					start_grid_timer();

					GPI_TRACE_RETURN_FAST();
				}
				if (s.non_receive < MAX_NON_RECEIVE)
					mx.non_update = 0;
			}

			MAIN_TIMER_CC_REG = MAIN_TIMER_CNT_REG + GPI_TICK_US_TO_FAST2(PAYLOAD_TIME);
			unmask_main_timer(1);

			GPI_TRACE_RETURN_FAST();
		}
	#elif MX_LBT_AFA
		if (s.lbt_valid_preamble)
		{
			s.lbt_valid_preamble = 0;
			memset( CODING_VECTOR_FIFO, 0, BITMAP_BYTE_INFO);
			SX1276Write( REG_LR_FIFOADDRPTR, SX1276Read( REG_LR_FIFORXCURRENTADDR ));
			SX1276ReadFifo( CODING_VECTOR_FIFO, BITMAP_BYTE_INFO );
			memcpy(&mx.lbt_packet_header.phy_payload_begin, CODING_VECTOR_FIFO, BITMAP_BYTE_INFO);
			// if (mx.occupied_channel_flag)
			// 	update_rx_channel(mx.lbt_packet_header.full_channel);
			if (mx.lbt_packet_header.sender_id < MX_NUM_NODES)
			{
				uint8_t coding_check;
				coding_check = coding_vector_check(&(CODING_VECTOR_FIFO[offsetof(Packet, coding_vector)]));
				if (!coding_check)
				{
					mx.lbt_coding_check_abort_rx = 1;
					SX1276Write( REG_LR_IRQFLAGSMASK, 0xFFFF );
					// turn radio off
					SX1276SetOpMode( RFLR_OPMODE_SLEEP );
					gpi_led_off(LED_RX);

					#if MX_VERBOSE_STATISTICS
					if (s.radio_start_timestamp & 1)
					{
						mx.stat_counter.radio_on_time += gpi_tick_fast_native() - s.radio_start_timestamp;
						s.radio_start_timestamp = 0;
					}
					#endif

					#if ENERGEST_CONF_ON
						ENERGEST_OFF(ENERGEST_TYPE_LISTEN);
					#endif

					if (s.rx_time < MAX_RX_TIME_IN_GRID)
					{
						s.rx_time ++;
						s.slot_state = IDLE;
						// (Gpi_Slow_Tick_Native)(s.rx_trigger_offset + ISR_LATENCY_SLOW) / HYBRID_SLOW_RATIO +
						// CHANNEL_DURATION * s.rx_time + CHANNEL_DURATION - BITMAP_TIME_SLOW;
						// MAIN_TIMER_CC_REG = MAIN_TIMER_CNT_REG + (Gpi_Slow_Tick_Native)(LP_TIMER_CMP_REG - LP_TIMER_CNT_REG) * HYBRID_SLOW_RATIO;
						// unmask_main_timer(1);

						Gpi_Hybrid_Reference r = gpi_tick_hybrid_reference();
						Gpi_Hybrid_Tick	t = s.next_grid_tick - MX_SLOT_LENGTH - s.rx_trigger_offset - radio.isr_latency_slow +
								(CHANNEL_DURATION * s.rx_time + CHANNEL_DURATION - BITMAP_TIME_SLOW) * HYBRID_SLOW_RATIO;
						MAIN_TIMER_CC_REG = r.fast_capture + (t - r.hybrid_tick) * FAST_HYBRID_RATIO;
						// s.hybrid_trigger = t;

						unmask_main_timer(1);
					}
					else
					{
						s.rx_time = 0;
						s.rx_wait_flag = 0;

						// update next trigger tick
						// s.next_trigger_tick = s.next_grid_tick -
						// 	((s.next_slot_task == TX) ? s.tx_trigger_offset : s.rx_trigger_offset);

						// s.next_trigger_tick_slow = s.next_grid_tick_slow -
						// 	(Gpi_Slow_Tick_Native)((s.next_slot_task == TX) ? s.tx_trigger_offset : s.rx_trigger_offset) / HYBRID_SLOW_RATIO;

						s.next_trigger_tick = s.next_grid_tick -
							((s.next_slot_task == TX) ? s.tx_trigger_offset : s.rx_trigger_offset);

						// handover to grid timer
						// NOTE: this is done automatically while RESYNC is running
						// (more precisely: if RESYNC was active before entering current function)
						start_grid_timer();
					}
					GPI_TRACE_RETURN_FAST();
				}
			}
			// MAIN_TIMER_CC_REG = MAIN_TIMER_CNT_REG + GPI_TICK_US_TO_FAST2(PAYLOAD_TIME);
			MAIN_TIMER_CC_REG = MAIN_TIMER_CNT_REG + GPI_TICK_US_TO_FAST2(PAYLOAD_TIME);

			unmask_main_timer(1);

			GPI_TRACE_RETURN_FAST();
		}
	#endif

	// mask interrupts (radio)
	// NOTE: stopping timeout timer is not needed since this is done implicitely below
	SX1276Write( REG_LR_IRQFLAGSMASK, 0xFFFF );

	// turn radio off
	SX1276SetOpMode( RFLR_OPMODE_SLEEP );
	gpi_led_off(LED_RX | LED_TX);

	#if MX_VERBOSE_STATISTICS
	if (s.radio_start_timestamp & 1)
	{
		mx.stat_counter.radio_on_time += gpi_tick_fast_native() - s.radio_start_timestamp;
		s.radio_start_timestamp = 0;
	}
	#endif

	#if ENERGEST_CONF_ON
		ENERGEST_OFF(ENERGEST_TYPE_LISTEN);
	#endif

	if (s.slot_state != RESYNC)
	{
		if (s.slot_state == RX_RUNNING)
		{
			#if MX_LBT_AFA
				if ((s.rx_time < MAX_RX_TIME_IN_GRID) && (s.rx_wait_flag))
				{
					s.rx_time ++;
					s.slot_state = IDLE;
					// (Gpi_Slow_Tick_Native)(s.rx_trigger_offset + ISR_LATENCY_SLOW) / HYBRID_SLOW_RATIO +
					// CHANNEL_DURATION * s.rx_time + CHANNEL_DURATION - BITMAP_TIME_SLOW;
					// MAIN_TIMER_CC_REG = MAIN_TIMER_CNT_REG + (Gpi_Slow_Tick_Native)(LP_TIMER_CMP_REG - LP_TIMER_CNT_REG) * HYBRID_SLOW_RATIO;
					Gpi_Hybrid_Reference r = gpi_tick_hybrid_reference();
					Gpi_Hybrid_Tick	t = s.next_grid_tick - MX_SLOT_LENGTH - s.rx_trigger_offset - radio.isr_latency_slow +
							(CHANNEL_DURATION * s.rx_time + CHANNEL_DURATION - BITMAP_TIME_SLOW) * HYBRID_SLOW_RATIO;
					MAIN_TIMER_CC_REG = r.fast_capture + (t - r.hybrid_tick) * FAST_HYBRID_RATIO;
					unmask_main_timer(1);
					GPI_TRACE_RETURN_FAST();
				}
				if (s.rx_time >= MAX_RX_TIME_IN_GRID)
				{
					s.rx_time = 0;
					s.rx_wait_flag = 0;
				}
			#endif

			// widen Rx time window
			#if MX_DOUBLE_BITMAP
				if (mx.start_up_flag)
					s.rx_trigger_offset = s.rx_trigger_offset;
					// s.rx_trigger_offset += RX_WINDOW_INCREMENT / 2;
				else
			#endif
				{
					#if MX_LBT_AFA
						// the last rx time
						if (!s.rx_time)
					#endif
						{
							// s.rx_trigger_offset += radio.rx_window_increment;
						}
				}
		}

		// if Rx window exceeds limit: start RESYNC
		// NOTE: we start RESYNC immediately (instead of saturating) because we probably lost
		// synchronization and hence do not expect to receive something anymore
		// if (s.rx_trigger_offset > RX_TO_GRID_OFFSET + RX_WINDOW_MAX)
		if (s.rx_trigger_offset > radio.rx_to_grid_offset + 10 * radio.rx_window_max)
		{
			#if MX_VERBOSE_STATISTICS
				mx.stat_counter.num_rx_window_overflow++;
				mx.stat_counter.num_resync++;
			#endif
			GPI_TRACE_MSG_FAST(TRACE_INFO, "Rx window overflow: %u > %u -> enter RESYNC",
				s.rx_trigger_offset, radio.rx_to_grid_offset + radio.rx_window_max);
			enter_resync(0);
        }
		else
		{
			// update next trigger tick
			s.next_trigger_tick = s.next_grid_tick -
				((s.next_slot_task == TX) ? s.tx_trigger_offset : s.rx_trigger_offset);

			// s.next_trigger_tick_slow = s.next_grid_tick_slow -
			// 	(Gpi_Slow_Tick_Native)((s.next_slot_task == TX) ? s.tx_trigger_offset : s.rx_trigger_offset) / HYBRID_SLOW_RATIO;

			// handover to grid timer
			// NOTE: this is done automatically while RESYNC is running
			// (more precisely: if RESYNC was active before entering current function)
			start_grid_timer();
		}
	}

	PROFILE_ISR("timeout ISR return");
#if	ENERGEST_CONF_ON
	ENERGEST_OFF(ENERGEST_TYPE_IRQ);
#endif
	GPI_TRACE_RETURN_FAST();
}

//**************************************************************************************************

// grid timer ISR
// this is one of the central transport layer routines
void LED_ISR(grid_timer_isr, LED_GRID_TIMER_ISR)
{
#if	ENERGEST_CONF_ON
	ENERGEST_ON(ENERGEST_TYPE_IRQ);
#endif

	GPI_TRACE_FUNCTION_FAST();
	PROFILE_ISR("grid timer ISR entry");
	// printf("grid_timer_isr\n");
	mask_main_timer();

	//clear IRQ
	__HAL_TIM_CLEAR_IT(&htim2, TIM_IT_CC1);

	mask_slow_timer();
	__HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_FLAG_CMPM);
	// printf("header:%d, %d, %d, %d\n", AFTER_HEADER_BITMAP, BITMAP_TIME, HEADER_TIME, BITMAP_BYTE);
	// printf("header:%lu, %lu, %lu\n", PACKET_AIR_TIME, GPI_TICK_US_TO_HYBRID2(42000));
	// printf("header1:%lu, %lu, %lu\n", GRID_DRIFT_MAX, AFTER_HEADER_TIME, HEADER_TIME);

	// if STOP requested: stop
	if (STOP == s.next_slot_task)
	{
		#if MX_VERBOSE_STATISTICS
			mx.stat_counter.slot_off = mx.slot_number;	// the old one (viewpoint: turn off after last processing)
		#endif
		set_event(STOPPED);
		GPI_TRACE_MSG_FAST(TRACE_INFO, "transport layer stopped");

		GPI_TRACE_RETURN_FAST();
    }

	// if Rx
	#if MX_LBT_AFA
	if ((RESYNC == s.slot_state) || ((RX == s.next_slot_task) && (!s.tx_time)) || (s.rx_time))
	#else
	if ((RESYNC == s.slot_state) || (RX == s.next_slot_task))
	#endif
	{
		PROFILE_ISR("grid timer ISR start Rx begin");

		Gpi_Hybrid_Reference	r;
		Gpi_Fast_Tick_Native 	trigger_tick;
		// Gpi_Slow_Tick_Native	trigger_tick_slow;
		int_fast8_t				late = 1;

		#if MX_LBT_AFA
			if (!s.rx_wait_flag)
			{
				s.rx_channel = 0;
				// LP_TIMER_CMP_REG += CHANNEL_DURATION - BITMAP_TIME_SLOW;
				// MAIN_TIMER_CC_REG = MAIN_TIMER_CNT_REG + (Gpi_Slow_Tick_Native)(LP_TIMER_CMP_REG - LP_TIMER_CNT_REG) * HYBRID_SLOW_RATIO;
				MAIN_TIMER_CC_REG += (CHANNEL_DURATION - BITMAP_TIME_SLOW) * HYBRID_SLOW_RATIO;

				r = gpi_tick_hybrid_reference();
				s.hybrid_trigger = r.hybrid_tick + (MAIN_TIMER_CC_REG - r.fast_capture) * FAST_HYBRID_RATIO;

				s.rx_wait_flag = 1;
				unmask_main_timer(1);
				GPI_TRACE_RETURN_FAST();
			}
		#endif

		// compute exact trigger time
		// trigger_tick = gpi_tick_fast_to_hybrid(MAIN_TIMER_CC_REG) + GPI_TICK_US_TO_FAST(ISR_LATENCY_BUFFER);
		// trigger_tick = s.next_grid_tick;
		// trigger_tick = s.hybrid_trigger;
		trigger_tick = MAIN_TIMER_CC_REG + GPI_TICK_US_TO_FAST(radio.isr_latency_buffer);

		// if(!gpi_tick_compare_slow_native(LP_TIMER_CNT_REG, LP_TIMER_CMP_REG))
		// 	trigger_tick = MAIN_TIMER_CNT_REG + (Gpi_Slow_Tick_Native)(LP_TIMER_CMP_REG - LP_TIMER_CNT_REG + (Gpi_Slow_Tick_Native)ISR_LATENCY_SLOW / HYBRID_SLOW_RATIO) * HYBRID_SLOW_RATIO;
		// else
		// 	trigger_tick = MAIN_TIMER_CNT_REG;

		// trigger_tick_slow = LP_TIMER_CMP_REG + (Gpi_Slow_Tick_Native)((Gpi_Fast_Tick_Native)(ISR_LATENCY_SLOW) / (Gpi_Fast_Tick_Native)HYBRID_SLOW_RATIO);

		// rx begin
		#if MX_LBT_AFA
			gpi_radio_set_channel(mx.current_channel[s.rx_channel]);
			if (RESYNC != s.slot_state)
				s.rx_channel ++;
		#endif
		#if MX_PSEUDO_CONFIG
		assert_reset((chirp_config.lora_bw >= 7)&&(chirp_config.lora_bw <= 9));
		#else
		ASSERT_CT((LORA_BANDWIDTH >= 7)&&(LORA_BANDWIDTH <= 9), worked_LORA_BANDWIDTH);
		#endif

		SX1276Write( REG_LR_INVERTIQ, ( ( SX1276Read( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_OFF ) );
		SX1276Write( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_OFF );

		SX1276Write( REG_LR_DETECTOPTIMIZE, SX1276Read( REG_LR_DETECTOPTIMIZE ) & 0x7F );
		SX1276Write( REG_LR_IFFREQ2, 0x00 );
		#if MX_PSEUDO_CONFIG
		if(chirp_config.lora_bw != 9)
		#else
		if(LORA_BANDWIDTH != 9)
		#endif

		{
			SX1276Write( REG_LR_IFFREQ1, 0x40 );
		}
		else
		{
			SX1276Write( REG_LR_DETECTOPTIMIZE, SX1276Read( REG_LR_DETECTOPTIMIZE ) | 0x80 );
		}

		if (RESYNC != s.slot_state)
		{
			// wait until trigger time has been reached
			PROFILE_ISR();

			// while (gpi_tick_compare_slow_native(gpi_tick_slow_native(), trigger_tick_slow) < 0)
			// while (gpi_tick_compare_hybrid(gpi_tick_hybrid(), trigger_tick) <= 0)
			while (gpi_tick_compare_fast_native(gpi_tick_fast_native(), trigger_tick) <= 0)
				late = 0;

			PROFILE_ISR();

			// set radio in rx mode
			SX1276SetOpMode( RFLR_OPMODE_RECEIVER );
			gpi_led_on(LED_RX);

			#if MX_VERBOSE_STATISTICS
				if (late)
					mx.stat_counter.num_rx_late++;
			#endif
		}

		// during RESYNC or if we are late: start manually (immediately)
		if (late)
		{
			// set radio in rx mode
			// while (gpi_tick_compare_fast_native(gpi_tick_fast_native(), trigger_tick) <= 0);
			SX1276SetOpMode( RFLR_OPMODE_RECEIVER );
			gpi_led_on(LED_RX);
			#if MX_VERBOSE_STATISTICS
				trigger_tick = gpi_tick_fast_native();
			#endif
        }

		#if MX_VERBOSE_STATISTICS
			s.radio_start_timestamp = trigger_tick | 1;
		#endif

		#if ENERGEST_CONF_ON
			ENERGEST_ON(ENERGEST_TYPE_LISTEN);
		#endif

		// unmask IRQs
		// enable RxDone, PayloadCrcError and ValidHeader interrupt
		SX1276Write( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
											//RFLR_IRQFLAGS_RXDONE |
											//RFLR_IRQFLAGS_PAYLOADCRCERROR |
											//RFLR_IRQFLAGS_VALIDHEADER |
											RFLR_IRQFLAGS_TXDONE |
											RFLR_IRQFLAGS_CADDONE |
											RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
											RFLR_IRQFLAGS_CADDETECTED );

		SX1276Write( REG_DIOMAPPING1, ( SX1276Read( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK &
		RFLR_DIOMAPPING1_DIO3_MASK) | RFLR_DIOMAPPING1_DIO0_00 | RFLR_DIOMAPPING1_DIO3_01);

		#if MX_HEADER_CHECK
			s.valid_header = 0;
		#endif

		#if MX_PREAMBLE_UPDATE
			s.valid_preamble = 0;
		#endif

		// allocate rx queue destination slot
        SX1276Write( REG_LR_FIFORXBASEADDR, 0 );
		// SX1276Write( REG_LR_FIFOADDRPTR, 0 );

		mx.rx_queue_num_writing = mx.rx_queue_num_written + 1;
		// s.event_tick_nominal_slow = s.next_grid_tick_slow + (Gpi_Slow_Tick_Native)((PACKET_AIR_TIME) / HYBRID_SLOW_RATIO);
		s.event_tick_nominal = s.next_grid_tick + radio.packet_air_time;
		// GPI_TICK_US_TO_HYBRID2(42734‬)
		// GPI_TICK_MS_TO_HYBRID(42734‬)
		// printf("l2:%lu\n", s.event_tick_nominal);

		#if MX_LBT_AFA
			s.event_tick_nominal += (CHANNEL_DURATION * s.rx_time + CHANNEL_DURATION - BITMAP_TIME_SLOW) * HYBRID_SLOW_RATIO;
			if (s.rx_time)
				s.event_tick_nominal -= MX_SLOT_LENGTH;
		#endif

		r = gpi_tick_hybrid_reference();

		// if RESYNC: restart grid timer (-> potentially long interval)
		// NOTE: timeout timer is called implicitly while RESYNC
		if (s.slot_state == RESYNC)
		{
			// printf("s.slot_state == RESYNC\n");
			// ATTENTION: don't do s.next_grid_tick += MX_SLOT_LENGTH_RESYNC because grid timer is also
			// triggered by frames from interferers (Rx -> SFD -> ... (broken/invalid) -> timeout
			// -> grid timer) and hence current time might be far away from s.next_grid_tick. With
			// s.next_grid_tick += MX_SLOT_LENGTH_RESYNC, s.next_grid_tick could end up in the far
			// future if it gets incremented frequently.
			// s.next_grid_tick = r.hybrid_tick + MX_SLOT_LENGTH_RESYNC;

			// s.next_trigger_tick = s.next_grid_tick - RX_TO_GRID_OFFSET;
			// s.next_trigger_tick_slow = s.next_grid_tick_slow - ((Gpi_Slow_Tick_Native)RX_TO_GRID_OFFSET / HYBRID_SLOW_RATIO);

			// MAIN_TIMER_CC_REG = s.next_trigger_tick - ISR_LATENCY_SLOW;
			// LP_TIMER_CMP_REG = s.next_trigger_tick_slow - ((Gpi_Slow_Tick_Native)ISR_LATENCY_SLOW / HYBRID_SLOW_RATIO);

			// unmask_main_timer(1);
			#if MX_PSEUDO_CONFIG
			s.next_grid_tick = r.hybrid_tick + ((chirp_config.mx_slot_length * 5) / 2);
			#else
			s.next_grid_tick = r.hybrid_tick + MX_SLOT_LENGTH_RESYNC;
			#endif
			s.next_trigger_tick = s.next_grid_tick;
			start_grid_timer();
		}

		// else start timeout timer
		else
		{
			// MAIN_TIMER_CC_REG = MAIN_TIMER_CNT_REG + s.rx_trigger_offset - RX_TO_GRID_OFFSET +
			// GPI_TICK_US_TO_HYBRID2(HEADER_TIME + 712) + 1 * GRID_DRIFT_OFFSET;
			Gpi_Hybrid_Tick		t;
			t = s.next_grid_tick + s.rx_trigger_offset - radio.rx_to_grid_offset +
			GPI_TICK_US_TO_HYBRID2(radio.header_time) + 4 * radio.grid_drift_offset;
			MAIN_TIMER_CC_REG = r.fast_capture + (t - r.hybrid_tick) * FAST_HYBRID_RATIO;
			s.slow_trigger = t;
			LP_TIMER_CMP_REG = LP_TIMER_CNT_REG + 5 * radio.max_tb_interval;
			s.grid_timer_flag = 0;
			__HAL_TIM_CLEAR_IT(&htim2, TIM_IT_CC1);

			unmask_main_timer(1);
			unmask_slow_timer(1);
			s.slot_state = RX_RUNNING;

			GPI_TRACE_MSG_FAST(TRACE_VERBOSE, "timeout: %lu", (long)gpi_tick_hybrid_to_us(t));

			#if MX_LBT_AFA
			if (s.rx_time)
				GPI_TRACE_RETURN_FAST();
			#endif
		}

		PROFILE_ISR("grid timer ISR start Rx end");
		GPI_TRACE_MSG_FAST(TRACE_INFO, "Rx started");
	}

	// if Tx
	else
	{
		PROFILE_ISR("grid timer ISR start Tx begin");

		#if MX_PSEUDO_CONFIG
		assert_reset(!(chirp_config.packet_len % sizeof(uint_fast_t)));
		#else
		ASSERT_CT(!((uintptr_t)&mx.tx_packet % sizeof(uint_fast_t)), alignment_issue);
		#endif
		ASSERT_CT(!((uintptr_t)&s.tx_fifo % sizeof(uint_fast_t)), alignment_issue);

		Gpi_Fast_Tick_Native 	trigger_tick;
		// Gpi_Slow_Tick_Native	trigger_tick_slow;
		int_fast8_t				late = 1;
		uint8_t					*p;

		#if MX_LBT_AFA
			// step1: guard time
			if ((!s.tx_guard_flag) && (s.tx_time < MAX_TX_TIME_IN_GRID))
			{
				// LP_TIMER_CMP_REG = s.next_grid_tick_slow + CHANNEL_DURATION * s.tx_channel + AFA_GUARD_TIME;
				// MAIN_TIMER_CC_REG = MAIN_TIMER_CNT_REG + (Gpi_Slow_Tick_Native)(LP_TIMER_CMP_REG - LP_TIMER_CNT_REG) * HYBRID_SLOW_RATIO;

				Gpi_Hybrid_Reference r = gpi_tick_hybrid_reference();
				Gpi_Hybrid_Tick	t = s.next_grid_tick + (CHANNEL_DURATION * s.tx_channel + AFA_GUARD_TIME) * HYBRID_SLOW_RATIO;
				MAIN_TIMER_CC_REG = r.fast_capture + (t - r.hybrid_tick) * FAST_HYBRID_RATIO;

				s.tx_guard_flag = 1;
				unmask_main_timer(1);
				s.tx_time ++;
				GPI_TRACE_RETURN_FAST();
			}
			// step2: CCA time: begin CCA in rx mode
			else if (!s.tx_dead_flag)
			{
				gpi_radio_set_channel(mx.current_channel[s.tx_channel]);
				SX1276SetOpMode( RFLR_OPMODE_RECEIVER );
				// trigger_tick_slow = LP_TIMER_CMP_REG + CCA_INTERVAL;
				trigger_tick = MAIN_TIMER_CC_REG + CCA_INTERVAL * HYBRID_SLOW_RATIO;
				RadioLoRaPacketHandler_t read_value;
				uint8_t rssi_detected = 0;
				gpi_led_on(LED_RX);
				// while (gpi_tick_compare_slow_native(gpi_tick_slow_native(), trigger_tick_slow) < 0)
				while (gpi_tick_compare_fast_native(gpi_tick_fast_native(), trigger_tick) <= 0)
				{
					read_value = gpi_read_rssi(0);
					if (read_value.RssiValue > SENSITIVITY_IN_DBM)
					{
						SX1276SetOpMode( RFLR_OPMODE_SLEEP );
						gpi_led_off(LED_RX);
						rssi_detected = 1;
						break;
					}
				}
				SX1276SetOpMode( RFLR_OPMODE_SLEEP );
				gpi_led_off(LED_RX);
				// step3: dead time or next channel
				if (rssi_detected)
				{
					uint8_t is_the_last_channel = 0, tx_channel_tmp;
					tx_channel_tmp = assign_tx_channel(s.tx_channel);
					if (s.tx_channel >= tx_channel_tmp)
					{
						is_the_last_channel = 1;
					}
					s.tx_channel = tx_channel_tmp;
					if ((s.tx_time >= MAX_TX_TIME_IN_GRID) || (is_the_last_channel))
					{
						s.tx_time = 0;
						goto slot_state_;
					}
					else
					{
						s.tx_guard_flag = 0;
						s.tx_dead_flag = 0;
						// LP_TIMER_CMP_REG = s.next_grid_tick_slow -
						// (Gpi_Slow_Tick_Native)(s.tx_trigger_offset + ISR_LATENCY_SLOW) / HYBRID_SLOW_RATIO +
						// CHANNEL_DURATION * s.tx_channel;
						// MAIN_TIMER_CC_REG = MAIN_TIMER_CNT_REG + (Gpi_Slow_Tick_Native)(LP_TIMER_CMP_REG - LP_TIMER_CNT_REG) * HYBRID_SLOW_RATIO;

						Gpi_Hybrid_Reference r = gpi_tick_hybrid_reference();
						Gpi_Hybrid_Tick	t = s.next_grid_tick - s.tx_trigger_offset + radio.isr_latency_slow +
						CHANNEL_DURATION * s.tx_channel * HYBRID_SLOW_RATIO;
						MAIN_TIMER_CC_REG = r.fast_capture + (t - r.hybrid_tick) * FAST_HYBRID_RATIO;

						unmask_main_timer(1);
						GPI_TRACE_RETURN_FAST();
					}
				}
				else
				{
					// LP_TIMER_CMP_REG += CCA_INTERVAL + DEAD_TIME;
					// MAIN_TIMER_CC_REG = MAIN_TIMER_CNT_REG + (Gpi_Slow_Tick_Native)(LP_TIMER_CMP_REG - LP_TIMER_CNT_REG) * HYBRID_SLOW_RATIO;

					Gpi_Hybrid_Reference r = gpi_tick_hybrid_reference();
					MAIN_TIMER_CC_REG += (CCA_INTERVAL + DEAD_TIME) * HYBRID_SLOW_RATIO;
					s.hybrid_trigger = r.hybrid_tick + (MAIN_TIMER_CC_REG - r.fast_capture) * FAST_HYBRID_RATIO;

					s.tx_dead_flag = 1;
					s.tx_time = 0;
					unmask_main_timer(1);
					GPI_TRACE_RETURN_FAST();
				}
			}
		#endif

		// compute exact trigger time
		// if(!gpi_tick_compare_slow_native(LP_TIMER_CNT_REG, LP_TIMER_CMP_REG))
		// 	trigger_tick = MAIN_TIMER_CNT_REG + (Gpi_Slow_Tick_Native)(LP_TIMER_CMP_REG - LP_TIMER_CNT_REG + (Gpi_Slow_Tick_Native)ISR_LATENCY_SLOW / HYBRID_SLOW_RATIO) * HYBRID_SLOW_RATIO;
		// else
		// 	trigger_tick = MAIN_TIMER_CNT_REG;
		// trigger_tick_slow = LP_TIMER_CMP_REG + (Gpi_Slow_Tick_Native)((Gpi_Fast_Tick_Native)(ISR_LATENCY_SLOW) / (Gpi_Fast_Tick_Native)HYBRID_SLOW_RATIO);

		// trigger_tick = gpi_tick_fast_to_hybrid(MAIN_TIMER_CC_REG) + GPI_TICK_US_TO_FAST(ISR_LATENCY_BUFFER);
		// trigger_tick = s.next_grid_tick;
		// trigger_tick = s.hybrid_trigger;
		trigger_tick = MAIN_TIMER_CC_REG + GPI_TICK_US_TO_FAST(radio.isr_latency_buffer);

		// wait until trigger time has been reached
		PROFILE_ISR();

		// while (gpi_tick_compare_slow_native(gpi_tick_slow_native(), trigger_tick_slow) < 0)
		// while (gpi_tick_compare_hybrid(gpi_tick_hybrid(), trigger_tick) <= 0)
		while (gpi_tick_compare_fast_native(gpi_tick_fast_native(), trigger_tick) < 0)
			late = 0;
		PROFILE_ISR();

		SX1276SetOpMode( RFLR_OPMODE_TRANSMITTER );

		#if MX_LBT_AFA
			s.tx_guard_flag = 0;
			s.tx_dead_flag = 0;
		#endif

		// if we are late: start manually (immediately)
		if (late)
		{
			// SX1276SetOpMode( RFLR_OPMODE_TRANSMITTER );

			#if MX_VERBOSE_STATISTICS
				trigger_tick = gpi_tick_fast_native();
			#endif
        }

		// init FIFO
		#if MX_PSEUDO_CONFIG
		SX1276Write( REG_LR_PAYLOADLENGTH, chirp_config.phy_payload_size + HASH_TAIL_CODE);
		#else
		SX1276Write( REG_LR_PAYLOADLENGTH, PHY_PAYLOAD_SIZE );
		#endif
		SX1276Write( REG_LR_INVERTIQ, ( ( SX1276Read( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_OFF ) );
		SX1276Write( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_OFF );
		// Full buffer used for Tx
		SX1276Write( REG_LR_FIFOTXBASEADDR, 0 );
		SX1276Write( REG_LR_FIFOADDRPTR, 0 );

		gpi_led_on(LED_TX);
		#if MX_VERBOSE_STATISTICS
			s.radio_start_timestamp = trigger_tick | 1;
			if (late)
				mx.stat_counter.num_tx_late++;
		#endif

		#if ENERGEST_CONF_ON
			ENERGEST_ON(ENERGEST_TYPE_TRANSMIT);
		#endif

		// finalize header
		{
			mx.tx_packet->app_header = chirp_config.packet_hash;
			uint16_t slot_number = mx.slot_number + 1;

			#if MX_PSEUDO_CONFIG
			mx.tx_packet->slot_number = slot_number;
			mx.tx_packet->flags.all = 0;
			#else
			mx.tx_packet.slot_number = slot_number;
			mx.tx_packet.flags.all = 0;
			#endif

			#if MX_PSEUDO_CONFIG
			if ((slot_number < chirp_config.mx_generation_size) && (0 == mx.matrix[slot_number]->birth_slot))
			#else
			if ((slot_number < MX_GENERATION_SIZE) && (0 == mx.matrix[slot_number].birth_slot))
			#endif
			{
				#if MX_PSEUDO_CONFIG
				mx.tx_packet->flags.has_next_payload = 1;
				#else
				mx.tx_packet.flags.has_next_payload = 1;
				#endif
			}

			#if MX_PSEUDO_CONFIG
			if (chirp_config.mx_generation_size == mx.rank)
			#else
			if (MX_GENERATION_SIZE == mx.rank)
			#endif
			{
				#if MX_PSEUDO_CONFIG
				mx.tx_packet->flags.is_full_rank = 1;
				#else
				mx.tx_packet.flags.is_full_rank = 1;
				#endif
				#if MX_SMART_SHUTDOWN
					// ATTENTION: testing mx.have_full_rank_neighbor is important
					// to avoid that initiator turns off immediately in 1-to-all scenarios
					if ((0 == mx_present_head->mx_num_nodes) && (mx.have_full_rank_neighbor))
					{
						#if MX_PSEUDO_CONFIG
						mx.tx_packet->flags.radio_off = 1;
						#else
						mx.tx_packet.flags.radio_off = 1;
						#endif
					}
				#endif
            }

			#if MX_REQUEST
			#if MX_PSEUDO_CONFIG
			else if (slot_number >= chirp_config.mx_generation_size)
			#else
			else if (slot_number >= MX_GENERATION_SIZE)
			#endif
			{
				// f(x) = (1 - 2 ^ (-0.125 * x)) / (1 - 2 ^ (-0.125))
				static const uint8_t LUT1[] =
				{
					0,  1,  2,  3,  4,  4,  5,  5,  6,  7,  7,
					7,  8,  8,  8,  9,  9,  9, 10, 10, 10, 10,
					10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11,
					11, 11, 11, 12
                };

				// f(x) = 2 ^ (-x) (scaled)
				// -> could be computed directly, but the (small) LUT is the faster variant
				static const uint8_t LUT2[] = {0x3f, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

				#if MX_PSEUDO_CONFIG
				uint8_t  	x = LUT1[MIN(chirp_config.mx_generation_size - mx.rank, NUM_ELEMENTS(LUT1) - 1)];
				#else
				uint8_t  	x = LUT1[MIN(MX_GENERATION_SIZE - mx.rank, NUM_ELEMENTS(LUT1) - 1)];
				#endif
				uint16_t 	age = slot_number - mx.recent_innovative_slot;
				#if MX_PSEUDO_CONFIG
				uint8_t	 	rand = mx.tx_packet->packet_chunk[chirp_config.rand.pos] & PACKET_RAND;		// prepared on thread level
				#else
				uint8_t	 	rand = mx.tx_packet.rand;		// prepared on thread level
				#endif

			#if MX_COORDINATED_TX
				if ((age >= x) || (0 == mx_present_head->mx_num_nodes))
			#else
				if ((age >= x))
			#endif
				{
					#if MX_PSEUDO_CONFIG
					if (rand < LUT2[MIN(mx.request->my_column_pending, NUM_ELEMENTS(LUT2) - 1)])
					#else
					if (rand < LUT2[MIN(mx.request.my_column_pending, NUM_ELEMENTS(LUT2) - 1)])
					#endif
					{
						#if MX_PSEUDO_CONFIG
						mx.tx_packet->flags.request_row = 1;
						#else
						mx.tx_packet.flags.request_row = 1;
						#endif
					}
					else
					{
						#if MX_PSEUDO_CONFIG
						mx.tx_packet->flags.request_column = 1;
						#else
						mx.tx_packet.flags.request_column = 1;
						#endif
					}
                }
            }
			#endif

			#if MX_LBT_AFA
				write_tx_fifo(&mx.tx_packet.phy_payload_begin,
				NULL, offsetof(Packet, full_channel) - offsetof(Packet, phy_payload_begin));
			#else
				#if MX_PSEUDO_CONFIG
				write_tx_fifo(&(mx.tx_packet->phy_payload_begin),
				NULL, offsetof(Packet, packet_chunk) - offsetof(Packet, phy_payload_begin) + chirp_config.coding_vector.pos);
				#else
				write_tx_fifo(&mx.tx_packet.phy_payload_begin,
				NULL, offsetof(Packet, coding_vector) - offsetof(Packet, phy_payload_begin));
				#endif
			#endif
		}

		#if MX_LBT_AFA
			// uint8_t channel_is_full = update_tx_channel(s.tx_channel);
			// if (channel_is_full)
			// 	mx.occupied_channel_flag = 1;
			uint8_t uu[3] = {1, 2, 3};
			// uint8_t *channel_state = mx.current_channel_used_num;
			uint8_t *channel_state = uu;

			write_tx_fifo(channel_state, NULL, sizeof(mx.current_channel_used_num));

			#if MX_REQUEST || MX_SMART_SHUTDOWN_MAP

				ASSERT_CT(offsetof(Packet, coding_vector) ==
					offsetof(Packet, info_vector) + sizeof(mx.tx_packet.info_vector),
					inconsistent_program);

				void *ps;

				#if MX_SMART_SHUTDOWN_MAP && MX_REQUEST
					if (mx.tx_packet.flags.is_full_rank)
						ps = &mx.full_rank_map.hash[0];
					else if (mx.tx_packet.flags.request_column)
						ps = &mx.request.my_column_mask[0];
					else ps = &mx.request.my_row_mask[0];
				#elif MX_SMART_SHUTDOWN_MAP
					ps = &mx.full_rank_map.hash[0];
				#elif MX_REQUEST
					if (mx.tx_packet.flags.request_column)
						ps = &mx.request.my_column_mask[0];
					else ps = &mx.request.my_row_mask[0];
				#else
					#error inconsistent code
				#endif

				write_tx_fifo(ps, NULL, sizeof(mx.tx_packet.info_vector));
			#endif
		#endif

		// write coding vector and payload
		{
			#if MX_PSEUDO_CONFIG
			assert_reset(chirp_config.payload.pos = chirp_config.coding_vector.pos + chirp_config.coding_vector.len);
			#else
			ASSERT_CT(offsetof(Packet, payload) ==
				offsetof(Packet, coding_vector) +
			#if MX_DOUBLE_BITMAP
				sizeof(mx.tx_packet.coding_vector) +
			#endif
				sizeof(mx.tx_packet.coding_vector),
				inconsistent_program);
			#endif

			#if MX_PSEUDO_CONFIG
			const unsigned int	CHUNK_SIZE = chirp_config.coding_vector.len + chirp_config.payload.len;
			#else
			const unsigned int	CHUNK_SIZE = sizeof(mx.tx_packet.coding_vector) +
			#if MX_DOUBLE_BITMAP
				sizeof(mx.tx_packet.coding_vector) +
			#endif
			sizeof(mx.tx_packet.payload);
			#endif

			#if MX_DOUBLE_BITMAP
			const unsigned int	BITMAP_SIZE = sizeof(mx.tx_packet.coding_vector);
			const unsigned int	PAYLOAD_SIZE = sizeof(mx.tx_packet.payload);
			#endif

			#if MX_PSEUDO_CONFIG
			p = &(mx.tx_packet->packet_chunk[chirp_config.coding_vector.pos]);
			#else
			p = &(mx.tx_packet.coding_vector[0]);
			#endif
			// printf("p:%lu\n", p);
			// uint8_t k;
			// for ( k = 0; k < 4; k++)
			// {
			// 	printf("%d ", p[k]);
			// }
			// printf("\n");
			// NOTE: we cast const away which is a bit dirty. We need this only to restore
			// sideload's packed version which is such a negligible change that we prefer
			// mx.tx_sideload to appear as const.
			uint8_t	*ps = (uint8_t*)mx.tx_sideload;
			// printf("ps1:%lu\n", ps);
			if(mx.tx_sideload == NULL)
				PRINTF("sideload == NULL\n");

			#if MX_DOUBLE_BITMAP
				// if the tx_sideload points to the matrix, coding_vector and payload will be written separately.
				uint8_t ps_is_martix = 0;
				uint8_t *pc;
			#endif

			#if MX_REQUEST

				#if MX_PSEUDO_CONFIG
				int16_t help_index = mx.request->help_index;
				#else
				int16_t help_index = mx.request.help_index;
				#endif
				if (help_index > 0)
				{
					help_index--;
					if (
						#if MX_PSEUDO_CONFIG
						(!((mx.tx_packet->packet_chunk[chirp_config.rand.pos] & PACKET_IS_READY) >> PACKET_IS_READY_POS))
						#else
						!mx.tx_packet.is_ready
						#endif
						||
						#if MX_PSEUDO_CONFIG
						!(((uint_fast_t*)p)[help_index / (sizeof(uint_fast_t) * 8)] & mx.request->help_bitmask)
						#else
						!(((uint_fast_t*)p)[help_index / (sizeof(uint_fast_t) * 8)] & mx.request.help_bitmask)
						#endif
						)
						{
						#if MX_PSEUDO_CONFIG
						ps = &(mx.matrix[help_index]->matrix_chunk_8[chirp_config.matrix_coding_vector_8.pos + 0]);
						#else
						ps = &(mx.matrix[help_index].coding_vector_8[0]);
						#endif
						}
					#if MX_PSEUDO_CONFIG
					mx.request->last_update_slot = mx.slot_number + 1;
					#else
					mx.request.last_update_slot = mx.slot_number + 1;
					#endif
				}
				else if (help_index < 0)
				{
					help_index = -help_index - 1;

					// NOTE: we don't have to check the packet because if there is one ready, then it
					// has been specifically build in response to the pending request. if the packet
					// is not ready, it is right to do the sideload anyway.
					// if (!mx.tx_packet.is_ready || (help_index < mx_get_leading_index(p)))
					{
						#if MX_PSEUDO_CONFIG
						ps = &(mx.matrix[help_index]->matrix_chunk_8[chirp_config.matrix_coding_vector_8.pos + 0]);
						#else
						ps = &(mx.matrix[help_index].coding_vector_8[0]);
						#endif
			// printf("ps3:%lu\n", ps);

						#if MX_PSEUDO_CONFIG
						mx.request->last_update_slot = mx.slot_number + 1;
						#else
						mx.request.last_update_slot = mx.slot_number + 1;
						#endif
						PRINTF("coding_vector_8[0]\n");
					}
				}

			#endif

			// if sideload points to a matrix row: restore its packed version (in place)
			// NOTE: if it points to rx queue, the format is still packed
			// NOTE: we could also do this when we set mx.tx_sideload (i.e. at a less time critical
			// point), but it is very easy to forget about that. Hence we do it here to avoid
			// programming mistakes.
			// NOTE: the outer condition is resolved at compile time
			#if MX_PSEUDO_CONFIG
			if (chirp_config.matrix_payload_8.pos != chirp_config.matrix_payload_8.pos * sizeof(uint_fast_t))
			#else
			if (offsetof(Matrix_Row, payload_8) != offsetof(Matrix_Row, payload))
			#endif
			{
				#if MX_PSEUDO_CONFIG
				// printf("p4s:%lu, %lu\n", ps, &(mx.matrix[0]->birth_slot));
				if ((uintptr_t)ps - (uintptr_t)&(mx.matrix[0]->birth_slot) < chirp_config.mx_generation_size * ((1 + chirp_config.matrix_chunk_32_len) * sizeof(uint_fast_t)))
				#else
				// printf("ps4:%lu, %lu\n", ps, &mx.matrix);
				if ((uintptr_t)ps - (uintptr_t)&mx.matrix < sizeof(mx.matrix))
				#endif
				{
					// printf("wrap_chunk \n");
					wrap_chunk(ps);
					// uint8_t k;
					// for ( k = 0; k < 8; k++)
					// {
					// 	printf("%d ", ps[k]);
					// }
					// printf("\n");
					#if MX_DOUBLE_BITMAP
						ps_is_martix = 1;
						unwrap_tx_sideload(ps);
						pc = &mx.sideload_coding_vector.coding_vector_8_1;
	PRINTF("packet2:%x, %x, %x\n", mx.sideload_coding_vector.coding_vector_8_1[0], mx.sideload_coding_vector.coding_vector_8_2[0], ps[1]);
					#endif
				}
			#if MX_DOUBLE_BITMAP
				else
				{
					if (ps != NULL)
					{
						// tx_sideload points to rx queue packet coding_vector_2
						unwrap_tx_sideload(ps + BITMAP_SIZE);
						pc = &mx.sideload_coding_vector.coding_vector_8_1;
	PRINTF("packet3:%x, %x, %x\n", mx.sideload_coding_vector.coding_vector_8_1[0], mx.sideload_coding_vector.coding_vector_8_2[0], ps[1]);

					}
				}
			#endif
            }

			#if MX_PSEUDO_CONFIG
			if (!((mx.tx_packet->packet_chunk[chirp_config.rand.pos] & PACKET_IS_READY) >> PACKET_IS_READY_POS))
			#else
			if (!mx.tx_packet.is_ready)
			#endif
			{
				assert_reset(NULL != ps);
				PRINTF("!mx.tx_packet.is_ready\n");

				#if MX_DOUBLE_BITMAP
					write_tx_fifo(pc, NULL, BITMAP_SIZE * 2);
					if (ps_is_martix)
						write_tx_fifo(ps + BITMAP_SIZE, NULL, PAYLOAD_SIZE);
					else
						write_tx_fifo(ps + BITMAP_SIZE * 2, NULL, PAYLOAD_SIZE);
				#else
					write_tx_fifo(ps, NULL, CHUNK_SIZE);
				#endif

				#if MX_VERBOSE_PACKETS || MX_REQUEST
					// mark the packet as broken since it could be possible that we interrupt
					// prepare_tx_packet() right now, hence writing data may damage the packet
					#if MX_PSEUDO_CONFIG
					mx.tx_packet->packet_chunk[chirp_config.rand.pos] &= PACKET_IS_VALID_MASK;
					#else
					mx.tx_packet.is_valid = 0;
					#endif
				#endif
			}

			else
			{
				#if MX_BENCHMARK_NO_SIDELOAD
					ps = NULL;
				#endif

//				if (!mx.tx_packet.use_sideload)
//					ps = NULL;

				// write coding vector with sideload and test if result is a zero packet;
				// if so: abort (below)
				// NOTE: another option might be to write some other useful information into the
				// packet instead of dropping it
				// NOTE: It would be a bit more precise to (1) write (only) the coding vector,
				// (2) test if zero, (3) write payload only if test is non-zero. However, we
				// write the full chunk at once to gain better performance in the typical case
				// (that is non-zero coding vector). Processing the full chunk at once not only
				// saves the second call, it also keeps the alignment(!). We expect that this way
				// is more efficient with relatively moderate packet sizes as in IEEE 802.15.4.
				#if MX_DOUBLE_BITMAP
				if (ps != NULL)
				{
					if (!write_tx_fifo(p, pc, BITMAP_SIZE * 2))
						p = NULL;
					else
					{
						if (ps_is_martix)
							write_tx_fifo(p + BITMAP_SIZE * 2, ps + BITMAP_SIZE, PAYLOAD_SIZE);
						else
							write_tx_fifo(p + BITMAP_SIZE * 2, ps + BITMAP_SIZE * 2, PAYLOAD_SIZE);
					}
					PRINTF("p:%x, %x\n", mx.tx_packet.coding_vector[0], mx.tx_packet.coding_vector_2[0]);
					PRINTF("pc:%x, %x\n", mx.sideload_coding_vector.coding_vector_8_1[0], mx.sideload_coding_vector.coding_vector_8_2[0]);
				}
				else
				#endif
				{
					if (!write_tx_fifo(p, ps, CHUNK_SIZE))
						p = NULL;
				}
			}
		}

		#if (!MX_LBT_AFA)
			// write info vector
			if (NULL != p)
			{
				#if MX_REQUEST || MX_SMART_SHUTDOWN_MAP

					#if MX_PSEUDO_CONFIG
					assert_reset(chirp_config.info_vector.pos == chirp_config.payload.pos + chirp_config.payload.len);
					#else
					ASSERT_CT(offsetof(Packet, info_vector) ==
						offsetof(Packet, payload) + sizeof(mx.tx_packet.payload),
						inconsistent_program);
					#endif

					void *ps;

					#if MX_SMART_SHUTDOWN_MAP && MX_REQUEST
						#if MX_PSEUDO_CONFIG
						if (mx.tx_packet->flags.is_full_rank)
						#else
						if (mx.tx_packet.flags.is_full_rank)
						#endif
						{
							#if MX_PSEUDO_CONFIG
							ps = (uint8_t *)&(mx.full_rank_map->map_hash[chirp_config.hash.pos + 0]);
							#else
							ps = &mx.full_rank_map.hash[0];
							#endif
						}
						#if MX_PSEUDO_CONFIG
						else if (mx.tx_packet->flags.request_column)
							ps = &(mx.request->mask[chirp_config.my_column_mask.pos]);
						else ps = &(mx.request->mask[chirp_config.my_row_mask.pos]);
						#else
						else if (mx.tx_packet.flags.request_column)
							ps = &mx.request.my_column_mask[0];
						else ps = &mx.request.my_row_mask[0];
						#endif
					#elif MX_SMART_SHUTDOWN_MAP
						#if MX_PSEUDO_CONFIG
						ps = (uint8_t *)&(mx.full_rank_map->map_hash[chirp_config.hash.pos + 0]);
						#else
						ps = &mx.full_rank_map.hash[0];
						#endif
					#elif MX_REQUEST
						#if MX_PSEUDO_CONFIG
						if (mx.tx_packet->flags.request_column)
							ps = &(mx.request->mask[chirp_config.my_column_mask.pos]);
						else ps = &(mx.request->mask[chirp_config.my_row_mask.pos]);
						#else
						if (mx.tx_packet.flags.request_column)
							ps = &mx.request.my_column_mask[0];
						else ps = &mx.request.my_row_mask[0];
						#endif
					#else
						#error inconsistent code
					#endif

					#if MX_PSEUDO_CONFIG
					write_tx_fifo(ps, NULL, chirp_config.info_vector.len);
					#else
					write_tx_fifo(ps, NULL, sizeof(mx.tx_packet.info_vector));
					#endif
				#endif
			}
		#endif
		// if zero packet: abort transmission
		if (NULL == p)
		{
			PRINTF("NULL == p\n");

			// turn radio off
			SX1276SetOpMode( RFLR_OPMODE_SLEEP );

			gpi_led_off(LED_TX);

			#if MX_VERBOSE_STATISTICS
				mx.stat_counter.radio_on_time += gpi_tick_fast_native() - s.radio_start_timestamp;
				s.radio_start_timestamp = 0;
				mx.stat_counter.num_tx_zero_packet++;
			#endif

			#if ENERGEST_CONF_ON
				ENERGEST_OFF(ENERGEST_TYPE_TRANSMIT);
			#endif

			GPI_TRACE_MSG_FAST(TRACE_INFO, "sideload produced zero-packet -> Tx aborted");
        }

		else
		{
			// update mx.tx_packet to enable evaluation on processing layer
			// NOTE: not all fields are needed for MX_REQUEST,
			// particularly payload could be dropped (e.g. if time is critical)
			// TODO:
			#if MX_PSEUDO_CONFIG
			uint8_t Buffer2[chirp_config.phy_payload_size];
			#else
			uint8_t Buffer2[PHY_PAYLOAD_SIZE];
			#endif

			SX1276Write( REG_LR_FIFOADDRPTR, 0);
			#if MX_PSEUDO_CONFIG
			SX1276ReadBuffer( 0, Buffer2, chirp_config.phy_payload_size );
			#else
			SX1276ReadBuffer( 0, Buffer2, PHY_PAYLOAD_SIZE );
			#endif
			// uint8_t j;
			// for ( j = 0; j < 50; j++)
			// {
			// 	printf("%d %d,", j, Buffer2[j]);
			// }
			// printf("\n");
			SX1276Write( REG_LR_FIFOADDRPTR, 0);

			uint16_t code_tail_hash_tx = Chirp_RSHash((uint8_t *)Buffer2, chirp_config.phy_payload_size);
			uint8_t hash_code_tx[2];
			hash_code_tx[0] = code_tail_hash_tx >> 8;
			hash_code_tx[1] = code_tail_hash_tx;
			SX1276Write( REG_LR_FIFOADDRPTR, chirp_config.phy_payload_size );
			write_tx_fifo(hash_code_tx, NULL, HASH_TAIL_CODE);

			// unmask IRQ
			SX1276Write( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
												RFLR_IRQFLAGS_RXDONE |
												RFLR_IRQFLAGS_PAYLOADCRCERROR |
												RFLR_IRQFLAGS_VALIDHEADER |
												//RFLR_IRQFLAGS_TXDONE |
												RFLR_IRQFLAGS_CADDONE |
												RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
												RFLR_IRQFLAGS_CADDETECTED );

			// DIO0=TxDone
			SX1276Write( REG_DIOMAPPING1, ( SX1276Read( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK ) | RFLR_DIOMAPPING1_DIO0_01 );

			#if MX_PSEUDO_CONFIG
			gpi_memcpy_dma_aligned(&(mx.tx_packet->phy_payload_begin), Buffer2, chirp_config.phy_payload_size);
			#else
			gpi_memcpy_dma_aligned(&mx.tx_packet.phy_payload_begin, Buffer2, PHY_PAYLOAD_SIZE);
			#endif

			set_event(TX_READY);

			mx.stat_counter.num_sent++;

			GPI_TRACE_MSG_FAST(TRACE_INFO, "Tx started");

        }

		s.slot_state = TX_RUNNING;

		#if MX_PSEUDO_CONFIG
		mx.tx_packet->packet_chunk[chirp_config.rand.pos] &= PACKET_IS_READY_MASK;
		#else
		mx.tx_packet.is_ready = 0;
		#endif
		mx.tx_sideload = NULL;
		PRINTF("NULL 5\n");
		s.next_slot_task = RX;
		PROFILE_ISR("grid timer ISR start Tx end");
	}

	slot_state_:
	#if MX_LBT_AFA
		if ((RESYNC != s.slot_state) && (!s.rx_time))
	#else
		if (RESYNC != s.slot_state)
	#endif
	{
		mx.slot_number++;
		set_event(SLOT_UPDATE);

		#if MX_PSEUDO_CONFIG
		s.next_grid_tick += chirp_config.mx_slot_length + s.grid_drift / (radio.grid_drift_filter_div * radio.grid_tick_update_div);
		#else
		s.next_grid_tick += MX_SLOT_LENGTH + s.grid_drift / (radio.grid_drift_filter_div * radio.grid_tick_update_div);
		#endif
		s.hybrid_trigger = s.next_grid_tick;

		// s.next_trigger_tick_slow = s.next_grid_tick_slow -
		// 	(Gpi_Slow_Tick_Native)((s.next_slot_task == TX) ? s.tx_trigger_offset : s.rx_trigger_offset) / HYBRID_SLOW_RATIO;

		s.next_trigger_tick = s.next_grid_tick -
			((s.next_slot_task == TX) ? s.tx_trigger_offset : s.rx_trigger_offset);
		Gpi_Hybrid_Reference r = gpi_tick_hybrid_reference();
		// printf("s1:%lu, %lu\n", s.next_grid_tick - r.hybrid_tick, s.next_grid_tick);

		if (TX_RUNNING == s.slot_state)
		{
			#if MX_LBT_AFA
				s.tx_guard_flag = 0;
				s.tx_dead_flag = 0;
			#endif
			start_grid_timer();
		}
	}

	GPI_TRACE_MSG_FAST(TRACE_VERBOSE, "slot_state: %d, next_grid: %lu, rx_offset: %u",
		s.slot_state, (unsigned long)gpi_tick_hybrid_to_us(s.next_grid_tick),
		(unsigned int)gpi_tick_hybrid_to_us(s.rx_trigger_offset));

	// set general purpose trigger event
	// compared to SLOT_UPDATE, TRIGGER_TICK is generated also during RESYNC periods (once
	// in a while). It can be used for maintenance tasks that are less time critical.
	set_event(TRIGGER_TICK);

	PROFILE_ISR("grid timer ISR return");

#if	ENERGEST_CONF_ON
	ENERGEST_OFF(ENERGEST_TYPE_IRQ);
#endif

	GPI_TRACE_RETURN_FAST();
}

//**************************************************************************************************
//***** Global Functions ***************************************************************************

void mixer_transport_init()
{
	GPI_TRACE_FUNCTION();

	mixer_transport_initiate_radio();
#if MX_VERBOSE_CONFIG

	#define PRINT(s) printf("%-25s = %" PRId32 "\n", #s, (int32_t)s)

	PRINT(radio.max_propagation_delay);
	PRINT(radio.rx_window_increment);
	PRINT(radio.rx_window_min);
	PRINT(radio.rx_window_max);
	PRINT(radio.grid_drift_filter_div);
	PRINT(radio.grid_tick_update_div);
	PRINT(radio.grid_drift_max);
	PRINT(radio.tx_offset_filter_div);
	PRINT(radio.tx_offset_max);

	#undef PRINT

#endif

	GPI_TRACE_RETURN();
}

//**************************************************************************************************

void mixer_transport_arm_initiator()
{
	GPI_TRACE_FUNCTION();

	s.grid_drift			= 0;
	s.grid_drift_cumulative = 0;
	s.tx_trigger_offset 	= radio.tx_to_grid_offset;
	s.rx_trigger_offset 	= radio.rx_to_grid_offset + radio.rx_window_max / 2;


	GPI_TRACE_RETURN();
}

//**************************************************************************************************

void mixer_transport_start()
{
	GPI_TRACE_FUNCTION_FAST();

	GPI_TRACE_MSG_FAST(TRACE_VERBOSE, "start grid timer");

	if (mx.tx_sideload)		// if initiator
		enter_resync(2);
	else
		enter_resync(1);

	GPI_TRACE_RETURN_FAST();
}

//**************************************************************************************************

int mixer_transport_set_next_slot_task(Slot_Activity next_task)
{
	GPI_TRACE_FUNCTION_FAST();
	// if next_task == RX: done
	// NOTE: it is the automatic standard selection at the beginning of each slot (with the
	// exception of RESYNC), so we can save the effort. Besides that, DMA ISR is allowed to
	// select TX during start-up phase -> don't overwrite that.
	if (RX != next_task)
	{
		Gpi_Hybrid_Reference	r;

		// compute a deadline such that we can check if we are too close to the grid trigger tick
		// NOTE: we have to cover three terms:
		// - the activity-to-grid offset, which is dependent on the selected task
		// - the ISR_LATENCY_BUFFER
		// - the execution time needed for the code block within the gpi_int_lock range below
		// Additionally, hybrid_tick returned by gpi_tick_hybrid_reference() may lie up to one
		// slow tick -- i.e. 1000000 / GPI_SLOW_CLOCK_RATE microseconds -- in the past.
		r = gpi_tick_hybrid_reference();
		r.hybrid_tick = s.next_trigger_tick - r.hybrid_tick;
		r.hybrid_tick -=
			MAX(radio.tx_offset_max, radio.rx_to_grid_offset + radio.rx_window_max) +
			GPI_TICK_US_TO_HYBRID(radio.isr_latency_buffer) +
			GPI_TICK_US_TO_HYBRID(1000000 / GPI_SLOW_CLOCK_RATE + 30);
		r.hybrid_tick *= FAST_HYBRID_RATIO;
		if (r.hybrid_tick > GPI_TICK_FAST_MAX / 2)
			r.fast_capture += GPI_TICK_FAST_MAX / 2;
		else r.fast_capture += r.hybrid_tick;

		int ie = gpi_int_lock();
		REORDER_BARRIER();

		if ((RESYNC != s.slot_state) || (STOP == next_task))
		{
			if (gpi_tick_compare_fast_native(gpi_tick_fast_native(), r.fast_capture) >= 0)
				next_task = -1;
			else
			{
				// NOTE: next_task == TX or STOP at this point

				gpi_led_toggle(LED_UPDATE_TASK);

				s.next_slot_task = next_task;
				s.next_trigger_tick = s.next_grid_tick - s.tx_trigger_offset;	// also ok for STOP
				s.next_trigger_tick_slow = s.next_grid_tick_slow - (uint16_t)((uint32_t)s.tx_trigger_offset / (uint32_t)HYBRID_SLOW_RATIO);

				// if IDLE or RESYNC (RESYNC only if next_task == STOP)
				if (RX_RUNNING != s.slot_state)
					start_grid_timer();

				gpi_led_toggle(LED_UPDATE_TASK);
			}
		}

		REORDER_BARRIER();
		gpi_int_unlock(ie);
	}

	if (-1 == next_task)
	{
		GPI_TRACE_MSG(TRACE_WARNING, "!!! WARNING: rx/tx decision was late -> check program, should not happen !!!");
		GPI_TRACE_RETURN(0);
    }
	else
	{
		GPI_TRACE_MSG(TRACE_INFO, "next slot task: %s", (next_task == TX) ? "TX" : ((next_task == RX) ? "RX" : "STOP"));
		GPI_TRACE_RETURN(1);
    }
}

//**************************************************************************************************
//**************************************************************************************************
