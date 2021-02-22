/***************************************************************************************************
 ***************************************************************************************************
 *
 *	Copyright (c) 2018 - 2019, Networked Embedded Systems Lab, TU Dresden
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
 *	@file					mixer.c
 *
 *	@brief					mixer API functions
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
#define TRACE_INFO			GPI_TRACE_MSG_TYPE_INFO
#define TRACE_VERBOSE		GPI_TRACE_MSG_TYPE_VERBOSE

// select active message groups, i.e., the messages to be printed (others will be dropped)
#ifndef GPI_TRACE_BASE_SELECTION
	#define GPI_TRACE_BASE_SELECTION	GPI_TRACE_LOG_STANDARD | GPI_TRACE_LOG_PROGRAM_FLOW
#endif
GPI_TRACE_CONFIG(mixer, GPI_TRACE_BASE_SELECTION | GPI_TRACE_LOG_USER);

//**************************************************************************************************
//**** Includes ************************************************************************************

#include "mixer_internal.h"

#include "gpi/olf.h"
#include "gpi/platform.h"
#include "gpi/interrupts.h"

#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>

#include "gpi/tools.h"
#include "L476RG_SX1276/memxor.h"

#if ENERGEST_CONF_ON
#include GPI_PLATFORM_PATH(energest.h)
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

//**************************************************************************************************
//***** Local Typedefs and Class Declarations ******************************************************



//**************************************************************************************************
//***** Forward Declarations ***********************************************************************



//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************



//**************************************************************************************************
//***** Global Variables ***************************************************************************

// mixer internal data
struct mx		mx;


/* Mixer packet and node configuration, Radio (LoRa) configuration */
Chirp_Config chirp_config;

//**************************************************************************************************
//***** Local Functions ****************************************************************************

#if GPI_TRACE_MODE & GPI_TRACE_MODE_TRACE

void mx_trace_dump(const char *header, const void *p, uint_fast16_t size)
{
	char 			msg[3 * 16 + 1];
	char	 		*m = &(msg[0]);
	const uint8_t	*pc = p;

	while (size-- > 0)
	{
		if (&msg[sizeof(msg)] - m <= 3)
		{
			GPI_TRACE_MSG(GPI_TRACE_MSG_TYPE_VERBOSE, "%s%s ...", header, msg);
			m = &(msg[0]);
        }

		m += sprintf(m, " %02" PRIx8, *pc++);
	}

	GPI_TRACE_MSG(GPI_TRACE_MSG_TYPE_VERBOSE, "%s%s", header, msg);
}

#endif	// GPI_TRACE_MODE

//**************************************************************************************************
//***** Global Functions ***************************************************************************

