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
 *	@file					mixer_internal.h
 *
 *	@brief					internal mixer declarations
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

#ifndef __MIXER_INTERNAL_H__
#define __MIXER_INTERNAL_H__

//**************************************************************************************************
//***** Includes ***********************************************************************************

#include "mixer.h"

#include "gpi/protothreads.h"
#include "gpi/tools.h"

#if MX_VERBOSE_PROFILE
	#include "gpi/profile.h"
#endif

#include <stdint.h>
#include "chirp_internal.h"

#ifdef MX_CONFIG_FILE
	#include STRINGIFY(MX_CONFIG_FILE)
#endif

// TODO:
#include "mixer_config.h"

//**************************************************************************************************
//***** Global (Public) Defines and Consts *********************************************************

// check if required settings are defined


#ifndef INFO_VECTOR_QUEUE
	#define INFO_VECTOR_QUEUE						1
#endif

#if PSEUDO_CONF
	#define MX_PSEUDO_CONFIG						1
#endif

#if MX_PSEUDO_CONFIG
	#ifndef CHIRP_OUTLINE
		#define CHIRP_OUTLINE							1
	#endif
#endif

#if (!MX_PSEUDO_CONFIG)
#ifndef MX_NUM_NODES
	#error MX_NUM_NODES is undefined
#endif

#ifndef MX_GENERATION_SIZE
	#error MX_GENERATION_SIZE is undefined
#endif

#if !MX_PAYLOAD_SIZE
	#error MX_PAYLOAD_SIZE is invalid
#endif

#ifndef MX_SLOT_LENGTH
	#error MX_SLOT_LENGTH is undefined
#endif

#if !MX_ROUND_LENGTH
	#error MX_ROUND_LENGTH is invalid
#endif
#endif

// default values for optional settings

#ifndef MX_COORDINATED_TX
	#define MX_COORDINATED_TX		1
#endif

#ifndef MX_REQUEST
	#define MX_REQUEST				1
	#undef  MX_REQUEST_HEURISTIC
	#define MX_REQUEST_HEURISTIC	2
#endif

#ifndef MX_SMART_SHUTDOWN
	#define MX_SMART_SHUTDOWN		1
	#undef  MX_SMART_SHUTDOWN_MAP
	#define MX_SMART_SHUTDOWN_MAP	1
#endif

#ifndef MX_BENCHMARK_NO_SIDELOAD
	#define MX_BENCHMARK_NO_SIDELOAD				0
#endif

#ifndef MX_BENCHMARK_NO_SYSTEMATIC_STARTUP
	#define MX_BENCHMARK_NO_SYSTEMATIC_STARTUP		0
#endif

#ifndef MX_BENCHMARK_NO_COORDINATED_STARTUP
	#define MX_BENCHMARK_NO_COORDINATED_STARTUP		0
#endif

#ifndef MX_BENCHMARK_FULL_RANDOM_TX
	#define MX_BENCHMARK_FULL_RANDOM_TX				0
#endif

#ifndef MX_BENCHMARK_PSEUDO_PAYLOAD
	#define MX_BENCHMARK_PSEUDO_PAYLOAD				0
#endif
// TP added:
#ifndef MX_DATA_SET
	#define MX_DATA_SET								1
#endif

#ifndef MX_DOUBLE_BITMAP
	#define MX_DOUBLE_BITMAP						0

	#undef  MX_PREAMBLE_UPDATE
	#define MX_PREAMBLE_UPDATE						0
#endif

#ifndef MX_DUTY_CYCLE
	#define MX_DUTY_CYCLE							0
#endif

#ifndef MX_PACKET_TABLE
	#define MX_PACKET_TABLE							0
#endif

#ifndef GPS_DATA
	#define GPS_DATA								1
#endif

#ifndef TEST_ROUND
	#define TEST_ROUND								0
#endif

#ifndef MX_LBT_AFA
	#define MX_LBT_AFA								0
#endif

#ifndef MX_FLASH_FILE
	#define MX_FLASH_FILE							1
#endif

#ifndef MX_HEADER_CHECK
	#define MX_HEADER_CHECK							1
#endif

#if (!MX_PSEUDO_CONFIG)
// internal settings
#define MX_SLOT_LENGTH_RESYNC						((MX_SLOT_LENGTH * 5) / 2)

#define MX_HISTORY_WINDOW							(3 * MX_NUM_NODES)
#define MX_HISTORY_WINDOW_FINISHED					(1 * MX_NUM_NODES)
#endif

#ifndef MX_AGE_TO_INCLUDE_PROBABILITY
	// uint16_t
	#define MX_AGE_TO_INCLUDE_PROBABILITY			{ 32768 }
#endif

#ifndef MX_AGE_TO_TX_PROBABILITY
	// uint8_t, last entry = f(age up to infinity) (should be 0 if MX_COORDINATED_TX is on)
	#if MX_COORDINATED_TX
		#define MX_AGE_TO_TX_PROBABILITY	{ /*0xff, 0x7f,*/ 0 }
	#else
		#define MX_AGE_TO_TX_PROBABILITY	{ 128 }		// full random
	#endif
#endif

// check settings

#if !MX_COORDINATED_TX
	#if MX_REQUEST && (MX_REQUEST_HEURISTIC > 0)
		#error MX_REQUEST_HEURISTIC > 0 needs MX_COORDINATED_TX turned on
	#endif
	#if MX_SMART_SHUTDOWN
		#warning MX_SMART_SHUTDOWN turned off because it needs MX_COORDINATED_TX
		#undef MX_SMART_SHUTDOWN
	#endif
#else
	#if MX_BENCHMARK_FULL_RANDOM_TX
		#error MX_BENCHMARK_FULL_RANDOM_TX contradicts MX_COORDINATED_TX
	#endif
