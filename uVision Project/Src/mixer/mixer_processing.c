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
 *	@file					mixer_processing.c
 *
 *	@brief					Mixer processing layer
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
#define TRACE_INFO				GPI_TRACE_MSG_TYPE_INFO
#define TRACE_WARNING			GPI_TRACE_MSG_TYPE_WARNING
#define TRACE_ERROR				GPI_TRACE_MSG_TYPE_ERROR
#define TRACE_VERBOSE			GPI_TRACE_MSG_TYPE_VERBOSE
#define TRACE_VERBOSE_MATRIX	GPI_TRACE_MSG_TYPE_VERBOSE

// select active message groups, i.e., the messages to be printed (others will be dropped)
#ifndef GPI_TRACE_BASE_SELECTION
	#define GPI_TRACE_BASE_SELECTION	GPI_TRACE_LOG_STANDARD | GPI_TRACE_LOG_PROGRAM_FLOW
#endif
GPI_TRACE_CONFIG(mixer_processing, GPI_TRACE_BASE_SELECTION | GPI_TRACE_LOG_USER);

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

//**************************************************************************************************
//**** Includes ************************************************************************************

#include "mixer_internal.h"

#include "gpi/tools.h"
#include "gpi/platform_spec.h"
#include "gpi/interrupts.h"
#include "gpi/olf.h"

#include <stdio.h>
#include <inttypes.h>

#if GPI_ARCH_IS_CORE(MSP430)
	#include "tmote/memxor.h"
#elif GPI_ARCH_IS_CORE(ARMv7M)
	#include "L476RG_SX1276/memxor.h"
#else
	#error unsupported architecture
#endif

#if MX_VERBOSE_PROFILE
	GPI_PROFILE_SETUP("mixer_processing.c", 1800, 4);
#endif

//**************************************************************************************************
//***** Local Defines and Consts *******************************************************************
// #define DEBUG 1
#if PRINTF_CHIRP
#define PRINTF_decode(...) printf(__VA_ARGS__)
#else
#define PRINTF_decode(...)
#endif


//**************************************************************************************************
//***** Local Typedefs and Class Declarations ******************************************************



//**************************************************************************************************
//***** Forward Declarations ***********************************************************************



//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************

Pt_Context						pt_data[3];

static Pt_Context* const		pt_update_slot 		= &pt_data[0];
static Pt_Context* const		pt_process_rx_data 	= &pt_data[1];
static Pt_Context* const		pt_maintenance 		= &pt_data[2];

//**************************************************************************************************
//***** Global Variables ***************************************************************************

extern uint8_t node_id_allocate;

extern Chirp_Config chirp_config;

//**************************************************************************************************
//***** Local Functions ****************************************************************************

#if MX_VERBOSE_PACKETS
	#define TRACE_PACKET(p)		trace_packet(p)

static void trace_packet(const Packet *p)
{
	char msg[300];
	char *ps = msg;
	int  i;

	ASSERT_CT(2 == sizeof(p->slot_number), check_PRI_formats);
	ASSERT_CT(1 == sizeof(p->sender_id), check_PRI_formats);
	ASSERT_CT(1 == sizeof(p->flags), check_PRI_formats);

	#if !(GPI_ARCH_IS_BOARD(TMOTE_FLOCKLAB) || GPI_ARCH_IS_BOARD(TMOTE_INDRIYA))
		#if MX_PSEUDO_CONFIG
		ps += sprintf(ps, "# ID:%u ", (int)mx.tx_packet->sender_id + 1);
		#else
		ps += sprintf(ps, "# ID:%u ", (int)mx.tx_packet.sender_id + 1);
		#endif
	#endif

	// node id MSB marks vector bit order (for log parser):
	// 0: LSB first, big-endian
	// 1: LSB first, little-endian
	ps += sprintf(ps, "%04" PRIx16 " - %04" PRIx16 " - %02" PRIx8 " - ",
		p->slot_number, (uint16_t)(p->sender_id |
		#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
			0x8000
		#elif (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
			0
		#else
			#error unsupported architecture
		#endif
		), p->flags.all);

	#if MX_PSEUDO_CONFIG
	for (i = 0; i < chirp_config.coding_vector.len; i++)
	#else
	for (i = 0; i < sizeof(p->coding_vector); i++)
	#endif
#if MX_REQUEST || MX_SMART_SHUTDOWN_MAP
		#if MX_PSEUDO_CONFIG
		ps += sprintf(ps, "%02" PRIx8, p->packet_chunk[chirp_config.info_vector.pos + i]);
		#else
		ps += sprintf(ps, "%02" PRIx8, p->info_vector[i]);
//		ps += sprintf(ps, "%02" PRIx8, (uint8_t)~(p->info_vector[i]));
		#endif
#else
		ps += sprintf(ps, "00");
#endif

	ps += sprintf(ps, " - ");

	#if MX_PSEUDO_CONFIG
	for (i = 0; i < chirp_config.coding_vector.len; i++)
		ps += sprintf(ps, "%02" PRIx8, p->packet_chunk[chirp_config.coding_vector.pos + i]);
	#else
	for (i = 0; i < sizeof(p->coding_vector); i++)
		ps += sprintf(ps, "%02" PRIx8, p->coding_vector[i]);
	#endif

	ps += sprintf(ps, " - ");

	#if MX_DOUBLE_BITMAP

		#if MX_PSEUDO_CONFIG
		for (i = 0; i < chirp_config.coding_vector.len; i++)
		#else
		for (i = 0; i < sizeof(p->coding_vector); i++)
		#endif
		{
			ps += sprintf(ps, "%02" PRIx8, p->coding_vector_2[i]);
		}

		ps += sprintf(ps, " - ");

	#endif

	// for (i = 0; i < sizeof(p->payload); i++)
	// 	ps += sprintf(ps, "%02" PRIx8, p->payload[i]);

	for (i = 0; i < 8; i++)
	{
		#if MX_PSEUDO_CONFIG
		ps += sprintf(ps, "%02" PRIx8, p->packet_chunk[chirp_config.payload.pos + i]);
		#else
		ps += sprintf(ps, "%02" PRIx8, p->payload[i]);
		#endif
	}

	PRINTF_CHIRP("%s\n", msg);
}

#else
	#define TRACE_PACKET(p)		while(0)
#endif	// MX_VERBOSE_PACKETS

//**************************************************************************************************

#if (GPI_TRACE_MODE & GPI_TRACE_MODE_TRACE)
	#define TRACE_MATRIX()		trace_matrix()

static void trace_matrix()
{
	char 		msg[128];
	char		*m = &(msg[0]);
	uint16_t	r, i;
	uint8_t		v;

	GPI_TRACE_MSG(TRACE_VERBOSE_MATRIX, "matrix:");
#if MX_PSEUDO_CONFIG
	for (r = 0; r < chirp_config.mx_generation_size; r++)
#else
	for (r = 0; r < NUM_ELEMENTS(mx.matrix); r++)
#endif
	{
		m = &(msg[0]);
		m += sprintf(m, "%3" PRId16 ":", r);

#if MX_PSEUDO_CONFIG
		for (i = 0; i < chirp_config.matrix_coding_vector_8.len; i++)
#else
		for (i = 0; i < NUM_ELEMENTS(mx.matrix[0].coding_vector_8); i++)
#endif
		{
#if MX_PSEUDO_CONFIG
			v = mx.matrix[r]->matrix_chunk_8[chirp_config.matrix_coding_vector_8.pos + i];
#else
			v = mx.matrix[r].coding_vector_8[i];
#endif
			if (&msg[sizeof(msg)] - m <= 3)
			{
				GPI_TRACE_MSG(TRACE_VERBOSE_MATRIX, "%s ...", msg);
				m = &(msg[0]);
			}

			m += sprintf(m, " %02" PRIx8, v);
        }

		if (&msg[sizeof(msg)] - m <= 3)
		{
			GPI_TRACE_MSG(TRACE_VERBOSE_MATRIX, "%s ...", msg);
			m = &(msg[0]);
		}

		*m++ = ' ';
		*m = '\0';

#if MX_PSEUDO_CONFIG
		for (i = 0; i < chirp_config.matrix_payload_8.len; i++)
#else
		for (i = 0; i < NUM_ELEMENTS(mx.matrix[0].payload_8); i++)
#endif
		{
			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Warray-bounds"

#if MX_PSEUDO_CONFIG
			const uint8_t	offset = (chirp_config.matrix_payload.pos) * 4 - chirp_config.matrix_payload_8.pos;

			if (i < offset)
				v = mx.matrix[r]->matrix_chunk_8[chirp_config.matrix_coding_vector_8.pos + i + chirp_config.matrix_coding_vector_8.len];
			else v = mx.matrix[r]->matrix_chunk_8[chirp_config.matrix_coding_vector_8.pos + i];
#else
			const uint8_t	offset = offsetof(Matrix_Row, payload) - offsetof(Matrix_Row, payload_8);

			if (i < offset)
				v = mx.matrix[r].payload_8[i + NUM_ELEMENTS(mx.matrix[0].payload_8)];
			else v = mx.matrix[r].payload_8[i];
#endif

			#pragma GCC diagnostic pop

			if (&msg[sizeof(msg)] - m <= 3)
			{
				GPI_TRACE_MSG(TRACE_VERBOSE_MATRIX, "%s ...", msg);
				m = &(msg[0]);
			}

			m += sprintf(m, " %02" PRIx8, v);
        }

		GPI_TRACE_MSG(TRACE_VERBOSE_MATRIX, "%s", msg);
	}
}

#else
	#define TRACE_MATRIX()		while (0)
#endif	// GPI_TRACE_MODE

//**************************************************************************************************
//**************************************************************************************************

static inline void clear_event(Event event)
{
	gpi_atomic_clear(&mx.events, BV(event));
}

//**************************************************************************************************
#if MX_SMART_SHUTDOWN_MAP

static void update_full_rank_map(const Packet *p)
{
	GPI_TRACE_FUNCTION();
	PROFILE("update_full_rank_map() entry");

	const uint8_t 	*ps;
	uint8_t			*pd;
	unsigned int 	i;

	// if we reached full rank
	if (NULL == p)
	{
		#if MX_PSEUDO_CONFIG
		mx.full_rank_map->map_hash[chirp_config.map.pos + mx.tx_packet->sender_id / 8] |= gpi_slu(1, mx.tx_packet->sender_id % 8);
		// mx.full_rank_map.map[mx.tx_packet->sender_id / 8] |= gpi_slu(1, mx.tx_packet->sender_id % 8);
		#else
		mx.full_rank_map.map[mx.tx_packet.sender_id / 8] |= gpi_slu(1, mx.tx_packet.sender_id % 8);
		#endif
	}

	// update map and history
	else
	{
		if (p->flags.is_full_rank)
		{
			#if MX_PSEUDO_CONFIG
			mx.full_rank_map->map_hash[chirp_config.map.pos + p->sender_id / 8] |= gpi_slu(1, p->sender_id % 8);
			#else
			mx.full_rank_map.map[p->sender_id / 8] |= gpi_slu(1, p->sender_id % 8);
			#endif
		}

		#if MX_PSEUDO_CONFIG
		for (pd = (uint8_t *)&(mx.full_rank_map->map_hash[chirp_config.map.pos + 0]); pd < (uint8_t *)&(mx.full_rank_map->map_hash[chirp_config.map.pos + chirp_config.map.len]); )
			for (ps = &(p->packet_chunk[chirp_config.info_vector.pos + 0]); ps < &(p->packet_chunk[chirp_config.info_vector.pos + chirp_config.info_vector.len]);)
		#else
		for (pd = &mx.full_rank_map.map[0]; pd < &mx.full_rank_map.map[NUM_ELEMENTS(mx.full_rank_map.map)]; )
			for (ps = &(p->info_vector[0]); ps < &(p->info_vector[sizeof(p->info_vector)]);)
		#endif
			{
				*pd++ |= *ps++;
			}

		Node *pn;
		#if MX_PSEUDO_CONFIG
		for (pn = mx.history[mx_present_head->next]; pn != mx_present_head;)
		#else
		for (pn = &mx.history[mx_present_head->next]; pn != mx_present_head;)
		#endif
		{
			#if MX_PSEUDO_CONFIG
			i = ARRAY_INDEX_SIZE_ADD(pn, &(mx.history[0]->prev), chirp_config.history_len_8);
			#else
			i = ARRAY_INDEX(pn, mx.history);
			#endif
			// ATTENTION: depending on sizeof(mx.history[0]), the compiler may generate an
			// expensive division operation. This is not such critical at this place because
			// every neighbor is moved to the finished list only once.

			// ATTENTION: update pn before changing the node, else pn->next may point to a finished
			// node and the for loop becomes an endless loop (because pn != mx_present_head will
			// never be true)
			#if MX_PSEUDO_CONFIG
			pn = mx.history[pn->next];
			#else
			pn = &mx.history[pn->next];
			#endif

			#if MX_PSEUDO_CONFIG
			if (mx.full_rank_map->map_hash[chirp_config.map.pos + i / 8] & gpi_slu(1, i % 8))
			#else
			if (mx.full_rank_map.map[i / 8] & gpi_slu(1, i % 8))
			#endif
			{
				Packet_Flags flags = {0};
				flags.is_full_rank = 1;

				mx_update_history(i, flags, mx.slot_number);

				// NOTE: using mx.slot_number causes a history update for node i. This is not critical
				// since node i is present (we took it from the present-list). If we would take the
				// slot number from node i's history entry, we could destroy the order in the
				// finished-list. For the same reason we don't take slot number from p because the
				// processing here can fall a bit behind the regular history updates (under high
				// CPU load).
			}
        }
    }

	// update hash
	#if MX_PSEUDO_CONFIG
	if (chirp_config.map.len <= chirp_config.hash.len)
		gpi_memcpy_dma_inline(&(mx.full_rank_map->map_hash[chirp_config.hash.pos]), &(mx.full_rank_map->map_hash[chirp_config.map.pos]), chirp_config.map.len);
	#else
	if (sizeof(mx.full_rank_map.map) <= sizeof(mx.full_rank_map.hash))
		gpi_memcpy_dma_inline(mx.full_rank_map.hash, mx.full_rank_map.map, sizeof(mx.full_rank_map.map));
	#endif
	else
	{
		#if MX_PSEUDO_CONFIG
		for (i = 0; i < chirp_config.hash.len; ++i)
		#else
		for (i = 0; i < NUM_ELEMENTS(mx.full_rank_map.hash); ++i)
		#endif
		{
			// ATTENTION: mx.full_rank_map.hash is also read on ISR level. Therefore it is important
			// to work on a temporary variable such that mx.full_rank_map.hash never marks unfinished
			// nodes.

			uint8_t	hash = -1;

			#if MX_PSEUDO_CONFIG
			for (ps = (uint8_t *)&(mx.full_rank_map->map_hash[chirp_config.map.pos + i]);
				ps < (uint8_t *)&(mx.full_rank_map->map_hash[chirp_config.map.len]);
				ps += chirp_config.hash.len)
			#else
			for (ps = &mx.full_rank_map.map[i];
				ps < &mx.full_rank_map.map[NUM_ELEMENTS(mx.full_rank_map.map)];
				ps += NUM_ELEMENTS(mx.full_rank_map.hash))
			#endif
			{
				hash &= *ps;
            }

			#if MX_PSEUDO_CONFIG
			*((volatile uint8_t*)&(mx.full_rank_map->map_hash[chirp_config.hash.pos + i])) = hash;
			#else
			*((volatile uint8_t*)&(mx.full_rank_map.hash[i])) = hash;
			#endif
        }
    }

	PROFILE("update_full_rank_map() return");

	#if MX_PSEUDO_CONFIG
	TRACE_DUMP(1, "full-rank map: ", &(mx.full_rank_map->map_hash[chirp_config.map.pos]), chirp_config.map.len);
	TRACE_DUMP(1, "full-rank hash:", &(mx.full_rank_map->map_hash[chirp_config.hash.pos]), chirp_config.hash.len);
	#else
	TRACE_DUMP(1, "full-rank map: ", mx.full_rank_map.map, sizeof(mx.full_rank_map.map));
	TRACE_DUMP(1, "full-rank hash:", mx.full_rank_map.hash, sizeof(mx.full_rank_map.hash));
	#endif
	GPI_TRACE_RETURN();
}

