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
 *	@file					gpi/platform_spec.h
 *
 *	@brief					defines platform specific attributes and settings
 *
 *	@version				$Id$
 *	@date					TODO
 *
 *	@author					Carsten Herrmann
 *
 ***************************************************************************************************

 	@details

	TODO

	@internal

	Intervals defined by <name> ... <name>_END are half open, i.e., <name>_END represents the
	first value outside the interval (it is the same concept as used with C++ STL containers).

 **************************************************************************************************/

#ifndef __GPI_PLATFORM_SPEC_H__
#define __GPI_PLATFORM_SPEC_H__

//**************************************************************************************************
//***** Includes ***********************************************************************************

#include "gpi/tools.h"		// STRINGIFY(), LSB(), ASSERT_CT()

#include <stdint.h>			// UINTxx_C()

//**************************************************************************************************
//***** Global Defines and Consts ******************************************************************

#define GPI_PLATFORM_PATH(file)				GPI_PLATFORM_PATH_2(GPI_PLATFORM_DIR, file)
#define GPI_PLATFORM_PATH_2(dir, file)		STRINGIFY(gpi/dir/file)

//**************************************************************************************************
// detect and adapt used platform specification method

// variant 1: GPI_ARCH_<attribute>_IS_<value>
// example:
//		#define GPI_ARCH_CORE_IS_MSP430
//		#define GPI_ARCH_DEVICE_IS_MSP430F16x
//		#define GPI_ARCH_BOARD_IS_TMOTE
//		#define GPI_ARCH_OS_IS_NONE
// -> simple, but somewhat unsafe
#if (!defined(GPI_ARCH_PLATFORM) && !defined(GPI_ARCH_DEVICE))

  #define GPI_ARCH_IS_CORE(x)		defined(GPI_ARCH_CORE_IS_ ## x)
  #define GPI_ARCH_IS_DEVICE(x)		defined(GPI_ARCH_DEVICE_IS_ ## x)
  #define GPI_ARCH_IS_BOARD(x)		defined(GPI_ARCH_BOARD_IS_ ## x)
  #define GPI_ARCH_IS_OS(x)			defined(GPI_ARCH_OS_IS_ ## x)

// advanced schemes
#else

	// variant 2: GPI_ARCH_<attribute> = ...
	// example:
	//		#define GPI_ARCH_DEVICE		GPI_ARCH_DEVICE_MSP430F16x
	//		#define GPI_ARCH_BOARD		GPI_ARCH_BOARD_TMOTE
	//		#define GPI_ARCH_OS			GPI_ARCH_OS_NONE
	#if (!defined(GPI_ARCH_PLATFORM))

		#define _GPI_DEVICE(a)					GPI_ARCH_DEVICE_ ## a
		#define _GPI_ARCH_TEST_GET(setting)		GPI_ARCH_ ## setting

	// variant 3: GPI_ARCH_PLATFORM = ...
	// example:
	//		#define GPI_ARCH_PLATFORM	(GPI_ARCH_BOARD_TMOTE | GPI_ARCH_OS_NONE)
	#else

		#define GPI_ARCH_OS_MASK				UINT32_C(0xF0000000)
		#define GPI_ARCH_DEVICE_MASK			UINT32_C(0x00FFF000)
		#define GPI_ARCH_BOARD_MASK				UINT32_C(0x00FFF00F)

		#define GPI_ARCH_OS_SHIFT				28
		#define GPI_ARCH_DEVICE_SHIFT			12
		#define GPI_ARCH_BOARD_SHIFT			0

		#define _GPI_DEVICE(a)					(GPI_ARCH_DEVICE_ ## a << GPI_ARCH_DEVICE_SHIFT)