#endif

#if MX_REQUEST
	#ifndef MX_REQUEST_HEURISTIC
		#error MX_REQUEST_HEURISTIC is undefined
	#endif
	#if MX_COORDINATED_TX && (MX_REQUEST_HEURISTIC == 0)
		#warning MX_REQUEST_HEURISTIC == 0 is not reasonable with MX_COORDINATED_TX on, changed it to 1
		#undef MX_REQUEST_HEURISTIC
		#define MX_REQUEST_HEURISTIC	1
	#endif
#endif

#if !MX_SMART_SHUTDOWN
	#undef MX_SMART_SHUTDOWN_MAP
#endif

#if MX_PSEUDO_CONFIG
	#define PACKET_RAND_MASK					0xC0
	#define PACKET_RAND                  		0x3F

	#define PACKET_IS_VALID_MASK                0xBF
	#define PACKET_IS_VALID                  	0x40
	#define PACKET_IS_VALID_POS                	0x06

	#define PACKET_IS_READY_MASK                0x7F
	#define PACKET_IS_READY                  	0x80
	#define PACKET_IS_READY_POS                	0x07
#endif
//**************************************************************************************************
//***** Local (Private) Defines and Consts *********************************************************

#define FAST_HYBRID_RATIO		(GPI_FAST_CLOCK_RATE / GPI_HYBRID_CLOCK_RATE)
#define HYBRID_SLOW_RATIO		(GPI_HYBRID_CLOCK_RATE / GPI_SLOW_CLOCK_RATE)

#if (!MX_PSEUDO_CONFIG)
#define PHY_PAYLOAD_SIZE		(offsetof(Packet, phy_payload_end) - offsetof(Packet, phy_payload_begin))
#endif

// NOTE: PADDING_SIZE() uses sizeof(uint_fast_t) instead of ALIGNMENT (from gpi/tools.h)
// because ALIGNMENT maybe greater than sizeof(uint_fast_t), which isn't necessary here
#define PADDING_SIZE(x)			((sizeof(uint_fast_t) - ((x) % sizeof(uint_fast_t))) % sizeof(uint_fast_t))
#define PADDING_MAX(a,b)		((int)(a) >= (int)(b) ? (a) : (b))

#if MX_VERBOSE_PROFILE
	#define PROFILE(...)				GPI_PROFILE(100, ## __VA_ARGS__)
	#define PROFILE_P(priority, ...)	GPI_PROFILE(priority, ## __VA_ARGS__)
#else
	#define PROFILE(...)		while (0)
	#define PROFILE_P(...)		while (0)
#endif

#if (GPI_TRACE_MODE & GPI_TRACE_MODE_TRACE)
	#define TRACE_DUMP(group, fmt, src, size)					\
		do {													\
			if ((group) & gpi_trace_module_desc.msg_config)		\
				mx_trace_dump(fmt, src, size);					\
		} while (0)
#else
	#define TRACE_DUMP(group, fmt, src, size)	while (0)
#endif

//**************************************************************************************************

#if GPI_ARCH_IS_BOARD(TMOTE_FLOCKLAB)

	#define LED_GRID_TIMER_ISR	GPI_LED_1
	#define LED_SFD_ISR			GPI_LED_2
	#define LED_DMA_ISR			GPI_LED_2
	#define LED_TIMEOUT_ISR		GPI_LED_3
	#define LED_RX				GPI_LED_4
	#define LED_TX				GPI_LED_5
	#define LED_UPDATE_TASK		GPI_LED_5

#elif GPI_ARCH_IS_BOARD(TMOTE_PURE)

	#define LED_GRID_TIMER_ISR	GPI_LED_1
	#define LED_SFD_ISR			GPI_LED_2
	#define LED_DMA_ISR			GPI_LED_2
	#define LED_TIMEOUT_ISR		GPI_LED_3
	#define LED_RX				GPI_LED_NONE
	#define LED_TX				GPI_LED_NONE
	#define LED_UPDATE_TASK		GPI_LED_NONE

#elif GPI_ARCH_IS_BOARD(nRF_PCA10056)

	#define LED_GRID_TIMER_ISR	GPI_LED_1
	#define LED_RADIO_ISR		GPI_LED_3
	#define LED_TIMEOUT_ISR		GPI_LED_2
	#define LED_RX				GPI_LED_4
	#define LED_TX				GPI_LED_4
	#define LED_UPDATE_TASK		GPI_LED_NONE

#elif GPI_ARCH_IS_BOARD(NUCLEOL476RG)

	#define LED_GRID_TIMER_ISR	GPI_LED_1
	#define LED_TIMEOUT_ISR		GPI_LED_NONE
	// #define LED_TIMEOUT_ISR		GPI_LED_2
	#define LED_DIO0_ISR		GPI_LED_3
	#define LED_DIO3_ISR		GPI_LED_3
	#define LED_RX				GPI_LED_4
	#define LED_TX				GPI_LED_4
	#define LED_UPDATE_TASK		GPI_LED_NONE

#else
	#pragma message "mixer diagnostic LEDs are deactivated because GPI_ARCH_BOARD is unknown"
#endif

//**************************************************************************************************
//***** Forward Class and Struct Declarations ******************************************************



//**************************************************************************************************
//***** Global Typedefs and Class Declarations *****************************************************

#if GPI_ARCH_IS_CORE(MSP430)

	typedef uint16_t	uint_fast_t;
	typedef int16_t		int_fast_t;

	#define PRIdFAST	PRId16
	#define PRIuFAST	PRIu16
	#define PRIxFAST	"04" PRIx16