#endif
//**************************************************************************************************
//***** Global Functions ***************************************************************************

static void prepare_tx_packet()
{
	GPI_TRACE_FUNCTION();
	PROFILE("prepare_tx_packet() entry");
	#if MX_PSEUDO_CONFIG
	const uint16_t	CHUNK_SIZE = chirp_config.coding_vector.len + chirp_config.payload.len;
	#else
	const uint16_t	CHUNK_SIZE = sizeof(mx.tx_packet.coding_vector) + sizeof(mx.tx_packet.payload);
	#endif
	Matrix_Row		*p;
	void			*pp[MEMXOR_BLOCKSIZE];
	int				pp_used = 0;
	int_fast16_t	used = 0;
	#if MX_REQUEST
		Matrix_Row	*help_row = 0;
	#endif

	assert_msg(NULL != mx.tx_reserve, "Tx without data -> must not happen");

	// clear mx.tx_packet by adding itself to the xor list
#if (!MX_DOUBLE_BITMAP)
	#if MX_PSEUDO_CONFIG
	pp[pp_used++] = &(mx.tx_packet->packet_chunk[chirp_config.coding_vector.pos]);
	#else
	pp[pp_used++] = &mx.tx_packet.coding_vector;
	#endif
#else
	pp[pp_used++] = &mx.tx_packet.coding_vector_2;
#endif

#if !MX_BENCHMARK_NO_SYSTEMATIC_STARTUP

#if MX_PSEUDO_CONFIG
	if (mx.next_own_row < (Matrix_Row *)&(mx.matrix[chirp_config.mx_generation_size - 1]->matrix_chunk[chirp_config.matrix_chunk_32_len]))
#else
	if (mx.next_own_row < &mx.matrix[NUM_ELEMENTS(mx.matrix)])
#endif
	{
		p = mx.next_own_row;

		// mark that we don't need the reserve
		used++;

		// restore packed version (in place)
		#if MX_PSEUDO_CONFIG
		wrap_chunk((uint8_t *)&(p->matrix_chunk_8[chirp_config.matrix_coding_vector_8.pos]));
		#else
		wrap_chunk(p->coding_vector_8);
		#endif

		// add it to xor list
		#if MX_PSEUDO_CONFIG
		pp[pp_used++] = &(p->matrix_chunk[chirp_config.matrix_coding_vector.pos]);
		#else
		pp[pp_used++] = &(p->coding_vector);
		#endif

		// look for next own row
		#if MX_PSEUDO_CONFIG
		mx.next_own_row += chirp_config.matrix_size_32;
		while (mx.next_own_row < (Matrix_Row *)&(mx.matrix[chirp_config.mx_generation_size - 1]->matrix_chunk[chirp_config.matrix_chunk_32_len]))
		#else
		while (++mx.next_own_row < &mx.matrix[NUM_ELEMENTS(mx.matrix)])
		#endif
		{
			if (0 == mx.next_own_row->birth_slot)
				break;
			#if MX_PSEUDO_CONFIG
			mx.next_own_row += chirp_config.matrix_size_32;
			#endif
		}
	}

#endif

	if (!used)
	{
		#if MX_REQUEST
			#if MX_PSEUDO_CONFIG
			if (mx.request->help_index < 0)
			#else
			if (mx.request.help_index < 0)
			#endif
			{
				#if MX_PSEUDO_CONFIG
				help_row = (Matrix_Row *)&(mx.matrix[-mx.request->help_index - 1]->birth_slot);
				// help_row = (Matrix_Row *)&(mx.matrix[-mx.request.help_index - 1]->birth_slot);
				#else
				help_row = &mx.matrix[-mx.request.help_index - 1];
				#endif
			}
		#endif

		// traverse matrix
		#if MX_PSEUDO_CONFIG
		for (p = (Matrix_Row *)&(mx.matrix[0]->birth_slot); p < (Matrix_Row *)&(mx.matrix[chirp_config.mx_generation_size - 1]->matrix_chunk[chirp_config.matrix_chunk_32_len]); p += chirp_config.matrix_size_32)
		#else
		for (p = &mx.matrix[0]; p < &mx.matrix[NUM_ELEMENTS(mx.matrix)]; p++)
		#endif
		{
			if (UINT16_MAX == p->birth_slot)
				continue;

			#if MX_REQUEST
				// if row request help index selected: skip all up to that row
				// NOTE: the help row itself will be automatically included by sideload
				if (p <= help_row)
					continue;
			#endif

			PROFILE("prepare_tx_packet() mixer_rand() begin");

			uint16_t r = mixer_rand();

			PROFILE("prepare_tx_packet() mixer_rand() end");

			// choose any available row as reserve, update from time to time
			// -> as reserve sideload and for the case that we select nothing by rolling the dice
			if (!(r & 7))
				mx.tx_reserve = p;

			// include current row?
			{
				static const uint16_t LUT[] = MX_AGE_TO_INCLUDE_PROBABILITY;
				ASSERT_CT(sizeof(LUT) > 0, MX_AGE_TO_INCLUDE_PROBABILITY_is_invalid);

				if (!(r < LUT[MIN(mx.slot_number - p->birth_slot, NUM_ELEMENTS(LUT) - 1)]))
					continue;
            }

			// mark that we don't need the reserve
			used++;

			// restore packed version (in place)
			#if MX_PSEUDO_CONFIG
			wrap_chunk((uint8_t *)&(p->matrix_chunk_8[chirp_config.matrix_coding_vector_8.pos]));
			#else
			wrap_chunk(p->coding_vector_8);
			#endif

			// add it to xor list, work through if needed
			#if MX_PSEUDO_CONFIG
			pp[pp_used++] = &(p->matrix_chunk[chirp_config.matrix_coding_vector.pos]);
			#else
			pp[pp_used++] = &(p->coding_vector);
			#endif
			if(NUM_ELEMENTS(pp) == pp_used)
			{
				PROFILE("prepare_tx_packet() memxor_block(full) begin");

				// NOTE: calling with NUM_ELEMENTS(pp) instead of pp_used leads to a bit better
				// code because NUM_ELEMENTS(pp) is a constant (msp430-gcc 4.6.3)
				#if (!MX_DOUBLE_BITMAP)
				#if MX_PSEUDO_CONFIG
				memxor_block(&(mx.tx_packet->packet_chunk[chirp_config.coding_vector.pos]), pp, CHUNK_SIZE, NUM_ELEMENTS(pp));
				#else
				memxor_block(mx.tx_packet.coding_vector, pp, CHUNK_SIZE, NUM_ELEMENTS(pp));
				#endif
				#else
				memxor_block(mx.tx_packet.coding_vector_2, pp, CHUNK_SIZE, NUM_ELEMENTS(pp));
				#endif
				pp_used = 0;

				PROFILE("prepare_tx_packet() memxor_block(full) end");
			}

			#if MX_PSEUDO_CONFIG
			assert_reset(!((offsetof(Packet, packet_chunk) + chirp_config.coding_vector.pos) % sizeof(uint_fast_t)));
			#else
			ASSERT_CT(!(offsetof(Packet, coding_vector) % sizeof(uint_fast_t)), alignment_issue);
			#endif
			#if MX_PSEUDO_CONFIG
			assert_reset(chirp_config.payload.pos == chirp_config.coding_vector.pos + chirp_config.coding_vector.len);
			assert_reset(chirp_config.matrix_payload_8.pos == chirp_config.matrix_coding_vector.pos + chirp_config.matrix_coding_vector_8.len);
			#else
			ASSERT_CT(offsetof(Packet, payload) ==
					offsetof(Packet, coding_vector) +
		#if MX_DOUBLE_BITMAP
					sizeof_member(Packet, coding_vector) +
		#endif
					sizeof_member(Packet, coding_vector),
				inconsistent_program);
			ASSERT_CT(offsetof(Matrix_Row, payload_8) ==
				offsetof(Matrix_Row, coding_vector) + sizeof_member(Matrix_Row, coding_vector_8),
				inconsistent_program);
			#endif
		}
	}

	// if we didn't select any row: use the reserve
	// NOTE: mx.tx_reserve != NULL checked by assertion above
	// ATTENTION: don't do that if request is active to make sure that helper sideload is successfull.
	// This may generate an empty packet which is no problem because of the sideload. The other way
	// around could lead to a critical corner case if the sideload points to the same row as
	// mx.tx_reserve: The result would be a zero packet, i.e. no successful transmission and - in case
	// of high tx probability - a subsequent try to transmit. Since the request situation does not
	// change in this time, there is a good chance that we rebuild the same packet. If this happens,
	// the whole procedure starts again and again and does not end before mx.tx_reserve gets updated.
	// But this never happens if the requested row is the last one in the matrix.
	#if MX_REQUEST
		if (!used && !help_row)
	#else
		if (!used)
	#endif
	{
		// NOTE: we cast const away which is a bit dirty. We need this only to restore packed
		// version which is such a negligible change that we prefer mx.tx_reserve to appear as const.
		p = (Matrix_Row *)mx.tx_reserve;

		// restore packed version (in place)
		#if MX_PSEUDO_CONFIG
		wrap_chunk((uint8_t *)&(p->matrix_chunk_8[chirp_config.matrix_coding_vector_8.pos]));
		#else
		wrap_chunk(p->coding_vector_8);
		#endif

		// add it to xor list
		// NOTE: memcpy instead of memxor would also be possible here,
		// but the situation is not very time critical (xored nothing up to here)
		#if MX_PSEUDO_CONFIG
		pp[pp_used++] = &(p->matrix_chunk[chirp_config.matrix_coding_vector.pos]);
		#else
		pp[pp_used++] = &(p->coding_vector);
		#endif
	}

	// work through the xor list
	if (pp_used)
	{
	#if (!MX_DOUBLE_BITMAP)
		#if MX_PSEUDO_CONFIG
		memxor_block(&(mx.tx_packet->packet_chunk[chirp_config.coding_vector.pos]), pp, CHUNK_SIZE, pp_used);
		#else
		memxor_block(mx.tx_packet.coding_vector, pp, CHUNK_SIZE, pp_used);
		#endif
	#else
		memxor_block(mx.tx_packet.coding_vector_2, pp, CHUNK_SIZE, pp_used);
	#endif
	}

#if MX_DOUBLE_BITMAP
	unwrap_coding_vector(mx.tx_packet.coding_vector, mx.tx_packet.coding_vector_2, 1);
	PRINTF("packet1:%x, %x, %x\n", mx.tx_packet.coding_vector[0], mx.tx_packet.coding_vector_2[0], mx.tx_packet.payload[0]);
#endif

	PROFILE("prepare_tx_packet() return");
	#if MX_PSEUDO_CONFIG
	TRACE_DUMP(1, "tx_packet:", &(mx.tx_packet->phy_payload_begin), chirp_config.phy_payload_size);
	#else
	TRACE_DUMP(1, "tx_packet:", &(mx.tx_packet.phy_payload_begin), PHY_PAYLOAD_SIZE);
	#endif

	GPI_TRACE_RETURN();
}

//**************************************************************************************************

