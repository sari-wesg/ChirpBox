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
#if USE_FOR_CHIRPBOX
	#include "chirp_internal.h"
#endif
#if USE_FOR_LORAWAN
	#include "lorawan_internal.h"
#endif

// TODO:
#include "mixer_config.h"
#include "loradisc.h"

//**************************************************************************************************
//***** Global (Public) Defines and Consts *********************************************************

// check if required settings are defined


#ifndef INFO_VECTOR_QUEUE
	#define INFO_VECTOR_QUEUE						1
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
	#define MX_SMART_SHUTDOWN		0
	#undef  MX_SMART_SHUTDOWN_MAP
	#define MX_SMART_SHUTDOWN_MAP	0
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

	#define PACKET_RAND_MASK					0xC0
	#define PACKET_RAND                  		0x3F

	#define PACKET_IS_VALID_MASK                0xBF
	#define PACKET_IS_VALID                  	0x40
	#define PACKET_IS_VALID_POS                	0x06

	#define PACKET_IS_READY_MASK                0x7F
	#define PACKET_IS_READY                  	0x80
	#define PACKET_IS_READY_POS                	0x07
//**************************************************************************************************
//***** Local (Private) Defines and Consts *********************************************************

#define FAST_HYBRID_RATIO		(GPI_FAST_CLOCK_RATE / GPI_HYBRID_CLOCK_RATE)
#define HYBRID_SLOW_RATIO		(GPI_HYBRID_CLOCK_RATE / GPI_SLOW_CLOCK_RATE)

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
	#define LED_TIMEOUT_ISR		GPI_LED_2
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

/* local config */
typedef struct __attribute__((packed)) LoRaDisC_Config_tag
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

	uint8_t		full_rank;

	#if MX_LBT_ACCESS
		uint8_t		lbt_channel_primary;
		uint8_t 	lbt_channel_total;
		uint32_t 	lbt_channel_mask;

		Gpi_Fast_Tick_Native lbt_detect_duration_us;

		uint32_t 	lbt_channel_available;
		Chirp_Time	lbt_init_time;
		/* address must be 32-bit */
		uint32_t 	lbt_channel_time_us[LBT_CHANNEL_NUM];
		uint32_t 	lbt_channel_time_stats_us[LBT_CHANNEL_NUM];
	#endif

	uint8_t		full_column;

	uint16_t	packet_hash;
	uint8_t 	flooding_packet_header[FLOODING_SURPLUS_LENGTH];
	uint8_t 	flooding_packet_payload[FLOODING_LENGTH];

	/* Disc_Primitive */
	Disc_Primitive primitive;
} LoRaDisC_Config;

#if INFO_VECTOR_QUEUE
typedef struct Packet_info_vector_tag
{
	uint8_t		vector[0];
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
			struct __attribute__((packed))
			{
				uint16_t	app_header;

				uint8_t 	packet_header[2]; // used when flooding packets or for collect and dissem
			};
        };

		uint16_t		slot_number;

		uint8_t			sender_id;
		Packet_Flags	flags;

		uint8_t 		packet_chunk[0];
	};
} Packet;

ASSERT_CT_STATIC(!(offsetof(Packet, packet_chunk) % sizeof(uint_fast_t)), alignment_issue);

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
			uint8_t 			matrix_chunk_8[0];
		};

		struct
		{
			uint_fast_t			matrix_chunk[0];
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
	uint_fast_t			row_map_chunk[0];
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
	uint8_t		map_hash[0];
} Full_Rank_Map;

#endif
//**************************************************************************************************
typedef struct Request_Data_tag
{
	uint_fast16_t	row_any_pending;
	uint_fast16_t	column_any_pending;

	uint16_t		last_update_slot;
	int16_t			help_index;
	uint_fast_t		help_bitmask;

	uint16_t		my_column_pending;

	uint_fast_t		padding_mask;

	uint_fast_t		mask[0]; /* row, colum all_mask and any_mask, my_row_mask, my_column_mask*/
} Request_Data;

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

typedef struct pt 	Pt_Context;

//**************************************************************************************************
//***** Global Variables ***************************************************************************

// static internal data
extern struct mx
{
	volatile unsigned int		events;				// type must be compatible to gpi_atomic_...()

		Packet						*rx_queue[4];
		#if INFO_VECTOR_QUEUE
		Packet_info_vector				*code_queue[4];
		/* backup info_vector of received packets */
		Packet_info_vector				*info_queue[4];
		#endif

	volatile uint_fast_t		rx_queue_num_writing;
	volatile uint_fast_t		rx_queue_num_written;
	volatile uint_fast_t		rx_queue_num_read;

	Packet						*tx_packet;

	const uint8_t* volatile		tx_sideload;
	const Matrix_Row*			tx_reserve;

	volatile uint16_t			slot_number;
	// volatile unsigned int		events;				// type must be compatible to gpi_atomic_...()
	Gpi_Fast_Tick_Extended		round_deadline;
	uint16_t					round_deadline_update_slot;

	Matrix_Row					*matrix[MX_GENERATION_SIZE_MAX];

	uint16_t					rank;
	Matrix_Row*					empty_row;
	Matrix_Row*					next_own_row;
	uint16_t					recent_innovative_slot;

#if MX_COORDINATED_TX
	Node						*history[MX_NUM_NODES_MAX + 3];		// incl. 3 sentinel nodes
#endif

#if MX_SMART_SHUTDOWN
	uint8_t						have_full_rank_neighbor;
	#if MX_SMART_SHUTDOWN_MAP
		Full_Rank_Map			*full_rank_map;
	#endif
#endif

#if MX_REQUEST
	Request_Data				*request;
#endif

	Mixer_Stat_Counter			stat_counter;

#if MX_VERBOSE_STATISTICS
	Gpi_Fast_Tick_Native		wake_up_timestamp;
#endif
} mx;

	Node *mx_absent_head, *mx_present_head, *mx_finished_head;

// the following is significant for efficient queue handling
ASSERT_CT_STATIC(IS_POWER_OF_2(NUM_ELEMENTS(((struct mx*)0)->rx_queue)), rx_queue_size_must_be_power_of_2);

//**************************************************************************************************

extern Pt_Context				pt_data[3];

//**************************************************************************************************

extern LoRaDisC_Config loradisc_config;

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

void 			unwrap_chunk(uint8_t *p);
void 			unwrap_row(unsigned int i);
void 			wrap_chunk(uint8_t *p);

void 			clear_data();

void 			uart_read_data(uint8_t uart_isr_flag, uint8_t buffer_len);
void 			uart_read_command(uint8_t *p, uint8_t rxbuffer_len);

/* loradisc config */
	void 		loradisc_packet_config(uint8_t mx_num_nodes, uint8_t mx_generation_size, uint8_t mx_payload_size, Disc_Primitive primitive);
	void 		loradisc_slot_config(uint32_t mx_slot_length_in_us, uint16_t mx_round_length, uint32_t period_time_us_plus);
	void 		loradisc_radio_config(uint8_t lora_spreading_factor, uint8_t lora_codingrate, int8_t tx_output_power, uint32_t lora_frequency);
	void 		chirp_payload_distribution(ChirpBox_Task mx_task);

/* loradisc tools */
	uint32_t Chirp_RSHash(uint8_t* str, uint32_t len);

#ifdef __cplusplus
	}
#endif

//**************************************************************************************************

#endif // __MIXER_INTERNAL_H__