#elif GPI_ARCH_IS_CORE(ARMv7M)

	typedef uint32_t	uint_fast_t;
	typedef int32_t		int_fast_t;

	#define PRIdFAST	PRId32
	#define PRIuFAST	PRIu32
	#define PRIxFAST	"08" PRIx32

#else
	#error unsupported architecture
#endif

//**************************************************************************************************

typedef union __attribute__((packed)) Packet_Flags_tag
{
	uint8_t			all;

	struct __attribute__((packed))
	{
#if MX_REQUEST
		uint8_t		request_column		: 1;
		uint8_t		request_row			: 1;	// together: request_type (00 = none, 01 = row, 10 = column, 11 = reserved)
#else
		uint8_t							: 2;
#endif

		uint8_t		reserved			: 3;

#if MX_SMART_SHUTDOWN
		uint8_t		radio_off			: 1;
#else
		uint8_t							: 1;
#endif

		uint8_t		has_next_payload	: 1;	// could be overlayed with request if flag bits are scarce (together with some code changes)
		uint8_t		is_full_rank		: 1;
	};

} Packet_Flags;

//**************************************************************************************************

typedef struct Packet_Vector_Tag
{
	uint8_t pos;
	uint8_t len;
} Packet_Vector;

typedef struct __attribute__((packed)) Chirp_Config_tag
{
	/* mixer packet configuration and node number */
	uint16_t 	mx_num_nodes;
	uint16_t 	mx_generation_size;
	uint16_t 	mx_payload_size;

	Packet_Vector coding_vector;
	Packet_Vector payload;
	#if (MX_REQUEST || MX_SMART_SHUTDOWN_MAP)
		Packet_Vector info_vector;
	#endif
	Packet_Vector _padding_2;
	Packet_Vector rand;
	Packet_Vector _padding_3;
	uint16_t phy_payload_size;
	uint16_t packet_chunk_len;
	uint16_t packet_len;

	Packet_Vector matrix_coding_vector_8;
	Packet_Vector matrix_payload_8;
	Packet_Vector matrix_coding_vector;
	Packet_Vector matrix_payload;
	uint16_t matrix_chunk_8_len;
	uint16_t matrix_chunk_32_len;
	uint16_t matrix_size_32;

	uint16_t history_len_8;

	Packet_Vector map;
	Packet_Vector hash;

	Packet_Vector row_all_mask;
	Packet_Vector row_any_mask;
	Packet_Vector column_all_mask;
	Packet_Vector column_any_mask;
	Packet_Vector my_row_mask;
	Packet_Vector my_column_mask;

	/* mixer slot configuration (slot length and round length ) */
	uint32_t	mx_slot_length_in_us;
	// Gpi_Hybrid_Tick	mx_slot_length;
	Gpi_Fast_Tick_Native mx_slot_length;
	uint16_t	mx_round_length;
	uint16_t	mx_period_time_s;

	/* radio configuration (SF, BW, CR, PLEN, TX_PWR, FREQ) */
	uint8_t		lora_sf;
	uint8_t		lora_bw;
	uint8_t		lora_cr;
	uint8_t		lora_plen;
	int8_t		lora_tx_pwr;
	uint32_t	lora_freq;

	/* In the second dissemination mode, which we use to arrange a task or command, as well as to make sure all the nodes have received our commands. Each node that received a packet at first time should copy that packet to their own row. In the end, all nodes share the same packet that is from initiator. */
	uint8_t		disem_copy;
	uint8_t		full_rank;
	uint8_t		full_column;

	/* to short arrange session */
	Mixer_Task	task;
	uint8_t		update_slot;

	uint32_t	packet_hash;
} Chirp_Config;

#if INFO_VECTOR_QUEUE
typedef struct Packet_info_vector_tag
{
	#if MX_PSEUDO_CONFIG
	uint8_t		vector[0];
	#else
	uint8_t		vector[((MX_GENERATION_SIZE + 7) / 8)];
	#endif
} Packet_info_vector;
#endif

