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
 *	@file					mixer/armv7-m/memxor.c
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
//***** Trace Settings *****************************************************************************
#if 0
#include "gpi/trace.h"

// message groups for TRACE messages (used in GPI_TRACE_MSG() calls)
//#define TRACE_GROUP1		0x00000001
//#define TRACE_GROUP2		0x00000002

// select active message groups, i.e., the messages to be printed (others will be dropped)
#ifndef GPI_TRACE_BASE_SELECTION
	#define GPI_TRACE_BASE_SELECTION	GPI_TRACE_LOG_STANDARD | GPI_TRACE_LOG_PROGRAM_FLOW
#endif
GPI_TRACE_CONFIG(memxor, GPI_TRACE_BASE_SELECTION | GPI_TRACE_LOG_USER);
#endif
//**************************************************************************************************
//**** Includes ************************************************************************************

#include "memxor.h"

#include "gpi/tools.h"		// ASSERT_CT

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

// optimized memxor_block() implementation
// ATTENTION: code is dynamically modified at runtime (i.e., memxor_block() altogether uses
// self-modifying code), so it is important to have it in data section
// NOTE: __attribute__((section("<name>"))) gets translated to .section <name>,<attributes>.
// With <name> = .data this causes a warning message because changing .data's attributes is
// not allowed. Since we don't want to do that anyway, we absorb the attribute specification
// into a comment by including a comment marker into <name>.
void __attribute__((naked, section(".data @"))) memxor_block_core(void *dest, /*const*/ void *src[], unsigned int size)
{
	// relevant documents:
	// [1] ARM Cortex-M4 User Guide (ARM DUI 0553A ID121610)
	// [2] ARM Cortex-M4 Technical Reference Manual (ARM 100166_0001_00_en)
	// [3] ARM v7-M Architecture Reference Manual (ARM DDI 0403E.b ID120114)

	// Let a = dest. The inner loop core is ldr b; eor a, b. Implementing that straight-forward
	// would lead to an execution time of 3 clock cycles per operand (+ the outer loop).
	// The load/store instructions have limited pipelining capabilities, details can be found in
	// [3] section 3.3.3. In particular, it is possible to save some cycles by concatenating
	// ldr instructions. Therefore we interleave consecutive iterations to pack memory accesses
	// and gain some performance. Interleaving two operands (ldm, ldm, eor, eor) gives 5 cycles
	// per iteration, that is 2.5 cycles per operand. Packing more operands would increase the
	// gain (converging towards 2 cycles per operand). However, interleaving operands needs
	// additional scratch registers and therefore reduces possible MEMXOR_BLOCKSIZE. For the same
	// amount of input data this leads to a higher number of blocks and (outer) loop iterations.
	// Hence, there is a sweet spot:
	// Without interleaving, we have space for 9 pointers (r4-r12). Assuming the cost for the
	// outer loop = 6 cycles (approximately, it may differ a bit since some pipelining details
	// are missing in the documentation), we need 9 * 3 + 6 = 33 cycles for one iteration.
	// Interleaving 2 operands reduces the number of available pointer registers to 8 and we get
	// 4(=8/2) * 5 + 6 = 26 cycles. Taking the increased number of iterations into account, we end
	// up with 26 * 9 / 8 = 29,25 cycles as a fair comparision value. This scheme continues as
	// follows:
	//	#pointers	packing			#cycles
	//	9			9x1				33		(9*3 + 6)
	//	8			4x2				29,25	((4*5 + 6) * 9/8)
	//  7			2x3 + 1x1		29,57	((2*7 + 1*3 + 6) * 9/7)
	//  6			1x4 + 1x2		30		((1*9 + 1*5 + 6) * 9/6)
	//  6			2x3				30		((2*7 + 6) * 9/6)
	//	5			1x5				30,6	((1*11 + 6) * 9/5)
	// => Using 8 pointers with pairwise interleaving appears as the fastest variant.

	__asm__ volatile
	(
		// r0 = dest
		// r1 = src / scratch
		// r2 = size
		// r3 = scratch
		// r4 = scratch
		// r5...r12 = source pointers

		"push.w		{r4-r12}			\n"		// e92d 1ff0	[0]
		"ldm.w		r1, {r5-r12}		\n"		// e891 1fe0
		"1:								\n"
		"ldr.w		r1, [r0]			\n"		// f8d0 1000	[2]
		"ldr.w		r4, [r12], #4		\n"		// f85c 4b04	[3]
		"ldr.w		r3, [r11], #4		\n"		// f85b 3b04
		"eor.w		r1, r4				\n"		// ea81 0104
		"eor.w		r1, r3				\n"		// ea81 0103
		"ldr.w		r4, [r10], #4		\n"		// f85a 4b04	[7]
		"ldr.w		r3, [r9], #4		\n"		// f859 3b04
		"eor.w		r1, r4				\n"		// ea81 0104
		"eor.w		r1, r3				\n"		// ea81 0103
		"ldr.w		r4, [r8], #4		\n"		// f858 4b04	[11]
		"ldr.w		r3, [r7], #4		\n"		// f857 3b04
		"eor.w		r1, r4				\n"		// ea81 0104
		"eor.w		r1, r3				\n"		// ea81 0103
		"ldr.w		r4, [r6], #4		\n"		// f856 4b04	[15]
		"ldr.w		r3, [r5], #4		\n"		// f855 3b04
		"eor.w		r1, r4				\n"		// ea81 0104
		"eor.w		r1, r3				\n"		// ea81 0103
		"str.w		r1, [r0], #4		\n"		// f840 1b04	[19]
		"subs.w		r2, #4				\n"		// f1b2 0204
		"bgt.w		1b					\n"		// f73f afd8	[21]
		"pop.w		{r4-r12}			\n"		// e8bd 1ff0	[22]
		"bx			lr					\n"		// 4770
	);
}

