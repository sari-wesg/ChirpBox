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
 *	@file					gpi/olf_internal.h
 *
 *	@brief					internal stuff used to implement optimized low-level functions
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

#ifndef __GPI_OLF_INTERNAL_H__
#define __GPI_OLF_INTERNAL_H__

//**************************************************************************************************
//***** Includes ***********************************************************************************

#include "gpi/tools.h"		// ASSERT_CT

#include <stdint.h>

//**************************************************************************************************
//***** Global (Public) Defines and Consts *********************************************************

// helper macros for implementing variable-type macros like gpi_get_msb()
// NOTE: variants named ..._1 are for functions with one parameter, ..._2 for two and so on
// ATTENTION: do not implement such macros like this:
//	#define gpi_get_msb(a)	(
//		!!(1 == sizeof(a)) * gpi_get_msb_8(a) +
//		!!(2 == sizeof(a)) * gpi_get_msb_16(a) +
//		!!(4 == sizeof(a)) * gpi_get_msb_32(a) +
//		ASSERT_CT_EVAL(sizeof(a) <= 4, gpi_get_msb_invalid_parameter))
// because this can cause very critical side effects (e.g., think about calling gpi_get_msb(a++)
// or gpi_get_msb(f(a))! Instead use the helper macros, which interpose an inline function to
// avoid such critical effects. The up-cast to the largest type seems wasteful, but it gets
// optimized out due to combined inlining and constant propagation.
// NOTE: sizeof(a) and typeof(a) do not evaluate a, i.e., they do not cause side effects

#define _GPI_SIZE_DISPATCHER_FUNCTION_1_16(name, return_type)					\
	static inline __attribute__((always_inline)) return_type _ ## name ## _		\
		(uint_fast8_t _size_, uint16_t _param_) {								\
		switch (_size_) {														\
			case 1: return name ## _8  (_param_);								\
			case 2: return name ## _16 (_param_);								\
			default: assert_reset(0); return (return_type)-1; /* must not happen */	\
	}}

#define _GPI_SIZE_DISPATCHER_FUNCTION_1_32(name, return_type)					\
	static inline __attribute__((always_inline)) return_type _ ## name ## _		\
		(uint_fast8_t _size_, uint32_t _param_) {								\
		switch (_size_) {														\
			case 1: return name ## _8  (_param_);								\
			case 2: return name ## _16 (_param_);								\
			case 4: return name ## _32 (_param_);								\
			default: assert_reset(0); return (return_type)-1; /* must not happen */	\
	}}

#define _GPI_SIZE_DISPATCHER_FUNCTION_2_32(name, return_type, param2_type)		\
	static inline __attribute__((always_inline)) return_type _ ## name ## _		\
		(uint_fast8_t _size_, uint32_t _param_, param2_type _param2_) {			\
		switch (_size_) {														\
			case 1: return name ## _8  (_param_, _param2_);						\
			case 2: return name ## _16 (_param_, _param2_);						\
			case 4: return name ## _32 (_param_, _param2_);						\
			default: assert_reset(0); return (return_type)-1; /* must not happen */	\
	}}

#define _GPI_SIZE_DISPATCHER_1_16(name, param) (								\
	_ ## name ## _ (sizeof(param), param) +										\
	ASSERT_CT_EVAL(sizeof(param) <= 2, name ## _invalid_parameter_type)	)

#define _GPI_SIZE_DISPATCHER_1_32(name, param) (								\
	_ ## name ## _ (sizeof(param), param) +										\
	ASSERT_CT_EVAL(sizeof(param) <= 4, name ## _invalid_parameter_type)	)

#define _GPI_SIZE_DISPATCHER_2_32(name, param, param2) (						\
	_ ## name ## _ (sizeof(param), param, param2) +								\
	ASSERT_CT_EVAL(sizeof(param) <= 4, name ## _invalid_parameter_type)	)

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

#endif // __GPI_OLF_INTERNAL_H__
