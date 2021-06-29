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
 *	@file					gpi/arm/nordic/nrf52840/clocks.c
 *
 *	@brief					general-purpose slow, fast, and hybrid clock
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

#include "gpi/tools.h"
#include "gpi/clocks.h"
#include "gpi/interrupts.h"
#include "gpi/olf.h"

//**************************************************************************************************
//***** Local Defines and Consts *******************************************************************

// remove int locks in hybrid clock functions
// Setting this define avoids some int locks and improves performance a bit. On the other hand,
// it makes (some of) the hybrid clock functions non-reentrant. Since this is dangerous if
// enabled without care, the option is disabled by default.
#define GPI_CLOCKS_HYBRID_FRAGILE	0

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

// Gpi_Slow_Tick_Extended gpi_tick_slow_extended()
// {
// 	ASSERT_CT(sizeof(Gpi_Slow_Tick_Extended) == sizeof(uint32_t));

// 	static struct
// 	{
// 		uint16_t	high;
// 	} s	=
// 	{0};

// 	register Generic32	o;

// 	// TODO: check how the function is used and decide if we can remove the int-lock or
// 	// provide and unlocked version (using the same static variables)
// 	// ATTENTION: int-lock makes the function reentrant; it is not without the int-lock.
// 	// Alternatively, maybe it is possible to use a marker to avoid nested updates.

// 	int ie = gpi_int_lock();

// 	// read 24-bit value directly from COUNTER
// 	// NOTE: gpi_tick_slow_native() returns 16-bit
// 	o.u32 = hlptim1.Instance->CNT;

// 	// extend format
// 	// update high part using the 8 upper bits of the 24-bit value
// 	// ATTENTION: function has to be called periodically at least once per 0xFFFFFF ticks,
// 	// otherwise it will loose ticks in high part
// 	s.high += (uint8_t)(o.u8_hl - s.high);
// 	o.u16_h = s.high;

// 	gpi_int_unlock(ie);

// 	return o.u32;
// }

Gpi_Slow_Tick_Extended gpi_tick_slow_extended()
{
	ASSERT_CT(sizeof(Gpi_Slow_Tick_Native) == sizeof(uint16_t));
	ASSERT_CT(sizeof(Gpi_Slow_Tick_Extended) == sizeof(uint32_t));

	static struct
	{
		uint16_t		high;
		uint16_t		last;
	} s	=
	{0, 0};

	register Generic32	o;

	// TODO: check how the function is used and decide if we can remove the int-lock or
	// provide and unlocked version (using the same static variables)
	// ATTENTION: int-lock makes the function reentrant; it is not without the int-lock.
	// Maybe it is possible to use a marker to avoid nested updates, e.g. the LSB of s.last.

	int ie = gpi_int_lock();

	o.u16_l = gpi_tick_slow_native();

	// extend format
	// ATTENTION: function has to be called periodically at least once per 0xFFFF ticks,
	// otherwise it will loose ticks in high part
	if (o.u16_l < s.last)
	{
		// gpi_led_toggle(GPI_LED_2);
		s.high++;
	}
	s.last = o.u16_l;

	o.u16_h = s.high;

	gpi_int_unlock(ie);

	return o.u32;
}

//*************************************************************************************************

Gpi_Fast_Tick_Extended gpi_tick_fast_extended()
{
	ASSERT_CT(sizeof(Gpi_Fast_Tick_Native) == sizeof(uint32_t));
	ASSERT_CT(sizeof(Gpi_Fast_Tick_Extended) == sizeof(uint64_t));

	static struct
	{
		uint32_t		high;
		uint32_t		last;
	} s	=
	{0, 0};

	register Generic64	o;

	// TODO: check how the function is used and decide if we can remove the int-lock or
	// provide and unlocked version (using the same static variables)
	// ATTENTION: int-lock makes the function reentrant; it is not without the int-lock.
	// Maybe it is possible to use a marker to avoid nested updates, e.g. the LSB of s.last.

	int ie = gpi_int_lock();

	o.u32_l = gpi_tick_fast_native();

	// extend format
	// ATTENTION: function has to be called periodically at least once per 0xF...F ticks,
	// otherwise it will loose ticks in high part
	// To catch such situations, we could additionally compare the slow ticks. This would
	// decrease the probability of missed overruns (significantly).
	if (o.u32_l < s.last)
		s.high++;
	s.last = o.u32_l;

	o.u32_h = s.high;

	gpi_int_unlock(ie);

	return o.u64;
}

