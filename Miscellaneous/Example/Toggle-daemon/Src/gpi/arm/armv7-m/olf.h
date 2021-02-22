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
 *	@file					gpi/arm/armv7-m/olf.h
 *
 *	@brief					optimized low-level functions, tuned for ARMv7-M
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

#ifndef __GPI_ARMv7M_OLF_H__
#define __GPI_ARMv7M_OLF_H__

//**************************************************************************************************
//***** Includes ***********************************************************************************

#include "gpi/tools.h"
#include "gpi/olf_internal.h"

#include <stdint.h>

// this file is for ARMv7-M (don't use it for ARMv6-M without checking the mul and div capabilities)
#if !GPI_ARCH_IS_CORE(ARMv7M)
	#error unsupported architecture
#endif

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

extern const uint8_t	gpi_popcnt_lut[256];

//**************************************************************************************************
//***** Prototypes of Global Functions *************************************************************

#ifdef __cplusplus
	extern "C" {
#endif



#ifdef __cplusplus
	}
#endif

//**************************************************************************************************
//***** Implementations of Inline Functions ********************************************************

// shift left unsigned 32 bit
static ALWAYS_INLINE uint32_t gpi_slu_32(uint32_t x, unsigned int s)
{
	return x << s;
}

// shift left unsigned 16 bit
static ALWAYS_INLINE uint16_t gpi_slu_16(uint16_t x, unsigned int s)
{
	return x << s;
}

// shift left unsigned 8 bit
static ALWAYS_INLINE uint8_t gpi_slu_8(uint8_t x, unsigned int s)
{
	return x << s;
}

// shift left unsigned generic
#define gpi_slu(x, s)	((x) << (s))

//**************************************************************************************************
//*************************************************************************************************

static ALWAYS_INLINE uint32_t gpi_mulu_16x16(uint16_t a, uint16_t b)
{
	return (uint32_t)a * (uint32_t)b;
}

//*************************************************************************************************

static ALWAYS_INLINE uint32_t gpi_mulu_32x32(uint32_t a, uint32_t b)
{
	return a * b;
}

//*************************************************************************************************

static ALWAYS_INLINE uint32_t gpi_mulu_32x16(uint32_t a, uint16_t b)
{
	return a * (uint32_t)b;
}

//**************************************************************************************************

static ALWAYS_INLINE uint64_t gpi_mulu_32x32to64(uint32_t a, uint32_t b)
{
	register union
	{
		uint64_t		u64;
		struct
		{
			uint32_t	u32l;
			uint32_t	u32h;
        };

	} result;

	asm
	(
		"umull %0, %1, %2, %3"
		: "=r"(result.u32l), "=r"(result.u32h)
		: "r"(a), "r"(b)
	);

	return result.u64;
}

//**************************************************************************************************

static ALWAYS_INLINE uint64_t gpi_mulu_32x16to64(uint32_t a, uint16_t b)
{
	return gpi_mulu_32x32to64(a, b);
}

//**************************************************************************************************

#define gpi_mulu(a, b)		((uint32_t)(a) * (b))

//**************************************************************************************************
//**************************************************************************************************

static ALWAYS_INLINE int_fast8_t gpi_get_lsb_32_core(uint32_t x, const int test_zero, const int_fast8_t return_if_zero)
{
	register int	y;

	// handle input value 0 only if necessary
	// NOTE: test_zero gets resolved with constant propagation. We use it to explicitly avoid
	// unnecessary tests. The compiler cannot always determine such situations because it
	// has no semantics when inline asm is used.
	if (test_zero)
	{
		// NOTE: sub %0, %1, %2 (third line) is equivalent to mov %0, -%2
		// implementing it this way allows arbitrary negative values for return_if_zero
		// (remember the limited possibilities for immediate constants at operand2)
		asm
		(
			"cmp	%1, #0		\n"
			"itee	eq			\n"
			"subeq	%0, %1, %2	\n"
			"rbitne	%0, %1		\n"
			"clzne	%0, %0		\n"
			: "=r"(y)
			: "r"(x), "i"((int_fast8_t)-return_if_zero)
			: "cc"
		);
    }
	else
	{
		asm
		(
			"rbit	%0, %1		\n"
			"clz	%0, %0		\n"
			: "=r"(y)
			: "r"(x)
		);
    }

	return y;
}

