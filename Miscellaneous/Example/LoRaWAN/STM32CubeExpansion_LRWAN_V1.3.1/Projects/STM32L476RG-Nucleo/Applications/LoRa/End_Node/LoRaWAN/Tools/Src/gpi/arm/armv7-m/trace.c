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
 *	@file					gpi/arm/armv7-m/trace.c
 *
 *	@brief					generic ARMv7-M TRACE implementation
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

#include "gpi/trace.h"

#if (GPI_TRACE_MODE & GPI_TRACE_MODE_TRACE)

//**************************************************************************************************
//**** Includes ************************************************************************************

#include "gpi/tools.h"
#include "gpi/clocks.h"
#include "gpi/interrupts.h"
#include "gpi/olf.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <inttypes.h>

//**************************************************************************************************
//***** Local Defines and Consts *******************************************************************



//**************************************************************************************************
//***** Local Typedefs and Class Declarations ******************************************************



//**************************************************************************************************
//***** Forward Declarations ***********************************************************************



//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************

static Gpi_Trace_Msg			s_msg_queue[GPI_TRACE_BUFFER_ELEMENTS];
static volatile unsigned int	s_msg_queue_num_written = 0;
static volatile unsigned int	s_msg_queue_num_writing = 0;
static volatile unsigned int	s_msg_queue_num_read = 0;

//**************************************************************************************************
//***** Global Variables ***************************************************************************

extern char	_estack [1];

//**************************************************************************************************
//***** Local Functions ****************************************************************************

#if GPI_TRACE_USE_DSR

void GPI_TRACE_DSR_VECTOR()
{
	// ATTENTION: DSR has to be called < 0x10...0 fast ticks after gpi_trace_store_msg()
	// (such that gpi_tick_fast_to_hybrid() works right)
	// NOTE: we assume that nested interrupts are allowed based on priorities,
	// but TRACE DSR does not get nested with itself

	static unsigned int	s_num_read = 0;
	unsigned int		num_written;

	do
	{
		Gpi_Trace_Msg	*msg, *msg_end;
		unsigned int	num_read;

		// NOTE: we use our own num_read value and expect to be always in front of s_msg_queue_num_read
		num_written = s_msg_queue_num_written;
		num_read = s_num_read;

#if !GPI_TRACE_OVERFLOW_ON_WRITE
		if (num_written - num_read > NUM_ELEMENTS(s_msg_queue))
			num_read = num_written - NUM_ELEMENTS(s_msg_queue);
		// ATTENTION: to be absolutely safe that we do not end up with corrupt timestamps if
		// overflows occur, we should do overflow handling in this routine the full, clean way
		// (i.e. with locked write access and checking num_written before). However, we forgo this
		// topic in favor of performance because we expect that TRACE overflows are seen as
		// critical issues in typical applications and hence should be avoided completely by
		// choosing a big enough queue size.
#endif

		// avoid msg_end == msg in case of full queue
		// note: we process the remaining msg in a subsequent loop iteration
		if (num_written - num_read >= NUM_ELEMENTS(s_msg_queue))
			num_written = num_read + NUM_ELEMENTS(s_msg_queue) - 1;

		msg = &s_msg_queue[num_read % NUM_ELEMENTS(s_msg_queue)];
		msg_end = &s_msg_queue[num_written % NUM_ELEMENTS(s_msg_queue)];

		//// skip already processed messages
		//while (msg != msg_end)
		//{
		//	if (!((uintX_t)(msg->timestamp) & 1))
		//		break;
		//
		//	if (++msg >= &s_msg_queue[NUM_ELEMENTS(s_msg_queue)])
		//		msg = &s_msg_queue[0];
		//}

		if (msg != msg_end)
		{
			Gpi_Fast_Tick_Native	ts_fast;
			Gpi_Hybrid_Tick			ts_hybrid;

			// convert timestamp of first message
			ts_fast = msg->timestamp;
			ts_hybrid = gpi_tick_fast_to_hybrid(ts_fast);
			msg->timestamp = ts_hybrid | 1;

			if (++msg >= &s_msg_queue[NUM_ELEMENTS(s_msg_queue)])
				msg = &s_msg_queue[0];

			// convert subsequent messages in an efficient way
			// NOTE: we assume that they arrived < 0x10...0 fast ticks after the first message
			while (msg != msg_end)
			{
				ASSERT_CT(0 == GPI_FAST_CLOCK_RATE % GPI_HYBRID_CLOCK_RATE,
					FAST_HYBRID_ratio_must_be_integral);
				ASSERT_CT_WARN(IS_POWER_OF_2(GPI_FAST_CLOCK_RATE / GPI_HYBRID_CLOCK_RATE),
					FAST_HYBRID_ratio_not_power_of_2_is_expensive);

				msg->timestamp = ts_hybrid + ((Gpi_Fast_Tick_Native)(msg->timestamp) - ts_fast) / (GPI_FAST_CLOCK_RATE / GPI_HYBRID_CLOCK_RATE);
				msg->timestamp |= 1;

				if (++msg >= &s_msg_queue[NUM_ELEMENTS(s_msg_queue)])
					msg = &s_msg_queue[0];
			}

			s_num_read = num_written;
		}

		// if additional msgs arrived in between: reiterate
	}
	while (num_written != s_msg_queue_num_written);
}

