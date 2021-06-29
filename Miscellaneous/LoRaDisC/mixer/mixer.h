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
 *	@file					mixer.h
 *
 *	@brief					public Mixer API
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

#ifndef __MIXER_H__
#define __MIXER_H__

//**************************************************************************************************
//***** Includes ***********************************************************************************

#include "gpi/tools.h"		// STRINGIFY
#include "gpi/clocks.h"

#ifdef MX_CONFIG_FILE
	#include STRINGIFY(MX_CONFIG_FILE)
#endif

#include <stdint.h>
#include <stddef.h>			// size_t

//**************************************************************************************************
//***** Global (Public) Defines and Consts *********************************************************



//**************************************************************************************************
//***** Local (Private) Defines and Consts *********************************************************



//**************************************************************************************************
//***** Forward Class and Struct Declarations ******************************************************



//**************************************************************************************************
//***** Global Typedefs and Class Declarations *****************************************************

typedef enum Mixer_Start_Mode_tag
{
	MX_ARM_INITIATOR		= 1,
	MX_ARM_INFINITE_SCAN	= 2,

} Mixer_Start_Mode;

//**************************************************************************************************

typedef struct Mixer_Stat_Counter_tag
{
	uint16_t			num_sent;

#if MX_VERBOSE_STATISTICS

	uint16_t			num_received;
	uint16_t			num_resync;

	uint16_t			num_grid_drift_overflow;
	uint16_t			num_rx_window_overflow;

	uint16_t			num_rx_success;
	uint16_t			num_rx_broken;
	uint16_t			num_rx_timeout;
	uint16_t			num_rx_dma_timeout;
	uint16_t			num_rx_dma_late;
	uint16_t			num_rx_late;
	uint16_t			num_tx_late;
	uint16_t			num_tx_zero_packet;
	uint16_t			num_tx_fifo_late;
	uint16_t			num_grid_late;

	uint16_t			num_rx_slot_mismatch;
	uint16_t			num_rx_queue_overflow;
	uint16_t			num_rx_queue_overflow_full_rank;
	uint16_t			num_rx_queue_processed;

	uint16_t			slot_full_rank;
	uint16_t			slot_decoded;
	uint16_t			slot_off;

	Gpi_Hybrid_Tick		radio_on_time;
	Gpi_Hybrid_Tick		low_power_time;

#endif

} Mixer_Stat_Counter;

//**************************************************************************************************
//***** Global Variables ***************************************************************************



//**************************************************************************************************
//***** Prototypes of Global Functions *************************************************************

#ifdef __cplusplus
	extern "C" {
#endif

void				mixer_rand_seed(uint32_t seed);

void				mixer_init(uint8_t node_id);
size_t				mixer_write(unsigned int i, const void *msg, size_t size);
void				mixer_arm(Mixer_Start_Mode mode);
Gpi_Fast_Tick_Extended mixer_start();
void*				mixer_read(unsigned int i);
int16_t				mixer_stat_slot(unsigned int i);

		// ATTENTION: the accuracy of deadline returned by mixer_start() is in the range of
		// one slot plus some margin due to clock drift (the latter depends on when the node
		// received its last packet)
		// ATTENTION: it is possible that deadline returned by mixer_start() lies in the past
		// (This can happen in two cases:
		// * If the node receives an innovative packet in the very last slot(s). If this packet
		//   leads to full rank then it triggers decoding, which can take some time.
		// * If the node gets out of sync and starts resynchronization in the very last slot(s).
		//   For implementation specific reasons, this can cause a delay of < 5 slots.
		// Obviously, the probability for that to happen is quite small if Mixer is well configured.)


#ifdef __cplusplus
	}
#endif

//**************************************************************************************************
//***** Implementations of Inline Functions ********************************************************



//**************************************************************************************************
//**************************************************************************************************

#endif // __MIXER_H__