// ATTENTION: packet format is well designed with respect to alignment
typedef union __attribute__((packed)) Packet_tag
{
	uint8_t				raw[1];

	struct __attribute__((packed))
	{
		// some platforms store the PHR (LEN field) within the packet
		// ATTENTION: done in a way that ensures alignment
#if GPI_ARCH_IS_DEVICE(nRF52840)
		uint8_t			_padding_1[3];		// for alignment
		uint8_t			len;
#endif
		union __attribute__((packed))
		{
			uint8_t		phy_payload_begin;	// just a marker (e.g. for offsetof(Packet, phy_payload_begin))
			// uint16_t	slot_number;
			uint32_t	app_header;
        };

		uint16_t		slot_number;

		uint8_t			sender_id;
		Packet_Flags	flags;

#if MX_PSEUDO_CONFIG
		uint8_t 		packet_chunk[0];
#else

		#if MX_LBT_AFA
			uint8_t			full_channel[AFA_CHANNEL_NUM];
			#if (MX_REQUEST || MX_SMART_SHUTDOWN_MAP)
				uint8_t			info_vector[(MX_GENERATION_SIZE + 7) / 8];
			#endif
		#endif

		uint8_t			coding_vector[(MX_GENERATION_SIZE + 7) / 8];
		#if MX_DOUBLE_BITMAP
		uint8_t			coding_vector_2[(MX_GENERATION_SIZE + 7) / 8];
		#endif
		uint8_t			payload[MX_PAYLOAD_SIZE];

		#if (MX_REQUEST || MX_SMART_SHUTDOWN_MAP)
			#if MX_LBT_AFA
					uint8_t			padding_info_vector[(MX_GENERATION_SIZE + 7) / 8];
			#else
					uint8_t			info_vector[(MX_GENERATION_SIZE + 7) / 8];
			#endif
		#endif

		union __attribute__((packed))
		{
			int8_t		phy_payload_end[0];	// just a marker (e.g. for offsetof(Packet, phy_payload_end))

			// padding that absorbs unwrapping of coding_vector and payload
			int8_t		_padding_2[PADDING_MAX(0,
							PADDING_SIZE((MX_GENERATION_SIZE + 7) / 8)
			#if MX_DOUBLE_BITMAP
							+ PADDING_SIZE((MX_GENERATION_SIZE + 7) / 8)
			#endif
							+ PADDING_SIZE(MX_PAYLOAD_SIZE)
			#if (MX_REQUEST || MX_SMART_SHUTDOWN_MAP)
							- ((MX_GENERATION_SIZE + 7) / 8)
			#endif
							)];

#if GPI_ARCH_IS_BOARD(TMOTE)
			struct __attribute__((packed))
			{
				int8_t		rssi;

				union __attribute__((packed))
				{
					int8_t		crc_corr;

					struct __attribute__((packed))
					{
						uint8_t		correlation		: 7;
						uint8_t		crc_ok			: 1;
					};
				};
            };
#endif
		};

		// ATTENTION: members near to phy_payload_end may be overwritten by word based operations
		// (e.g. memxor()), so don't overlay sensitive information there
		union __attribute__((packed))
		{
			struct __attribute__((packed))
			{
				uint8_t		rand			: 6;
				uint8_t		is_valid		: 1;	// meaning while is_ready == 0
				uint8_t		is_ready		: 1;
            };

//			struct __attribute__((packed))
//			{
//				uint8_t						: 6;
//				uint8_t		use_sideload	: 1;	// meaning while is_ready == 1
//				uint8_t						: 1;
//			};
        };

		// pad such that sizeof(Packet) is aligned
		// ATTENTION: ensures that aligned fields are also aligned in memory in every Rx queue slot.
		// This is an important prerequisite to enable fast word based operations.
		// NOTE: manual computation is needed because outer struct is packed (constructs like
		// int_fast_t __attribute__((aligned)) _padding[0]; would not work because the attribute
		// would not align the offset of _padding, only its members)
		int8_t			_padding_3[PADDING_SIZE(
	#if MX_LBT_AFA
								AFA_CHANNEL_NUM +
								((MX_GENERATION_SIZE + 7) / 8) +		// coding_vector_2
	#endif
								((MX_GENERATION_SIZE + 7) / 8) +	// coding_vector
	#if MX_DOUBLE_BITMAP
								((MX_GENERATION_SIZE + 7) / 8) +		// coding_vector_2
	#endif
								MX_PAYLOAD_SIZE +					// payload
#if (MX_REQUEST || MX_SMART_SHUTDOWN_MAP)
								((MX_GENERATION_SIZE + 7) / 8) +	// info_vector
	#if !GPI_ARCH_IS_BOARD(TMOTE)
								PADDING_MAX(0,						// _padding_2
									PADDING_SIZE((MX_GENERATION_SIZE + 7) / 8)
		#if MX_DOUBLE_BITMAP
									+ PADDING_SIZE((MX_GENERATION_SIZE + 7) / 8)
		#endif
									+ PADDING_SIZE(MX_PAYLOAD_SIZE)
									- ((MX_GENERATION_SIZE + 7) / 8)
									) +
	#endif
#else
	#if !GPI_ARCH_IS_BOARD(TMOTE)
								PADDING_MAX(0,						// _padding_2
									PADDING_SIZE((MX_GENERATION_SIZE + 7) / 8)
		#if MX_DOUBLE_BITMAP
									+ PADDING_SIZE((MX_GENERATION_SIZE + 7) / 8)
		#endif
									+ PADDING_SIZE(MX_PAYLOAD_SIZE)) +
	#endif
#endif
								1)];								// flags
#endif
	};
} Packet;

#if MX_PSEUDO_CONFIG
ASSERT_CT_STATIC(!(offsetof(Packet, packet_chunk) % sizeof(uint_fast_t)), alignment_issue);
#else
ASSERT_CT_STATIC(!(sizeof(Packet) % sizeof(uint_fast_t)), alignment_issue);
#endif

//**************************************************************************************************

typedef struct Matrix_Row_tag
{
	union
	{
		uint16_t		birth_slot;
		uint_fast_t		_alignment_dummy;
	};

	union
	{
		struct
		{
			#if MX_PSEUDO_CONFIG
			uint8_t 			matrix_chunk_8[0];
			#else
			uint8_t				coding_vector_8[(MX_GENERATION_SIZE + 7) / 8];

			#if MX_BENCHMARK_PSEUDO_PAYLOAD
				uint8_t			payload_8[0];
			#else
				uint8_t			payload_8[MX_PAYLOAD_SIZE];
			#endif
			#endif
		};

		struct
		{
			#if MX_PSEUDO_CONFIG
			uint_fast_t			matrix_chunk[0];
			#else
			uint_fast_t			coding_vector[
									(MX_GENERATION_SIZE + (sizeof(uint_fast_t) * 8) - 1) /
									(sizeof(uint_fast_t) * 8)	];	// -> potentially longer

			#if MX_BENCHMARK_PSEUDO_PAYLOAD
				uint_fast_t		payload[0];
			#else
				uint_fast_t		payload[
									(MX_PAYLOAD_SIZE + sizeof(uint_fast_t) - 1) /
									sizeof(uint_fast_t)	];	// -> potentially shifted and longer
			#endif
			#endif
		};
	};

} Matrix_Row;

//**************************************************************************************************

