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
 *	@file					gpi/arm/armv7-m/interrupts.h
 *
 *	@brief					basic interrupt handling
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

#ifndef __GPI_ARM_ARMv7_STM32_INTERRUPTS_H__
#define __GPI_ARM_ARMv7_STM32_INTERRUPTS_H__

//**************************************************************************************************
//***** Includes ***********************************************************************************

#include "gpi/platform_spec.h"
// #include GPI_PLATFORM_PATH(cmsis_device.h)
#include "stm32l476xx.h"
#include "gpi/tools.h"

#ifdef GPI_SETUP_FILE
	#include STRINGIFY(GPI_SETUP_FILE)
#endif

#if !GPI_ARCH_IS_CORE(ARMv7M)
	#error unsupported architecture
#endif

//**************************************************************************************************
//***** Global (Public) Defines and Consts *********************************************************



//**************************************************************************************************
//***** Local (Private) Defines and Consts *********************************************************

// Since ARMv7-M there are specific instructions (load/store exclusive) to support unblocking
// synchronization. However, due to some implementation dependent details the usage of these
// instructions may be less efficient than the "classic" int-lock variant (depending on the
// application). So in case of problems it maybe an option to switch to the int-lock variant.
//
// (For details see "ARM Synchronization Primitives" (doc. DHT0008A ID081709). Critical points
// include the "Exclusives Reservation Granule", statements like "A local monitor can be
// implemented to tag an address for exclusive use, or it can contain a state machine that only
// tracks the issuing of Load-Exclusive and Store-Exclusive instructions.", as well as the
// question on what happens if the memory address gets overwritten by a standard write
// instruction between load/store-exclusive. Also, the Cortex-M4 User Guide (DUI0553A ID121610)
// says on page 3-36 that "The result of executing a Store-Exclusive instruction to an address
// that is different from that used in the preceding Load-Exclusive instruction is
// unpredictable." This must be wrong (because otherwise the whole concept would be
// inapplicable; I guess it refers only to the status return value, not the whole behavior),
// but a (detailed) description of the real behavior is missing.)
//
#ifndef _GPI_ARM_USE_UNBLOCKING_SYNC
	#define _GPI_ARM_USE_UNBLOCKING_SYNC	1
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

// available intrinsics: __disable_irq (-> cpsid i), __enable_irq (-> cpsie i)

static ALWAYS_INLINE void	gpi_int_enable()	{ __enable_irq();	}
static ALWAYS_INLINE void	gpi_int_disable()	{ __disable_irq();	}

//**************************************************************************************************

static ALWAYS_INLINE int gpi_int_lock()
{
	register int ie;

	REORDER_BARRIER();

	// NOTE: We do not use CMSIS functions at this point to avoid spill code in debug builds.
	// This may be a matter of taste (it is not absolutely necessary if performance is secondary).
	__ASM volatile
	(
		"mrs	%0, PRIMASK		\n"		// ie = __get_PRIMASK()
		"cpsid	i				\n"		// __set_PRIMASK(0) / __disable_irq()
		: "=r"(ie)
	);

	REORDER_BARRIER();
	__DMB();

	return ie;
}

//**************************************************************************************************

static ALWAYS_INLINE void gpi_int_unlock(int ie)
{
	REORDER_BARRIER();
	__DMB();

	// NOTE: we expect ie as it has been returned by gpi_int_lock()
	__set_PRIMASK(ie);

	REORDER_BARRIER();
}

//**************************************************************************************************

static ALWAYS_INLINE void gpi_atomic_or(volatile unsigned int *p, unsigned int mask)
{
	#if _GPI_ARM_USE_UNBLOCKING_SYNC

		register int	tmp1, tmp2;

		REORDER_BARRIER();
		__DMB();

		__asm__
		(
			"1:							\n"
			"ldrex		%1, %0			\n"
			"orr		%1, %1, %3		\n"
			"strex		%2, %1, %0		\n"		// ATTENTION: Rd must be different from Rt and Rn
			"cmp		%2, #0			\n"
			"bne		1b				\n"
			"dmb						\n"
			: "+m"(*p), "=&r"(tmp1), "=r"(tmp2)
			: "r"(mask)
			: "cc"
		);

		REORDER_BARRIER();

	#else

		register int ie = gpi_int_lock();

		*p |= mask;

		gpi_int_unlock(ie);

	#endif
}

//**************************************************************************************************

static ALWAYS_INLINE void gpi_atomic_and(volatile unsigned int *p, unsigned int mask)
{
	#if _GPI_ARM_USE_UNBLOCKING_SYNC

		register int	tmp1, tmp2;

		REORDER_BARRIER();
		__DMB();

		__asm__
		(
			"1:							\n"
			"ldrex		%1, %0			\n"
			"and		%1, %1, %3		\n"
			"strex		%2, %1, %0		\n"		// ATTENTION: Rd must be different from Rt and Rn
			"cmp		%2, #0			\n"
			"bne		1b				\n"
			"dmb						\n"
			: "+m"(*p), "=&r"(tmp1), "=r"(tmp2)
			: "r"(mask)
			: "cc"
		);

		REORDER_BARRIER();

	#else

		register int ie = gpi_int_lock();

		*p &= mask;

		gpi_int_unlock(ie);

	#endif
}

//**************************************************************************************************

static ALWAYS_INLINE void gpi_atomic_set(volatile unsigned int *p, unsigned int mask)
{
	gpi_atomic_or(p, mask);
}

//**************************************************************************************************

static ALWAYS_INLINE void gpi_atomic_clear(volatile unsigned int *p, unsigned int mask)
{
	// if performance is extremely critical, one can copy the code from gpi_atomic_and()
	// and replace "and" by "bic" instead of manually inverting the mask as done here

	gpi_atomic_and(p, ~mask);
}

//**************************************************************************************************
//**************************************************************************************************

#endif // __GPI_ARM_ARMv7_STM32_INTERRUPTS_H__
