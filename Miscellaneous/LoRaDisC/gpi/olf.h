/***************************************************************************************************
 ***************************************************************************************************
 *
 *	Copyright (c) 2018, Networked Embedded Systems Lab, TU Dresden
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
 *	@file					gpi/olf.h
 *
 *	@brief					optimized low-level functions
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

#ifndef __GPI_OLF_H__
#define __GPI_OLF_H__

//**************************************************************************************************
//***** Includes ***********************************************************************************

// include platform specific functionality
// note: it would be nice to include that after the declarations below, but that would prevent
// the platform specific code from selectively implementing functions as (static) inline
#include "gpi/platform_spec.h"
#include GPI_PLATFORM_PATH(olf.h)

#include <stdint.h>

//**************************************************************************************************
//***** Global (Public) Defines and Consts *********************************************************



//**************************************************************************************************
//***** Local (Private) Defines and Consts *********************************************************



//**************************************************************************************************
//***** Forward Class and Struct Declarations ******************************************************



//**************************************************************************************************
//***** Global Typedefs and Class Declarations *****************************************************



//**************************************************************************************************
//***** Global Variables ***************************************************************************



//**************************************************************************************************
//***** Prototypes of Global Functions *************************************************************

#ifdef __cplusplus
	extern "C" {
#endif

uint8_t			gpi_slu_8(uint8_t x, unsigned int s);
uint16_t		gpi_slu_16(uint16_t x, unsigned int s);
uint32_t		gpi_slu_32(uint32_t x, unsigned int s);
// uintX_t		gpi_slu(uintX_t x, unsigned int s);			// defined as a macro

uint32_t 		gpi_mulu_16x16(uint16_t a, uint16_t b);
uint32_t 		gpi_mulu_32x16(uint32_t a, uint16_t b);
// uintX_t		gpi_mulu(uintX_t a, uintX_t b);				// defined as a macro

uint16_t		gpi_divu_16x8(uint16_t x, uint8_t d, int accurate);

int_fast8_t		gpi_get_msb_8(uint8_t x);
int_fast8_t 	gpi_get_msb_16(uint16_t x);
int_fast8_t 	gpi_get_msb_32(uint32_t x);
// int_fast8_t	gpi_get_msb(uintX_t x);			// defined as macro

int_fast8_t		gpi_get_lsb_8(uint8_t x);
int_fast8_t 	gpi_get_lsb_16(uint16_t x);
int_fast8_t 	gpi_get_lsb_32(uint32_t x);
// int_fast8_t	gpi_get_lsb(uintX_t x);			// defined as macro

uint_fast8_t	gpi_popcnt_8(uint8_t x);
uint_fast8_t 	gpi_popcnt_16(uint16_t x);
uint_fast8_t 	gpi_popcnt_32(uint32_t x);
// uint_fast8_t	gpi_popcnt(uintX_t x);			// defined as macro

uint8_t			gpi_bitswap_8(uint8_t x);
uint16_t	 	gpi_bitswap_16(uint16_t x);
uint32_t 		gpi_bitswap_32(uint32_t x);
// uintX_t		gpi_bitswap(uintX_t x);			// defined as macro

void			gpi_memcpy_8(			void *dest, const void *src, size_t size);
void			gpi_memcpy_dma(			void *dest, const void *src, size_t size);
void 			gpi_memcpy_dma_aligned(	void *dest, const void *src, size_t size);
void 			gpi_memcpy_dma_inline(	void *dest, const void *src, size_t size);
void			gpi_memmove_dma(		void *dest, const void *src, size_t size);
void			gpi_memmove_dma_inline(	void *dest, const void *src, size_t size);

#ifdef __cplusplus
	}
#endif

//**************************************************************************************************
//***** Implementations of Inline Functions ********************************************************



//**************************************************************************************************
//**************************************************************************************************

#endif // __GPI_OLF_H__