//**************************************************************************************************

Gpi_Hybrid_Tick gpi_tick_hybrid()
{
	return gpi_tick_fast_to_hybrid(gpi_tick_fast_native());
}

//**************************************************************************************************
Gpi_Hybrid_Reference gpi_tick_hybrid_reference()
{
	ASSERT_CT(sizeof(Gpi_Slow_Tick_Native) == sizeof(uint16_t));
	ASSERT_CT(sizeof(Gpi_Fast_Tick_Native) == sizeof(uint32_t));
	ASSERT_CT(sizeof(Gpi_Slow_Tick_Extended) == sizeof(uint32_t));
	ASSERT_CT(sizeof(Gpi_Hybrid_Tick) == sizeof(uint32_t));

	register Generic32		t;
	register uint16_t		slow;
	register uint32_t		fast;
	Gpi_Hybrid_Reference	r;

	slow = hlptim1.Instance->CNT;
	fast = htim2.Instance->CNT;

	t.u32 = gpi_tick_slow_extended();
	if (t.u16_l < slow)
		t.u16_h--;
	t.u16_l = slow;

	// convert edge ticks to hybrid timebase
	{
		// conversion: hybrid_ticks = slow_ticks * hybrid_slow_ratio
		// NOTE: In general hybrid_slow_ratio is not an integer, but it holds
		// 16000000 = 15625 * 1024 = 15625 * 32768 / 32 -> ticks_16M = ticks_32k * 15625 / 32
		// with HYBRID_CLOCK_RATE = 16M / 2^x and SLOW_CLOCK_RATE = 32768:
		// hybrid_ticks = slow_ticks * 15625 / (512M / HYBRID_CLOCK_RATE)

		ASSERT_CT(32768 == GPI_SLOW_CLOCK_RATE, GPI_SLOW_CLOCK_RATE_unsupported);
		ASSERT_CT((512000000u == (512000000u / GPI_HYBRID_CLOCK_RATE) * GPI_HYBRID_CLOCK_RATE) &&
			IS_POWER_OF_2(512000000u / GPI_HYBRID_CLOCK_RATE),
			hybrid_slow_ratio_unsupported);

		t.u32 = gpi_mulu_32x16to64(t.u32, 15625) >> MSB(512000000u / GPI_HYBRID_CLOCK_RATE);
    }

	r.hybrid_tick = t.u32;
	r.fast_capture = fast;

	return r;
}

//*************************************************************************************************

Gpi_Hybrid_Tick gpi_tick_fast_to_hybrid(Gpi_Fast_Tick_Native fast_tick)
{
	register Gpi_Hybrid_Reference	r;
	register Gpi_Fast_Tick_Native	delta;

	// get last edge ticks
	// ATTENTION: this must happen <= 0xF...F fast ticks after the interesting point in time
	r = gpi_tick_hybrid_reference();

	ASSERT_CT(GPI_FAST_CLOCK_RATE / GPI_SLOW_CLOCK_RATE < 0x10000, fast_slow_ratio_to_high);
	ASSERT_CT(IS_POWER_OF_2(GPI_FAST_CLOCK_RATE / GPI_HYBRID_CLOCK_RATE), fast_hybrid_ratio_must_be_power_of_2);
	// ASSERT_CT(IS_POWER_OF_2(GPI_HYBRID_CLOCK_RATE / GPI_SLOW_CLOCK_RATE), hybrid_slow_ratio_must_be_power_of_2);

	// compute delta between edge capture and past value
	// attention: we expect that fast_tick is before edge tick in the typical case. But the edge is
	// from the (near) past, so it is also possible that fast_tick stems from a period < 1 slow
	// clock cycles after the edge. In this case, -delta < GPI_FAST_CLOCK_RATE / GPI_SLOW_CLOCK_RATE
	// (+ some tolerance if fast clock is asynchronous to slow clock, e.g. DCO vs. XO). Hence we
	// split the interpretation of delta at this value.
	delta = r.fast_capture - fast_tick;

	// compute hybrid tick:
	// if fast_tick is behind edge for sure: sub delta with respect to datatypes
	// ATTENTION: we add a safety margin that compensates for clock drift (i.e. delta may be
	// > GPI_FAST_CLOCK_RATE / GPI_SLOW_CLOCK_RATE even if fast_tick is between edge and next edge)
	if (-delta < ((GPI_FAST_CLOCK_RATE / GPI_SLOW_CLOCK_RATE) * 103) / 100)
		r.hybrid_tick += -delta / (GPI_FAST_CLOCK_RATE / GPI_HYBRID_CLOCK_RATE);

	// else sub delta in standard way
	else
		r.hybrid_tick -= delta / (GPI_FAST_CLOCK_RATE / GPI_HYBRID_CLOCK_RATE);

	return r.hybrid_tick;
}
//*************************************************************************************************

