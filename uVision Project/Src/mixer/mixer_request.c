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
 *	@file					mixer_request.c
 *
 *	@brief					Mixer request management
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
//#define TRACE_GROUP1		0x00000001
//#define TRACE_GROUP2		0x00000002

// select active message groups, i.e., the messages to be printed (others will be dropped)
#ifndef GPI_TRACE_BASE_SELECTION
	#define GPI_TRACE_BASE_SELECTION	GPI_TRACE_LOG_STANDARD | GPI_TRACE_LOG_PROGRAM_FLOW
#endif
GPI_TRACE_CONFIG(mixer_request, GPI_TRACE_BASE_SELECTION | GPI_TRACE_LOG_USER);

//**************************************************************************************************
//**** Includes ************************************************************************************

#include "mixer_internal.h"

#include "gpi/olf.h"

#if MX_REQUEST

#if MX_VERBOSE_PROFILE
	GPI_PROFILE_SETUP("mixer_request.c", 300, 4);
#endif

//**************************************************************************************************
//***** Local Defines and Consts *******************************************************************



//**************************************************************************************************
//***** Local Typedefs and Class Declarations ******************************************************
#if MX_PSEUDO_CONFIG

typedef enum Request_Flag_Tag
{
	Request_row = 0,
	Request_column,
} Request_Flag;
#endif



//**************************************************************************************************
//***** Forward Declarations ***********************************************************************



//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************



//**************************************************************************************************
//***** Global Variables ***************************************************************************



//**************************************************************************************************
//***** Local Functions ****************************************************************************

static inline void request_or(uint_fast_t *dest, const uint8_t *src, unsigned int size)
{
	// NOTE: process 8-bit-wise because src may be unaligned

	uint8_t	*pd = (uint8_t*)dest;

	while (pd < (uint8_t*)&dest[size / sizeof(*dest)])
		*pd++ |= *src++;
}

//**************************************************************************************************

static inline void request_and(uint_fast_t *dest, const void *src, unsigned int size)
{
	// NOTE: process 8-bit-wise because src may be unaligned

	uint8_t			*pd = (uint8_t*)dest;
	const uint8_t	*ps = (const uint8_t*)src;

	while (pd < (uint8_t*)&dest[size / sizeof(*dest)])
		*pd++ &= *ps++;
}

//**************************************************************************************************

static inline __attribute__((always_inline)) uint16_t request_clear(uint_fast_t *dest, const void *src, unsigned int size)
{
	// NOTE: process 8-bit-wise because src may be unaligned

	uint8_t			*pd = (uint8_t*)dest;
	const uint8_t	*ps = (const uint8_t*)src;
	uint16_t		 c = 0;

	while (pd < (uint8_t*)&dest[size / sizeof(*dest)])
	{
		*pd &= ~(*ps++);
		c += gpi_popcnt_8(*pd++);
    }

	return c;
}

//**************************************************************************************************
#if MX_PSEUDO_CONFIG
static void update_request_marker(Request_Flag flag, const Packet *p)
{
	PROFILE("update_request_marker() begin");

	uint_fast_t *any_mask, *all_mask;
	uint_fast16_t *any_pending;
	if (flag == Request_row)
	{
		any_mask = (uint_fast_t *)&(mx.request->mask[chirp_config.row_any_mask.pos + 0]);
		all_mask = (uint_fast_t *)&(mx.request->mask[chirp_config.row_all_mask.pos + 0]);
		any_pending = (uint_fast16_t *)&(mx.request->row_any_pending);
	}
	else if(flag == Request_column)
	{
		any_mask = (uint_fast_t *)&(mx.request->mask[chirp_config.column_any_mask.pos + 0]);
		all_mask = (uint_fast_t *)&(mx.request->mask[chirp_config.column_all_mask.pos + 0]);
		any_pending = (uint_fast16_t *)&(mx.request->column_any_pending);
	}

	if (!(*any_pending))
	{
		gpi_memcpy_dma_inline(any_mask, &(p->packet_chunk[chirp_config.info_vector.pos]), chirp_config.matrix_coding_vector.len * sizeof(uint_fast_t));
		gpi_memcpy_dma_inline(all_mask, &(p->packet_chunk[chirp_config.info_vector.pos]), chirp_config.matrix_coding_vector.len * sizeof(uint_fast_t));

		*any_pending = 1;	// temporary, will be updated together with following coding vector snoop
	}
	else
	{
		request_or (any_mask, &(p->packet_chunk[chirp_config.info_vector.pos]), chirp_config.matrix_coding_vector.len * sizeof(uint_fast_t));
		request_and(all_mask, &(p->packet_chunk[chirp_config.info_vector.pos]), chirp_config.matrix_coding_vector.len * sizeof(uint_fast_t));
	}

	any_mask[chirp_config.matrix_coding_vector.len - 1] &= mx.request->padding_mask;
	all_mask[chirp_config.matrix_coding_vector.len - 1] &= mx.request->padding_mask;

	mx.request->last_update_slot = p->slot_number;

	PROFILE("update_request_marker() end");
}
#else
static void update_request_marker(Request_Marker *m, const Packet *p)
{
	PROFILE("update_request_marker() begin");

	if (!(m->any_pending))
	{
		gpi_memcpy_dma_inline(m->any_mask, p->info_vector, sizeof(m->any_mask));
		gpi_memcpy_dma_inline(m->all_mask, p->info_vector, sizeof(m->all_mask));

		m->any_pending = 1;	// temporary, will be updated together with following coding vector snoop
	}
	else
	{
		request_or (m->any_mask, p->info_vector, sizeof(m->any_mask));
		request_and(m->all_mask, p->info_vector, sizeof(m->all_mask));
	}

	m->any_mask[NUM_ELEMENTS(m->any_mask) - 1] &= mx.request.padding_mask;
	m->all_mask[NUM_ELEMENTS(m->all_mask) - 1] &= mx.request.padding_mask;

	mx.request.last_update_slot = p->slot_number;

	PROFILE("update_request_marker() end");
}
#endif