void mixer_init(uint8_t node_id)
{
	GPI_TRACE_FUNCTION();

	// set the state to mixer for config the isr functions
	chirp_isr.state = ISR_MIXER;

	assert_reset((node_id < chirp_config.mx_num_nodes));

	// in case NDEBUG is set
	if (node_id >= chirp_config.mx_num_nodes)
		GPI_TRACE_RETURN();

	unsigned int i;

#if (MX_VERBOSE_CONFIG && !GPI_ARCH_IS_BOARD(TMOTE_INDRIYA))

	#define PRINT(s) printf("%-25s = %" PRId32 "\n", #s, (int32_t)s)

	PRINT(sizeof(Packet));

	PRINT(chirp_config.mx_num_nodes);
	PRINT(chirp_config.mx_generation_size);
	PRINT(chirp_config.mx_payload_size);
	PRINT(chirp_config.mx_slot_length);
	PRINT(chirp_config.mx_round_length);

	PRINT(MX_AGE_TO_INCLUDE_PROBABILITY);
	PRINT(MX_AGE_TO_TX_PROBABILITY);

	#if MX_COORDINATED_TX
		PRINT(MX_COORDINATED_TX);
	#endif
	#if MX_REQUEST
		PRINT(MX_REQUEST);
		PRINT(MX_REQUEST_HEURISTIC);
	#endif
	#if MX_SMART_SHUTDOWN
		PRINT(MX_SMART_SHUTDOWN);
		#if MX_SMART_SHUTDOWN_MAP
			PRINT(MX_SMART_SHUTDOWN_MAP);
		#endif
	#endif
	#if MX_VERBOSE_STATISTICS
		PRINT(MX_VERBOSE_STATISTICS);
	#endif
	#if MX_VERBOSE_PROFILE
		PRINT(MX_VERBOSE_PROFILE);
	#endif
	#if MX_VERBOSE_PACKETS
		PRINT(MX_VERBOSE_PACKETS);
	#endif

	#undef PRINT

#endif

    chirp_config.update_slot = 0;

	mx.rx_queue[0] = (Packet *)malloc((sizeof(mx.rx_queue) / sizeof(mx.rx_queue[0])) * (chirp_config.packet_len));
	for (i = 1; i < (sizeof(mx.rx_queue) / sizeof(mx.rx_queue[0])); i++)
		mx.rx_queue[i] = (Packet *)&(mx.rx_queue[i-1]->packet_chunk[chirp_config.packet_chunk_len]);
	memset(mx.rx_queue[0], 0, (sizeof(mx.rx_queue) / sizeof(mx.rx_queue[0])) * (chirp_config.packet_len));

	if (chirp_config.primitive != FLOODING)
	{
	#if INFO_VECTOR_QUEUE
	mx.code_queue[0] = (Packet_info_vector *)malloc((sizeof(mx.code_queue) / sizeof(mx.code_queue[0])) * (chirp_config.coding_vector.len));
	for (i = 1; i < (sizeof(mx.code_queue) / sizeof(mx.code_queue[0])); i++)
		mx.code_queue[i] = (Packet_info_vector *)&(mx.code_queue[i-1]->vector[chirp_config.coding_vector.len]);
	memset(mx.code_queue[0], 0, (sizeof(mx.code_queue) / sizeof(mx.code_queue[0])) * (chirp_config.coding_vector.len));

	mx.info_queue[0] = (Packet_info_vector *)malloc((sizeof(mx.info_queue) / sizeof(mx.info_queue[0])) * (chirp_config.info_vector.len));
	for (i = 1; i < (sizeof(mx.info_queue) / sizeof(mx.info_queue[0])); i++)
		mx.info_queue[i] = (Packet_info_vector *)&(mx.info_queue[i-1]->vector[chirp_config.info_vector.len]);
	memset(mx.info_queue[0], 0, (sizeof(mx.info_queue) / sizeof(mx.info_queue[0])) * (chirp_config.info_vector.len));
	#endif
	}

	mx.tx_packet = (Packet *)malloc(chirp_config.packet_len);
	memset(mx.tx_packet, 0, chirp_config.packet_len);

	if (chirp_config.primitive != FLOODING)
	{
	mx.matrix[0] = (Matrix_Row *)malloc(chirp_config.mx_generation_size * ((1 + chirp_config.matrix_chunk_32_len) * sizeof(uint_fast_t)));
	for (i = 1; i < chirp_config.mx_generation_size; i++)
		mx.matrix[i] = (Matrix_Row *)&(mx.matrix[i-1]->matrix_chunk[chirp_config.matrix_chunk_32_len]);
	memset(mx.matrix[0], 0, chirp_config.mx_generation_size * ((1 + chirp_config.matrix_chunk_32_len) * sizeof(uint_fast_t)));
	}

	mx.history[0] = (Node *)malloc((chirp_config.mx_num_nodes + 3) * (chirp_config.history_len_8));
	for (i = 1; i < chirp_config.mx_num_nodes + 3; i++)
		mx.history[i] = (Node *)&(mx.history[i-1]->row_map_chunk[chirp_config.matrix_coding_vector.len]);
	memset(mx.history[0], 0, (chirp_config.mx_num_nodes + 3) * (chirp_config.history_len_8));

	mx_absent_head = mx.history[chirp_config.mx_num_nodes + 3 - 3];
	mx_present_head = mx.history[chirp_config.mx_num_nodes + 3 - 2];
	mx_finished_head = mx.history[chirp_config.mx_num_nodes + 3 - 1];
	#if MX_SMART_SHUTDOWN
	mx.full_rank_map = (Full_Rank_Map *)malloc(chirp_config.map.len + chirp_config.hash.len);
	#endif

	mx.request = (Request_Data *)malloc(offsetof(Request_Data, mask) + 6 * chirp_config.matrix_coding_vector.len * sizeof(uint_fast_t));

	mixer_transport_init();

	mx.rx_queue_num_writing = 0;
	mx.rx_queue_num_written = 0;
	mx.rx_queue_num_read = 0;

	mx.tx_packet->sender_id = node_id;
	mx.tx_packet->flags.all = 0;

	mx.tx_reserve = NULL;

	for (i = 0; i < chirp_config.mx_generation_size; i++)
		mx.matrix[i]->birth_slot = UINT16_MAX;

	mx.rank = 0;

	mx.next_own_row = (Matrix_Row *)&(mx.matrix[chirp_config.mx_generation_size - 1]->matrix_chunk[chirp_config.matrix_chunk_32_len]);

	mx.recent_innovative_slot = 0;

	mx.events = 0;

	memset(&mx.stat_counter, 0, sizeof(mx.stat_counter));

	for (i = 0; i < NUM_ELEMENTS(pt_data); ++i)
		PT_INIT(&pt_data[i]);

	#if MX_REQUEST
		memset(mx.request, 0, offsetof(Request_Data, mask) + 6 * chirp_config.matrix_coding_vector.len * sizeof(uint_fast_t));
		memset(&(mx.request->mask[chirp_config.my_row_mask.pos]), -1, chirp_config.matrix_coding_vector.len * sizeof(uint_fast_t));
		memset(&(mx.request->mask[chirp_config.my_column_mask.pos]), -1, chirp_config.matrix_coding_vector.len * sizeof(uint_fast_t));

		// ATTENTION: signed is important
		int_fast_t mask = 1 << (sizeof(uint_fast_t) * 8 - 1);
		for (i = chirp_config.matrix_coding_vector.len * sizeof(uint_fast_t) * 8; i-- > chirp_config.mx_generation_size;)
			mask >>= 1;
		mx.request->padding_mask = ~(mask << 1);
		GPI_TRACE_MSG(TRACE_VERBOSE, "request padding mask: %0*x",
			sizeof(uint_fast_t) * 2, mx.request->padding_mask);

		i = chirp_config.matrix_coding_vector.len - 1;
		mx.request->mask[chirp_config.my_row_mask.pos + i] &= mx.request->padding_mask;
		mx.request->mask[chirp_config.my_column_mask.pos + i] &= mx.request->padding_mask;
	#endif

	#if MX_COORDINATED_TX
		mx_init_history();
	#endif

	mx.rx_queue_num_read = mx.rx_queue_num_written;

	mx.slot_number = 0;

	mx.tx_packet->packet_chunk[chirp_config.rand.pos] &= PACKET_IS_READY_MASK;

	mx.tx_sideload = NULL;

	#if MX_SMART_SHUTDOWN
		mx.have_full_rank_neighbor = 0;
		#if MX_SMART_SHUTDOWN_MAP
			memset(mx.full_rank_map, 0, chirp_config.map.len + chirp_config.hash.len);
		#endif
	#endif

	#if ENERGEST_CONF_ON
		// Initialize Energest values.
		energest_init();
	#endif

	GPI_TRACE_RETURN();
}