//**************************************************************************************************

static ALWAYS_INLINE int_fast8_t gpi_get_lsb_32(uint32_t x)
{
	return gpi_get_lsb_32_core(x, 1, -1);
}

//**************************************************************************************************

static ALWAYS_INLINE int_fast8_t gpi_get_lsb_64(uint64_t x)
{
	register Generic64	y;

	y.u64 = x;

	if (y.u32_l == 0)
		return 32 + gpi_get_lsb_32_core(y.u32_h, 1, -33);
	else return gpi_get_lsb_32_core(y.u32_l, 0, -1);
}

//**************************************************************************************************

#define gpi_get_lsb(x)	(										\
	gpi_get_lsb_32((x) & (-1u >> ((4 - sizeof(x)) * 8))) +		\
	ASSERT_CT_EVAL(sizeof(x) <= 4, gpi_get_lsb_param_size_exceeded))

//**************************************************************************************************
//**************************************************************************************************

static ALWAYS_INLINE int_fast8_t gpi_get_msb_32(uint32_t x)
{
	register int	y;

	asm
	(
		"clz	%0, %1		\n"
		"rsb	%0, #31		\n"
		: "=r"(y)
		: "r"(x)
	);

	return y;
}

//**************************************************************************************************

static ALWAYS_INLINE int_fast8_t gpi_get_msb_64(uint64_t x)
{
	register Generic64	y;

	y.u64 = x;

	if (y.u32_h != 0)
		return 32 + gpi_get_msb_32(y.u32_h);
	else return gpi_get_msb_32(y.u32_l);

	// code generated by the following variant is less efficient
	// return (y.u32_h != 0 ? 32 : 0) + gpi_get_msb_32(y.u32_h != 0 ? y.u32_h : y.u32_l);
}

//**************************************************************************************************

#define gpi_get_msb(x)	(										\
	gpi_get_msb_32((x) & (-1u >> ((4 - sizeof(x)) * 8))) +		\
	ASSERT_CT_EVAL(sizeof(x) <= 4, gpi_get_msb_param_size_exceeded))

//**************************************************************************************************
//**************************************************************************************************

static ALWAYS_INLINE uint_fast8_t gpi_popcnt_8(uint8_t x)
{
	return gpi_popcnt_lut[x];
}

static ALWAYS_INLINE uint_fast8_t gpi_popcnt_16(uint16_t x)
{
	return gpi_popcnt_8(x & 0xFF) + gpi_popcnt_8(x >> 8);
}

static ALWAYS_INLINE uint_fast8_t gpi_popcnt_32(uint32_t x)
{
	return gpi_popcnt_16(x & 0xFFFF) + gpi_popcnt_16(x >> 16);
}

_GPI_SIZE_DISPATCHER_FUNCTION_1_32(gpi_popcnt, uint_fast8_t)

#define gpi_popcnt(x)	 _GPI_SIZE_DISPATCHER_1_32(gpi_popcnt, x)

//**************************************************************************************************
//**************************************************************************************************

static ALWAYS_INLINE uint32_t gpi_bitswap_32(uint32_t x)
{
	asm("rbit %0, %0" : "+r"(x));
	return x;
}

static ALWAYS_INLINE uint8_t gpi_bitswap_8(uint8_t x)
{
	return gpi_bitswap_32(x) >> 24;
}

static ALWAYS_INLINE uint16_t gpi_bitswap_16(uint16_t x)
{
	return gpi_bitswap_32(x) >> 16;
}

_GPI_SIZE_DISPATCHER_FUNCTION_1_32(gpi_bitswap, uint32_t)

#define gpi_bitswap(x)	 _GPI_SIZE_DISPATCHER_1_32(gpi_bitswap, x)

//**************************************************************************************************
//**************************************************************************************************

static ALWAYS_INLINE uint16_t gpi_divu_16x8(uint16_t x, uint8_t d, int accurate)
{
	register uint32_t	r;

	asm("udiv %0, %1, %2" : "=r"(r) : "r"(x), "r"(d));

	return r;
}

//**************************************************************************************************
//**************************************************************************************************

#endif // __GPI_ARMv7M_OLF_H__
