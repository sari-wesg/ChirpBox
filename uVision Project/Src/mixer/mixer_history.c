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
 *	@file					mixer_history.c
 *
 *	@brief					Mixer node history management
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
GPI_TRACE_CONFIG(mixer_history, GPI_TRACE_BASE_SELECTION | GPI_TRACE_LOG_USER);

//**************************************************************************************************
//**** Includes ************************************************************************************

#include "mixer_internal.h"

#include "gpi/olf.h"

#include <string.h>

#if MX_COORDINATED_TX

//**************************************************************************************************
//***** Local Defines and Consts *******************************************************************

//**************************************************************************************************
//***** Local Typedefs and Class Declarations ******************************************************



//**************************************************************************************************
//***** Forward Declarations ***********************************************************************



//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************



//**************************************************************************************************
//***** Global Variables ***************************************************************************



//**************************************************************************************************
//***** Local Functions ****************************************************************************

// remove node from list
static void unlink_node(uint16_t node_id)
{
	assert_reset(node_id < chirp_config.mx_num_nodes);

	Node *list_head = (Node *)mx.history[mx.history[node_id]->list_id + chirp_config.mx_num_nodes];

	assert_reset(list_head->mx_num_nodes > 0);

	mx.history[mx.history[node_id]->prev]->next = mx.history[node_id]->next;
	mx.history[mx.history[node_id]->next]->prev = mx.history[node_id]->prev;

	--(list_head->mx_num_nodes);
}

//**************************************************************************************************

// insert node at end of list
static void append_node(uint16_t node_id, Node *list_head)
{
	uint8_t	head_index;

	int i;

	// ATTENTION: list_head is variable; so depending on sizeof(mx.history[0]), ARRAY_INDEX() may
	// generate an expensive division operation. To avoid that, we manually decide what to do.
	// NOTE: the condition checks get resolved at compile time
	if (IS_POWER_OF_2(chirp_config.history_len_8))
		head_index = ARRAY_INDEX_SIZE_ADD(list_head, &(mx.history[0]->prev), chirp_config.history_len_8);
	else if (chirp_config.history_len_8 < 0x100)
		head_index = gpi_divu_16x8((uintptr_t)list_head - (uintptr_t)&(mx.history[0]->prev), chirp_config.history_len_8, 1);
//	else assert_reset(0, "inefficient program, see source code comments");
	assert_reset(IS_POWER_OF_2(chirp_config.history_len_8) || chirp_config.history_len_8 < 0x100);

	// link node
	mx.history[node_id]->prev = list_head->prev;
	mx.history[node_id]->next = head_index;
	mx.history[list_head->prev]->next = node_id;
	list_head->prev = node_id;

	mx.history[node_id]->list_id = head_index - chirp_config.mx_num_nodes;

	++(list_head->mx_num_nodes);
}

//**************************************************************************************************
//***** Global Functions ***************************************************************************