//**************************************************************************************************

size_t mixer_write(unsigned int i, const void *msg, size_t size)
{
	GPI_TRACE_FUNCTION();

	assert_reset((i < chirp_config.mx_generation_size));

	// in case NDEBUG is set
	if (i >= chirp_config.mx_generation_size)
		GPI_TRACE_RETURN(0);

	size = MIN(size, chirp_config.matrix_payload_8.len);

	gpi_memcpy_dma(&(mx.matrix[i]->matrix_chunk_8[chirp_config.matrix_payload_8.pos]), msg, size);

	uint32_t payload_hash = Chirp_RSHash((uint8_t *)&(mx.matrix[i]->matrix_chunk_8[chirp_config.matrix_payload_8.pos]), chirp_config.matrix_payload_8.len - 2);
	mx.matrix[i]->matrix_chunk_8[chirp_config.matrix_payload_8.pos + chirp_config.mx_payload_size - 2] = payload_hash >> 8;
	mx.matrix[i]->matrix_chunk_8[chirp_config.matrix_payload_8.pos + chirp_config.mx_payload_size - 1] = payload_hash;

	unwrap_row(i);

	memset(&(mx.matrix[i]->matrix_chunk[chirp_config.matrix_coding_vector.pos]), 0, sizeof(uint_fast_t) * chirp_config.matrix_coding_vector.len);
	mx.matrix[i]->matrix_chunk_8[chirp_config.matrix_coding_vector_8.pos + i / 8] |= 1 << (i % 8);
	mx.matrix[i]->birth_slot = 0;

	mx.rank++;

	if (NULL == mx.tx_reserve)
	{
		mx.tx_reserve = &(mx.matrix[i]->birth_slot);
	}

	if (mx.next_own_row > &(mx.matrix[i]->birth_slot))
		mx.next_own_row = &(mx.matrix[i]->birth_slot);

	#if MX_REQUEST
		const unsigned int BITS_PER_WORD = sizeof(uint_fast_t) * 8;
		mx.request->mask[chirp_config.my_row_mask.pos + i / BITS_PER_WORD] &= ~(1 << (i % BITS_PER_WORD));
		mx.request->mask[chirp_config.my_column_mask.pos + i / BITS_PER_WORD] &= ~(1 << (i % BITS_PER_WORD));
	#endif

	GPI_TRACE_RETURN(size);
}