//		#define _GPI_DEVICE(a)					(GPI_ARCH_DEVICE_ ## a << LSB(GPI_ARCH_DEVICE_MASK))

		#define _GPI_ARCH_TEST_GET(setting)		\
			((GPI_ARCH_PLATFORM & GPI_ARCH_ ## setting ## _MASK) >> GPI_ARCH_ ## setting ## _SHIFT)
//			((GPI_ARCH_PLATFORM & GPI_ARCH_ ## setting ## _MASK) >> LSB(GPI_ARCH_ ## setting ## _MASK))

	#endif

/*	#define _GPI_ARCH_TEST(setting, attr, value)		\
		(_GPI_ARCH_TEST_GET(setting) == GPI_ARCH_ ## attr ## _ ## value)
*/
	#define _GPI_ARCH_TEST_GROUP(setting, attr, group)	\
		((_GPI_ARCH_TEST_GET(setting) >= GPI_ARCH_ ## attr ## _ ## group) &&	\
		 (_GPI_ARCH_TEST_GET(setting) <  GPI_ARCH_ ## attr ## _ ## group ## _END))

	#define GPI_ARCH_IS_CORE(x)				_GPI_ARCH_TEST_GROUP(DEVICE, CORE, x)
//	#define GPI_ARCH_IS_DEVICE_FAMILY(x)	_GPI_ARCH_TEST_GROUP(DEVICE, DEVICE_FAMILY, x)
	#define GPI_ARCH_IS_DEVICE(x)			_GPI_ARCH_TEST_GROUP(DEVICE, DEVICE, x)
	#define GPI_ARCH_IS_BOARD(x)			_GPI_ARCH_TEST_GROUP(BOARD, BOARD, x)
	#define GPI_ARCH_IS_OS(x)				_GPI_ARCH_TEST_GROUP(OS, OS, x)
//	#define GPI_ARCH_IS_DEVICE(x)			_GPI_ARCH_TEST(DEVICE, DEVICE, x)
//	#define GPI_ARCH_IS_BOARD(x)			_GPI_ARCH_TEST(BOARD, BOARD, x)
//	#define GPI_ARCH_IS_OS(x)				_GPI_ARCH_TEST(OS, OS, x)

#endif

//**************************************************************************************************
//***** Platform Identifiers and Handling **********************************************************

// operating systems
#define GPI_ARCH_OS_NONE						0
#define GPI_ARCH_OS_NONE_END					1

//**************************************************************************************************
// Texas Instruments MSP430
// see here for part numbering scheme:
// http://www.ti-msp430.com/en/overview/naming-rules-0002.html
// https://en.wikipedia.org/wiki/TI_MSP430#MSP430_part_numbering

#define GPI_ARCH_CORE_MSP430					UINT32_C(1)		// don't use 0 as a valid value
#define GPI_ARCH_CORE_MSP430_END				(GPI_ARCH_CORE_MSP430 + 99)		// = 100

// generation 0
//#define GPI_ARCH_DEVICE_FAMILY_MSP430x0xx		(GPI_ARCH_CORE_MSP430 +   )
//#define GPI_ARCH_DEVICE_FAMILY_MSP430x0xx_END	(GPI_ARCH_CORE_MSP430 + 10)

// generation 1
//#define GPI_ARCH_DEVICE_FAMILY_MSP430x1xx		(GPI_ARCH_CORE_MSP430 + 10)
#define GPI_ARCH_DEVICE_MSP430x1xx				(GPI_ARCH_CORE_MSP430 + 10)
	#define GPI_ARCH_DEVICE_MSP430F15x				(GPI_ARCH_CORE_MSP430 + 15)
	#define GPI_ARCH_DEVICE_MSP430F15x_END			(GPI_ARCH_CORE_MSP430 + 16)
	#define GPI_ARCH_DEVICE_MSP430F16x				(GPI_ARCH_CORE_MSP430 + 16)
	#define GPI_ARCH_DEVICE_MSP430F16x_END			(GPI_ARCH_CORE_MSP430 + 17)
#define GPI_ARCH_DEVICE_MSP430x1xx_END			(GPI_ARCH_CORE_MSP430 + 20)
//#define GPI_ARCH_DEVICE_FAMILY_MSP430x1xx_END	(GPI_ARCH_CORE_MSP430 + 20)

#define GPI_ARCH_BOARD_TMOTE					(_GPI_DEVICE(MSP430F16x) + 1)
	#define GPI_ARCH_BOARD_TMOTE_PURE				(GPI_ARCH_BOARD_TMOTE + 0)
	#define GPI_ARCH_BOARD_TMOTE_PURE_END			(GPI_ARCH_BOARD_TMOTE + 1)
	#define GPI_ARCH_BOARD_TMOTE_FLOCKLAB			(GPI_ARCH_BOARD_TMOTE + 1)
	#define GPI_ARCH_BOARD_TMOTE_FLOCKLAB_END		(GPI_ARCH_BOARD_TMOTE + 2)
	#define GPI_ARCH_BOARD_TMOTE_INDRIYA			(GPI_ARCH_BOARD_TMOTE + 2)
	#define GPI_ARCH_BOARD_TMOTE_INDRIYA_END		(GPI_ARCH_BOARD_TMOTE + 3)
#define GPI_ARCH_BOARD_TMOTE_END				(GPI_ARCH_BOARD_TMOTE + 3)

// ...
//#define GPI_ARCH_DEVICE_FAMILY_MSP430x2xx		(GPI_ARCH_CORE_MSP430 + 20)
//#define GPI_ARCH_DEVICE_FAMILY_MSP430x2xx_END	(GPI_ARCH_CORE_MSP430 + 30)
//#define GPI_ARCH_DEVICE_FAMILY_MSP430x3xx		(GPI_ARCH_CORE_MSP430 + 30)
//#define GPI_ARCH_DEVICE_FAMILY_MSP430x3xx_END	(GPI_ARCH_CORE_MSP430 + 40)
//#define GPI_ARCH_DEVICE_FAMILY_MSP430x4xx		(GPI_ARCH_CORE_MSP430 + 40)
//#define GPI_ARCH_DEVICE_FAMILY_MSP430x4xx_END	(GPI_ARCH_CORE_MSP430 + 50)
//#define GPI_ARCH_DEVICE_FAMILY_MSP430x5xx		(GPI_ARCH_CORE_MSP430 + 50)
//#define GPI_ARCH_DEVICE_FAMILY_MSP430x5xx_END	(GPI_ARCH_CORE_MSP430 + 60)
//#define GPI_ARCH_DEVICE_FAMILY_MSP430x6xx		(GPI_ARCH_CORE_MSP430 + 60)
//#define GPI_ARCH_DEVICE_FAMILY_MSP430x6xx_END	(GPI_ARCH_CORE_MSP430 + 70)

#if (MSP430 || __MSP430__)

	// ATTENTION: the macro MSP430 breaks things like GPI_ARCH_IS_CORE(MSP430),
	// therefore we undefine it here (in favor of __MSP430__)
	#undef MSP430
	#ifndef __MSP430__
		#define __MSP430__
	#endif

	#if !GPI_ARCH_IS_CORE(MSP430)
		#warning GPI_ARCH_PLATFORM not specified or mismatch; assuming GPI_ARCH_CORE_IS_MSP430
		#define GPI_ARCH_CORE_IS_MSP430
	#endif

	#if GPI_ARCH_IS_BOARD(TMOTE)
		#define GPI_PLATFORM_DIR	msp430/tmote
	#else
		#define GPI_PLATFORM_DIR	msp430/msp430_common
	#endif

	#define VA_SIZE_GRANULARITY		2

	// safety-check
	ASSERT_CT_STATIC(ALIGNMENT == 2, alignment_mismatch);

#elif GPI_ARCH_IS_CORE(MSP430)
	#error platform mismatch
#endif

//**************************************************************************************************
// ARM

#define GPI_ARCH_CORE_ARM					GPI_ARCH_CORE_MSP430_END
#define GPI_ARCH_CORE_ARM_END				(GPI_ARCH_CORE_ARM + 1000)

// ARM architectures
// ARMv1 ... ARMv5: unsupported (obsolete)

// ARMv6 (e.g. Cortex-M0(+))
#define GPI_ARCH_CORE_ARMv6					(GPI_ARCH_CORE_ARM   +   0)
	#define GPI_ARCH_CORE_ARM_M0				(GPI_ARCH_CORE_ARMv6 +   0)
	#define GPI_ARCH_CORE_ARM_M0_END			(GPI_ARCH_CORE_ARMv6 + 100)
#define GPI_ARCH_CORE_ARMv6_END				(GPI_ARCH_CORE_ARMv6 + 100)

// ARMv7
// ARMv7-M = microcontroller profile  (e.g. Cortex-M3, Cortex-M4)
// ARMv7E-M = ARMv7-M + SIMD instructions (-> Cortex-M4)
// ARMv7-R = realtime profile (e.g. Cortex-R4)
// ARMv7-A = application profile (e.g. Cortex-A8)
#define GPI_ARCH_CORE_ARMv7					(GPI_ARCH_CORE_ARMv6_END)
	#define GPI_ARCH_CORE_ARMv7M				(GPI_ARCH_CORE_ARMv7  +   0)
		#define GPI_ARCH_CORE_ARM_M3				(GPI_ARCH_CORE_ARMv7M +   0)
		#define GPI_ARCH_CORE_ARM_M3_END			(GPI_ARCH_CORE_ARMv7M + 100)
		#define GPI_ARCH_CORE_ARM_M4				(GPI_ARCH_CORE_ARMv7M + 100)
			#define GPI_ARCH_CORE_ARM_M4F				(GPI_ARCH_CORE_ARMv7M + 150)	// incl. FPU
			#define GPI_ARCH_CORE_ARM_M4F_END			(GPI_ARCH_CORE_ARMv7M + 200)
		#define GPI_ARCH_CORE_ARM_M4_END			(GPI_ARCH_CORE_ARMv7M + 200)
	#define GPI_ARCH_CORE_ARMv7M_END			(GPI_ARCH_CORE_ARMv7  + 200)
	#define GPI_ARCH_CORE_ARMv7A				(GPI_ARCH_CORE_ARMv7M_END + 0)
	#define GPI_ARCH_CORE_ARMv7A_END			(GPI_ARCH_CORE_ARMv7M_END + 1)
	#define GPI_ARCH_CORE_ARMv7R				(GPI_ARCH_CORE_ARMv7M_END + 1)
	#define GPI_ARCH_CORE_ARMv7R_END			(GPI_ARCH_CORE_ARMv7M_END + 2)
#define GPI_ARCH_CORE_ARMv7_END				(GPI_ARCH_CORE_ARMv7  + 202)

// ARM devices and boards

// Nordic Semiconductor nRF52840
// NOTE: PCA... is the reliable way to refer to a specific Nordic board. Names like nRF52840DK
// stand for kits (board + accessories) and may change.
#define GPI_ARCH_DEVICE_nRF52840			(GPI_ARCH_CORE_ARM_M4F + 0)
#define GPI_ARCH_DEVICE_nRF52840_END		(GPI_ARCH_CORE_ARM_M4F + 1)
#define GPI_ARCH_DEVICE_STM32L476RG			(GPI_ARCH_CORE_ARM_M4F + 2)
#define GPI_ARCH_DEVICE_STM32L476RG_END		(GPI_ARCH_CORE_ARM_M4F + 3)

#define GPI_ARCH_BOARD_nRF_PCA10056			(_GPI_DEVICE(nRF52840) + 1)		// = nRF52840 DK
#define GPI_ARCH_BOARD_nRF_PCA10056_END		(_GPI_DEVICE(nRF52840) + 2)
#define GPI_ARCH_BOARD_nRF_PCA10059			(_GPI_DEVICE(nRF52840) + 2)		// = nRF52840 USB Dongle
#define GPI_ARCH_BOARD_nRF_PCA10059_END		(_GPI_DEVICE(nRF52840) + 3)
#define GPI_ARCH_BOARD_NUCLEOL476RG			(_GPI_DEVICE(STM32L476RG) + 1)		// = nRF52840 USB Dongle
#define GPI_ARCH_BOARD_NUCLEOL476RG_END		(_GPI_DEVICE(STM32L476RG) + 2)

#if (__ARM_ARCH)

	#if !GPI_ARCH_IS_CORE(ARM)
		#warning GPI_ARCH_PLATFORM not specified or mismatch; assuming GPI_ARCH_CORE_IS_ARM
		#define GPI_ARCH_CORE_IS_ARM
	#endif

	#if GPI_ARCH_IS_BOARD(nRF_PCA10056)
		#define GPI_PLATFORM_DIR	arm/nordic/pca10056
	#else
		#define GPI_PLATFORM_DIR	arm/L476RG_SX1276
	#endif

	#if __ARM_32BIT_STATE

		// ATTENTION: VA_SIZE handling on 32-bit ARM is tricky for two reasons:
		// 1) While the granularity is 4, the alignment is defined as 8.
		// 2) Variadic arguments of type float are casted to double.
		// (defined in ABI specification, document ARM IHI 0042F)
		// These definitions make exact size determination complicated. As a compromise, we set
		// VA_SIZE_GRANULARITY to 8, leading to overestimated results. This wastes a bit of memory
		// (in some sense, e.g., size checks may fail although there is enough space), but it is
		// more safe than the other way around. If memory is an issue then VA_SIZE_GRANULARITY can
		// be set to 4, but with adequate care regarding the mentioned points (e.g., it is safe if
		// the program never uses floating point or 64-bit values as variadic arguments).
		#ifdef VA_SIZE_GRANULARITY
			#if ((VA_SIZE_GRANULARITY < 4) || (VA_SIZE_GRANULARITY & 3))
				#error VA_SIZE_GRANULARITY is invalid
			#endif
		#else
			#define VA_SIZE_GRANULARITY		8
		#endif

		// safety-check
		ASSERT_CT_STATIC(ALIGNMENT == 8, alignment_mismatch);

	#else

		#error unsupported architecture
		// TODO: add support for 64 bit cores

	#endif

#elif GPI_ARCH_IS_CORE(ARM)
	#error platform mismatch
#endif

//**************************************************************************************************
// future architectures

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



#ifdef __cplusplus
	}
#endif

//**************************************************************************************************
//***** Implementations of Inline Functions ********************************************************



//**************************************************************************************************
//**************************************************************************************************

#endif // __GPI_PLATFORM_SPEC_H__