PT_THREAD(mixer_update_slot())
{
	Pt_Context* const	pt = pt_update_slot;

	PT_BEGIN(pt);

#if (MX_COORDINATED_TX || MX_REQUEST)
	static unsigned int	rx_queue_num_read_2;
#endif
#if MX_COORDINATED_TX
	static uint16_t		owner, last_owner_update;
#endif

	// init variables at thread startup
	// NOTE: approach is useful because thread gets reinitialized (PT_INIT) on each mixer round
#if (MX_COORDINATED_TX || MX_REQUEST)
	rx_queue_num_read_2	= 0;
#endif
#if MX_COORDINATED_TX
	owner 			  = 0;
	last_owner_update = 0;
#endif

	while (1)
	{
		PT_WAIT_UNTIL(pt, mx.events & BV(SLOT_UPDATE));
		clear_event(SLOT_UPDATE);
		if (chirp_config.task != MX_GLOSSY)
		{
		gpi_watchdog_periodic();
		#if MX_VERBOSE_PACKETS
			if (mx.events & BV(TX_READY))
			{
				PRINTF_CHIRP("Tx: ");
				#if MX_PSEUDO_CONFIG
				TRACE_PACKET(&(mx.tx_packet->phy_payload_begin));
				#else
				TRACE_PACKET(&mx.tx_packet);
				#endif
				// #if (!BANK_1_RUN)
				// if (chirp_config.task == CHIRP_VERSION)
				// {
				// 	uint_fast32_t tx_flash[2];
				// 	memset(tx_flash, 0, sizeof(tx_flash));
				// 	tx_flash[0] |= 1 << 24;
				// 	memcpy(&(tx_flash[0]), &(mx.tx_packet->packet_chunk[chirp_config.coding_vector.pos]), chirp_config.coding_vector.len);
				// 	memcpy(&(tx_flash[1]), &(mx.tx_packet->packet_chunk[chirp_config.payload.pos + 8]), 3);
				// 	tx_flash[1] |= mx.slot_number << 24;
				// 	FLASH_If_Write(TOPO_FLASH_ADDRESS + mx.slot_number * 8, (uint32_t *)(&(tx_flash[0])), sizeof(tx_flash) / sizeof(uint32_t));
				// }
				// #endif
			}
		#endif

//TRACE_DUMP(1, "my_row_mask:", mx.request.my_row_mask, sizeof(mx.request.my_row_mask));
//TRACE_DUMP(1, "my_column_mask:", mx.request.my_column_mask, sizeof(mx.request.my_column_mask));

		Slot_Activity		next_task;
		uint16_t			p = 0;

		#if MX_COORDINATED_TX
			Packet_Flags	flags = {0};
		#endif

		// use local variable since mx.slot_number is volatile (and for performance)
		// NOTE: some pieces of code rely on the assumption that slot_number doesn't change
		// while the thread is active. Although this is true if system runs without overload,
		// we use a local variable to be absolutely safe.
		uint16_t	slot_number = mx.slot_number;

		GPI_TRACE_MSG_FAST(TRACE_INFO, "slot %" PRIu16, slot_number);
		PROFILE("mixer_update_slot() begin");
		// maintain request status
		#if MX_REQUEST
			#if MX_PSEUDO_CONFIG
			if (slot_number - mx.request->last_update_slot > 3)
			{
				mx.request->row_any_pending = 0;
				mx.request->column_any_pending = 0;
			}
			#else
			if (slot_number - mx.request.last_update_slot > 3)
			{
				mx.request.row.any_pending = 0;
				mx.request.column.any_pending = 0;
			}
			#endif
			else if (mx.events & BV(TX_READY))
			{
				#if MX_PSEUDO_CONFIG
				mx_update_request(mx.tx_packet);
				#else
				mx_update_request(&mx.tx_packet);
				#endif
			}

			PROFILE("mixer_update_slot() update request status done");
		#endif

		// read rx packet if available and update history
		#if (MX_COORDINATED_TX || MX_REQUEST)
			while (rx_queue_num_read_2 != mx.rx_queue_num_written)
			{
				PROFILE("mixer_update_slot() update history begin");

				#if MX_PSEUDO_CONFIG
				Packet *p = mx.rx_queue[rx_queue_num_read_2 % NUM_ELEMENTS(mx.rx_queue)];
				#else
				Packet *p = &mx.rx_queue[rx_queue_num_read_2 % NUM_ELEMENTS(mx.rx_queue)];
				#endif
				uint8_t  		sender_id   = p->sender_id;
				#if MX_COORDINATED_TX
					uint16_t	slot_number = p->slot_number;
								flags		= p->flags;
				#endif

				#if MX_PSEUDO_CONFIG
				if (sender_id >= chirp_config.mx_num_nodes)
				#else
				if (sender_id >= MX_NUM_NODES)
				#endif
				{
					// don't do much here, it is handled in Rx processing
					rx_queue_num_read_2++;
					continue;
				}

				#if INFO_VECTOR_QUEUE
					#if MX_PSEUDO_CONFIG
					// TP TODO:
					// gpi_memcpy_dma_inline((uint8_t *)&(mx.rx_queue[rx_queue_num_read_2 % NUM_ELEMENTS(mx.rx_queue)]->packet_chunk[chirp_config.coding_vector.pos]), (uint8_t *)&(mx.code_queue[rx_queue_num_read_2 % NUM_ELEMENTS(mx.info_queue)]->vector[0]), chirp_config.coding_vector.len);
					gpi_memcpy_dma_inline((uint8_t *)&(mx.rx_queue[rx_queue_num_read_2 % NUM_ELEMENTS(mx.rx_queue)]->packet_chunk[chirp_config.info_vector.pos]), (uint8_t *)&(mx.info_queue[rx_queue_num_read_2 % NUM_ELEMENTS(mx.info_queue)]->vector[0]), chirp_config.info_vector.len);

					#else
					gpi_memcpy_dma_inline(mx.rx_queue[rx_queue_num_read_2 % NUM_ELEMENTS(mx.rx_queue)].coding_vector, mx.code_queue[rx_queue_num_read_2 % NUM_ELEMENTS(mx.code_queue)].vector, sizeof_member(Packet, coding_vector));
					gpi_memcpy_dma_inline(mx.rx_queue[rx_queue_num_read_2 % NUM_ELEMENTS(mx.rx_queue)].info_vector, mx.info_queue[rx_queue_num_read_2 % NUM_ELEMENTS(mx.info_queue)].vector, sizeof_member(Packet, info_vector));

					#endif
				#endif

				#if MX_REQUEST
					mx_update_request(p);
				#endif

				REORDER_BARRIER();

				// NOTE: since the current thread has higher priority than Rx packet processing,
				// we should never see an overflow here. Nevertheless we test it for safety. If
				// it would happen we would lose some history updates which is not very critical.
				// In addition request data may get hurt which again is not such critical.
				if (mx.rx_queue_num_writing - rx_queue_num_read_2 > NUM_ELEMENTS(mx.rx_queue))
				{
					GPI_TRACE_MSG(TRACE_WARNING, "WARNING: rx queue num_read_2 overflow -> check program, should not happen");
					rx_queue_num_read_2 = mx.rx_queue_num_written;
					continue;
				}
				else rx_queue_num_read_2++;

				#if MX_COORDINATED_TX
					mx_update_history(sender_id, flags, slot_number);
					GPI_TRACE_MSG(TRACE_INFO, "node %u history update", sender_id);
				#endif

				PROFILE("mixer_update_slot() update history end");
			}

			#if MX_PREAMBLE_UPDATE
				if (mx.preamble_update_abort_rx)
				{
					uint8_t  		sender_id   = mx.packet_header.sender_id;
					#if MX_COORDINATED_TX
						uint16_t	slot_number = mx.packet_header.slot_number;
									flags		= mx.packet_header.flags;
					#endif

					#if MX_REQUEST
						mx_update_request(&mx.packet_header);
					#endif

					#if MX_COORDINATED_TX
						mx_update_history(sender_id, flags, slot_number);
						GPI_TRACE_MSG(TRACE_INFO, "node %u history update", sender_id);
					#endif

					memset(&mx.packet_header, 0, sizeof(mx.packet_header));
					mx.preamble_update_abort_rx = 0;
				}
			#endif
		#endif

		PROFILE("mixer_update_slot() tx decision begin");

		// decide what to do in next slot
		do {
			// don't TX as long as we have no data (i.e. we are not initiated)
			if (mx.rank < 1)
				break;

			// stop if done
			#if MX_SMART_SHUTDOWN
				#if MX_PSEUDO_CONFIG
				if (mx.tx_packet->flags.radio_off)
				#else
				if (mx.tx_packet.flags.radio_off)
				#endif
				{
					PRINTF("radio_off");
					GPI_TRACE_MSG(TRACE_INFO, "smart shutdown initiated");
					while (!mixer_transport_set_next_slot_task(STOP));
					PT_EXIT(pt);
				}
			#endif

			#if MX_COORDINATED_TX
				uint16_t	density = 1 + mx_present_head->mx_num_nodes + mx_finished_head->mx_num_nodes;
				// printf("d:%d\n", density);
				assert_reset(density < 256);
			#endif

			#if MX_REQUEST
				uint16_t __attribute__((unused)) relative_rank = 0;
			#endif
			int_fast8_t		is_helper = 0;

			#if MX_PSEUDO_CONFIG
			assert_reset(chirp_config.mx_num_nodes < 256);
			#else
			ASSERT_CT(MX_NUM_NODES < 256, inconsistent_program);
			#endif

			// determine request help index
			#if MX_REQUEST
				#if MX_COORDINATED_TX && !MX_BENCHMARK_NO_COORDINATED_STARTUP
					// don't process requests during startup phase
					// attention: during startup phase, it is possible that DMA ISR decides to TX
					// (in case of flags.hasNextPayload) if mx.tx_packet is ready. Therefore it is
					// important that mx.tx_packet doesn't get invalidated on thread level during this
					// phase. The condition avoids that this could happen in case of row requests.
					#if MX_PSEUDO_CONFIG
					if (mx.slot_number > chirp_config.mx_generation_size)
					#else
					if (mx.slot_number > MX_GENERATION_SIZE)
					#endif
				#endif
			{
				PROFILE("mixer_update_slot() tx decision request help 1");

				uint_fast_t		help_bitmask = 0;
				uint_fast_t		*pr;

				#if MX_PSEUDO_CONFIG
				mx.request->help_index = 0;
				#else
				mx.request.help_index = 0;
				#endif
				// scan column requests
				// start with all_mask
				#if MX_PSEUDO_CONFIG
				pr = (uint_fast_t *)&(mx.request->mask[chirp_config.column_all_mask.pos + 0]);
				while (mx.request->column_any_pending)
				#else
				pr = &mx.request.column.all_mask[0];
				while (mx.request.column.any_pending)
				#endif
				{
					is_helper = -1;

					#if MX_PSEUDO_CONFIG
					uint_fast_t		*po = (uint_fast_t *)&(mx.request->mask[chirp_config.my_row_mask.pos + 0]);
					#else
					uint_fast_t		*po = &mx.request.my_row_mask[0];
					#endif
					uint_fast_t		x;

					#if MX_PSEUDO_CONFIG
					for (x = *pr++; po < (uint_fast_t *)&(mx.request->mask[chirp_config.my_row_mask.pos + chirp_config.my_row_mask.len]);)
					#else
					for (x = *pr++; po < &mx.request.my_row_mask[NUM_ELEMENTS(mx.request.my_row_mask)];)
					#endif
					{
						if (!x)
						{
							x = *pr++;		// ATTENTION: dirty in the sense of access violation
							po++;
							continue;
                        }

						#ifndef __BYTE_ORDER__
							#error __BYTE_ORDER__ is undefined
						#elif !((__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) || (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__))
							#error __BYTE_ORDER__ is invalid
						#endif

						// isolate first set bit
						#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
							help_bitmask = x & -x;			// isolate LSB
						#else
							#error TODO						// isolate MSB
						#endif

						// if we can help: exit loop
						if (!(*po & help_bitmask))
							break;

						// else clear bit in x
						x &= ~help_bitmask;
                    }

					// if we can help: continue below
					#if MX_PSEUDO_CONFIG
					if (po < (uint_fast_t *)&(mx.request->mask[chirp_config.my_row_mask.pos + chirp_config.my_row_mask.len]))
					#else
					if (po < &mx.request.my_row_mask[NUM_ELEMENTS(mx.request.my_row_mask)])
					#endif
					{
						// NOTE: help_bitmask has only one bit set,
						// so it doesn't matter if we use get_msb() or get_lsb()
						#if MX_PSEUDO_CONFIG
						mx.request->help_index = 1 + ARRAY_INDEX_SIZE_ADD(po, &(mx.request->mask[chirp_config.my_row_mask.pos]), sizeof(uint_fast_t) * chirp_config.my_row_mask.len) * sizeof(uint_fast_t) * 8 + gpi_get_msb(help_bitmask);
						mx.request->help_bitmask = help_bitmask;
						#else
						mx.request.help_index = 1 + ARRAY_INDEX(po, mx.request.my_row_mask) * sizeof(uint_fast_t) * 8 + gpi_get_msb(help_bitmask);
						mx.request.help_bitmask = help_bitmask;
						#endif
						is_helper = 1;
						break;
                    }

					// break after scanning any_mask
					// NOTE: -2 matches position of pr
					#if MX_PSEUDO_CONFIG
					if (ARRAY_INDEX_SIZE_ADD(pr, &(mx.request->mask[chirp_config.column_any_mask.pos]), sizeof(uint_fast_t) * chirp_config.column_any_mask.len) - 2 < chirp_config.column_all_mask.len)
						break;

					// scan any_mask (after scanning all_mask)
					pr = (uint_fast_t *)&(mx.request->mask[chirp_config.column_any_mask.pos + 0]);
					#else
					if (ARRAY_INDEX(pr, mx.request.column.any_mask) - 2 < NUM_ELEMENTS(mx.request.column.any_mask))
						break;

					// scan any_mask (after scanning all_mask)
					pr = &mx.request.column.any_mask[0];
					#endif
                }

				// scan row requests
				// start with all_mask
				#if MX_PSEUDO_CONFIG
				pr = (uint_fast_t *)&(mx.request->mask[chirp_config.row_all_mask.pos + 0]);
				while ((is_helper <= 0) && (mx.request->row_any_pending))
				#else
				while ((is_helper <= 0) && mx.request.row.any_pending)
				#endif
				{
					is_helper = -1;

					uint_fast_t		x;
					#if MX_PSEUDO_CONFIG
					uint_fast_t		*po = (uint_fast_t *)&(mx.request->mask[chirp_config.my_row_mask.pos + 0]);
					for (x = *pr++; po < (uint_fast_t *)&(mx.request->mask[chirp_config.my_row_mask.pos + chirp_config.my_row_mask.len]);)
					#else
					uint_fast_t		*po = &mx.request.my_row_mask[0];
					for (x = *pr++; po < &mx.request.my_row_mask[NUM_ELEMENTS(mx.request.my_row_mask)];)
					#endif
					{
						if (!x)
						{
							x = *pr++;		// ATTENTION: dirty in the sense of access violation
							po++;
							continue;
                        }

						#ifndef __BYTE_ORDER__
							#error __BYTE_ORDER__ is undefined
						#elif !((__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) || (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__))
							#error __BYTE_ORDER__ is invalid
						#endif

						// isolate first set bit
						#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
							help_bitmask = x & -x;			// isolate LSB
						#else
							#error TODO						// isolate MSB
						#endif

						// if we can help: exit loop
						if (!(*po & help_bitmask))
							break;

						// else clear bit in x
						x &= ~help_bitmask;
                    }

					// if we can help: continue below
					#if MX_PSEUDO_CONFIG
					if (po < (uint_fast_t *)&(mx.request->mask[chirp_config.my_row_mask.pos + chirp_config.my_row_mask.len]))
					#else
					if (po < &mx.request.my_row_mask[NUM_ELEMENTS(mx.request.my_row_mask)])
					#endif
					{
						// NOTE: help_bitmask has only one bit set,
						// so it doesn't matter if we use get_msb() or get_lsb()
						#if MX_PSEUDO_CONFIG
						int16_t help_index = ARRAY_INDEX_SIZE_ADD(po, &(mx.request->mask[chirp_config.my_row_mask.pos]), sizeof(uint_fast_t) * chirp_config.my_row_mask.len) * sizeof(uint_fast_t) * 8 + gpi_get_msb(help_bitmask);
						mx.request->help_index = -(1 + help_index);
						mx.request->help_bitmask = help_bitmask;
						#else
						int16_t help_index = ARRAY_INDEX(po, mx.request.my_row_mask) * sizeof(uint_fast_t) * 8 + gpi_get_msb(help_bitmask);
						mx.request.help_index = -(1 + help_index);
						mx.request.help_bitmask = help_bitmask;
						#endif
						is_helper = 1;

						// invalidate tx packet if it is not able to help
						// NOTE: it is rebuild in this case
						// NOTE: a side effect of this is that the grid timer ISR doesn't
						// need to check the packet before sideloading the helper row
						#if MX_PSEUDO_CONFIG
						if (((mx.tx_packet->packet_chunk[chirp_config.rand.pos] & PACKET_IS_READY) >> PACKET_IS_READY_POS))
						#else
						if (mx.tx_packet.is_ready)
						#endif
						{
						#if (!MX_DOUBLE_BITMAP)
							#if MX_PSEUDO_CONFIG
							if (mx_get_leading_index(&(mx.tx_packet->packet_chunk[chirp_config.coding_vector.pos])) <= help_index)
							#else
							if (mx_get_leading_index(mx.tx_packet.coding_vector) <= help_index)
							#endif
						#else
							if (MIN(mx_get_leading_index(mx.tx_packet.coding_vector), mx_get_leading_index(mx.tx_packet.coding_vector_2)) <= help_index)
						#endif
							{
								#if MX_PSEUDO_CONFIG
								mx.tx_packet->packet_chunk[chirp_config.rand.pos] &= PACKET_IS_READY_MASK;
								#else
								mx.tx_packet.is_ready = 0;
								#endif
							}
                        }

						break;
                    }

					// break after scanning any_mask
					// NOTE: -2 matches position of pr
					#if MX_PSEUDO_CONFIG
					if (ARRAY_INDEX_SIZE_ADD(pr, &(mx.request->mask[chirp_config.row_any_mask.pos]), sizeof(uint_fast_t) * chirp_config.row_any_mask.len) - 2 < chirp_config.row_any_mask.len)
						break;

					// scan any_mask (after scanning all_mask)
					pr = &(mx.request->mask[chirp_config.row_any_mask.pos]);
					#else
					if (ARRAY_INDEX(pr, mx.request.row.any_mask) - 2 < NUM_ELEMENTS(mx.request.row.any_mask))
						break;

					// scan any_mask (after scanning all_mask)
					pr = &mx.request.row.any_mask[0];
					#endif
                }

				PROFILE("mixer_update_slot() tx decision request help 2");

				if (is_helper != 0)
				{
					#if MX_PSEUDO_CONFIG
					assert_reset(chirp_config.mx_num_nodes < 256);
					#else
					ASSERT_CT(MX_NUM_NODES < 256, inconsistent_program);
					#endif

					PROFILE("mixer_update_slot() tx decision request help 3");

					// relative rank = rank / MX_GENERATION_SIZE,
					// stored in 0.16 signed fixed point format
					#if MX_PSEUDO_CONFIG
					relative_rank = gpi_mulu_16x16(mx.rank, 0xffff / chirp_config.mx_generation_size);
					#else
					relative_rank = gpi_mulu_16x16(mx.rank, 0xffff / MX_GENERATION_SIZE);
					#endif

					// n = number of potential helpers
					uint_fast8_t n = 0;

					// all full rank neighbors can help
					#if MX_COORDINATED_TX
						n += mx_finished_head->mx_num_nodes;
						GPI_TRACE_MSG(TRACE_VERBOSE, "n_finished: %" PRIuFAST8, n);
					#endif

					// if I can help
					if (is_helper >= 0)
					{
						// add me
						n++;
						GPI_TRACE_MSG(TRACE_VERBOSE, "+me: %" PRIuFAST8, n);

						// add non-full rank neighbors which are also able to help
						#if MX_REQUEST_HEURISTIC == 0
							n += 1;
						#elif MX_REQUEST_HEURISTIC == 1
							n += (UINT16_C(3) * mx_present_head->mx_num_nodes + 2) / 4;
						#elif MX_REQUEST_HEURISTIC == 2

							#if MX_PSEUDO_CONFIG
							uint_fast16_t i = (ABS(mx.request->help_index) - 1) / (8 * sizeof(uint_fast_t));
							#else
							uint_fast16_t i = (ABS(mx.request.help_index) - 1) / (8 * sizeof_member(Node, row_map[0]));
							#endif
							GPI_TRACE_MSG(TRACE_VERBOSE, "i: %" PRIdFAST16", m: %" PRIxFAST, i, help_bitmask);

							Node *p;
							#if MX_PSEUDO_CONFIG
							for (p = mx.history[mx_present_head->next]; p != mx_present_head; p = mx.history[p->next])
							#else
							for (p = &mx.history[mx_present_head->next]; p != mx_present_head; p = &mx.history[p->next])
							#endif
							{
								#if MX_PSEUDO_CONFIG
								if (!(p->row_map_chunk[i] & help_bitmask))
								#else
								if (!(p->row_map[i] & help_bitmask))
								#endif
								{
									n++;
									#if MX_PSEUDO_CONFIG
									GPI_TRACE_MSG(TRACE_VERBOSE, "+node %u: %" PRIuFAST8, (int)ARRAY_INDEX_SIZE_ADD(p, &(mx.history[0]->prev), chirp_config.history_len_8), n);
									#else
									GPI_TRACE_MSG(TRACE_VERBOSE, "+node %u: %" PRIuFAST8, (int)ARRAY_INDEX(p, mx.history), n);
									#endif
								}
							}

						#else
							#error MX_REQUEST_HEURISTIC is invalid
						#endif
                    }
					else
					{
						// don't know which index other helpers choose -> improvise:
						// guess number of helpers; heuristic: one quarter of the neighbors
						#if MX_REQUEST_HEURISTIC == 0
							n += 1;
						#else
							n += (mx_present_head->mx_num_nodes + 2) / 4;
						#endif

						// NOTE: intuition behind heuristic 1: since we are neighbors, there is an
						// increased probability that the majority of us is in the same situation,
						// i.e., probably the majority considers the same index; and if I can(not)
						// help than probably they can(not). estimate the majority as 3/4 of us
						// (with rounding)
					}

					GPI_TRACE_MSG(TRACE_VERBOSE, "+heuristic: %" PRIuFAST8, n);

					if (is_helper > 0)
					{
						// p = 1 / n
						if (n < 2)
							p = UINT16_MAX;
						else p = gpi_divu_16x8(UINT16_MAX, n, 0);
					}
					else
					{
						// p = (1 / e) / (d - n)
						#if !MX_COORDINATED_TX
							p = 24109 / 2;
						#else
							if ((density - n) < 2)
								p = 24109;
							else p = gpi_divu_16x8(24109, density - n, 0);
						#endif
					}

					// tx probability:
					// is_helper		my slot				foreign slot	concurrent slot
					// 0			1					0				1 / (d + 1), evtl. incl. aging
					// +			p+ * rr + (1 - rr)	p+ * rr			p+ = 1 / n
					// -			p- * rr + (1 - rr)	p- * rr			p- = (1 / e) / (d - n)
					// (rr = relative rank, d = density, n = number of helpers, e = Euler's number)

					PROFILE("mixer_update_slot() tx decision request help 4");

					#if MX_PSEUDO_CONFIG
					GPI_TRACE_MSG(TRACE_VERBOSE, "request: is_helper = %" PRIdFAST8", p = %" PRIu16
						", rr = %" PRIu16 ", n = %" PRIuFAST8 ", index = %" PRId16,
						is_helper, p, relative_rank, n, mx.request->help_index);
					#else
					GPI_TRACE_MSG(TRACE_VERBOSE, "request: is_helper = %" PRIdFAST8", p = %" PRIu16
						", rr = %" PRIu16 ", n = %" PRIuFAST8 ", index = %" PRId16,
						is_helper, p, relative_rank, n, mx.request.help_index);
					#endif
				}
			}
			#endif

			PROFILE("mixer_update_slot() tx decision coord");

			#if MX_COORDINATED_TX

				// determine owner of next slot:
				// 		owner = (slot_number + 1) % MX_NUM_NODES;
				// NOTE: slot_number + 1 is the theoretical value. We do - 1 because the first
				// slot has number 1 while node IDs are 0-based. Hence we assign slot 1 to the
				// first node (with ID 0).
				// NOTE: modulo/division is expensive -> do it more efficient:
				// Instead of dividing, we simply increment owner from slot to slot with manual
				// wrap-around. Some checks ensure that it also works if slot_number jumps (e.g.
				// because of resynchronization).

				uint16_t diff = slot_number - last_owner_update;

				// limit number of potential loop iterations
				#if MX_PSEUDO_CONFIG
				if (diff >= 8 * chirp_config.mx_num_nodes)
					owner = (slot_number + 1 - 1) % chirp_config.mx_num_nodes;
				else
				{
					// skip full wrap-around cycles
					while (diff >= chirp_config.mx_num_nodes)
						diff -= chirp_config.mx_num_nodes;

					// update owner
					owner += diff;
					if (owner >= chirp_config.mx_num_nodes)
						owner -= chirp_config.mx_num_nodes;
                }
				#else
				if (diff >= 8 * MX_NUM_NODES)
					owner = (slot_number + 1 - 1) % MX_NUM_NODES;
				else
				{
					// skip full wrap-around cycles
					while (diff >= MX_NUM_NODES)
						diff -= MX_NUM_NODES;

					// update owner
					owner += diff;
					if (owner >= MX_NUM_NODES)
						owner -= MX_NUM_NODES;
                }
				#endif
				last_owner_update = slot_number;

				GPI_TRACE_MSG(TRACE_VERBOSE, "owner of slot %" PRIu16 ": %" PRIu16, slot_number + 1, owner);

				// if my slot: TX
				#if MX_PSEUDO_CONFIG
				if (owner == mx.tx_packet->sender_id)
				#else
				if (owner == mx.tx_packet.sender_id)
				#endif
				{
					GPI_TRACE_MSG(TRACE_VERBOSE, "tx decision: my slot");

					#if MX_REQUEST
						// adapt tx probability if request pending
						// -> possibly place our slot at the disposal of other helpers
						// formula: p = p * rr + (1 - rr); p = 0.16, rr = 0.16
						if (is_helper < 0)
							p = (gpi_mulu_16x16(p, relative_rank) >> 16) - relative_rank;
						else
					#endif

					p = UINT16_MAX;
					break;
                }

			#endif

			// TX in last slot -> don't TX (except it is our slot)
			#if !MX_BENCHMARK_FULL_RANDOM_TX
			if (mx.events & BV(TX_READY))
			{
				// with one exception: if we did tx in slot 1 -- i.e. we are the initiator --
				// we also transmit in slot 2 because we know that no other node uses slot 2
				// (in best case they received the first packet in slot 1 and prepare their
				// first tx packet during slot 2)
				if (slot_number == 1)
				{
					p = UINT16_MAX;
					GPI_TRACE_MSG(TRACE_VERBOSE, "tx decision: initiator in slot 2");
					break;
				}

				p = 0;
				GPI_TRACE_MSG(TRACE_VERBOSE, "tx decision: tx in previous slot");
				break;
			}
			#endif

			#if MX_COORDINATED_TX

				// during start-up phase
				#if !MX_BENCHMARK_NO_COORDINATED_STARTUP
				#if MX_PSEUDO_CONFIG
				if (slot_number < chirp_config.mx_generation_size)
				#else
				if (slot_number < MX_GENERATION_SIZE)
				#endif
				{
					GPI_TRACE_MSG(TRACE_VERBOSE, "tx decision: start-up phase");

					// if transmitter of current packet has next payload: TX
					// NOTE: originator of next payload won't TX (except for the unprobable case
					// that it is the slot owner) since TX in consecutive slots is prohibited. All
					// receivers use the slots to push the payload into its environment (more far
					// away from the transmitter). By the way they generate a wake-up wave for
					// their back-country.
					// ATTENTION: while the approach is right, it would be wrong to do that here
					// because it would be too late. Here, we are already within the next slot.
					// Therefore this case is handled in DMA ISR (as an exception), immediately
					// after the packet has been received.
					// if (flags.has_next_payload)
					// {
					//	GPI_TRACE_MSG(TRACE_VERBOSE, "tx decision: has_next_payload set");
					//	p = UINT16_MAX;
					//	break;
					// }

					// if we are the owner of the next payload: TX
					// NOTE: in start-up phase, the slots are assigned to owners by node IDs
					// *and initial payloads* (i.e. slots can have two owners in this phase).
					#if MX_PSEUDO_CONFIG
					if ((slot_number + 1 < chirp_config.mx_generation_size) && (0 == mx.matrix[slot_number + 1]->birth_slot))
					#else
					if ((slot_number + 1 < MX_GENERATION_SIZE) && (0 == mx.matrix[slot_number + 1].birth_slot))
					#endif
					{
						GPI_TRACE_MSG(TRACE_VERBOSE, "tx decision: being start-up owner");
						p = UINT16_MAX;
						break;
                    }

					// TX with probability 1 / slot_number, approximated by the shift
					// and lower bounded by 1 / 16
					p = UINT16_MAX >> MIN(gpi_get_msb(slot_number) + 1, 4);
					break;
                }
				#endif

				// foreign slot
				#if MX_PSEUDO_CONFIG
				if (mx.history[owner]->list_id != ARRAY_INDEX_SIZE_ADD(mx_absent_head, &(mx.history[0]->prev), chirp_config.history_len_8) - chirp_config.mx_num_nodes)
				#else
				if (mx.history[owner].list_id != ARRAY_INDEX(mx_absent_head, mx.history) - MX_NUM_NODES)
				#endif
				{
					GPI_TRACE_MSG(TRACE_VERBOSE, "foreign slot");

					#if MX_REQUEST
						// adapt tx probability if request pending
						// -> possibly jump in as helper
						// formula: p = p * rr; p = 0.16, rr = 0.16
						if (is_helper != 0)
							p = gpi_mulu_16x16(p, relative_rank) >> 16;
						else
					#endif

					p = 0;
					break;
				}

			#endif

			// concurrent arbitration slot
			{
				GPI_TRACE_MSG(TRACE_VERBOSE, "concurrent slot");

				// if request pending: p has been computed already
				if (is_helper != 0)
					break;

				static const uint8_t age_to_tx_LUT[] = MX_AGE_TO_TX_PROBABILITY;
				ASSERT_CT(sizeof(age_to_tx_LUT) > 0, MX_AGE_TO_TX_PROBABILITY_is_invalid);

				uint16_t age = slot_number - mx.recent_innovative_slot;

				// formula to realize:
				// p = 1 / (d + 1) + d / (d + 1) * LUT[age]
				//   = A / B with A := 1 + d * LUT[age] and B := d + 1

				// compute A, store it in 8.8 fixed point format
				p = age_to_tx_LUT[MIN(age, NUM_ELEMENTS(age_to_tx_LUT) - 1)];
			#if !MX_COORDINATED_TX
				p <<= 8;
			#else
				p *= density;
				p += 0x100;

				// compute A / B, store it in 0.16 fixed point format
				p = gpi_divu_16x8(p, density + 1, 0) << 8;

				GPI_TRACE_MSG(TRACE_VERBOSE, "tx decision age: %" PRIu16 ", density: %" PRIu16, age, density);
			#endif
            }

		} while (0);
		PROFILE("mixer_update_slot() tx decision p done");
		GPI_TRACE_MSG(TRACE_INFO, "tx decision p: %" PRIu16, p);

		next_task = RX;

		#if MX_DUTY_CYCLE
				if ((p && (mixer_rand() <= p)) & (mx.slot_number - mx.last_tx_slot >= SLOT_INTERVAL))
		#else
				if (p && (mixer_rand() <= p))
		#endif
				{
					next_task = TX;
					#if MX_DOUBLE_BITMAP
						mx.next_task_own_update = 0;
					#endif
				}

		// nodes should generate new message according to the interval
		#if MX_DOUBLE_BITMAP
			uint8_t update_now = 0;
			if (mx.start_up_flag)
			{
				if (node_id_allocate < SENSOR_NUM * GROUP_NUM)
					update_now = update_new_message(mx.slot_number);
				else
					mx.non_update++;

				if (update_now & UPDATE_NOW)
				{
					#if MX_DUTY_CYCLE
						if (mx.slot_number - mx.last_tx_slot >= SLOT_INTERVAL)
					#endif
						{
							next_task = TX;
							mx.tx_packet.is_ready = 0;
							mx.non_update = 0;
							mx.next_task_own_update = 0;
						}
				}
				else if (((update_now & UPDATE_CLOSE) && (next_task == TX)) | (mx.non_update > 100))
					next_task = RX;
				else if (mx.next_task_own_update)
				{
					next_task = TX;
					mx.next_task_own_update = 0;
				}

				if (update_now != UPDATE_NOW)
					mx.non_update++;

				clear_time_table();
			}
		#endif

		#if MX_DUTY_CYCLE
			if (next_task == TX)
				mx.last_tx_slot = mx.slot_number;
		#endif

		clear_event(TX_READY);

		PROFILE("mixer_update_slot() tx decision activate 1");

		if (TX == next_task)
		{
			// if TX and packet preparation pending: select short-term transmit data
			// in case there is not enough time to finish the full packet
			#if MX_PSEUDO_CONFIG
			if (!((mx.tx_packet->packet_chunk[chirp_config.rand.pos] & PACKET_IS_READY) >> PACKET_IS_READY_POS))
			#else
			if (!mx.tx_packet.is_ready)
			#endif
			{
				int ie = gpi_int_lock();

				#if MX_DOUBLE_BITMAP
					if (update_now & UPDATE_NOW)
						mx.tx_sideload = mx.matrix[mx.tx_packet.sender_id].coding_vector;
				#endif

				if (NULL == mx.tx_sideload)
				{
					#if MX_PSEUDO_CONFIG
					mx.tx_sideload = (uint8_t *)&(mx.tx_reserve->matrix_chunk_8[chirp_config.matrix_coding_vector_8.pos + 0]);
					#else
					mx.tx_sideload = &(mx.tx_reserve->coding_vector_8[0]);
					#endif
				}

				// NOTE: (next_task == TX) => (rank > 0) => (mx.tx_reserve != NULL) for sure

				gpi_int_unlock(ie);
			}
		}

		mixer_transport_set_next_slot_task(next_task);

		PROFILE("mixer_update_slot() tx decision activate 2");
		// if we need a new tx packet: build one
		// ATTENTION: we have to make sure that at least one of mx.tx_sideload or mx.tx_packet is valid
		// in case of TX. Since mx.tx_sideload may be reset by rx processing, we have to provide a
		// valid packet before leaving current thread if next task == TX. Therefore we don't rely
		// on TX_READY because that one is not signaled before our first transmission. Thereafter,
		// TX_READY and !is_ready are quiet equivalent - except for the fact that mx.tx_packet.is_ready
		// may also be reset during tx decision (to enforce assembly of a new packet in response to
		// request processing). Hence, checking is_ready is the right way here.
		#if MX_PSEUDO_CONFIG
		if (!((mx.tx_packet->packet_chunk[chirp_config.rand.pos] & PACKET_IS_READY) >> PACKET_IS_READY_POS) && (mx.rank > 0))
		#else
		if (!mx.tx_packet.is_ready && (mx.rank > 0))
		#endif
		{
			PROFILE("mixer_update_slot() prepare tx packet begin");

			// is_valid is used to detect if the packet may have been hurt by the ISR while preparing it
			#if MX_PSEUDO_CONFIG
			mx.tx_packet->packet_chunk[chirp_config.rand.pos] = (mx.tx_packet->packet_chunk[chirp_config.rand.pos] & PACKET_IS_VALID_MASK) | (1 << PACKET_IS_VALID_POS);
			#else
			mx.tx_packet.is_valid = 1;
			#endif
			REORDER_BARRIER();

			prepare_tx_packet();

			REORDER_BARRIER();
			#if MX_PSEUDO_CONFIG
			if (!((mx.tx_packet->packet_chunk[chirp_config.rand.pos] & PACKET_IS_VALID) >> PACKET_IS_VALID_POS))
			#else
			if (!mx.tx_packet.is_valid)
			#endif
			{
				// if mx.tx_packet gets hurt by ISR, then we can not use it. On the other hand we
				// know that next_task TX has been done already, using the sideload (while we
				// prepared the packet, that is why we are here). So the packet is broken, but
				// we don't need it anymore.

				GPI_TRACE_MSG(TRACE_VERBOSE, "tx packet hurt by ISR -> dropped it");
			}
			else
			{
				// prepare a random number for ISR
				#if MX_PSEUDO_CONFIG
				mx.tx_packet->packet_chunk[chirp_config.rand.pos] = (mx.tx_packet->packet_chunk[chirp_config.rand.pos] & PACKET_RAND_MASK) | (mixer_rand() & PACKET_RAND_MASK);
				#else
				mx.tx_packet.rand = mixer_rand();
				#endif
				REORDER_BARRIER();
				int ie = gpi_int_lock();

				// NOTE: it is possible that ISR changed the packet (by applying the sideload)
				// between testing is_valid above and the gpi_int_lock() call. Nevertheless the packet
				// is valid in this case since applying the sideload doesn't break it. So we could
				// declare it as ready anyway. The reason why we don't do that is that we don't
				// want to send the same packet twice.

				#if MX_PSEUDO_CONFIG
				if ((mx.tx_packet->packet_chunk[chirp_config.rand.pos] & PACKET_IS_VALID) >> PACKET_IS_VALID_POS)
				#else
				if (mx.tx_packet.is_valid)
				#endif
				{
					#if MX_PSEUDO_CONFIG
					mx.tx_packet->packet_chunk[chirp_config.rand.pos] |= PACKET_IS_READY;
					#else
					mx.tx_packet.is_ready = 1;
					#endif

					// reset mx.tx_sideload if it points into the matrix
					// NOTE: prepare_tx_packet() already considered all matrix rows. So if we wouldn't
					// reset mx.tx_sideload, there would be a probability that we transmit a zero-
					// packet. In particular, this would happen for sure if rank is 1.
					// NOTE: don't change mx.tx_sideload if it points to rx queue. This may also result
					// in a zero-packet, but 1) this is unprobable and 2) it happens at most once per
					// rx packet.
					// NOTE: don't touch mx.tx_sideload if it points to mx.empty_row. It plays a special
					// role and is temporarily used by rx processing. mx.tx_sideload pointing to
					// mx.empty_row is the same as pointing to rx queue.
					#if MX_PSEUDO_CONFIG
					// printf("uintptr_t:%02x, %02x, %02x, %02x, %02x\n", mx.tx_sideload, (uintptr_t)&(mx.matrix[0]->birth_slot), chirp_config.mx_generation_size * ((1 + chirp_config.matrix_chunk_32_len) * sizeof(uint_fast_t)), mx.tx_sideload - 1 * sizeof(uint_fast_t), mx.empty_row);
					// if (((uintptr_t)mx.tx_sideload - (uintptr_t)&(mx.matrix[0]->birth_slot) < chirp_config.mx_generation_size * ((1 + chirp_config.matrix_chunk_32_len) * sizeof(uint_fast_t))) && ((Matrix_Row *)(mx.tx_sideload - 1 * sizeof(uint_fast_t)) != mx.empty_row))
					if (((uintptr_t)mx.tx_sideload - (uintptr_t)&(mx.matrix[0]->birth_slot) < chirp_config.mx_generation_size * ((1 + chirp_config.matrix_chunk_32_len) * sizeof(uint_fast_t))) && ((Matrix_Row *)(mx.tx_sideload - chirp_config.matrix_coding_vector.len * sizeof(uint_fast_t)) != mx.empty_row))
					#else
					if (((uintptr_t)mx.tx_sideload - (uintptr_t)&mx.matrix < sizeof(mx.matrix)) &&(Matrix_Row *)(mx.tx_sideload - offsetof(Matrix_Row, coding_vector)) != mx.empty_row)
					#endif
						{
							// TP TODO:
							PRINTF("NULL 2\n");
							mx.tx_sideload = NULL;
						}
					REORDER_BARRIER();

					// NOTE: mx.tx_packet.is_ready is reset on ISR level (after transmission)
				}

				gpi_int_unlock(ie);
            }

			PROFILE("mixer_update_slot() prepare tx packet end");
        }

		// maintain history
		#if MX_COORDINATED_TX
			mx_purge_history();
		#endif
		#if MX_COORDINATED_TX
			#if (GPI_TRACE_MODE & GPI_TRACE_MODE_TRACE)
			{
				int i;
				#if MX_PSEUDO_CONFIG
				for (i = 0; i < chirp_config.mx_num_nodes; ++i)
				#else
				for (i = 0; i < MX_NUM_NODES; ++i)
				#endif
//					GPI_TRACE_MSG(TRACE_VERBOSE, "history %2u: %u %u", i,
//						(int)(mx.history[i].list_id), (int)(mx.history[i].last_slot_number));

				#if MX_PSEUDO_CONFIG
				for (i = 0; i < chirp_config.mx_num_nodes; ++i)
				#else
				for (i = 0; i < MX_NUM_NODES; ++i)
				#endif
				{
					char msg[16];
					sprintf(msg, "row map %2u:", i);
//					TRACE_DUMP(1, msg, mx.history[i].row_map, sizeof(mx.history[0].row_map));
				}
			}
			#endif
		#endif

		PROFILE("mixer_update_slot() end");
		// while(1);
    }
	else
	{
		if (mx.events & BV(TX_READY))
		{
			clear_event(TX_READY);
			mixer_transport_set_next_slot_task(RX);
		}
		else
		{
			mixer_transport_set_next_slot_task(TX);
		}
		PT_YIELD(pt);
	}
	}
	PT_END(pt);
}

