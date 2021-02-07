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
 *	@file					mixer/armv7-m/memxor.h
 *
 *	@brief					memxor implementation, optimized for ARMv7-M
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

#ifndef __MEMXOR_ARMv7M_H__
#define __MEMXOR_ARMv7M_H__

//**************************************************************************************************
//***** Includes ***********************************************************************************
#include "gpi/tools.h"

#include <assert.h>
#include <stdint.h>								// uintptr_t

#include "gpi/platform_spec.h"					// GPI_PLATFORM_PATH()
// #include GPI_PLATFORM_PATH(cmsis_device.h)		// __DMB(), __ISB()
#include "stm32l476xx.h"

//**************************************************************************************************
//***** Global (Public) Defines and Consts *********************************************************

#define MEMXOR_BLOCKSIZE	8

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

static inline void	memxor(void *dest, const void *src, unsigned int size);
static inline void	memxor_block(void *dest, /*const*/ void *src[], unsigned int size, int num_src);

// internal forward declarations
void memxor_block_core(void *dest, /*const*/ void *src[], unsigned int size);
void memxor_block_straight(void *dest, /*const*/ void *src[], unsigned int size, int num_src);

#ifdef __cplusplus
	}
#endif

//**************************************************************************************************
//***** Implementations of Inline Functions ********************************************************

// ATTENTION: dest, src and size have to be aligned on word size
// NOTE: inlining is important to resolve as much tests as possible at compile time
static inline __attribute__((always_inline)) void memxor(void *dest, const void *src, unsigned int size)
{
	if (0 == size)
		return;

	const unsigned int	*s = (const unsigned int*)src;
	unsigned int		*d = (unsigned int*)dest;

	// align size and translate it to number of words
	size = (size + sizeof(int) - 1) / sizeof(int);

	for (; size != 0; --size)
		*d++ ^= *s++;
}

//**************************************************************************************************

// ATTENTION: dest, src and size have to be aligned on word size
// NOTE: inlining is important to resolve as much tests as possible at compile time
static inline __attribute__((always_inline)) void memxor_block(void *dest, /*const*/ void *src[], unsigned int size, int num_src)
{
	unsigned int	loop_start;
	union {
		uint16_t	*h;
		uint32_t	*w;
    }				ins;

	assert_reset(num_src <= MEMXOR_BLOCKSIZE);

	if ((0 == num_src) || (0 == size))
		return;

	// if size - i.e. number of needed loop iterations - is small, the overhead for adapting
	// the machine code exceeds the savings. Therefore we use a more straight-forward version
	// in these situations which is slower per iteration but comes with appropriate less overhead.
	if ((size <= 8) && (num_src < MEMXOR_BLOCKSIZE))
	{
		memxor_block_straight(dest, src, size, num_src);
		return;
    }

	// adapt machine code to num_src
	if (num_src < MEMXOR_BLOCKSIZE)
	{
		// ATTENTION: &memxor_block_core LSB marks ARM Thumb mode,
		// so it must be cleared to get the real address
		ins.w = (uint32_t*)((uintptr_t)&memxor_block_core & ~1u);

		// (don't) update number of saved and initialized pointer registers (push/pop and ldm)
		// NOTE: The influence of that step is limited because the changed instructions are
		// outside of the core loop. Adapting the instructions saves some clock cycles (depending
		// on num_src), but it needs extra cycles to do the adaptations. So overall, depending on
		// num_src the savings are small or even negative, and we safe the effort.
		// uint16_t reglist = (0x0020 << num_src) - 0x0010;
		// ins.h[1] = reglist;
		// ins.h[45] = reglist;
		// ins.h[3] = reglist & ~0x0010;

		// calculate loop entry point and move load instruction to the right place
		loop_start = 18 - ((num_src + 1) / 2) * 4;
		if (num_src & 1)
		{
			loop_start += 2;
			ins.w[loop_start + 1] = ins.w[2];
        }
		else
			ins.w[loop_start] = ins.w[2];

		// inject branch to loop entry point
		ins.h[4] = 0xe000 | ((loop_start - 3) * 2);

		// shorten loop
		ins.h[43] = 0xa800 | (((loop_start - 22) * 2) & 0x07ff);

		// flush memory and pipeline
		// NOTE: This is not really needed because:
		// a) The distance of the first changed instruction is greater than the pipeline length.
		// b) The following branch (function call) seems to cause an implicit flush (The ARM
		//    Cortex-M4 technical reference manual (ARM 100166_0001_00_en) states that an
		//    unconditional branch takes 1 + P cycles).
		// However, we do it to be absolutely safe (and as a good practice).
		__DMB();
		__ISB();
	}

	// call core loop
	memxor_block_core(dest, src, size);

	// restore original machine code
	if (num_src < MEMXOR_BLOCKSIZE)
	{
		// (don't) restore number of saved and initialized pointer registers (push/pop and ldm)
		// for the reason see comment above
		// ins.h[1] = 0x1ff0;
		// ins.h[45] = 0x1ff0;
		// ins.h[3] = 0x1fe0;

		if (num_src & 1)
			ins.w[loop_start + 1] = 0x0104ea81;
		else
			ins.w[loop_start] = 0x0103ea81;

		ins.h[4] = 0xf8d0;
		ins.h[43] = 0xafd8;

		// flush memory and pipeline
		// NOTE: It is extremely unlikely that this is needed (the only potential case is an
		// immediately following call to memxor_block with num_src == MEMXOR_BLOCK_SIZE).
		// However, we do it to be absolutely safe (and as a good practice).
		__DMB();
		__ISB();
	}
}

//**************************************************************************************************
//**************************************************************************************************

#endif // __MEMXOR_ARMv7M_H__