//**************************************************************************************************
//***** Global Functions ***************************************************************************

uint16_t mx_request_clear(uint_fast_t *dest, const void *src, unsigned int size)
{
	return request_clear(dest, src, size);
}

//**************************************************************************************************

void mx_update_request(const Packet *p)
{
	GPI_TRACE_FUNCTION();
	PROFILE("mx_update_request() begin");

	#if MX_PSEUDO_CONFIG
	if (p != mx.tx_packet)
	#else
	if (p != &mx.tx_packet)
	#endif
	{
		#if MX_PSEUDO_CONFIG
		if (p->flags.request_column)
			update_request_marker(Request_column, p);
		else if (p->flags.request_row)
			update_request_marker(Request_row, p);
		#else
		if (p->flags.request_column)
			update_request_marker(&mx.request.column, p);
		else if (p->flags.request_row)
			update_request_marker(&mx.request.row, p);
		#endif

		#if MX_REQUEST_HEURISTIC > 1
			if (!(p->flags.request_column || p->flags.is_full_rank))
			{
				// ATTENTION: in extreme overload situations (which can not happen if system is configured
				// reasonable), the content of p can change while we are working on it. It would hurt the
				// request information, which is a significant, but not fatal, error. In contrast, we have
				// to make absolutely sure that there are no side effects to memory outside of the request
				// information. This could happen at the following memcpy() if we don't handle sender_id in
				// a save way.
				uint8_t sender_id = p->sender_id;
				#if MX_PSEUDO_CONFIG
				if (sender_id >= chirp_config.mx_num_nodes)
				#else
				if (sender_id >= MX_NUM_NODES)
				#endif
				{
					return;
				}

				#if MX_PREAMBLE_UPDATE
				if (!mx.preamble_update_abort_rx)
				#endif
				{
					#if MX_PSEUDO_CONFIG
					gpi_memcpy_dma_inline(&(mx.history[sender_id]->row_map_chunk[0]), &(p->packet_chunk[chirp_config.info_vector.pos]), chirp_config.matrix_coding_vector.len * sizeof(uint_fast_t));
					#else
					gpi_memcpy_dma_inline(mx.history[sender_id].row_map, p->info_vector, sizeof(mx.history[0].row_map));
					#endif
				}
			}
		#endif
	}

	// snoop coding vector and update any_pending data

	#if MX_DOUBLE_BITMAP
		uint8_t			wrapped_coding_vector[(MX_GENERATION_SIZE + 7) / 8];
		wrap_coding_vector(wrapped_coding_vector, p->coding_vector);
	#endif

	#if MX_PSEUDO_CONFIG
	if (mx.request->column_any_pending)
	#else
	if (mx.request.column.any_pending)
	#endif
	{
		#if MX_PSEUDO_CONFIG
		uint_fast16_t last = mx.request->column_any_pending;
		#else
		uint_fast16_t last = mx.request.column.any_pending;
		#endif

		#if MX_DOUBLE_BITMAP
			mx.request.column.any_pending =
				request_clear(mx.request.column.any_mask, wrapped_coding_vector, sizeof(mx.request.column.any_mask));
			request_clear(mx.request.column.all_mask, wrapped_coding_vector, sizeof(mx.request.column.all_mask));
		#else
			#if MX_PSEUDO_CONFIG
			mx.request->column_any_pending =
				request_clear((uint_fast_t *)&(mx.request->mask[chirp_config.column_any_mask.pos]), &(p->packet_chunk[chirp_config.coding_vector.pos]), chirp_config.matrix_coding_vector.len * sizeof(uint_fast_t));
			request_clear(&(mx.request->mask[chirp_config.column_all_mask.pos]), &(p->packet_chunk[chirp_config.coding_vector.pos]), chirp_config.matrix_coding_vector.len * sizeof(uint_fast_t));
			#else
			mx.request.column.any_pending =
				request_clear(mx.request.column.any_mask, p->coding_vector, sizeof(mx.request.column.any_mask));
			request_clear(mx.request.column.all_mask, p->coding_vector, sizeof(mx.request.column.all_mask));
			#endif
		#endif

		#if MX_PSEUDO_CONFIG
		if (mx.request->column_any_pending != last)
			mx.request->last_update_slot = p->slot_number;
		#else
		if (mx.request.column.any_pending != last)
			mx.request.last_update_slot = p->slot_number;
		#endif
	}

	#if MX_PSEUDO_CONFIG
	if (mx.request->row_any_pending)
	#else
	if (mx.request.row.any_pending)
	#endif
	{
		#if MX_DOUBLE_BITMAP
			int_fast16_t i = mx_get_leading_index(wrapped_coding_vector);
		#else
			#if MX_PSEUDO_CONFIG
			int_fast16_t i = mx_get_leading_index(&(p->packet_chunk[chirp_config.coding_vector.pos]));
			#else
			int_fast16_t i = mx_get_leading_index(p->coding_vector);
			#endif
		#endif
		if (i >= 0)
		{
			uint_fast_t m = gpi_slu(1, i);

			#if MX_PSEUDO_CONFIG
			if (mx.request->mask[chirp_config.row_any_mask.pos + i / (sizeof(m) * 8)] & m)
			{
				mx.request->mask[chirp_config.row_any_mask.pos + i / (sizeof(m) * 8)] &= ~m;
				mx.request->mask[chirp_config.row_all_mask.pos + i / (sizeof(m) * 8)] &= ~m;

				mx.request->last_update_slot = p->slot_number;
            }
			#else
			if (mx.request.row.any_mask[i / (sizeof(m) * 8)] & m)
			{
				mx.request.row.any_mask[i / (sizeof(m) * 8)] &= ~m;
				mx.request.row.all_mask[i / (sizeof(m) * 8)] &= ~m;

				mx.request.last_update_slot = p->slot_number;
            }
			#endif
        }

		#if MX_PSEUDO_CONFIG
		mx.request->row_any_pending = 0;
		for (i = chirp_config.matrix_coding_vector.len; i-- > 0;)
		{
			if (mx.request->mask[chirp_config.row_any_mask.pos + i])
			{
				mx.request->row_any_pending = 1;
				break;
            }
        }
		#else
		mx.request.row.any_pending = 0;
		for (i = NUM_ELEMENTS(mx.request.row.any_mask); i-- > 0;)
		{
			if (mx.request.row.any_mask[i])
			{
				mx.request.row.any_pending = 1;
				break;
            }
        }
		#endif
	}

	PROFILE("mx_update_request() end");
	#if MX_PSEUDO_CONFIG
	TRACE_DUMP(1, "any_row_mask:", &(mx.request->mask[chirp_config.row_any_mask.pos]), chirp_config.matrix_coding_vector.len * sizeof(uint_fast_t));
	TRACE_DUMP(1, "any_column_mask:", &(mx.request->mask[chirp_config.column_any_mask.pos]), chirp_config.matrix_coding_vector.len * sizeof(uint_fast_t));
	#else
	TRACE_DUMP(1, "any_row_mask:", mx.request.row.any_mask, sizeof(mx.request.row.any_mask));
	TRACE_DUMP(1, "any_column_mask:", mx.request.column.any_mask, sizeof(mx.request.column.any_mask));
	#endif

	GPI_TRACE_RETURN();
}

//**************************************************************************************************
//**************************************************************************************************

#endif	// MX_REQUEST
