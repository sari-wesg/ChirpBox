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
 *	@file					mixer_rand.c
 *
 *	@brief					random number generator used by Mixer
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

#include "mixer_internal.h"

#include "gpi/platform_spec.h"

//**************************************************************************************************
//***** Local Defines and Consts *******************************************************************



//**************************************************************************************************
//***** Local Typedefs and Class Declarations ******************************************************



//**************************************************************************************************
//***** Forward Declarations ***********************************************************************



//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************

static uint32_t		rand_state = 1;

//**************************************************************************************************
//***** Global Variables ***************************************************************************



//**************************************************************************************************
//***** Local Functions ****************************************************************************



//**************************************************************************************************
//***** Global Functions ***************************************************************************

void mixer_rand_seed(uint32_t seed)
{
	rand_state = seed;
}

//**************************************************************************************************

// RNG (XorShift)
uint16_t mixer_rand()
{
	#if GPI_ARCH_IS_CORE(MSP430)

		static unsigned int i = 0;

		if (!(++i & 1))
			return rand_state >> 16;

		register uint32_t	tmp;

		asm
		(
			"mov	%A0, %A1	\n"
			"mov	%B0, %B1	\n"

			//	x ^= x << 2;
			"rla	%A1			\n"
			"rlc	%B1			\n"
			"rla	%A1			\n"
			"rlc	%B1			\n"
			"xor	%A0, %A1	\n"
			"xor	%B0, %B1	\n"

			//	x ^= x >> 15;
			"mov	%A1, %B2	\n"
			"mov	%B1, %A2	\n"
			"rla	%B2			\n"
			"clr	%B2			\n"
			"rlc	%A2			\n"
			"rlc	%B2			\n"
			"xor	%A2, %A1	\n"
			"xor	%B2, %B1	\n"

			//	x ^= x << 17;
			"mov	%A1, %B2	\n"
			"rla	%B2			\n"
			"xor	%B2, %B1	\n"

			"mov	%A1, %A0	\n"
			"mov	%B1, %B0	\n"

			: "+m"(rand_state), "=&r"(tmp), "=&r"(tmp)
			:
			: "cc"
		);

	#else

		#if GPI_ARCH_IS_CORE(ARM)
			// ok (efficiency checked)
		#else

			#warning mixer_rand() uses generic implementation -> may be slow

			static unsigned int i = 0;

			if (!(++i & 1))
				return rand_state >> 16;

		#endif

		rand_state ^= rand_state << 2;
		rand_state ^= rand_state >> 15;
		rand_state ^= rand_state << 17;

	#endif

	return rand_state;
}

//**************************************************************************************************
//**************************************************************************************************