//**************************************************************************************************

void mixer_arm(Mixer_Start_Mode mode)
{
	GPI_TRACE_FUNCTION();
	if (chirp_config.primitive != FLOODING)
	{
	// mark an empty row (used by rx processing)
	mx.empty_row = NULL;
	if (mx.rank < chirp_config.mx_generation_size)
	{
		Matrix_Row *p = (Matrix_Row *)&(mx.matrix[chirp_config.mx_generation_size - 1]->matrix_chunk[chirp_config.matrix_chunk_32_len]);
		while (p > 0)
		{
			p -= chirp_config.matrix_size_32;
			if (UINT16_MAX == p->birth_slot)
			{
				mx.empty_row = p;
				break;
			}
		}
	}

	// if initiator: arm TX (instead of RESYNC)
	if (mode & MX_ARM_INITIATOR)
	{
		assert_reset((NULL != mx.tx_reserve));

		mx.tx_sideload = &(mx.next_own_row->matrix_chunk_8[chirp_config.matrix_coding_vector_8.pos + 0]);

		mx.next_own_row += chirp_config.matrix_size_32;
		while (mx.next_own_row < (Matrix_Row *)&(mx.matrix[chirp_config.mx_generation_size - 1]->matrix_chunk[chirp_config.matrix_chunk_32_len]))
		{
			if (0 == mx.next_own_row->birth_slot)
				break;

			mx.next_own_row += chirp_config.matrix_size_32;
		}

		mixer_transport_arm_initiator();
	}

	// launch threads
	// NOTE: this gives all threads the opportunity to init thread-local data
	(void) PT_SCHEDULE(mixer_maintenance());
	(void) PT_SCHEDULE(mixer_update_slot());
	(void) PT_SCHEDULE(mixer_process_rx_data());

	// if sync round: don't update deadline before first packet reception
	// ATTENTION: the way of doing that here is a bit crude. It is associated to the
	// maintenance thread; look there for details.
	if (mode & MX_ARM_INFINITE_SCAN)
		mx.round_deadline_update_slot = 0;

	GPI_TRACE_RETURN();
	}
}

//**************************************************************************************************