void mx_init_history()
{

	GPI_TRACE_FUNCTION();
	uint16_t	i;

	// Initially all nodes are chained together in the absent list.

	for (i = 0; i < chirp_config.mx_num_nodes; i++)
	{

		mx.history[i]->prev 		= i - 1;
		mx.history[i]->next 		= i + 1;
		mx.history[i]->value		= 0;
		mx.history[i]->list_id	= ARRAY_INDEX_SIZE_ADD(mx_absent_head, &(mx.history[0]->prev), chirp_config.history_len_8) - chirp_config.mx_num_nodes;

#if MX_REQUEST && (MX_REQUEST_HEURISTIC > 1)
		memset(&(mx.history[i]->row_map_chunk[0]), 0, chirp_config.matrix_coding_vector.len * sizeof(uint_fast_t));
#endif
	}

	mx.history[0]->prev			= ARRAY_INDEX_SIZE_ADD(mx_absent_head, &(mx.history[0]->prev), chirp_config.history_len_8);
	mx.history[--i]->next 		= ARRAY_INDEX_SIZE_ADD(mx_absent_head, &(mx.history[0]->prev), chirp_config.history_len_8);

	mx_absent_head->next		= 0;
	mx_absent_head->prev 		= i;
	mx_absent_head->mx_num_nodes 	= ++i;

	mx_present_head->next   	= ARRAY_INDEX_SIZE_ADD(mx_present_head, &(mx.history[0]->prev), chirp_config.history_len_8);
	mx_present_head->prev   	= ARRAY_INDEX_SIZE_ADD(mx_present_head, &(mx.history[0]->prev), chirp_config.history_len_8);
	mx_present_head->mx_num_nodes	= 0;

	mx_finished_head->next  	= ARRAY_INDEX_SIZE_ADD(mx_finished_head, &(mx.history[0]->prev), chirp_config.history_len_8);
	mx_finished_head->prev  	= ARRAY_INDEX_SIZE_ADD(mx_finished_head, &(mx.history[0]->prev), chirp_config.history_len_8);
	mx_finished_head->mx_num_nodes	= 0;

	GPI_TRACE_RETURN();
}

//**************************************************************************************************

void mx_update_history(uint16_t node_id, Packet_Flags flags, uint16_t slot_number)
{
	GPI_TRACE_FUNCTION();

	mx.history[node_id]->last_slot_number = slot_number;

	#if MX_SMART_SHUTDOWN
		if (flags.is_full_rank)
			mx.have_full_rank_neighbor = 1;
	#endif

	// ATTENTION: it is important to unlink/append a node also if it already is in the right list.
	// The relink ensures that the list is sorted by age in descending order.

#if MX_SMART_SHUTDOWN
	if (flags.radio_off)
	{
		unlink_node(node_id);
		append_node(node_id, mx_absent_head);
	}
	else
#endif
	if (flags.is_full_rank)
	{
		unlink_node(node_id);
		append_node(node_id, mx_finished_head);
    }
	else
	{
		unlink_node(node_id);
		append_node(node_id, mx_present_head);
    }

	GPI_TRACE_RETURN();
}

//**************************************************************************************************

void mx_purge_history()
{
	GPI_TRACE_FUNCTION();

	uint16_t	reference = mx.slot_number << 2;
	uint16_t 	node, age;

	// assert sizeof(list_id) == 2 bits
	// list_id is not used itself, but the shifts are optimized to produce efficient code
	// NOTE: this is a runtime assertion, but it is completely removed by optimization if valid
	const Node __attribute__((unused)) assert_node = {0, 0, {-1}};
	assert_msg(assert_node.list_id == 3, "inconsistent code for sizeof(list_id) != 2 bits");

	// note: the lists are sorted by slot_number in ascending order,
	// i.e. descending by age (oldest entry comes first)

	// walk through present nodes
	while (mx_present_head->mx_num_nodes)
	{
		node = mx_present_head->next;

		age = reference - (mx.history[node]->last_slot_number << 2);

		uint16_t history_window = 3 * chirp_config.mx_num_nodes;
		if (age <= (history_window << 2))
		{
			break;
		}

		GPI_TRACE_MSG(1, "purging node %u (present) from history (age = %u)", node, age >> 2);

		unlink_node(node);

		append_node(node, mx_absent_head);
	}

	// walk through finished nodes
	while (mx_finished_head->mx_num_nodes)
	{
		node = mx_finished_head->next;

		age = reference - (mx.history[node]->last_slot_number << 2);

		uint16_t history_window_finished = 1 * chirp_config.mx_num_nodes;
		if (age <= (history_window_finished << 2))
			break;

		GPI_TRACE_MSG(1, "purging node %u (finished) from history (age = %u)", node, age >> 2);

		unlink_node(node);
		append_node(node, mx_absent_head);
	}

	GPI_TRACE_RETURN();
}

//**************************************************************************************************
//**************************************************************************************************

#endif	// MX_COORDINATED_TX