typedef struct Node_tag
{
	// linked list pointers
	uint8_t				prev;
	uint8_t				next;

	union
	{
		uint16_t		value;					// for raw access
		uint16_t		mx_num_nodes;				// for sentinel nodes
		struct									// for real nodes
		{
		#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)

			uint16_t	list_id				: 2;
			uint16_t	last_slot_number	: 14;

		#elif (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)

			uint16_t	last_slot_number	: 14;
			uint16_t	list_id				: 2;

		#else
			#error byte order is unknown
		#endif
        };
    };

#if (MX_REQUEST_HEURISTIC > 1)
	#if MX_PSEUDO_CONFIG
	uint_fast_t			row_map_chunk[0];
	#else
	uint_fast_t			row_map[sizeof_member(Matrix_Row, coding_vector) / sizeof(uint_fast_t)];
	#endif
#endif

	// value is either the slot in which the node was heard last or the number of nodes in the list
	// in case of the sentinel node. Slot numbers are shifted left by 2 and the 2 LSBs indicate a
	// node's current list/state (present, absent, finished). The shift allows to do computations
	// without masking the state bits.

} Node;

//**************************************************************************************************
#if MX_SMART_SHUTDOWN_MAP

typedef struct Full_Rank_Map_tag
{
	#if MX_PSEUDO_CONFIG
	uint8_t		map_hash[0];
	#else
//	#define HASH_FACTOR		((MX_NUM_NODES + MX_GENERATION_SIZE - 1) / MX_GENERATION_SIZE)
	#define HASH_FACTOR		(((MX_NUM_NODES + 7) / 8 + sizeof_member(Packet, info_vector) - 1) / sizeof_member(Packet, info_vector))

	uint8_t		map[HASH_FACTOR * sizeof_member(Packet, info_vector)];
	uint8_t		hash[sizeof_member(Packet, info_vector)];
	#endif
} Full_Rank_Map;

#endif
//**************************************************************************************************
#if (!MX_PSEUDO_CONFIG)
typedef struct Request_Marker_tag
{
	uint_fast_t		all_mask[sizeof_member(Matrix_Row, coding_vector) / sizeof(uint_fast_t)];
	uint_fast_t		any_mask[sizeof_member(Matrix_Row, coding_vector) / sizeof(uint_fast_t)];
	uint_fast16_t	any_pending;

} Request_Marker;
#endif

typedef struct Request_Data_tag
{
	#if MX_PSEUDO_CONFIG
	uint_fast16_t	row_any_pending;
	uint_fast16_t	column_any_pending;

	uint16_t		last_update_slot;
	int16_t			help_index;
	uint_fast_t		help_bitmask;

	uint16_t		my_column_pending;

	uint_fast_t		padding_mask;

	uint_fast_t		mask[0]; /* row, colum all_mask and any_mask, my_row_mask, my_column_mask*/
	#else
	Request_Marker	row;
	Request_Marker	column;
	uint16_t		last_update_slot;
	int16_t			help_index;
	uint_fast_t		help_bitmask;

	uint_fast_t		my_row_mask[sizeof_member(Matrix_Row, coding_vector) / sizeof(uint_fast_t)];
	uint_fast_t		my_column_mask[sizeof_member(Matrix_Row, coding_vector) / sizeof(uint_fast_t)];
	uint16_t		my_column_pending;

	uint_fast_t		padding_mask;
	#endif
} Request_Data;

//**************************************************************************************************
#if MX_DOUBLE_BITMAP

typedef struct Double_map_tag
{
	uint8_t			coding_vector_8_1[(MX_GENERATION_SIZE + 7) / 8];
	uint8_t			coding_vector_8_2[(MX_GENERATION_SIZE + 7) / 8];
} Double_map;

typedef struct Bitmapping_tag
{
	union
	{
		struct
		{
			uint8_t				coding_vector_8[(MX_GENERATION_SIZE + 7) / 8];
		};

		struct
		{
			uint_fast_t			coding_vector[
									(MX_GENERATION_SIZE + (sizeof(uint_fast_t) * 8) - 1) /
									(sizeof(uint_fast_t) * 8)	];	// -> potentially longer
		};
	};
} Bitmapping;


typedef struct Time_table_tag
{
	Double_map			time_bit;
	uint8_t				time_value[MX_GENERATION_SIZE * 2];
} Time_table;
#endif

//**************************************************************************************************
#if MX_PACKET_TABLE
typedef struct Packet_table_tag
{
	uint8_t				flag;
	uint16_t			latency;
	uint8_t				node_id;
	uint16_t			gps_time;
	uint8_t				set;
} Packet_table;

typedef struct Result_packet_tag
{
	uint32_t			sensor_reliability;
	uint32_t			sensor_latency;
	uint32_t			energy;
	// ------------------------------------//
	uint32_t			action_reliability;
	uint32_t			action_latency;
} Result_packet;
#endif