#endif	// GPI_TRACE_USE_DSR

//**************************************************************************************************
//***** Global Functions ***************************************************************************

void gpi_trace_store_msg(const char* fmt, ...)
{
	ASSERT_CT(sizeof(Gpi_Hybrid_Tick) >= sizeof(Gpi_Fast_Tick_Native));

	unsigned int			num_writing;
	Gpi_Trace_Msg			*msg;
	Gpi_Fast_Tick_Native	timestamp;
	int						ie;

	ie = gpi_int_lock();	// implies REORDER_BARRIER() ...

#if GPI_TRACE_OVERFLOW_ON_WRITE
	static unsigned int	s_num_lost = 0;
	unsigned int		num_lost = 0;

	if (s_msg_queue_num_writing - s_msg_queue_num_read >= NUM_ELEMENTS(s_msg_queue))
	{
		if (s_num_lost < -2u)
			s_num_lost++;

		num_lost = -1;
    }
	else
	{
#endif
		timestamp = gpi_tick_fast_native();
		num_writing = s_msg_queue_num_writing++;

#if GPI_TRACE_OVERFLOW_ON_WRITE
		num_lost = s_num_lost;
		s_num_lost = 0;
    }
#endif

	gpi_int_unlock(ie);		// implies REORDER_BARRIER() ...

#if GPI_TRACE_OVERFLOW_ON_WRITE
	if (-1u == num_lost)
		return;
#endif

	msg = &s_msg_queue[num_writing % NUM_ELEMENTS(s_msg_queue)];

#if GPI_TRACE_USE_DSR
	msg->timestamp = timestamp & ~(typeof(timestamp))1;
#else
	msg->timestamp = gpi_tick_fast_to_hybrid(timestamp) | 1;
#endif

#if GPI_TRACE_OVERFLOW_ON_WRITE
	if (num_lost > 0)
	{
		msg->msg = "!!! TRACE buffer overflow, %u message(s) lost !!!\n";
		msg->var_args[0] = ++num_lost;
    }
	else
#endif
	{
		msg->msg = fmt;

		// ATTENTION: Evaluating va_list manually is very critical and must be designed with care.
		// It is possible because its behavior is fixed in the Application Binary Interface (ABI).
		// See the ARM Procedure Call Standard (document number ARM IHI 0042F) for details.
		va_list va;
		va_start(va, fmt);

		// ATTENTION: If the stack is almost empty, then copying a fixed-size va block can exceed
		// the stack range and in consequence throw an exception (due to the access violation).
		// To circumvent that, the critical situations must be handled explicitly.
		size_t va_size_max = (uintptr_t)_estack - (uintptr_t)(va.__ap);
		if (sizeof(msg->var_args) > va_size_max)
			gpi_memcpy_dma_aligned(msg->var_args, va.__ap, va_size_max);
		else gpi_memcpy_dma_aligned(msg->var_args, va.__ap, sizeof(msg->var_args));

		va_end(va);
    }

	ie = gpi_int_lock();	// implies REORDER_BARRIER() ...

	if (s_msg_queue_num_written == num_writing)
	{
		s_msg_queue_num_written = s_msg_queue_num_writing;

#if GPI_TRACE_USE_DSR
		// trigger postprocessing IRQ
		gpi_trace_trigger_dsr();
#endif
    }

	gpi_int_unlock(ie);		// implies REORDER_BARRIER() ...
}

//**************************************************************************************************