//**************************************************************************************************

PT_THREAD(mixer_process_rx_data())
{
	Pt_Context* const	pt = pt_process_rx_data;

	#if MX_PSEUDO_CONFIG
	const unsigned int PAYLOAD_SHIFT =
		chirp_config.matrix_payload.pos * sizeof(uint_fast_t) - chirp_config.matrix_payload_8.pos;
	#else
	static const unsigned int PAYLOAD_SHIFT =
		offsetof(Matrix_Row, payload) - offsetof(Matrix_Row, payload_8);
	#endif

	// ATTENTION: ensure that PAYLOAD_SIZE is aligned because memxor_block() may rely on that
	// ATTENTION: don't use sizeof(mx.matrix[0].payload) because it might be too small due to
	// MX_BENCHMARK_PSEUDO_PAYLOAD
	#if MX_PSEUDO_CONFIG
	const unsigned int PAYLOAD_SIZE =
		chirp_config.payload.len * sizeof(uint8_t) + PADDING_SIZE(chirp_config.payload.len * sizeof(uint8_t));
	#else
	static const unsigned int PAYLOAD_SIZE =
		FIELD_SIZEOF(Packet, payload) + PADDING_SIZE(FIELD_SIZEOF(Packet, payload));
	#endif

	PT_BEGIN(pt);

	while (1)
	{
		PT_WAIT_UNTIL(pt, mx.events & BV(RX_READY));

		clear_event(RX_READY);
		if (chirp_config.task != MX_GLOSSY)
		{
		while (mx.rx_queue_num_read != mx.rx_queue_num_written)
		{
			PROFILE("mixer_process_rx_data() begin");

			// if we yield within the loop, we must declare persistent variables as static
			static Packet	*p;
			void			*pp[MEMXOR_BLOCKSIZE];
			unsigned int	pp_used;
			int_fast16_t	i;

			#if MX_PSEUDO_CONFIG
			p = mx.rx_queue[mx.rx_queue_num_read % NUM_ELEMENTS(mx.rx_queue)];
			#else
			p = &mx.rx_queue[mx.rx_queue_num_read % NUM_ELEMENTS(mx.rx_queue)];
			#endif

			#if MX_PSEUDO_CONFIG
			if (p->sender_id >= chirp_config.mx_num_nodes)
			#else
			if (p->sender_id >= MX_NUM_NODES)
			#endif
			{
				GPI_TRACE_MSG(TRACE_INFO, "Rx: invalid sender_id %u -> drop packet", p->sender_id);
				goto continue_;
			}

			#if MX_PSEUDO_CONFIG
			TRACE_DUMP(1, "Rx packet:", &(p->phy_payload_begin), chirp_config.phy_payload_size);
			#else
			TRACE_DUMP(1, "Rx packet:", &(p->phy_payload_begin), PHY_PAYLOAD_SIZE);
			#endif
			PRINTF_CHIRP("Rx: ");

			TRACE_PACKET(p);
			// #if (!BANK_1_RUN)
			// if (chirp_config.task == CHIRP_VERSION)
			// {
			// 	uint_fast32_t rx_flash[2];
			// 	memset(rx_flash, 0, sizeof(rx_flash));
			// 	memcpy(&(rx_flash[0]), &(p->packet_chunk[chirp_config.coding_vector.pos]), chirp_config.coding_vector.len);
			// 	memcpy(&(rx_flash[1]), &(p->packet_chunk[chirp_config.payload.pos + 8]), 3);
			// 	rx_flash[1] |= mx.slot_number << 24;
			// 	FLASH_If_Write(TOPO_FLASH_ADDRESS + mx.slot_number * 8, (uint32_t *)(&(rx_flash[0])), sizeof(rx_flash) / sizeof(uint32_t));
			// }
			// #endif

			/* when receive a packet at first time */
			if ((!mx.rank) && (chirp_config.disem_copy))
			{
				#if MX_PSEUDO_CONFIG
				mixer_write(node_id_allocate, &(p->packet_chunk[chirp_config.payload.pos]), chirp_config.mx_payload_size);
				if (node_id_allocate == chirp_config.mx_generation_size - 1)
					mx.empty_row -= chirp_config.matrix_size_32;
				#else
				mixer_write(node_id_allocate, p->coding_vector, chirp_config.mx_payload_size);
				if (node_id_allocate == MX_GENERATION_SIZE - 1)
					mx.empty_row --;
				#endif
			}

			#if MX_DOUBLE_BITMAP
				if (!mx.is_local_map_full)
					mx.is_local_map_full = fill_local_map(p->coding_vector);
				// clear_time_table();
				else if (mx.start_up_flag)
				{
					uint8_t update_check = bitmap_update_check(p->coding_vector, mx.tx_packet.sender_id);
					PRINTF("check:%d\n", update_check);
					#if (!MX_PREAMBLE_UPDATE)
						if(update_check != OTHER_UPDATE)
							mx.non_update += 2;
					#endif

					#if (!MX_PREAMBLE_UPDATE)
						if (update_check & OWN_UPDATE)
						{
							gpi_memcpy_dma_aligned(mx.tx_packet.coding_vector_2, mx.matrix[mx.tx_packet.sender_id].coding_vector, sizeof(mx.matrix[0].coding_vector) + sizeof(mx.matrix[0].payload));
							unwrap_coding_vector(mx.tx_packet.coding_vector, mx.tx_packet.coding_vector_2, 1);
							mx.tx_packet.is_ready = 1;
							mx.tx_sideload = NULL;

							mx.next_task_own_update = 1;
						}
						else if (update_check & OTHER_UPDATE)
					#else
						if (update_check & OTHER_UPDATE)
					#endif
						{
							#if (!MX_PREAMBLE_UPDATE)
								mx.non_update = 0;
							#endif
							static Pt_Context	pt_update_matrix;
							PT_INIT(&pt_update_matrix);
							while (PT_SCHEDULE(mixer_update_matrix(&pt_update_matrix)));

							mx.tx_packet.is_ready = 0;

							mx.tx_sideload = &(p->coding_vector[0]);

							goto continue_;
						}
				}
			#endif

			// update full-rank map
			#if MX_SMART_SHUTDOWN_MAP
				if (p->flags.is_full_rank)
				{
					update_full_rank_map(p);

					// if we are not finished: yield
					// because update_full_rank_map() might have taken a bit of time
					#if MX_PSEUDO_CONFIG
					if (mx.rank < chirp_config.mx_generation_size)
					#else
					if (mx.rank < MX_GENERATION_SIZE)
					#endif
					{
						PT_YIELD(pt);
					}
				}
			#endif

			// if we already have full rank: done
			#if MX_PSEUDO_CONFIG
			if (mx.rank >= chirp_config.mx_generation_size)
			#else
			if (mx.rank >= MX_GENERATION_SIZE)
			#endif
			{
				goto continue_;
			}

			PROFILE("mixer_process_rx_data() checkpoint 1");

			#if MX_DOUBLE_BITMAP
				wrap_coding_vector(p->coding_vector_2, p->coding_vector);
			#endif

			// if mx.tx_sideload points to current packet: move mx.tx_sideload away because data in
			// Rx queue slot will become invalid while processing
			// NOTE: there is no problem in cases where we simply free the slot by incrementing
			// mx.rx_queue_num_read (without touching the slot content) because the instance which
			// overrides the data - Rx ISR processing - updates mx.tx_sideload by itself (and is
			// never active in parallel to Tx).
			// ATTENTION: it is important to understand that the slot in progress (-> num_writing)
			// is vacant, i.e. it may be filled with data without updating mx.tx_sideload afterwards
			// (e.g. in response to a missed CRC check). This is no problem as long as we never set
			// mx.tx_sideload back to an older queue entry than it is (if it points into the queue).
			// If we don't do that, then the ISR ensures that mx.tx_sideload never points to the
			// vacant slot.
			#if MX_PSEUDO_CONFIG
			if (mx.tx_sideload == &(p->packet_chunk[chirp_config.coding_vector.pos]))
			#else
			if (mx.tx_sideload == &(p->coding_vector[0]))
			#endif
			{

				uint8_t	*pr;

				// if there is an empty row available (which is always the case as long as not
				// full rank): copy the packet to this row and use it as sideload
				// NOTE: it is important that we don't simply invalidate mx.tx_sideload because
				// the case that it points to the current packet is standard (except for high load
				// situations). If we invalidate it, there is a significant probability that fast
				// tx update doesn't happen (only if rx processing finishes before next tx slot).
				if (NULL == mx.empty_row)
					pr = NULL;
				else
				{
					#if MX_PSEUDO_CONFIG
					pr = (uint8_t *)&(mx.empty_row->matrix_chunk_8[chirp_config.matrix_coding_vector_8.pos + 0]);
					#else
					pr = &(mx.empty_row->coding_vector_8[0]);
					#endif

					#if (!MX_DOUBLE_BITMAP)
						#if MX_PSEUDO_CONFIG
						gpi_memcpy_dma_aligned(pr, &(p->packet_chunk[chirp_config.coding_vector.pos]),
							(chirp_config.matrix_coding_vector.len + chirp_config.matrix_payload.len) * sizeof(uint_fast_t));
						// gpi_memcpy_dma_aligned(pr, p->coding_vector,
						// 	(chirp_config.matrix_coding_vector.len + chirp_config.matrix_payload.len) * sizeof(uint_fast_t));
						#else
						gpi_memcpy_dma_aligned(pr, p->coding_vector,
							sizeof(mx.matrix[0].coding_vector) + sizeof(mx.matrix[0].payload));
						#endif
					#else
						gpi_memcpy_dma_aligned(pr, p->coding_vector_2,
							sizeof(mx.matrix[0].coding_vector) + sizeof(mx.matrix[0].payload));
					#endif

					#if MX_BENCHMARK_PSEUDO_PAYLOAD
						#if GPI_ARCH_IS_CORE(MSP430)
							__delay_cycles(MX_PAYLOAD_SIZE);
						#else
							#error MX_BENCHMARK_PSEUDO_PAYLOAD is unsupported on current architecture
						#endif
					#else

					unwrap_chunk(pr);

					#endif
                }

				int ie = gpi_int_lock();

				#if MX_PSEUDO_CONFIG
				if (mx.tx_sideload == &(p->packet_chunk[chirp_config.coding_vector.pos]))
				#else
				if (mx.tx_sideload == &(p->coding_vector[0]))
				#endif
				{
					mx.tx_sideload = pr;
				}

				gpi_int_unlock(ie);
            }

			// align packet elements
			#if (!MX_DOUBLE_BITMAP)
				#if MX_PSEUDO_CONFIG
				unwrap_chunk(&(p->packet_chunk[chirp_config.coding_vector.pos]));
				#else
				unwrap_chunk(&(p->coding_vector[0]));
				#endif
			#else
				unwrap_chunk(&(p->coding_vector_2[0]));
			#endif

			PROFILE("mixer_process_rx_data() checkpoint 2");

			// traverse matrix / coding vector
			pp_used = 0;
			while (1)
			{
				PROFILE("mixer_process_rx_data() matrix iteration begin");

				// get leading coefficient
				#if (!MX_DOUBLE_BITMAP)
					#if MX_PSEUDO_CONFIG
					i = mx_get_leading_index(&(p->packet_chunk[chirp_config.coding_vector.pos]));
					#else
					i = mx_get_leading_index(p->coding_vector);
					#endif
				#else
					i = mx_get_leading_index(p->coding_vector_2);
				#endif

				if (i < 0)
				{
					// if this is the last received packed: invalidate mx.tx_sideload because the
					// packet was not innovative -> ensures that the prepared tx packet won't
					// get hurt
					{
						int ie = gpi_int_lock();

						if (mx.rx_queue_num_written - mx.rx_queue_num_read == 1)
							mx.tx_sideload = NULL;

						gpi_int_unlock(ie);
					}

					break;
                }

				// if corresponding row is empty (i.e. packet is innovative): fill it, rank increase
				#if MX_PSEUDO_CONFIG
				if (UINT16_MAX == mx.matrix[i]->birth_slot)
				#else
				if (UINT16_MAX == mx.matrix[i].birth_slot)
				#endif
				{
					PROFILE("mixer_process_rx_data() new row begin");
					#if MX_PSEUDO_CONFIG
					mx.matrix[i]->birth_slot = p->slot_number;
					#else
					mx.matrix[i].birth_slot = p->slot_number;
					#endif
					mx.recent_innovative_slot = p->slot_number;

					#if MX_PSEUDO_CONFIG
					assert_reset(chirp_config.payload.pos == chirp_config.coding_vector.pos + chirp_config.coding_vector.len);
					assert_reset(chirp_config.matrix_payload.pos == chirp_config.matrix_coding_vector.pos + chirp_config.matrix_coding_vector.len);
					#else
					ASSERT_CT(offsetof(Packet, payload) ==
							offsetof(Packet, coding_vector) +
					#if MX_DOUBLE_BITMAP
							sizeof_member(Packet, coding_vector) +
					#endif
							sizeof_member(Packet, coding_vector),
						inconsistent_program);
					ASSERT_CT(offsetof(Matrix_Row, payload) ==
						offsetof(Matrix_Row, coding_vector) + sizeof_member(Matrix_Row, coding_vector),
						inconsistent_program);
					#endif

					if (pp_used)
					{
						#if MX_PSEUDO_CONFIG
						memxor_block(&(p->packet_chunk[chirp_config.payload.pos + PAYLOAD_SHIFT]), pp, PAYLOAD_SIZE, pp_used);
						#else
						memxor_block(&(p->payload[PAYLOAD_SHIFT]), pp, PAYLOAD_SIZE, pp_used);
						#endif
					}

					// if mx.tx_sideload doesn't point into rx queue: set mx.tx_sideload to current
					// rx queue packet
					// NOTE: First, if mx.tx_sideload points to current row, it must be changed
					// because row is inconsistent while copying. Second, we are here because the
					// rx packet is innovative. So the only reason not to change mx.tx_sideload is
					// that it points to a newer packet in the rx queue which we didn't process yet.
					// NOTE: at this point the rx queue packet is valid (again) since processing
					// has been done (actually, the rx queue packet will be copied into the row)
					// NOTE: there is no problem if mx.tx_reserve points to current row here
					// because it is not used on ISR level
					{
						int ie = gpi_int_lock();

					#if MX_PSEUDO_CONFIG
					// TP TODO:
					// printf("tx_sideload:%lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu\n", mx.tx_sideload, mx.rx_queue[0], sizeof(mx.rx_queue), sizeof(Packet), chirp_config.coding_vector.pos, chirp_config.phy_payload_size, chirp_config.packet_len, (uintptr_t)mx.tx_sideload - (uintptr_t)mx.rx_queue[0]);
						// if ((uintptr_t)mx.tx_sideload - (uintptr_t)mx.rx_queue[0] >= sizeof(mx.rx_queue))
						if ((uintptr_t)mx.tx_sideload - (uintptr_t)mx.rx_queue[0] >= 4 * chirp_config.packet_len)
							mx.tx_sideload = &(p->packet_chunk[chirp_config.coding_vector.pos]);
					#else
						if ((uintptr_t)mx.tx_sideload - (uintptr_t)&mx.rx_queue >= sizeof(mx.rx_queue))
							mx.tx_sideload = &(p->coding_vector[0]);
					#endif

						gpi_int_unlock(ie);
					}

					// if mx.request.help_index points to current row: invalidate mx.request.help_index
					// because row is inconsistent while copying
					// NOTE: if the program is correct, it is impossible that help_index points to
					// an empty row, so we use an assertion. Nevertheless we handle the situation
					// for the case that assertions are inactive (i.e. NDEBUG).
					// NOTE: assert() sits within the condition body to keep time with interrupts
					// locked as short as possible in the normal case
					#if MX_REQUEST
					{
						int ie = gpi_int_lock();

						#if MX_PSEUDO_CONFIG
						if (ABS(mx.request->help_index) - 1 == i)
						#else
						if (ABS(mx.request.help_index) - 1 == i)
						#endif
						{
							#if MX_DOUBLE_BITMAP
								if (!mx.start_up_flag)
							#endif
								{
									#if MX_PSEUDO_CONFIG
									assert_reset(ABS(mx.request->help_index) - 1 != i);
									#else
									assert_reset(ABS(mx.request.help_index) - 1 != i);
									#endif
									GPI_TRACE_MSG_FAST(TRACE_ERROR, "!!! request help index points to empty row -> check program, must not happen !!!");
								}
							#if MX_PSEUDO_CONFIG
							mx.request->help_index = 0;
							#else
							mx.request.help_index = 0;
							#endif
                        }

						gpi_int_unlock(ie);
					}
					#endif

					#if (!MX_DOUBLE_BITMAP)
							#if MX_PSEUDO_CONFIG
							gpi_memcpy_dma_aligned(&(mx.matrix[i]->matrix_chunk[chirp_config.matrix_coding_vector.pos]), &(p->packet_chunk[chirp_config.coding_vector.pos]),
								(chirp_config.matrix_coding_vector.len + chirp_config.matrix_payload.len) * sizeof(uint_fast_t));
							// gpi_memcpy_dma_aligned(&(mx.matrix[i]->matrix_chunk[chirp_config.matrix_coding_vector.pos]), p->coding_vector,
							// 								(chirp_config.matrix_coding_vector.len + chirp_config.matrix_payload.len) * sizeof(uint_fast_t));
							#else
							gpi_memcpy_dma_aligned(mx.matrix[i].coding_vector, p->coding_vector,
								sizeof(mx.matrix[0].coding_vector) + sizeof(mx.matrix[0].payload));
							#endif
					#else
							gpi_memcpy_dma_aligned(mx.matrix[i].coding_vector, p->coding_vector_2,
								sizeof(mx.matrix[0].coding_vector) + sizeof(mx.matrix[0].payload));
					#endif

					#if MX_BENCHMARK_PSEUDO_PAYLOAD
						#if GPI_ARCH_IS_CORE(MSP430)
							__delay_cycles(MX_PAYLOAD_SIZE);
						#else
							#error MX_BENCHMARK_PSEUDO_PAYLOAD is unsupported on current architecture
						#endif
					#endif

					mx.rank++;

					// update mx.tx_reserve
					// NOTE: there are two reasons to do so:
					// 1) init mx.tx_reserve if it is NULL
					// 2) it may be beneficial if it points to a quite new row
					#if 0	// activate only for special purposes like evaluating most stupid behavior
					if (NULL == mx.tx_reserve)
					#endif
					#if MX_PSEUDO_CONFIG
					mx.tx_reserve = (Matrix_Row *)&(mx.matrix[i]->birth_slot);
					#else
					mx.tx_reserve = &mx.matrix[i];
					#endif

					// update mx.empty_row if needed
					// NOTE: mx.empty_row is kept static to avoid expensive search runs everytime
					// an empty row is needed. starting from its last position is much cheaper.
					#if MX_PSEUDO_CONFIG
					if (mx.empty_row == (Matrix_Row *)&(mx.matrix[i]->birth_slot))
					#else
					if (mx.empty_row == &mx.matrix[i])
					#endif
					{
						#if MX_PSEUDO_CONFIG
						if (chirp_config.mx_generation_size == mx.rank)
						#else
						if (MX_GENERATION_SIZE == mx.rank)
						#endif
						{
							mx.empty_row = NULL;
						}
						else
						{
							#if MX_PSEUDO_CONFIG
							while (mx.empty_row > (Matrix_Row *)&(mx.matrix[0]->birth_slot))
							#else
							while (mx.empty_row-- > &mx.matrix[0])
							#endif
							{
								#if MX_PSEUDO_CONFIG
								mx.empty_row -= chirp_config.matrix_size_32;
								#endif
								if (UINT16_MAX == mx.empty_row->birth_slot)
									break;
							}

							#if MX_PSEUDO_CONFIG
							if (mx.empty_row < (Matrix_Row *)&(mx.matrix[0]->birth_slot))
							#else
							if (mx.empty_row < &mx.matrix[0])
							#endif
							{
								#if MX_PSEUDO_CONFIG
								mx.empty_row = (Matrix_Row *)&(mx.matrix[chirp_config.mx_generation_size - 1]->matrix_chunk[chirp_config.matrix_chunk_32_len]);
								while (mx.empty_row > (Matrix_Row *)&(mx.matrix[0]->birth_slot))
								#else
								mx.empty_row = &mx.matrix[NUM_ELEMENTS(mx.matrix)];
								while (mx.empty_row-- > &mx.matrix[0])
								#endif
								{
									#if MX_PSEUDO_CONFIG
									mx.empty_row -= chirp_config.matrix_size_32;
									#endif
									if (UINT16_MAX == mx.empty_row->birth_slot)
										break;
                                }
                            }

							#if MX_PSEUDO_CONFIG
							assert_reset(mx.empty_row >= (Matrix_Row *)&(mx.matrix[0]->birth_slot));
							#else
							assert_reset(mx.empty_row >= &mx.matrix[0]);
							#endif
                        }
                    }

					// update request mask
					#if MX_REQUEST
						#if MX_PSEUDO_CONFIG
						mx.request->mask[chirp_config.my_row_mask.pos + i / (sizeof(uint_fast_t) * 8)] &=
							~gpi_slu(1, (i % (sizeof(uint_fast_t) * 8)));
						mx.request->my_column_pending =
							mx_request_clear((uint_fast_t *)&(mx.request->mask[chirp_config.my_column_mask.pos]), &(mx.matrix[i]->matrix_chunk[chirp_config.matrix_coding_vector.pos]), chirp_config.matrix_coding_vector.len * sizeof(uint_fast_t));
						if (!mx.request->my_column_pending)
						{
							chirp_config.full_column = 0;
							PRINTF_CHIRP("-----column_pending = 0-----\n");
						}
						#else
						mx.request.my_row_mask[i / (sizeof(uint_fast_t) * 8)] &=
							~gpi_slu(1, (i % (sizeof(uint_fast_t) * 8)));
						mx.request.my_column_pending =
							mx_request_clear(mx.request.my_column_mask, mx.matrix[i].coding_vector, sizeof(mx.request.my_column_mask));
						#endif

					#endif

					PROFILE("mixer_process_rx_data() new row done");

					GPI_TRACE_MSG(TRACE_VERBOSE, "new row %u, rank: %u", i, mx.rank);
					TRACE_MATRIX();
					#if MX_PSEUDO_CONFIG
					GPI_TRACE_MSG(TRACE_VERBOSE, "empty row: %d", (NULL == mx.empty_row) ? -1 :  ARRAY_INDEX_SIZE_ADD(mx.empty_row, &(mx.matrix[0]->birth_slot), (1 + chirp_config.matrix_chunk_32_len) * sizeof(uint_fast_t)));
					#else
					GPI_TRACE_MSG(TRACE_VERBOSE, "empty row: %d", (NULL == mx.empty_row) ? -1 :  ARRAY_INDEX(mx.empty_row, mx.matrix));
					#endif

					// if we reached full rank with current packet: solve (decode)
					// NOTE: this may take some time. Although it would not be very critical if we
					// lose some packets meanwhile, we still yield to transmit something from time
					// to time.
					#if MX_PSEUDO_CONFIG
					if (chirp_config.mx_generation_size == mx.rank)
					#else
					if (MX_GENERATION_SIZE == mx.rank)
					#endif
					{
						PRINTF_CHIRP("------------full_rank------------:%d\n", mx.slot_number);

						static Pt_Context	pt_decode;

						#if MX_VERBOSE_STATISTICS
							mx.stat_counter.slot_full_rank = p->slot_number;
						#endif

						// make sure that mx.tx_sideload doesn't point into rx queue anymore
						// ATTENTION: this is important because Rx ISR doesn't update mx.tx_sideload
						// anymore after full rank has been reached. If we wouldn't change it here,
						// then it may point to an invalid entry after queue wrap-around.
						// NOTE: gpi_int_lock() is only needed if access to pointers is not atomic
						// (e.g. on 8 bit machines)
						REORDER_BARRIER();		// make sure that mx.rank is written back
						int ie = gpi_int_lock();
						#if MX_DOUBLE_BITMAP
							if (!mx.start_up_flag)
						#endif
							{
								#if MX_PSEUDO_CONFIG
								if ((mx.tx_packet->packet_chunk[chirp_config.rand.pos] & PACKET_IS_READY)>> PACKET_IS_READY_POS)
								#else
								if (mx.tx_packet.is_ready)
								#endif
								{
									mx.tx_sideload = NULL;
								}
							}
						gpi_int_unlock(ie);

						// yield because packet processing may already have taken some time
						PT_YIELD(pt);

						PROFILE("mixer_process_rx_data() decode begin");

						// start decode thread
						// ATTENTION: don't use PT_SPAWN() because it returns PT_WAITING if child
						// thread yields. Here, we have to make sure that we return PT_YIELDED in
						// this case.
						// PT_SPAWN(pt, &pt_decode, decode(&pt_decode));
						PT_INIT(&pt_decode);
						while (PT_SCHEDULE(mixer_decode(&pt_decode)))
							PT_YIELD(pt);

						#if MX_SMART_SHUTDOWN_MAP
							update_full_rank_map(NULL);
						#endif

						PROFILE("mixer_process_rx_data() decode end");
                    }

					break;
				}

				PROFILE("mixer_process_rx_data() matrix iteration checkpoint A");

				// else substitute
				#if (!MX_DOUBLE_BITMAP)
					#if MX_PSEUDO_CONFIG
					memxor(&(p->packet_chunk[chirp_config.coding_vector.pos]), &(mx.matrix[i]->matrix_chunk[chirp_config.matrix_coding_vector.pos]), chirp_config.matrix_coding_vector.len * sizeof(uint_fast_t));
					// memxor(p->coding_vector, &(mx.matrix[i]->matrix_chunk[chirp_config.matrix_coding_vector.pos]), chirp_config.matrix_coding_vector.len * sizeof(uint_fast_t));
					#else
					memxor(p->coding_vector, mx.matrix[i].coding_vector, sizeof(mx.matrix[0].coding_vector));
					#endif
				#else
					memxor(p->coding_vector_2, mx.matrix[i].coding_vector, sizeof(mx.matrix[0].coding_vector));
				#endif

				#if MX_PSEUDO_CONFIG
				pp[pp_used++] = &(mx.matrix[i]->matrix_chunk[chirp_config.matrix_payload.pos]);
				#else
				pp[pp_used++] = &(mx.matrix[i].payload);
				#endif
				if (NUM_ELEMENTS(pp) == pp_used)
				{
					// NOTE: calling with NUM_ELEMENTS(pp) instead of pp_used leads to a bit
					// better code because NUM_ELEMENTS(pp) is a constant (msp430-gcc 4.6.3)
					#if MX_PSEUDO_CONFIG
					memxor_block(&(p->packet_chunk[chirp_config.payload.pos + PAYLOAD_SHIFT]), pp, PAYLOAD_SIZE, NUM_ELEMENTS(pp));
					#else
					memxor_block(&(p->payload[PAYLOAD_SHIFT]), pp, PAYLOAD_SIZE, NUM_ELEMENTS(pp));
					#endif

					// yield after each block to keep thread-level response time small (enough)
					PT_YIELD(pt);

					pp_used = 0;
                }

				PROFILE("mixer_process_rx_data() matrix iteration end");
			}

			continue_:

			mx.rx_queue_num_read++;

			#if MX_VERBOSE_STATISTICS
				mx.stat_counter.num_rx_queue_processed++;
			#endif

			PROFILE("mixer_process_rx_data() end");
			PT_YIELD(pt);
		}
	}
	else
	{
		printf("rx\n");
		PT_YIELD(pt);
	}
	}
	PT_END(pt);
}