ASSERT_CT_STATIC(MEMXOR_BLOCKSIZE == 8, inconsistent_code);

//**************************************************************************************************

// straight version for cases where size is small
void memxor_block_straight(void *dest, /*const*/ void *src[], unsigned int size, int num_src)
{
	register int tmp1, tmp2, tmp3;

	__asm__ volatile
	(
		"1:								\n"
		"ldr		%0, [%[d], %[i]]	\n"
		"mov		%1, %[s]			\n"
		".align 2						\n"
		"add.n		pc, %[n]			\n"		// jump into unrolled loop
		"nop							\n"
		"ldr.w		%2, [%1], #4		\n"
		"ldr.w		%2, [%2, %[i]]		\n"
		"eor.w		%0, %2				\n"
		"ldr.w		%2, [%1], #4		\n"
		"ldr.w		%2, [%2, %[i]]		\n"
		"eor.w		%0, %2				\n"
		"ldr.w		%2, [%1], #4		\n"
		"ldr.w		%2, [%2, %[i]]		\n"
		"eor.w		%0, %2				\n"
		"ldr.w		%2, [%1], #4		\n"
		"ldr.w		%2, [%2, %[i]]		\n"
		"eor.w		%0, %2				\n"
		"ldr.w		%2, [%1], #4		\n"
		"ldr.w		%2, [%2, %[i]]		\n"
		"eor.w		%0, %2				\n"
		"ldr.w		%2, [%1], #4		\n"
		"ldr.w		%2, [%2, %[i]]		\n"
		"eor.w		%0, %2				\n"
		"ldr.w		%2, [%1], #4		\n"
		"ldr.w		%2, [%2, %[i]]		\n"
		"eor.w		%0, %2				\n"
		"ldr.w		%2, [%1], #4		\n"
		"ldr.w		%2, [%2, %[i]]		\n"
		"eor.w		%0, %2				\n"
		""
		"str		%0, [%[d], %[i]]	\n"
		"subs		%[i], #4			\n"
		"bge		1b					\n"
		: "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3), [d] "+&r"(dest)
		: [i] "r"((size - 1) & ~3u), [s] "r"(src), [n] "r"(96 - 12 * num_src)
		: "cc", "memory"
	);

	ASSERT_CT(MEMXOR_BLOCKSIZE == 8, inconsistent_code);
}

//**************************************************************************************************
//**************************************************************************************************
#if 0
void memxor_test()
{
	uint8_t		dest[8];
	uint8_t		src[8];
	int			i;

	for (i = 0; i < NUM_ELEMENTS(src); ++i)
		src[i] = i + 1;

	for (i = 0; i < NUM_ELEMENTS(dest); ++i)
		dest[i] = (i + 1) << 4;

	GPI_TRACE_MSG(1, "test full block size...");
	memxor(dest, src, sizeof(dest));
//	TRACE_DUMP(1, "result:", dest, sizeof(dest));

	GPI_TRACE_MSG(1, "test half block size...");
	memxor(dest, src, sizeof(dest) / 2);
//	TRACE_DUMP(1, "result:", dest, sizeof(dest));

	GPI_TRACE_MSG(1, "done");
}
#endif
//**************************************************************************************************
#if 0
void memxor_block_test()
{
	uint8_t		dest[16];
	uint8_t		src[MEMXOR_BLOCKSIZE][sizeof(dest)];
	void*		pp[MEMXOR_BLOCKSIZE];
	int			i, k;

	for (i = 0; i < NUM_ELEMENTS(pp); ++i)
		pp[i] = &src[i][0];

	for (i = 0; i < sizeof(src); ++i)
		((uint8_t*)src)[i] = i;

	mx_trace_dump(1, "src:", src, sizeof(src));

	for (k = 1; k <= MEMXOR_BLOCKSIZE; ++k)
	{
		for (i = 0; i < NUM_ELEMENTS(dest); ++i)
			dest[i] = i << 4;

		GPI_TRACE_MSG(1, "test full block size with %u sources ...", k);
		memxor_block(dest, pp, sizeof(dest), k);
		mx_trace_dump(1, "result:", dest, sizeof(dest));

		for (i = 0; i < NUM_ELEMENTS(dest); ++i)
		{
			uint8_t x = i << 4;

			for (int l = 0; l < k; ++l)
				x ^= src[l][i];

			if (x != dest[i])
				GPI_TRACE_MSG(1, "offset %u: %u != %u", i, dest[i], x);
			else GPI_TRACE_MSG(1, "result ok");
        }

		for (i = 0; i < NUM_ELEMENTS(dest); ++i)
			dest[i] = i << 4;

		GPI_TRACE_MSG(1, "test half block size with %u sources ...", k);
		memxor_block(dest, pp, sizeof(dest) / 2, k);
		mx_trace_dump(1, "result:", dest, sizeof(dest));
	}

	GPI_TRACE_MSG(1, "done");
}
#endif
//**************************************************************************************************
//**************************************************************************************************