Gpi_Fast_Tick_Extended mixer_start()
{
	GPI_TRACE_FUNCTION_FAST();

	mixer_transport_start();

	unsigned int event_mask = BV(SLOT_UPDATE) | BV(TRIGGER_TICK);

	while (event_mask)
	{
		// isolate highest priority pending event
		unsigned int event = mx.events & event_mask;
		event &= -event;	// mask LSB

		switch (event)
		{
			// if a thread exits, it triggered the stop procedure of Mixer -> mask all events
			// except STOPPED. This ensures that the exited thread gets no longer scheduled.
			// ATTENTION: The thread may rely on that behavior.

			case BV(SLOT_UPDATE):
				if (!PT_SCHEDULE(mixer_update_slot()))
					event_mask = BV(STOPPED);
				break;

			case BV(TRIGGER_TICK):
				if (!PT_SCHEDULE(mixer_maintenance()))
					event_mask = BV(STOPPED);
				break;

			default:
			{
				GPI_TRACE_FLUSH();

				if (PT_WAITING == PT_SCHEDULE_STATE(mixer_process_rx_data()))
				{
					// after graceful stop has been performed
					if (BV(STOPPED) == event)
					{
						GPI_TRACE_MSG(TRACE_INFO, "interrupts stopped");

						// if deadline reached: set mx.slot_number to last slot
						// NOTE: Since the deadline has been reached, we know that we are there.
						// If we wouldn't update mx.slot_number then it would be possible that
						// some of the stat_counters (slot_off...) get wrong values if node was
						// in RESYNC when stopping.
						if (mx.events & BV(DEADLINE_REACHED))
						{
							mx.slot_number = chirp_config.mx_round_length;
						}

						// exit loop
						event_mask = 0;
						break;
                    }

					// enter low-power mode
					gpi_int_disable();
					if (!(mx.events & event_mask))
					{
						#if MX_VERBOSE_STATISTICS

							ASSERT_CT(!(GPI_FAST_CLOCK_RATE % GPI_HYBRID_CLOCK_RATE), FAST_HYBRID_ratio_must_be_integer);
							ASSERT_CT(IS_POWER_OF_2(FAST_HYBRID_RATIO), FAST_HYBRID_ratio_must_be_power_of_2);

							#if (GPI_TRACE_MODE & GPI_TRACE_MODE_TRACE)
								const int USE_NATIVE = 0;
							#else
								const int USE_NATIVE =
									((((chirp_config.mx_slot_length * 5) / 2) * FAST_HYBRID_RATIO + 0x1000) <
									(Gpi_Fast_Tick_Native)GPI_TICK_FAST_MAX);
							#endif

							Gpi_Hybrid_Tick	time;

							if (USE_NATIVE)
								time = gpi_tick_fast_native();
							else time = gpi_tick_hybrid();

						#endif

						#if ENERGEST_CONF_ON
							static unsigned long irq_energest = 0;
							/* Re-enable interrupts and go to sleep atomically. */
							ENERGEST_OFF(ENERGEST_TYPE_CPU);
							ENERGEST_ON(ENERGEST_TYPE_LPM);
							/* We only want to measure the processing done in IRQs when we
							are asleep, so we discard the processing time done when we
							were awake. */
							energest_type_set(ENERGEST_TYPE_IRQ, irq_energest);
						#endif

						// enter sleep mode
						// NOTE: reenables interrupts (they serve as wake-up events)
						gpi_sleep();

						// ...
						// awake again

						#if MX_VERBOSE_STATISTICS
							// ATTENTION: time up to here includes execution time of one or more ISRs.
							// To support low-power time measurements, every (relevant) ISR stores
							// the wake-up timestamp on ISR entry (in case it is a wake-up event).
							// This is what we use here.
							if (USE_NATIVE)
								time = (Gpi_Fast_Tick_Native)(mx.wake_up_timestamp - time) / FAST_HYBRID_RATIO;
							else time = gpi_tick_fast_to_hybrid(mx.wake_up_timestamp) - time;
							mx.stat_counter.low_power_time += time;
						#endif

						#if ENERGEST_CONF_ON
							irq_energest = energest_type_time(ENERGEST_TYPE_IRQ);
							ENERGEST_OFF(ENERGEST_TYPE_LPM);
							ENERGEST_ON(ENERGEST_TYPE_CPU);
						#endif

					}
					else gpi_int_enable();
				}
			}
		}
	}

	// try to solve (if not done already)
	if (mx.rank < chirp_config.mx_generation_size)
	{
		Pt_Context *pt = &pt_data[0];
		PT_INIT(pt);
		while (PT_SCHEDULE(mixer_decode(pt)));
    }

	// GPI_TRACE_MSG(TRACE_INFO, "mixer stopped");

	// #if MX_VERBOSE_STATISTICS

	// 	mx.stat_counter.radio_on_time /= FAST_HYBRID_RATIO;

	// #ifdef PRINT
	// 	#error change macro name
	// #endif

	// printf("statistics:\n");

	// #define PRINT(n)	\
	// 	ASSERT_CT(sizeof(mx.stat_counter.n) == sizeof(uint16_t), n);	\
	// 	printf(#n ": %" PRIu16 "\n", mx.stat_counter.n)

	// PRINT(num_sent);
	// PRINT(num_received);
	// PRINT(num_resync);
	// PRINT(num_grid_drift_overflow);
	// PRINT(num_rx_window_overflow);
	// PRINT(num_rx_success);
	// PRINT(num_rx_broken);
	// PRINT(num_rx_timeout);
	// PRINT(num_rx_dma_timeout);
	// PRINT(num_rx_dma_late);
	// PRINT(num_rx_late);
	// PRINT(num_tx_late);
	// PRINT(num_tx_zero_packet);
	// PRINT(num_tx_fifo_late);
	// PRINT(num_grid_late);
	// PRINT(num_rx_slot_mismatch);
	// PRINT(num_rx_queue_overflow);
	// PRINT(num_rx_queue_overflow_full_rank);
	// PRINT(num_rx_queue_processed);
	// PRINT(slot_full_rank);
	// PRINT(slot_decoded);
	// PRINT(slot_off);

	// #undef PRINT
	// #define PRINT(n)	\
	// 	ASSERT_CT(sizeof(Gpi_Hybrid_Tick) <= sizeof(long), n);		\
	// 	printf(#n ": %lu us\n", (unsigned long)gpi_tick_hybrid_to_us(mx.stat_counter.n))

	// PRINT(radio_on_time);
	// PRINT(low_power_time);

	// #undef PRINT

	// #endif

	#if ENERGEST_CONF_ON

	unsigned long avg_energy1 = ((((unsigned long)gpi_tick_hybrid_to_us(energest_type_time(ENERGEST_TYPE_LISTEN) + energest_type_time(ENERGEST_TYPE_TRANSMIT)))) / (unsigned long)(chirp_config.mx_period_time_s));

	printf("E 1:%lu.%03lu \n", avg_energy1 / 1000, avg_energy1 % 1000);

	#endif

	free(mx.rx_queue[0]);
	if (chirp_config.primitive != FLOODING)
	{
		#if INFO_VECTOR_QUEUE
		free(mx.code_queue[0]);
		free(mx.info_queue[0]);
		#endif
	}
	free(mx.tx_packet);
	free(mx.history[0]);
	#if MX_SMART_SHUTDOWN
		free(mx.full_rank_map);
	#endif
	if (chirp_config.primitive == FLOODING)
		free(mx.request);

	GPI_TRACE_RETURN(mx.round_deadline);
}