//**************************************************************************************************

PT_THREAD(mixer_decode(Pt_Context *pt))
{
	int ii = 0;
	PT_BEGIN(pt);

	static int_fast16_t		i;

	#if MX_DOUBLE_BITMAP
		mx.decode_done = 0;
	#endif

	GPI_TRACE_MSG_FAST(TRACE_INFO, "start decoding...");
	PROFILE("mixer_decode() entry");

	#if MX_PSEUDO_CONFIG
	for (i = chirp_config.mx_generation_size; i-- > 0;)
	#else
	for (i = NUM_ELEMENTS(mx.matrix); i-- > 0;)
	#endif
	{
		const unsigned int	SZB = sizeof(uint_fast_t) * 8;
		void				*pp[MEMXOR_BLOCKSIZE];
		unsigned int		pp_used;
		uint_fast_t			k, m, *pcv;

		// check if row is empty
		// ATTENTION: this is needed if decode() called before reaching full rank
		// (e.g. at end of round)
		#if MX_PSEUDO_CONFIG
		if (UINT16_MAX == mx.matrix[i]->birth_slot)
		#else
		if (UINT16_MAX == mx.matrix[i].birth_slot)
		#endif
		{
			continue;
		}

		PROFILE("mixer_decode() row begin");

		// if mx.tx_sideload points into the matrix: invalidate mx.tx_sideload because rows are
		// inconsistent while solving. The same holds for s_request.help_index.
		// ATTENTION: we have to redo this check after every yield
		// NOTE: there is no problem if s_tx_reserve points into the matrix
		// because it is not used on ISR level
		{
			#if MX_PSEUDO_CONFIG
			uint8_t	*p = &(mx.matrix[i]->matrix_chunk_8[chirp_config.matrix_coding_vector_8.pos]);
			#else
			uint8_t	*p = &(mx.matrix[i].coding_vector_8[0]);
			#endif

			COMPUTE_BARRIER(p);

			int ie = gpi_int_lock();

			if (mx.tx_sideload == p)
			{
		PRINTF("NULL 1\n");
				mx.tx_sideload = NULL;
			}

			gpi_int_unlock(ie);

			#if MX_REQUEST
				ie = gpi_int_lock();

				#if MX_PSEUDO_CONFIG
				if (ABS(mx.request->help_index) - 1 == i)
					mx.request->help_index = 0;
				#else
				if (ABS(mx.request.help_index) - 1 == i)
					mx.request.help_index = 0;
				#endif

				gpi_int_unlock(ie);
			#endif
		}

		pp_used = 0;

		k = i + 1;
		#if MX_PSEUDO_CONFIG
		pcv = &(mx.matrix[i]->matrix_chunk[chirp_config.matrix_coding_vector.pos + k / SZB]);
		#else
		pcv = &(mx.matrix[i].coding_vector[k / SZB]);
		#endif
		m = *pcv++ & ((-1 << (SZB - 1)) >> ((SZB - 1) - (k % SZB)));
		k &= ~(SZB - 1);

		#if MX_PSEUDO_CONFIG
		while (k < chirp_config.mx_generation_size)
		#else
		while (k < NUM_ELEMENTS(mx.matrix))
		#endif
		{
			if (!m)
			{
				m = *pcv++;
				k += SZB;
				continue;

				// NOTE: dereferencing pcv is dirty at this point because it can
				// point behind the coding vector. This is not critical because
				// we don't use this value (hence we don't catch it in favor of
				// performance), but in the strict sense this is an access violation.
			}

			k += gpi_get_lsb(m);

			#if MX_PSEUDO_CONFIG
			if (k >= chirp_config.mx_generation_size)
			#else
			if (k >= NUM_ELEMENTS(mx.matrix))
			#endif
			{
				break;
			}

			// check if row to substitute is empty
			// ATTENTION: this is needed if decode() called before reaching full rank
			// (e.g. at end of round)
			#if MX_PSEUDO_CONFIG
			if (UINT16_MAX != mx.matrix[k]->birth_slot)
			#else
			if (UINT16_MAX != mx.matrix[k].birth_slot)
			#endif
			{
				#if MX_PSEUDO_CONFIG
				pp[pp_used++] = &(mx.matrix[k]->matrix_chunk[chirp_config.matrix_coding_vector.pos + 0]);
				#else
				pp[pp_used++] = &(mx.matrix[k].coding_vector[0]);
				#endif
				if (NUM_ELEMENTS(pp) == pp_used)
				{
					#if MX_PSEUDO_CONFIG
					assert_reset(chirp_config.matrix_payload.pos == chirp_config.matrix_coding_vector.pos + chirp_config.matrix_coding_vector.len);
					#else
					ASSERT_CT(offsetof(Matrix_Row, payload) ==
						offsetof(Matrix_Row, coding_vector) + sizeof_member(Matrix_Row, coding_vector),
						inconsistent_code);
					#endif

					PROFILE("mixer_decode() row memxor_block(full) begin");

					// NOTE: calling with NUM_ELEMENTS(pp) instead of pp_used leads to a bit better
					// code because NUM_ELEMENTS(pp) is a constant (msp430-gcc 4.6.3)
					#if MX_PSEUDO_CONFIG
					memxor_block(&(mx.matrix[i]->matrix_chunk[chirp_config.matrix_coding_vector.pos + 0]), pp,
						(chirp_config.matrix_coding_vector.len + chirp_config.matrix_payload.len) * sizeof(uint_fast_t), NUM_ELEMENTS(pp));
					#else
					memxor_block(&(mx.matrix[i].coding_vector[0]), pp,
						sizeof(mx.matrix[0].coding_vector) + sizeof(mx.matrix[0].payload), NUM_ELEMENTS(pp));
					#endif


					#if MX_BENCHMARK_PSEUDO_PAYLOAD
						#if GPI_ARCH_IS_CORE(MSP430)
							__delay_cycles((3 + NUM_ELEMENTS(pp)) * MX_PAYLOAD_SIZE);
						#else
							#error MX_BENCHMARK_PSEUDO_PAYLOAD is unsupported on current architecture
						#endif
					#endif

					pp_used = 0;

					PROFILE("mixer_decode() row memxor_block(full) end");
				}
			}

			k &= ~(SZB - 1);
			m &= m - 1;
		}

		if (pp_used)
		{
			#if MX_PSEUDO_CONFIG
			assert_reset(chirp_config.matrix_payload.pos = chirp_config.matrix_coding_vector.pos + chirp_config.matrix_coding_vector.len);
			#else
			ASSERT_CT(offsetof(Matrix_Row, payload) ==
				offsetof(Matrix_Row, coding_vector) + sizeof_member(Matrix_Row, coding_vector),
				inconsistent_code);
			#endif

			#if MX_PSEUDO_CONFIG
			memxor_block(&(mx.matrix[i]->matrix_chunk[chirp_config.matrix_coding_vector.pos + 0]), pp,
				(chirp_config.matrix_coding_vector.len + chirp_config.matrix_payload.len) * sizeof(uint_fast_t), pp_used);
			#else
			memxor_block(&(mx.matrix[i].coding_vector[0]), pp,
				sizeof(mx.matrix[0].coding_vector) + sizeof(mx.matrix[0].payload), pp_used);
			#endif

			#if MX_BENCHMARK_PSEUDO_PAYLOAD
				#if GPI_ARCH_IS_CORE(MSP430)
					__delay_cycles(3 * MX_PAYLOAD_SIZE);
					while (pp_used--)
						__delay_cycles(MX_PAYLOAD_SIZE);
				#else
					#error MX_BENCHMARK_PSEUDO_PAYLOAD is unsupported on current architecture
				#endif
			#endif
		}

		PROFILE("mixer_decode() row end");

		// yield after each row to keep thread-level response time small (enough)
		// ATTENTION: matrix has to be in a consistent state at this point
		PT_YIELD(pt);
	}

	#if MX_VERBOSE_STATISTICS
		mx.stat_counter.slot_decoded = mx.slot_number;
	#endif

	GPI_TRACE_MSG(TRACE_INFO, "decoding done");

	#if MX_DOUBLE_BITMAP
		mx.decode_done = 1;
		if (mx.start_up_flag)
			update_packet_table();
	#endif

	TRACE_MATRIX();

	PT_END(pt);
}