void gpi_trace_print_all_msgs()
{
	unsigned int	num_read;
	Gpi_Trace_Msg	*msg;

	num_read = s_msg_queue_num_read;

	if (num_read == s_msg_queue_num_written)
		return;

#define CATCH_NESTED_CALLS	1
#if CATCH_NESTED_CALLS
	static volatile void	*s_outer_call = NULL;
	{
		register void	*return_address;
		register int	ie;

		return_address = __builtin_return_address(0);

		ie = gpi_int_lock();

		if (NULL != s_outer_call)
		{
			printf("\n!!! PANIC: nested (non-fast) TRACE call !!!\n");
			printf("called from %p, outer call from %p\n", return_address, s_outer_call);
			printf("system halted\n");

			// would be nice to print stack trace and enter minimalistic debug console here

			while (1);
		}

		s_outer_call = return_address;

		gpi_int_unlock(ie);
	}
#endif

#if !GPI_TRACE_OVERFLOW_ON_WRITE
	static Gpi_Trace_Msg	s_msg;
	msg = &s_msg;
#endif

	while (num_read != s_msg_queue_num_written)
	{
#if !GPI_TRACE_OVERFLOW_ON_WRITE
		gpi_memcpy_dma_aligned(msg, &s_msg_queue[num_read % NUM_ELEMENTS(s_msg_queue)], sizeof(Gpi_Trace_Msg));

		unsigned int num_open = s_msg_queue_num_writing - num_read;
		if (num_open > NUM_ELEMENTS(s_msg_queue))
		{
			num_open -= NUM_ELEMENTS(s_msg_queue);
			num_read += num_open;

			msg->msg = "!!! TRACE buffer overflow, %u message(s) lost !!!\n";
			msg->var_args[0] = num_open;
			msg->timestamp = 0;
        }
		else num_read++;

		REORDER_BARRIER();
		__DMB();

		s_msg_queue_num_read = num_read;
#else
		msg = &s_msg_queue[num_read % NUM_ELEMENTS(s_msg_queue)];
#endif
		// ATTENTION: Manipulating va_list manually is very critical and must be designed with care.
		// It is possible because its behavior is fixed in the Application Binary Interface (ABI).
		// See the ARM Procedure Call Standard (document number ARM IHI 0042F) for details.
		va_list va = { &(msg->var_args) };

		int msg_type = -1;

		// parse internal header (see trace.h for details)
		while ('\b' == msg->msg[1])
		{
			switch (msg->msg[0])
			{
				// remove full path if requested
				// msg[2] = index of var_arg that contains __FILE__
				case 'A':
				{
					const char	**file = &(((const char**)&(msg->var_args))[msg->msg[2] - '0']);
					const char	*s = *file + strlen(*file);
					
					while (s-- != *file)
					{
						if ((*s == '/') || (*s == '\\'))
							break;
					}
					
					*file = ++s;
					
					msg->msg += 4;
					break;
                }
				
				// set message type
				case 'B':
				{
					msg_type = (unsigned char)(msg->msg[2] - '0');
					
					msg->msg += 4;
					break;
                }

				// skip command / arg				
				default:
					msg->msg += 2;
            }
        }

		if (msg_type >= 0)
		{
			static const char * const fmt[] = GPI_TRACE_TYPE_FORMAT_LUT;

			if (msg_type >= NUM_ELEMENTS(fmt))
				msg_type = 0;
						
			printf(fmt[msg_type]);
		}
		
		if (!(msg->timestamp & 1))
			printf("???.???.??? ");
		else
		{
			Gpi_Hybrid_Tick		ts;
			unsigned int		s;
			char				ms[8];

			ts = msg->timestamp;

			s = ts / GPI_HYBRID_CLOCK_RATE;
			ts %= GPI_HYBRID_CLOCK_RATE;

			snprintf(ms, sizeof(ms), "%06" PRIu32, (uint32_t)gpi_tick_hybrid_to_us(ts));
			memmove(&ms[4], &ms[3], sizeof(ms) - 4);
			ms[sizeof(ms) - 1] = 0;
			ms[3] = '.';

			printf("%3u.%s ", s, ms);
        }

		vprintf(msg->msg, va);
		
		if (msg_type >= 0)
		{
			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Wformat-zero-length"
			
			printf(GPI_TRACE_TYPE_FORMAT_RESET);
			
			#pragma GCC diagnostic pop
        }

		printf("\n");

#if GPI_TRACE_OVERFLOW_ON_WRITE
		num_read++;
		REORDER_BARRIER();
		__DMB();
		s_msg_queue_num_read = num_read;
#endif
    }

#if CATCH_NESTED_CALLS
	s_outer_call = NULL;
#endif

}

//**************************************************************************************************
//**************************************************************************************************

#endif // (TRACE_MODE & TRACE_MODE_TRACE)