//**************************************************************************************************

void* mixer_read(unsigned int i)
{
	GPI_TRACE_FUNCTION();

	assert_reset((i < chirp_config.mx_generation_size));

	// in case NDEBUG is set
	if (i >= chirp_config.mx_generation_size)
		GPI_TRACE_RETURN((void*)NULL);

	if (UINT16_MAX == mx.matrix[i]->birth_slot)
		GPI_TRACE_RETURN((void*)NULL);

	uint8_t m = 1 << (i % 8);

	mx.matrix[i]->matrix_chunk_8[chirp_config.matrix_coding_vector_8.pos + i / 8] ^= m;
	int_fast16_t k = mx_get_leading_index(&(mx.matrix[i]->matrix_chunk_8[chirp_config.matrix_coding_vector_8.pos]));
	mx.matrix[i]->matrix_chunk_8[chirp_config.matrix_coding_vector_8.pos + i / 8] ^= m;

	if (k >= 0)
		GPI_TRACE_RETURN((void*)NULL);

	unwrap_row(i);

	GPI_TRACE_RETURN(&(mx.matrix[i]->matrix_chunk_8[chirp_config.matrix_payload_8.pos]));
}

//**************************************************************************************************

int16_t mixer_stat_slot(unsigned int i)
{
	assert_reset((i < chirp_config.mx_generation_size));

	// in case NDEBUG is set
	if (i >= chirp_config.mx_generation_size)
		return -1;

	return mx.matrix[i]->birth_slot;
}

//**************************************************************************************************
//**************************************************************************************************