//**************************************************************************************************

PT_THREAD(mixer_maintenance())
{
	Pt_Context* const	pt = pt_maintenance;
	// PRINTF("TRIGGER_TICK\n");

	PT_BEGIN(pt);

	// init variables at thread startup
	// NOTE: approach is useful because thread gets reinitialized (-> PT_INIT) on each mixer round
	// mx.round_deadline = gpi_tick_fast_native() + (GPI_TICK_FAST_MAX / 2);
	mx.round_deadline = gpi_tick_fast_extended() + (GPI_TICK_FAST_MAX / 2);

	mx.round_deadline_update_slot = UINT16_MAX;

	while (1)
	{
		PT_WAIT_UNTIL(pt, mx.events & BV(TRIGGER_TICK));

		clear_event(TRIGGER_TICK);
		gpi_watchdog_periodic();

		// Gpi_Fast_Tick_Native now = gpi_tick_fast_native();
		Gpi_Fast_Tick_Extended now = gpi_tick_fast_extended();

		chirp_config.update_slot++;
        PRINTF_CHIRP("l:%llu\n", (mx.round_deadline - now) / 16000000);

		// monitor round length
		// NOTE: we test once per slot, and STOP executes gracefully at the next slot boundary
		// (or both a bit relaxed during RESYNC). Hence, the timing (e.g. when in the slot is
		// "now"?) is not very critical here.
	#if MX_DOUBLE_BITMAP
		if (!mx.start_up_flag)
		{
	#endif
			#if MX_PSEUDO_CONFIG
			if (((mx.slot_number >= chirp_config.mx_round_length) || (gpi_tick_compare_fast_extended(now, mx.round_deadline) >= 0)) || ((chirp_config.task == MX_ARRANGE) && (!mx.rank) && (chirp_config.update_slot >= 6)))
			// if (((mx.slot_number >= chirp_config.mx_round_length) || (gpi_tick_compare_fast_native(now, mx.round_deadline) >= 0)) || ((chirp_config.task == MX_ARRANGE) && (!mx.rank) && (chirp_config.update_slot >= 5)))
			#else
			if ((mx.slot_number >= MX_ROUND_LENGTH) || (gpi_tick_compare_fast_native(now, mx.round_deadline) >= 0))
			#endif
			{
				#if MX_PSEUDO_CONFIG
				mx.slot_number = chirp_config.mx_round_length;
				#else
				mx.slot_number = MX_ROUND_LENGTH;
				#endif
				mx.round_deadline_update_slot = mx.slot_number;

				#if MX_PSEUDO_CONFIG
				mx.round_deadline = now +
					gpi_mulu_32x16to64((Gpi_Fast_Tick_Native)chirp_config.mx_slot_length, (typeof(mx.slot_number))chirp_config.mx_round_length - mx.slot_number);
				#else
				mx.round_deadline = now +
					gpi_mulu((Gpi_Fast_Tick_Native)MX_SLOT_LENGTH, (typeof(mx.slot_number))MX_ROUND_LENGTH - mx.slot_number);
				#endif

				GPI_TRACE_MSG(TRACE_INFO, "max. round length reached -> STOP initiated");

				gpi_atomic_set(&mx.events, BV(DEADLINE_REACHED));

				while (!mixer_transport_set_next_slot_task(STOP));
				PT_EXIT(pt);
			}
			else if (mx.round_deadline_update_slot != mx.slot_number)
			{
				// ATTENTION: updating round deadline only on slot_number updates is important
				// for right behaviour during RESYNC phases
				#if MX_PSEUDO_CONFIG
				assert_reset((GPI_TICK_FAST_MAX / 2) / chirp_config.mx_slot_length >= chirp_config.mx_round_length);
				#else
				ASSERT_CT((GPI_TICK_HYBRID_MAX / 2) / MX_SLOT_LENGTH >= MX_ROUND_LENGTH, round_period_overflow);
				#endif

				mx.round_deadline_update_slot = mx.slot_number;
				#if MX_PSEUDO_CONFIG
				mx.round_deadline = now +
					gpi_mulu_32x16to64((Gpi_Fast_Tick_Native)chirp_config.mx_slot_length, (typeof(mx.slot_number))chirp_config.mx_round_length - mx.slot_number);
				#else
				mx.round_deadline = now +
					gpi_mulu((Gpi_Fast_Tick_Native)MX_SLOT_LENGTH, (typeof(mx.slot_number))MX_ROUND_LENGTH - mx.slot_number);
				#endif

				GPI_TRACE_MSG(TRACE_INFO, "round deadline: %lu (%luus from now)",
					(unsigned long)mx.round_deadline, (unsigned long)gpi_tick_fast_to_us(mx.round_deadline - now));
			}

			#if MX_VERBOSE_PROFILE

				static unsigned int s_snapshot_index = 0;

				Gpi_Profile_Ticket	ticket;
				const char			*module_name;
				uint16_t			line;
				uint32_t			timestamp;

				gpi_milli_sleep(10);

				s_snapshot_index++;

				memset(&ticket, 0, sizeof(ticket));

				while (gpi_profile_read(&ticket, &module_name, &line, &timestamp))
				{
					#if !(GPI_ARCH_IS_BOARD(TMOTE_FLOCKLAB) || GPI_ARCH_IS_BOARD(TMOTE_INDRIYA))
						#if MX_PSEUDO_CONFIG
						PRINTF_CHIRP("# ID:%u ", mx.tx_packet->sender_id + 1);
						#else
						PRINTF_CHIRP("# ID:%u ", mx.tx_packet.sender_id + 1);
						#endif
					#endif

					PRINTF_CHIRP("profile %u %s %4" PRIu16 ": %" PRIu32 "\n", s_snapshot_index, module_name, line, timestamp);
				}

			#endif
	#if MX_DOUBLE_BITMAP
		}
	#endif

	#if MX_DOUBLE_BITMAP
		if (mx.start_up_flag)
		{
			#if GPS_DATA
				if ( now_pps_count() >= MX_SESSION_LENGTH )
			#else
				if ( mx.slot_number >= MX_SESSION_LENGTH )
			#endif
				{
					GPI_TRACE_MSG(TRACE_INFO, "max. round length reached -> STOP initiated");
					PRINTF_CHIRP("MX_SESSION_LENGTH\n");
					gpi_atomic_set(&mx.events, BV(DEADLINE_REACHED));

					while (!mixer_transport_set_next_slot_task(STOP));
					PT_EXIT(pt);
				}
		}
	#endif
	}

	PT_END(pt);
}

