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
 *	@file					gpi/clocks.h
 *
 *	@brief					general-purpose slow (lo-res), fast (hi-res), and hybrid clock
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

#ifndef __GPI_CLOCKS_H__
#define __GPI_CLOCKS_H__

//**************************************************************************************************
//***** Includes ***********************************************************************************

#include "gpi/platform_spec.h"			// GPI_PLATFORM_PATH
#include GPI_PLATFORM_PATH(clocks.h)	// platform specific functionality

//**************************************************************************************************
//***** Global (Public) Defines and Consts *********************************************************

#ifndef GPI_TICK_FAST_MAX
	#define GPI_TICK_FAST_MAX		((Gpi_Fast_Tick_Extended)(UINT64_C(0xFFFFFFFFFFFFFFFF)))
#endif

#ifndef GPI_TICK_SLOW_MAX
	#define GPI_TICK_SLOW_MAX		((Gpi_Slow_Tick_Extended)(UINT64_C(0xFFFFFFFFFFFFFFFF)))
#endif

#ifndef GPI_TICK_HYBRID_MAX
	#define GPI_TICK_HYBRID_MAX		((Gpi_Hybrid_Tick)(UINT64_C(0xFFFFFFFFFFFFFFFF)))
#endif

#define GPI_TICK_US_TO_FAST(us)		_GPI_TICK_T_TO_X(us, 1000000, FAST)
#define GPI_TICK_MS_TO_FAST(ms)		_GPI_TICK_T_TO_X(ms, 1000, FAST)
#define GPI_TICK_S_TO_FAST(s)		_GPI_TICK_T_TO_X(s, 1, FAST)
#define GPI_TICK_MS_TO_FAST2(ms)	( GPI_TICK_S_TO_FAST(  (ms) / 1000) + GPI_TICK_MS_TO_FAST((ms) % 1000) )
#define GPI_TICK_US_TO_FAST2(us)	( GPI_TICK_MS_TO_FAST2((us) / 1000) + GPI_TICK_US_TO_FAST((us) % 1000) )

#define GPI_TICK_MS_TO_SLOW(ms)		_GPI_TICK_T_TO_X(ms, 1000, SLOW)
#define GPI_TICK_S_TO_SLOW(s)		_GPI_TICK_T_TO_X(s, 1, SLOW)

#define GPI_TICK_US_TO_HYBRID(us)	_GPI_TICK_T_TO_X(us, 1000000, HYBRID)
#define GPI_TICK_MS_TO_HYBRID(ms)	_GPI_TICK_T_TO_X(ms, 1000, HYBRID)
#define GPI_TICK_S_TO_HYBRID(s)		_GPI_TICK_T_TO_X(s, 1, HYBRID)
#define GPI_TICK_MS_TO_HYBRID2(ms)	( GPI_TICK_S_TO_HYBRID(  (ms) / 1000) + GPI_TICK_MS_TO_HYBRID((ms) % 1000) )
#define GPI_TICK_US_TO_HYBRID2(us)	( GPI_TICK_MS_TO_HYBRID2((us) / 1000) + GPI_TICK_US_TO_HYBRID((us) % 1000) )

//**************************************************************************************************
//***** Local (Private) Defines and Consts *********************************************************