// convert timestamp from slow ticks to microseconds
uint32_t gpi_tick_slow_to_us(Gpi_Slow_Tick_Extended ticks)
{
	ASSERT_CT(sizeof(Gpi_Slow_Tick_Extended) == sizeof(uint32_t));
	ASSERT_CT(GPI_SLOW_CLOCK_RATE >= 64, GPI_SLOW_CLOCK_RATE_too_small);
	ASSERT_CT(IS_POWER_OF_2(GPI_SLOW_CLOCK_RATE), GPI_SLOW_CLOCK_RATE_must_be_power_of_2);
		// to make sure that div gets replaced by shift

	// result = ticks * 1000000 / GPI_SLOW_CLOCK_RATE
	// observe that 1000000 = 15625 * 64 ->
	// result = ticks * 15625 * 64 / GPI_SLOW_CLOCK_RATE = ticks * 15625 / (GPI_SLOW_CLOCK_RATE / 64)

	uint64_t temp;

	temp = gpi_mulu_32x16to64(ticks, 15625);
	return temp / (GPI_SLOW_CLOCK_RATE / 64);
}

//*************************************************************************************************

#if !(__OPTIMIZE__)
	// enable constant propagation to avoid errors with ASSERT_CT()
	__attribute__((optimize("Og")))
#endif
void gpi_milli_sleep(uint16_t ms)
{
	const uint16_t			TICKS_PER_MS = GPI_FAST_CLOCK_RATE / 1000u;
	Gpi_Fast_Tick_Native	tick;

	ASSERT_CT(GPI_FAST_CLOCK_RATE == TICKS_PER_MS * 1000u, GPI_FAST_CLOCK_RATE_unsupported);
	ASSERT_CT(sizeof(tick) >= sizeof(uint32_t));

	// ensure that ms * TICKS_PER_MS < INT32_MAX (signed)
	ASSERT_CT(TICKS_PER_MS < 0x8000);

	tick = gpi_tick_fast_native();

	tick += (Gpi_Fast_Tick_Native)ms * TICKS_PER_MS;

	while (gpi_tick_compare_fast_native(gpi_tick_fast_native(), tick) <= 0);
}

//*************************************************************************************************

#if !(__OPTIMIZE__)
	// enable constant propagation to avoid errors with ASSERT_CT()
	__attribute__((optimize("Og")))
#endif
void gpi_micro_sleep(uint16_t us)
{
	// ATTENTION: gpi_micro_sleep() should be fast

	const uint16_t			TICKS_PER_US = GPI_FAST_CLOCK_RATE / 1000000u;
	Gpi_Fast_Tick_Native	tick;

	ASSERT_CT(GPI_FAST_CLOCK_RATE == TICKS_PER_US * 1000000u, GPI_FAST_CLOCK_RATE_unsupported);
	ASSERT_CT(sizeof(tick) >= sizeof(uint32_t));

	// ensure that us * TICKS_PER_US < INT32_MAX (signed)
	ASSERT_CT(TICKS_PER_US < 0x8000);

	tick = gpi_tick_fast_native();

	tick += (Gpi_Fast_Tick_Native)us * TICKS_PER_US;

	while (gpi_tick_compare_fast_native(gpi_tick_fast_native(), tick) <= 0);
}

//**************************************************************************************************
//**************************************************************************************************