//**************************************************************************************************

#if MX_DOUBLE_BITMAP

// update a packet using mixer_update_matrix not mixer_write
PT_THREAD(mixer_update_matrix(Pt_Context *pt))
{
	PT_BEGIN(pt);
	PRINTF("mixer_update_matrix\n");
	PRINTF("local1:%x, %x\n", mx.local_double_map.coding_vector_8_1[0], mx.local_double_map.coding_vector_8_2[0]);

	// step0: find the (x_lsb) of the updated matrix for added into matrix
	// step1: find the (x_msb) to eliminate from x_msb
	// step2: (check) if we need to decode the matrix, else, goto step4
	// step3: if decode is needed, (decode) matrix, then step4
	// step4: (eliminate) the corrsponding rows and (add) lsb row into matrix

	// handled by mx.rx_queue[] or directly by matrix, so we should provided the related row number
	// ----------------------------------------------------------
	static uint8_t	x_msb = 0, x_lsb = 0, x_msb_index = 0, x_lsb_index = 0;
	static uint8_t	x_lsb_alter = 0, x_lsb_index_alter = 0;
	uint_fast_t		*pc;

	// step0:
	// int ie = gpi_int_lock();
	uint_fast_t		updated_coding_vector[sizeof_member(Matrix_Row, coding_vector) / sizeof(uint_fast_t)];
	uint_fast_t		updated_coding_vector_alter[sizeof_member(Matrix_Row, coding_vector) / sizeof(uint_fast_t)];
	gpi_memcpy_dma_aligned(updated_coding_vector, mx.update_row, sizeof(updated_coding_vector));
	gpi_memcpy_dma_aligned(updated_coding_vector_alter, mx.altered_coding_vector.coding_vector, sizeof(updated_coding_vector_alter));
	updated_coding_vector[NUM_ELEMENTS(updated_coding_vector) - 1] &= mx.coding_vector_mask;
	updated_coding_vector_alter[NUM_ELEMENTS(updated_coding_vector_alter) - 1] &= mx.coding_vector_mask;

	printf("alter:%lu, %lu\n", updated_coding_vector[0], updated_coding_vector_alter[0]);

	pc = &updated_coding_vector[0];
	for (; pc < &updated_coding_vector[NUM_ELEMENTS(updated_coding_vector)];)
	{
		if(!(*pc))
		{
			pc++;
			continue;
		}

		x_lsb = gpi_get_lsb_32(*pc);
		x_lsb_index = ARRAY_INDEX(pc, updated_coding_vector);
		break;
	}

	pc = &updated_coding_vector_alter[0];
	for (; pc < &updated_coding_vector_alter[NUM_ELEMENTS(updated_coding_vector_alter)];)
	{
		if(!(*pc))
		{
			pc++;
			continue;
		}

		x_lsb_alter = gpi_get_lsb_32(*pc);
		x_lsb_index_alter = ARRAY_INDEX(pc, updated_coding_vector_alter);
		break;
	}

	// gpi_int_unlock(ie);
	PRINTF("x_lsb: %lu, %lu, %lu\n", updated_coding_vector[0], x_lsb, x_lsb_index);
	PRINTF("pc: %lu, %lu\n", updated_coding_vector[0], *pc);
	// ----------------------------------------------------------
	// ie = gpi_int_lock();

	Matrix_Row		*p;
	pc = &mx.altered_coding_vector.coding_vector[NUM_ELEMENTS(mx.altered_coding_vector.coding_vector) - 1];
	uint_fast_t		x;
	uint_fast_t		x_msb_isolate = 0;
	uint8_t 		x_index = ARRAY_INDEX(pc, mx.altered_coding_vector.coding_vector);
	static uint8_t	need_decode = 0;

	// for step 4
	Matrix_Row				*pr;
	uint_fast_t				*q;
	uint8_t 				coding_index;
	static uint8_t			matrix_index;

	for ( x = *pc; pc >= &(mx.altered_coding_vector.coding_vector[0]); )
	{
		if(!x)
		{
			x = *(--pc);
			x_index--;
			continue;
		}

		// step1
		x_msb = gpi_get_msb_32(x);
		x_msb_index = x_index;

		PRINTF("x_msb: %lu, %lu, %lu\n", x, x_msb, x_msb_index);
		PRINTF("pc: %lu, %lu\n", updated_coding_vector[0], *pc);

		if ((MX_GENERATION_SIZE == mx.rank) && (mx.decode_done))
			goto continue_eliminate;

		// step2
		x_msb_isolate = gpi_slu_32(1, x_msb);		// isolate MSB

		p = &mx.matrix[x_index * sizeof(uint_fast_t) * 8 + gpi_get_msb_32(x_msb_isolate)];

		for (; p >= &mx.matrix[0]; p--)
		{
			// check if row is empty
			if (UINT16_MAX == p->birth_slot)
				continue;

			if(p->coding_vector[x_index] & x_msb_isolate)
			{
				need_decode = 1;
				goto continue_decode;
			}
		}

		// clear bit in x
		x &= ~x_msb_isolate;
	}
	// gpi_int_unlock(ie);

	// PT_YIELD(pt);
	// ----------------------------------------------------------
	// step3
	continue_decode:
	PRINTF("continue_decode\n");
	// TODO: needed to change tx_sideload and tx_reserve

	if(need_decode)
	{
		static Pt_Context	pt_decode;
		PT_INIT(&pt_decode);
		// while (PT_SCHEDULE(mixer_decode(&pt_decode)))
		// 	PT_YIELD(pt);
		while (PT_SCHEDULE(mixer_decode(&pt_decode)));
	}

	// ----------------------------------------------------------
	// step4
	continue_eliminate:
	matrix_index = x_msb_index * sizeof(uint_fast_t) * 8 + x_msb + 1;

	PRINTF("continue_eliminate\n");
	PRINTF("0: %x, %x, %x, %x\n", matrix_index, x_msb_index, x_msb, x_msb_index * sizeof(uint_fast_t) * 8 + x_msb + 1);

	while (matrix_index)
	{
		pr = &mx.matrix[--matrix_index];

		// // add the lsb row into matrix
		// if (ARRAY_INDEX(pr, mx.matrix) == (x_lsb_index * sizeof(uint_fast_t) * 8 + x_lsb))
		// {
		// 	PRINTF("2: %x, %x, %x\n", matrix_index, ARRAY_INDEX(pr, mx.matrix),(x_lsb_index * sizeof(uint_fast_t) * 8 + x_lsb));
		// 	ie = gpi_int_lock();

		// 	gpi_memcpy_dma_aligned(&(pr->coding_vector[0]), &mx.update_row, sizeof(mx.update_row));
		// 	if (pr->birth_slot == UINT16_MAX)
		// 		mx.rank ++;
		// 	pr->birth_slot = mx.slot_number;
		// 	gpi_int_unlock(ie);

		// 	PT_YIELD(pt);
		// 	continue;
		// }

		// check if row is empty
		// else if (UINT16_MAX == pr->birth_slot)
		if (UINT16_MAX == pr->birth_slot)
			continue;
		else
		{
			q = &(pr->coding_vector[x_msb_index]);
			coding_index = x_msb_index;

			// ie = gpi_int_lock();

			for (; q >= &(pr->coding_vector[0]); coding_index--, q--)
			{
				if ((*q) & mx.altered_coding_vector.coding_vector[coding_index])
				{
					pr->birth_slot = UINT16_MAX;
					mx.rank--;
					break;
				}
			}

			// gpi_int_unlock(ie);
			// PT_YIELD(pt);
		}
		memset(&mx.altered_coding_vector, 0, sizeof(mx.altered_coding_vector));
	}

	// step5: add the altered_coding_vector into the matrix, while memxor the low-order bit
	uint8_t 		alter_add_to_matrix = 0;
	if (x_lsb_index_alter * sizeof(uint_fast_t) * 8 + x_lsb_alter > x_lsb_index * sizeof(uint_fast_t) * 8 + x_lsb)
	{
		pr = &mx.matrix[x_lsb_index_alter * sizeof(uint_fast_t) * 8 + x_lsb_alter - 1];
		for (; pr >= &mx.matrix[x_lsb_index * sizeof(uint_fast_t) * 8 + x_lsb]; pr--)
		{
			matrix_index = ARRAY_INDEX(pr, mx.matrix);
			if (mx.update_row[matrix_index / (sizeof(uint_fast_t) * 8)] & (1 << (matrix_index % (sizeof(uint_fast_t) * 8))))
			{
				if (pr->birth_slot != UINT16_MAX)
					memxor(mx.update_row, pr->coding_vector, sizeof(mx.update_row));
				else
				{
					alter_add_to_matrix = 1;
					break;
				}
			}
		}
	}

	if (alter_add_to_matrix)
	{
		pr = &mx.matrix[matrix_index];
	}
	else
	{
		pr = &mx.matrix[x_lsb_index_alter * sizeof(uint_fast_t) * 8 + x_lsb_alter];
	}
	printf("add:%lu, %lu, %lu, %lu, %lu, %lu\n", x_lsb_index_alter, x_lsb_alter, x_lsb_index, x_lsb, alter_add_to_matrix, matrix_index);
	printf("matrix0:%d, %d, %d, %d, %d, %d\n", mx.matrix[0].coding_vector_8[0], mx.matrix[1].coding_vector_8[0], mx.matrix[2].coding_vector_8[0],
	mx.matrix[3].coding_vector_8[0], mx.matrix[4].coding_vector_8[0], mx.matrix[5].coding_vector_8[0]);
	uint8_t i;
	for ( i = 0; i < MX_GENERATION_SIZE; i++)
	{
	printf("p0:%d, %d, %d, %d, %d, %d, %d, %d\n", mx.matrix[i].payload_8[0], mx.matrix[i].payload_8[1], mx.matrix[i].payload_8[2],
	mx.matrix[i].payload_8[3], mx.matrix[i].payload_8[4], mx.matrix[i].payload_8[5], mx.matrix[i].payload_8[6],
	mx.matrix[i].payload_8[7]);
	}
	if (pr->birth_slot == UINT16_MAX)
		mx.rank ++;
	pr->birth_slot = mx.slot_number;
	gpi_memcpy_dma_aligned(&(pr->coding_vector[0]), &mx.update_row, sizeof(mx.update_row));

	printf("mx.rank:%d\n", mx.rank);
	printf("matrix1:%d, %d, %d, %d, %d, %d\n", mx.matrix[0].coding_vector_8[0], mx.matrix[1].coding_vector_8[0], mx.matrix[2].coding_vector_8[0],
	mx.matrix[3].coding_vector_8[0], mx.matrix[4].coding_vector_8[0], mx.matrix[5].coding_vector_8[0]);
	for ( i = 0; i < MX_GENERATION_SIZE; i++)
	{
	printf("p1:%d, %d, %d, %d, %d, %d, %d, %d\n", mx.matrix[i].payload_8[0], mx.matrix[i].payload_8[1], mx.matrix[i].payload_8[2],
	mx.matrix[i].payload_8[3], mx.matrix[i].payload_8[4], mx.matrix[i].payload_8[5], mx.matrix[i].payload_8[6],
	mx.matrix[i].payload_8[7]);
	}

	static Pt_Context	pt_decode;
	PT_INIT(&pt_decode);
	while (PT_SCHEDULE(mixer_decode(&pt_decode)));
	printf("matrix2:%d, %d, %d, %d, %d, %d\n", mx.matrix[0].coding_vector_8[0], mx.matrix[1].coding_vector_8[0], mx.matrix[2].coding_vector_8[0],
	mx.matrix[3].coding_vector_8[0], mx.matrix[4].coding_vector_8[0], mx.matrix[5].coding_vector_8[0]);
	for ( i = 0; i < MX_GENERATION_SIZE; i++)
	{
	printf("p2:%d, %d, %d, %d, %d, %d, %d, %d\n", mx.matrix[i].payload_8[0], mx.matrix[i].payload_8[1], mx.matrix[i].payload_8[2],
	mx.matrix[i].payload_8[3], mx.matrix[i].payload_8[4], mx.matrix[i].payload_8[5], mx.matrix[i].payload_8[6],
	mx.matrix[i].payload_8[7]);
	}

	PT_END(pt);
}