#define _GPI_TICK_T_TO_X(x, m, t)		(								\
	((GPI_ ## t ## _CLOCK_RATE % m) ?									\
		(((x) * GPI_ ## t ## _CLOCK_RATE) / m + ((m > 1) ? 1 : 0)) :	\
		((x) * (GPI_ ## t ## _CLOCK_RATE / m))							\
	) + ASSERT_CT_EVAL((x) <= (GPI_TICK_ ## t ## _MAX / GPI_ ## t ## _CLOCK_RATE) ||	\
		!(GPI_ ## t ## _CLOCK_RATE % m), GPI_TICK_TO_ ## t ## _overflow)	)

// helper function for timestamp comparisons
// return value indicates a in relation to b: </> 0 : a is earlier/later than b (== 0 : a equals b)
#define _GPI_TICK_COMPARE_FUNCTION(name, type)									\
	static inline __attribute__((always_inline)) int_fast8_t					\
		gpi_tick_compare_ ## name (type a, type b)								\
	{																			\
		ASSERT_CT(sizeof(a) <= 8, gpi_tick_compare_ ## name ## _overflow);		\
		a -= b;																	\
		switch (sizeof(a))														\
		{																		\
			case 2: return ((int16_t)a < 0) ? -1 : !!(a);						\
			case 4: return ((int32_t)a < 0) ? -1 : !!(a);						\
			case 8: return ((int64_t)a < 0) ? -1 : !!(a);						\
			default: return 0;	/* must not happen (catch with ASSERT_CT) */	\
		}																		\
	}

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

Gpi_Slow_Tick_Native 		gpi_tick_slow_native();
Gpi_Slow_Tick_Extended		gpi_tick_slow_extended();

// provide these functions if they would be helpful, decide later
//uint16_t					gpi_tick_slow_16();
//uint32_t					gpi_tick_slow_32();

Gpi_Fast_Tick_Native 		gpi_tick_fast_native();
Gpi_Fast_Tick_Extended		gpi_tick_fast_extended();

// provide these functions if they would be helpful (and if they are available), decide later
//uint16_t					gpi_tick_fast_16();
//uint32_t					gpi_tick_fast_32();
//uint64_t					gpi_tick_fast_64();

Gpi_Hybrid_Tick				gpi_tick_hybrid();
Gpi_Hybrid_Reference		gpi_tick_hybrid_reference();
Gpi_Hybrid_Tick				gpi_tick_fast_to_hybrid(Gpi_Fast_Tick_Native fast_tick);

// uint32_t 					gpi_tick_fast_to_us(Gpi_Fast_Tick_Extended ticks);
uint32_t 					gpi_tick_fast_to_us(Gpi_Fast_Tick_Native ticks);
uint32_t 					gpi_tick_slow_to_us(Gpi_Slow_Tick_Extended ticks);
uint32_t 					gpi_tick_hybrid_to_us(Gpi_Hybrid_Tick ticks);

void						gpi_milli_sleep(uint16_t ms);
void						gpi_micro_sleep(uint16_t us);

static inline int_fast8_t	gpi_tick_compare_slow_native(Gpi_Slow_Tick_Native a, Gpi_Slow_Tick_Native b);
static inline int_fast8_t	gpi_tick_compare_slow_extended(Gpi_Slow_Tick_Extended a, Gpi_Slow_Tick_Extended b);
static inline int_fast8_t	gpi_tick_compare_fast_native(Gpi_Fast_Tick_Native a, Gpi_Fast_Tick_Native b);
static inline int_fast8_t	gpi_tick_compare_fast_extended(Gpi_Fast_Tick_Extended a, Gpi_Fast_Tick_Extended b);
static inline int_fast8_t	gpi_tick_compare_hybrid(Gpi_Hybrid_Tick a, Gpi_Hybrid_Tick b);

#ifdef __cplusplus
	}
#endif

//**************************************************************************************************
//***** Implementations of Inline Functions ********************************************************

_GPI_TICK_COMPARE_FUNCTION(slow_native, 	Gpi_Slow_Tick_Native)
_GPI_TICK_COMPARE_FUNCTION(slow_extended, 	Gpi_Slow_Tick_Extended)
_GPI_TICK_COMPARE_FUNCTION(fast_native, 	Gpi_Fast_Tick_Native)
_GPI_TICK_COMPARE_FUNCTION(fast_extended, 	Gpi_Fast_Tick_Extended)
_GPI_TICK_COMPARE_FUNCTION(hybrid, 			Gpi_Hybrid_Tick)

//**************************************************************************************************
//**************************************************************************************************

#endif // __GPI_CLOCKS_H__