typedef struct __attribute__((packed)) Chirp_Outline_tag
{
	Mixer_Task 			task;

	uint16_t			round; 				/* current round num */
	uint16_t			round_max; 			/* desired round num to carriage task */
	uint8_t				round_setup; 		/* setup round for all nodes synchronization */

	Mixer_Task 			arrange_task;		/* MX_ARRANGE: to arrange the next task */

	uint32_t			packet_time;
	uint32_t			default_sf;
	uint32_t			default_freq;
	uint8_t				default_payload_len;

	uint32_t			hash_header;

	/* CHIRP_START: mixer config */
	uint16_t			start_year;
	uint8_t				start_month;
	uint8_t				start_date;
	uint8_t				start_hour;
	uint8_t				start_min;
	uint8_t				start_sec;

	uint16_t			end_year;
	uint8_t				end_month;
	uint8_t				end_date;
	uint8_t				end_hour;
	uint8_t				end_min;
	uint8_t				end_sec;

	uint8_t				flash_protection;

	/* MX_DISSEMINATE / MX_COLLECT / CHIRP_TOPO: mixer config */
	uint8_t				num_nodes;
	uint8_t				generation_size;
	uint8_t				payload_len;
	uint16_t 			file_chunk_len;

	/* MX_DISSEMINATE */
	uint32_t			firmware_size;
	uint16_t			version_hash;

	uint8_t				patch_update;
	uint8_t 			patch_bank;

	uint8_t				patch_page;
	uint32_t			old_firmware_size;

	uint8_t				disem_flag;
	uint16_t			disem_file_index;
	uint16_t			disem_file_max;
	uint16_t			disem_file_index_stay;
	uint32_t			*disem_file_memory;

	/* MX_COLLECT */
	uint32_t			collect_addr_start;
	uint32_t			collect_addr_end;
	uint32_t			collect_length;

	/* CHIRP_CONNECTIVITY */
	uint8_t				sf;
	uint32_t			freq;
	int8_t				tx_power;

	/* CHIRP_SNIFF */
	uint8_t				sniff_nodes_num; /* number of sniff nodes */
	uint8_t				sniff_flag; /* if the node itself is a sniffer */
	Sniff_Net			sniff_net;
	uint32_t			sniff_freq; /* sniff frequency (if the node is a sniffer) */
	Sniff_Config		*sniff_node[0];
} Chirp_Outl;

//**************************************************************************************************

typedef enum Slot_Activity_tag
{
	RX			= 0,
	TX			= 1,
	STOP		= 2

} Slot_Activity;

//**************************************************************************************************

typedef enum Event_tag
{
	// order = priority: the lower the value, the higher its priority
	STOPPED = 0,
	SLOT_UPDATE,
	TX_READY,
	TRIGGER_TICK,
	RX_READY,
	DEADLINE_REACHED,

} Event;

//**************************************************************************************************
#if MX_DOUBLE_BITMAP

typedef enum Bitmap_Update_Mode_tag
{
	OTHER_UPDATE			= 1,
	OWN_UPDATE				= 2,

} Bitmap_Update_Mode;

typedef enum Update_Message_tag
{
	UPDATE_NOW				= 1,
	UPDATE_CLOSE			= 2,

} Update_Message;

#endif
//**************************************************************************************************
#if MX_DATA_SET
typedef enum Data_read_tag
{
	NO_READ			= 0,
	READ_RESULTS	= 1,
	READ_TOPOLOGY	= 2

} Data_read;
#endif
//**************************************************************************************************

//**************************************************************************************************

typedef struct pt 	Pt_Context;

//**************************************************************************************************
//***** Global Variables ***************************************************************************

// static internal data
extern struct mx
{
	volatile unsigned int		events;				// type must be compatible to gpi_atomic_...()

	#if MX_PSEUDO_CONFIG
		Packet						*rx_queue[4];
		#if INFO_VECTOR_QUEUE
		Packet_info_vector				*code_queue[4];
		/* backup info_vector of received packets */
		Packet_info_vector				*info_queue[4];
		#endif
	#else
		Packet						rx_queue[4];
		#if INFO_VECTOR_QUEUE
		Packet_info_vector				code_queue[4];
		Packet_info_vector				info_queue[4];
		#endif
	#endif
	volatile uint_fast_t		rx_queue_num_writing;
	volatile uint_fast_t		rx_queue_num_written;
	volatile uint_fast_t		rx_queue_num_read;

	#if MX_PSEUDO_CONFIG
		Packet						*tx_packet;
	#else
		Packet						tx_packet;
	#endif
	const uint8_t* volatile		tx_sideload;
	const Matrix_Row*			tx_reserve;

	volatile uint16_t			slot_number;
	// volatile unsigned int		events;				// type must be compatible to gpi_atomic_...()
	Gpi_Fast_Tick_Extended		round_deadline;
	uint16_t					round_deadline_update_slot;

	#if MX_PSEUDO_CONFIG
	Matrix_Row					*matrix[MX_GENERATION_SIZE_MAX];
	#else
	Matrix_Row					matrix[MX_GENERATION_SIZE];
	#endif
	uint16_t					rank;
	Matrix_Row*					empty_row;
	Matrix_Row*					next_own_row;
	uint16_t					recent_innovative_slot;

#if MX_COORDINATED_TX
	#if MX_PSEUDO_CONFIG
	Node						*history[MX_NUM_NODES_MAX + 3];		// incl. 3 sentinel nodes
	#else
	Node						history[MX_NUM_NODES + 3];		// incl. 3 sentinel nodes
	#endif
#endif

#if MX_SMART_SHUTDOWN
	uint8_t						have_full_rank_neighbor;
	#if MX_SMART_SHUTDOWN_MAP
		#if MX_PSEUDO_CONFIG
		Full_Rank_Map			*full_rank_map;
		#else
		Full_Rank_Map			full_rank_map;
		#endif
	#endif
#endif

#if MX_REQUEST
	#if MX_PSEUDO_CONFIG
	Request_Data				*request;
	#else
	Request_Data				request;
	#endif
#endif

	Mixer_Stat_Counter			stat_counter;

#if MX_VERBOSE_STATISTICS
	Gpi_Fast_Tick_Native		wake_up_timestamp;
#endif

#if MX_DOUBLE_BITMAP
	// flags
	uint8_t						start_up_flag;
	uint8_t						decode_done;
	uint8_t 					is_local_map_full;
	uint8_t 					non_update;
	uint8_t 					update_flag;
	uint8_t 					next_task_own_update;
	#if MX_PREAMBLE_UPDATE
		uint8_t 					preamble_update_abort_rx;
		Packet						packet_header;
	#endif
	// coding_vector mask
	uint_fast_t					coding_vector_mask;

	Double_map					local_double_map;
	Double_map					sideload_coding_vector;

	Bitmapping					altered_coding_vector;
	Time_table					transition_time_table;
	uint_fast_t					update_row[(sizeof_member(Matrix_Row, coding_vector) + sizeof_member(Matrix_Row, payload)) / sizeof(uint_fast_t)];
#endif

#if MX_DUTY_CYCLE
	uint_fast_t					last_tx_slot;
#endif

#if MX_LBT_AFA
	uint8_t						current_channel[AFA_CHANNEL_NUM]; // current using channel: 3, 1, 4
	uint8_t						current_channel_used_num[AFA_CHANNEL_NUM]; //for all nodes
	uint8_t 					current_channel_occupy; // for tx channel
	uint8_t						occupied_channel_flag; // if channel is full, flag is 1, if use a new channel, flag is 0

	uint8_t						coding_vector_map[(MX_GENERATION_SIZE + 7) / 8];
	Packet						lbt_packet_header;
	uint8_t 					lbt_coding_check_abort_rx;
#endif

} mx;