void unwrap_coding_vector(uint8_t *coding_vector_1, uint8_t *coding_vector_2, uint8_t self_unwrap)
{
	const uint16_t	BITMAP_SIZE = sizeof_member(Packet, coding_vector);
	gpi_memcpy_dma_inline(coding_vector_1, coding_vector_2, BITMAP_SIZE);
	if (!self_unwrap)
		gpi_memcpy_dma_inline(coding_vector_1 + BITMAP_SIZE, coding_vector_2, BITMAP_SIZE);

	uint8_t *pc = coding_vector_1;
	uint8_t *pl = mx.local_double_map.coding_vector_8_1;
	for ( ; pc < coding_vector_1 + BITMAP_SIZE * 2; )
		*(pc++) &= *(pl++);
}

void unwrap_tx_sideload(uint8_t *tx_sideload)
{
	unwrap_coding_vector(mx.sideload_coding_vector.coding_vector_8_1, tx_sideload, 0);
}


void wrap_coding_vector(uint8_t *coding_vector_2, uint8_t *coding_vector_1)
{
	const uint16_t	BITMAP_SIZE = sizeof_member(Packet, coding_vector);
	uint8_t *p2 = coding_vector_2;
	uint8_t *p1 = coding_vector_1;
	for ( ; p2 < coding_vector_2 + BITMAP_SIZE; )
		*(p2++) |= *(p1++);
}



size_t mixer_rewrite(unsigned int i, const void *msg, size_t size)
{
	GPI_TRACE_FUNCTION();

	assert_reset(i < MX_GENERATION_SIZE);

	size = MIN(size, sizeof(mx.matrix[0].payload_8));

	gpi_memcpy_dma(mx.matrix[i].payload_8, msg, size);

	unwrap_row(i);

	memset(mx.matrix[i].coding_vector, 0, sizeof(mx.matrix[0].coding_vector));
	mx.matrix[i].coding_vector_8[i / 8] |= 1 << (i % 8);

	wrap_chunk(mx.matrix[i].coding_vector_8);

	if(UINT16_MAX == mx.matrix[i].birth_slot)
	{
		mx.matrix[i].birth_slot = mx.slot_number;
		mx.recent_innovative_slot = mx.slot_number;
		mx.rank++;
	}

	// PRINTF("matrix1\n");
	// int j;
	// for ( j = 0; j < MX_GENERATION_SIZE; j++)
	// {
	// 	PRINTF("%d:%x, %x, %x\n", j, mx.matrix[j].payload[1], mx.matrix[j].payload[0], mx.matrix[j].coding_vector[0]);
	// }

	if (NULL == mx.tx_reserve)
		mx.tx_reserve = &mx.matrix[i];

	if (mx.next_own_row > &mx.matrix[i])
		mx.next_own_row = &mx.matrix[i];

	// update local bitmap and the whole matrix with related row
	mx.local_double_map.coding_vector_8_1[i / 8] ^= mx.matrix[i].coding_vector_8[i / 8];
	mx.local_double_map.coding_vector_8_2[i / 8] ^= mx.matrix[i].coding_vector_8[i / 8];

	mx.altered_coding_vector.coding_vector_8[i / 8] = 1 << (i % 8);
	// update matrix with the i row
	gpi_memcpy_dma(mx.update_row, mx.matrix[i].coding_vector, sizeof(mx.update_row));

	// mixer_update_matrix
	static Pt_Context	pt_update_matrix;
	PT_INIT(&pt_update_matrix);
	while (PT_SCHEDULE(mixer_update_matrix(&pt_update_matrix)));

	// PRINTF("matrix2\n");
	// for ( j = 0; j < MX_GENERATION_SIZE; j++)
	// {
	// 	PRINTF("%d:%x, %x, %x\n", j, mx.matrix[j].payload[1], mx.matrix[j].payload[0], mx.matrix[j].coding_vector[0]);
	// }
	PRINTF("local:%x, %x\n", mx.local_double_map.coding_vector_8_1[0], mx.local_double_map.coding_vector_8_2[0]);


	// tx_sideload
	mx.tx_sideload = mx.matrix[i].coding_vector_8;

	GPI_TRACE_RETURN(size);
}

#endif

//**************************************************************************************************
//**************************************************************************************************
