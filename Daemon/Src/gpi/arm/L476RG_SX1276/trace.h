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
 *	@file					gpi/arm/nordic/nrf52840/trace.h
 *
 *	@brief					TRACE settings for Nordic nRF52840
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

#ifndef __GPI_STM32_TRACE_H__
#define __GPI_STM32_TRACE_H__

//**************************************************************************************************
//***** Includes ***********************************************************************************

#include "gpi/tools.h"
#include "gpi/clocks.h"

// #include <nrf.h>

// post includes see below

//**************************************************************************************************
//***** Global (Public) Defines and Consts *********************************************************



//**************************************************************************************************
//***** Local (Private) Defines and Consts *********************************************************

// select whether TRACE functions internally use a DSR (delayed service routine)
// pro: better timing when using TRACE on interrupt level
// con: uses an interrupt (interrupts must be enabled, "asynchronous" execution)
#ifndef GPI_TRACE_USE_DSR
	#define GPI_TRACE_USE_DSR			GPI_HYBRID_CLOCK_USE_VHT

	ASSERT_CT_WARN_STATIC(GPI_TRACE_USE_DSR ||
		(!GPI_HYBRID_CLOCK_USE_VHT && sizeof(Gpi_Hybrid_Tick) == sizeof(Gpi_Fast_Tick_Native)),
		enabling_GPI_TRACE_USE_DSR_could_be_beneficial);
#endif

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



#ifdef __cplusplus
	}
#endif

//**************************************************************************************************
//***** Implementations of Inline Functions ********************************************************

#if GPI_TRACE_USE_DSR

	#define GPI_TRACE_DSR_IRQ					CRYPTOCELL_IRQn
	#define GPI_TRACE_DSR_VECTOR				CRYPTOCELL_IRQHandler

	static inline void gpi_trace_trigger_dsr()	{ NVIC->STIR = GPI_TRACE_DSR_IRQ;		}

#endif

//**************************************************************************************************
//***** Post Includes ******************************************************************************

// include the generic file not before the specific settings made above
#include "gpi/arm/armv7-m/trace.h"

//**************************************************************************************************
//**************************************************************************************************

#endif // __GPI_STM32_TRACE_H__