#if MX_PACKET_TABLE
extern struct evaluation
{
	Packet_table				packet_table[MX_PACKET_TABLE_SIZE];
	Result_packet				result_packet;
} evaluation;
#endif

#if (!MX_PSEUDO_CONFIG)
#if MX_COORDINATED_TX
	static Node* const			mx_absent_head = &mx.history[NUM_ELEMENTS(mx.history) - 3];
	static Node* const			mx_present_head = &mx.history[NUM_ELEMENTS(mx.history) - 2];
	static Node* const			mx_finished_head = &mx.history[NUM_ELEMENTS(mx.history) - 1];
#endif
#else
	Node *mx_absent_head, *mx_present_head, *mx_finished_head;
#endif

// the following is significant for efficient queue handling
ASSERT_CT_STATIC(IS_POWER_OF_2(NUM_ELEMENTS(((struct mx*)0)->rx_queue)), rx_queue_size_must_be_power_of_2);
#if (!MX_PSEUDO_CONFIG)
ASSERT_CT_STATIC(!((uintptr_t)&mx.rx_queue & (sizeof(uint_fast_t) - 1)), rx_queue_is_not_aligned);
ASSERT_CT_STATIC(!((uintptr_t)&mx.tx_packet & (sizeof(uint_fast_t) - 1)), rx_queue_is_not_aligned);
#endif
//**************************************************************************************************

extern Pt_Context				pt_data[3];

//**************************************************************************************************

extern Chirp_Config chirp_config;

//**************************************************************************************************
//***** Prototypes of Global Functions *************************************************************

#ifdef __cplusplus
	extern "C" {
#endif

void 			mx_trace_dump(const char *header, const void *p, uint_fast16_t size);

// internal helper functions
uint16_t 		mixer_rand();
int_fast16_t	mx_get_leading_index(const uint8_t *coding_vector);
void			mx_init_history();
void			mx_update_history(uint16_t node_id, Packet_Flags flags, uint16_t slot_number);
void			mx_purge_history();
//static uint8_t	mx_history_num(const Node *head);
uint16_t		mx_request_clear(uint_fast_t *dest, const void *src, unsigned int size);
void			mx_update_request(const Packet *p);

// void 			SX1276OnDio0Irq();
// void 			SX1276OnDio3Irq();

void			mixer_transport_init();
void			mixer_transport_arm_initiator();
void			mixer_transport_start();
int 			mixer_transport_set_next_slot_task(Slot_Activity task);

PT_THREAD(		mixer_update_slot());
PT_THREAD(		mixer_maintenance());
PT_THREAD(		mixer_process_rx_data());
PT_THREAD(		mixer_decode(Pt_Context *pt));

#if MX_PSEUDO_CONFIG

void 			unwrap_chunk(uint8_t *p);
void 			unwrap_row(unsigned int i);
void 			wrap_chunk(uint8_t *p);

#endif

#if MX_DOUBLE_BITMAP
	// processing layer
	PT_THREAD(		mixer_update_matrix());

	void 			unwrap_coding_vector(uint8_t *coding_vector_1, uint8_t *coding_vector_2, uint8_t self_unwrap);
	void 			unwrap_tx_sideload(uint8_t *tx_sideload);
	void 			wrap_coding_vector(uint8_t *coding_vector_2, uint8_t *coding_vector_1);

	// mixer_bitmap
	void 			set_start_up_flag();
	void 			clear_start_up_flag();
	uint8_t 		fill_local_map(uint8_t *p_packet_coding);
	uint8_t 		bitmap_update_check(uint8_t *p_packet_coding, uint8_t node_id);
	void 			add_to_time_table(uint8_t x, uint8_t index);
	void			clear_time_table();

	#if MX_PREAMBLE_UPDATE
		uint8_t 		bitmap_update_check_header(uint8_t *p_packet_coding, uint8_t node_id);
	#endif
#endif


#if MX_DATA_SET
	void 			clear_data();
	void 			startup_message(uint32_t mixer_round, uint8_t node_id, uint8_t mx_task);
	uint8_t 		read_message(uint32_t *round, uint8_t *mx_task);

	uint8_t 		update_new_message(uint16_t slot_number);
	void 			clear_dataset();


	#if SEND_RESULT
		void 		clear_data_result();
		uint32_t 	read_result_message(uint8_t read_case);
		void 		sensor_send_results_in_mixer(uint8_t send_case);
		// send results
		void 		sensor_result_message(uint32_t mixer_round, uint8_t node_id);
		void 		decode_sensor_results(uint8_t *data_result);
		// send topology
		void 		sensor_topology_write(uint32_t mixer_round, uint8_t node_id);
		void 		decode_topology_results(uint8_t *data_result);
	#endif
		void 		uart_read_data(uint8_t uart_isr_flag, uint8_t buffer_len);
		void 		uart_read_command(uint8_t *p, uint8_t rxbuffer_len);
	#if MX_PSEUDO_CONFIG
		void 		chirp_mx_packet_config(uint8_t mx_num_nodes, uint8_t mx_generation_size, uint8_t mx_payload_size);
		void 		chirp_mx_slot_config(uint32_t mx_slot_length_in_us, uint16_t mx_round_length, uint32_t mx_period_time_us);
		void 		chirp_mx_radio_config(uint8_t lora_spreading_factor, uint8_t lora_bandwidth, uint8_t lora_codingrate, uint8_t lora_preamble_length, int8_t tx_output_power, uint32_t lora_frequency);
		void 		chirp_mx_payload_distribution(Mixer_Task mx_task);

		#if CHIRP_OUTLINE
			void 		chirp_write(uint8_t node_id, Chirp_Outl *chirp_outl);
			uint8_t 	chirp_recv(uint8_t node_id, Chirp_Outl *chirp_outl);
			uint8_t		chirp_mx_round(uint8_t node_id, Chirp_Outl *chirp_outl);
		#endif
	#endif
#endif

#if MX_PACKET_TABLE
	void 			update_packet_table();
	void 			read_packet_table();
	void 			get_real_packet_group();
	void 			calculate_action_time();
#endif

#if MX_LBT_AFA
	uint8_t			assign_tx_channel(uint8_t tx_channel);
	uint8_t 		coding_vector_check(uint8_t *p_packet_coding);
	uint8_t 		update_tx_channel(uint8_t tx_channel);
	void			update_rx_channel(uint8_t *current_channel_used_num);
#endif

#ifdef __cplusplus
	}
