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
 *	@file					nrf52840/mixer_internal.c
 *
 *	@brief					internal helper functions optimized for Nordic nRF52840
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



//**************************************************************************************************
//**** Includes ************************************************************************************

#include "../mixer_internal.h"

#include "gpi/olf.h"

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



//**************************************************************************************************
//***** Global Functions ***************************************************************************

int_fast16_t mx_get_leading_index(const uint8_t *pcv)
{
	const uint32_t	*p = (const uint32_t*)pcv;
	int_fast16_t	i;

	for (i = 0; i < loradisc_config.coding_vector.len * 32; i += 32, p++)
	{
		if (*p)
		{
			i += gpi_get_lsb(*p);

			// ATTENTION: unused coding vector bits may be non-zero
			return (i < loradisc_config.mx_generation_size) ? i : -1;
        }
	}

	return -1;
}

void unwrap_chunk(uint8_t *p)
{
	/* double-check alignment of packet fields */
	// ASSERT_CT(!(offsetof(Packet, coding_vector) % sizeof(uint_fast_t)), inconsistent_alignment);
	assert_reset(!((offsetof(Packet, packet_chunk) - offsetof(Packet, phy_payload_begin)) % sizeof(uint_fast_t)));
// 	ASSERT_CT(
// 		offsetof(Packet, payload) ==
// 			offsetof(Packet, coding_vector) +
// 			sizeof_member(Packet, coding_vector),
// 		inconsistent_alignment);
	assert_reset((
		loradisc_config.payload.pos ==
		loradisc_config.coding_vector.pos +
		loradisc_config.coding_vector.len));

	/* double-check alignment of matrix row fields */
	// ASSERT_CT(!(offsetof(Matrix_Row, coding_vector) % sizeof(uint_fast_t)),inconsistent_alignment);
	// ASSERT_CT(!(offsetof(Matrix_Row, matrix_chunk) % sizeof(uint_fast_t)),inconsistent_alignment);
	// ASSERT_CT(!(offsetof(Matrix_Row, payload) % sizeof(uint_fast_t)),inconsistent_alignment);
	assert_reset(!((loradisc_config.matrix_payload.pos * sizeof(uint_fast_t)) % sizeof(uint_fast_t)));
	// ASSERT_CT(
	// 	offsetof(Matrix_Row, coding_vector_8) == offsetof(Matrix_Row, coding_vector),
	// 	inconsisten_alignment);
	ASSERT_CT(offsetof(Matrix_Row, matrix_chunk_8) == offsetof(Matrix_Row, matrix_chunk),
		inconsistent_alignment);
	// ASSERT_CT(
	// 	offsetof(Matrix_Row, payload_8) ==
	// 		offsetof(Matrix_Row, coding_vector_8) + sizeof_member(Matrix_Row, coding_vector_8),
	// 	inconsisten_alignment);
	assert_reset((loradisc_config.matrix_payload_8.pos == loradisc_config.matrix_coding_vector_8.pos + loradisc_config.matrix_coding_vector_8.len));

	/* NOTE: condition gets resolved at compile time */
	if (offsetof(Matrix_Row, matrix_chunk_8) + loradisc_config.matrix_payload_8.pos != offsetof(Matrix_Row, matrix_chunk) + (loradisc_config.matrix_payload.pos) * sizeof(uint_fast_t))
	{
		/* #pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Warray-bounds" */

		uint8_t			*s = p + loradisc_config.matrix_coding_vector_8.len;
		uint8_t			*d = s + loradisc_config.matrix_payload_8.len;
		unsigned int	i;

		for (i = (offsetof(Matrix_Row, matrix_chunk) + (loradisc_config.matrix_payload.pos) * sizeof(uint_fast_t)) - (offsetof(Matrix_Row, matrix_chunk_8) + loradisc_config.matrix_payload_8.pos); i-- > 0;)
			*d++ = *s++;

		/* #pragma GCC diagnostic pop */
    }
}

//**************************************************************************************************

void unwrap_row(unsigned int i)
{
	unwrap_chunk(&(mx.matrix[i]->matrix_chunk_8[loradisc_config.matrix_coding_vector_8.pos]));
}

//**************************************************************************************************

void wrap_chunk(uint8_t *p)
{
	// NOTE: condition gets resolved at compile time
	if (offsetof(Matrix_Row, matrix_chunk_8) + loradisc_config.matrix_payload_8.pos != offsetof(Matrix_Row, matrix_chunk) + (loradisc_config.matrix_payload.pos) * sizeof(uint_fast_t))
	{
//		#pragma GCC diagnostic push
//		#pragma GCC diagnostic ignored "-Warray-bounds"

		uint8_t			*d = p + loradisc_config.matrix_coding_vector_8.len;
		uint8_t			*s = d + loradisc_config.matrix_payload_8.len;
		unsigned int	i;

		for (i = (offsetof(Matrix_Row, matrix_chunk) + (loradisc_config.matrix_payload.pos) * sizeof(uint_fast_t)) - (offsetof(Matrix_Row, matrix_chunk_8) + loradisc_config.matrix_payload_8.pos); i-- > 0;)
			*d++ = *s++;

//		#pragma GCC diagnostic pop
    }
}

//**************************************************************************************************
//**************************************************************************************************