#endif

//**************************************************************************************************
//***** Implementations of Inline Functions ********************************************************

#if (!MX_PSEUDO_CONFIG)

static inline __attribute__((always_inline)) void unwrap_chunk(uint8_t *p)
{
	// double-check alignment of packet fields
	#if MX_LBT_AFA
		ASSERT_CT(
			!((offsetof(Packet, coding_vector) - sizeof_member(Packet, info_vector) - sizeof_member(Packet, full_channel)) % sizeof(uint_fast_t)),
			inconsistent_alignment);
	#else
		ASSERT_CT(
			!(offsetof(Packet, coding_vector) % sizeof(uint_fast_t)),
			inconsistent_alignment);
	#endif
	ASSERT_CT(
		offsetof(Packet, payload) ==
			offsetof(Packet, coding_vector) +
#if MX_DOUBLE_BITMAP
			sizeof_member(Packet, coding_vector) +
#endif
			sizeof_member(Packet, coding_vector),
		inconsistent_alignment);

	// double-check alignment of matrix row fields
	ASSERT_CT(
		!(offsetof(Matrix_Row, coding_vector) % sizeof(uint_fast_t)),
		inconsistent_alignment);
	ASSERT_CT(
		!(offsetof(Matrix_Row, payload) % sizeof(uint_fast_t)),
		inconsistent_alignment);
	ASSERT_CT(
		offsetof(Matrix_Row, coding_vector_8) == offsetof(Matrix_Row, coding_vector),
		inconsisten_alignment);
	ASSERT_CT(
		offsetof(Matrix_Row, payload_8) ==
			offsetof(Matrix_Row, coding_vector_8) + sizeof_member(Matrix_Row, coding_vector_8),
		inconsisten_alignment);

	// NOTE: condition gets resolved at compile time
	if (offsetof(Matrix_Row, payload_8) != offsetof(Matrix_Row, payload))
	{
//		#pragma GCC diagnostic push
//		#pragma GCC diagnostic ignored "-Warray-bounds"

		uint8_t			*s = p + sizeof_member(Matrix_Row, coding_vector_8);
		uint8_t			*d = s + sizeof_member(Matrix_Row, payload_8);
		unsigned int	i;

		for (i = offsetof(Matrix_Row, payload) - offsetof(Matrix_Row, payload_8); i-- > 0;)
			*d++ = *s++;

//		#pragma GCC diagnostic pop
    }
}

//**************************************************************************************************

static inline __attribute__((always_inline)) void unwrap_row(unsigned int i)
{
	unwrap_chunk(&(mx.matrix[i].coding_vector_8[0]));
}

//**************************************************************************************************

static inline __attribute__((always_inline)) void wrap_chunk(uint8_t *p)
{
	// NOTE: condition gets resolved at compile time
	if (offsetof(Matrix_Row, payload_8) != offsetof(Matrix_Row, payload))
	{
//		#pragma GCC diagnostic push
//		#pragma GCC diagnostic ignored "-Warray-bounds"

		uint8_t			*d = p + sizeof_member(Matrix_Row, coding_vector_8);
		uint8_t			*s = d + sizeof_member(Matrix_Row, payload_8);
		unsigned int	i;

		for (i = offsetof(Matrix_Row, payload) - offsetof(Matrix_Row, payload_8); i-- > 0;)
			*d++ = *s++;

//		#pragma GCC diagnostic pop
    }
}

#endif
//**************************************************************************************************
//#if MX_COORDINATED_TX
//
//static inline __attribute__((always_inline)) uint8_t mx_history_num(const Node *head)
//{
//	return head->mx_num_nodes;
//}
//
//#endif	// MX_COORDINATED_TX
//**************************************************************************************************
//**************************************************************************************************

#endif // __MIXER_INTERNAL_H__
