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
 *	@file					gpi/tools.h
 *
 *	@brief					common macros, types, ... generally useful to improve code quality
 *							(portability, error-proneness, readability)
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

#ifndef __GPI_TOOLS_H__
#define __GPI_TOOLS_H__

//**************************************************************************************************
//***** Includes ***********************************************************************************

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

//**************************************************************************************************
//***** Global Defines and Consts ******************************************************************

//**************************************************************************************************
// macro args handling

/// helper macro for stringification
//  note: two-step approach is necessary (do not remove it!)
#define STRINGIFY(...)		STRINGIFY2(__VA_ARGS__)
#define STRINGIFY2(...)		#__VA_ARGS__

/// helper macro for concatenation with macro expansion
//  note: two-step approach is essential (do not remove it!)
#define CONCAT(a, b)		CONCAT2(a, b)
#define CONCAT2(a, b)		a ## b

/// @brief get number of variadic args (__VA_ARGS__) in a function-like preprocessor macro
/// @details
/// current implementation handles 0...10 arguments (can be extended straightforward if required)
#define VA_NUM(a...)		VA_NUM1(a)

/// @brief get size of variadic args (__VA_ARGS__) in a function-like preprocessor macro
/// @details
/// current implementation handles 0...10 arguments (can be extended straightforward if required)
#define VA_SIZE(a...)		VA_SIZE1(a)

//**************************************************************************************************
#define assert_Instrumentation(arg)                     \
({                                                      \
    if ((arg == 0)) {                         			\
        __disable_fault_irq();                          \
		NVIC_SystemReset();								\
    }													\
})

#define assert_reset(arg) 								\
	do{													\
		if (!(arg)) { 				     				\
		assert_Instrumentation(arg);					\
		} 												\
	} while(0)

// #define assert_reset(arg) assert(arg)

// assertions (in particular compile-time assertions)

// internal core construct to implement compile-time assertions
#define ASSERT_CT_CORE(condition, ...)		\
	sizeof(struct { int assertion_failed__ ## __VA_ARGS__ : 1 - 2 * !(condition); })

/// @brief compile-time assertion
/// @details The assertion is evaluated at compile-time and leads to a compilation error if the
/// check fails. It does not generate machine code.
/// @param condition	condition that is asserted to be true
/// @param ...			(optional) identifier that makes the potential error message more meaningful
/// @internal accommodate to Static_assert() (ISO C11) after that has become common
#define ASSERT_CT(condition, ...)			((void) ASSERT_CT_CORE(condition, ##__VA_ARGS__))

/// @brief function-like variant of ASSERT_CT()
/// @details ASSERT_CT_EVAL() basically works like ASSERT_CT() but provides a return value
/// (0 if the assertion is true, irrelevant otherwise due to the generated compilation error).
/// This makes it possible to check the assertion inside of another expression.
#define ASSERT_CT_EVAL(condition, ...)		(!ASSERT_CT_CORE(condition, ##__VA_ARGS__))

/// @brief variant of ASSERT_CT() that can be used on file scope
/// @details ASSERT_CT_STATIC() basically works like ASSERT_CT(). But while ASSERT_CT() is used
/// inside functions, ASSERT_CT_STATIC() can be used on file scope, i.e., outside of functions.
//#define ASSERT_CT_STATIC(condition, ...)	ASSERT_CT_STATIC2(__COUNTER__, condition, __VA_ARGS__)
#define ASSERT_CT_STATIC(condition, ...)	\
	static inline void CONCAT(assert_dummy_, __COUNTER__) () 	{	\
		ASSERT_CT(condition, ##__VA_ARGS__);					}

/// @brief like ASSERT_CT(), but generates a warning instead of an error
#define ASSERT_CT_WARN(condition, ...)									\
	do {																\
		_Pragma("GCC diagnostic push");									\
		_Pragma("GCC diagnostic warning \"-Wpadded\"");					\
		struct T_ {														\
			uint8_t		a[1 + !!(condition)]; 							\
			uint16_t	assertion_warning__ ## __VA_ARGS__;	};			\
		_Pragma("GCC diagnostic pop");									\
	} while (0)

/// @brief like ASSERT_CT_STATIC(), but generates a warning instead of an error
#define ASSERT_CT_WARN_STATIC(condition, ...)	\
	static inline void CONCAT(assert_dummy_, __COUNTER__) () 	{	\
		ASSERT_CT_WARN(condition, ##__VA_ARGS__);				}

/// @brief determine if c is a constant expression
/// @details One use case for this is to ensure that some macro M can be used only with constants
/// (to prohibit accidental and maybe expensive unfolding). To this end, M can be defined as follows:
/// @code #define M(a) (... + ASSERT_CT_EVAL(IS_CONST_EXPRESSION(a))) @endcode
#define IS_CONST_EXPRESSION(c)				__builtin_constant_p(c)

/// @brief flexible assertion
/// @details If possible, the assertion is realized as compile-time assertion, otherwise it
/// redirects to the standard assert() macro. By setting ASSERT_WARN_CT, it is possible to
/// generate a warning if compile-time evaluation is impossible. This can be used as a hint
/// to either rework the condition or switch to assert().
#define ASSERT(condition, ...)																	\
	do {																						\
		if (__builtin_constant_p(condition)) {													\
			/* here we use variable-length arrays, a feature introduced with C99 */				\
			const int trigger_ = __builtin_constant_p(condition) * !(condition);				\
			int assertion_failed_ ## __VA_ARGS__ [1 - 2 * trigger_] __attribute__((unused));	\
		} else {																				\
			_Pragma("GCC diagnostic push");														\
			_Pragma(ASSERT_DIAG_MODE);															\
			struct T_ {																			\
				uint8_t		a[1 + !!__builtin_constant_p(condition)]; 							\
				uint16_t	unable_to_evaluate_ASSERT_at_compile_time;		};					\
			_Pragma("GCC diagnostic pop");														\
			assert_reset(condition);																	\
		}																						\
	} while (0)

#if (!defined(ASSERT_WARN_CT) && defined(NDEBUG))
	#define ASSERT_WARN_CT	1
#endif
#if ASSERT_WARN_CT
	#define ASSERT_DIAG_MODE	"GCC diagnostic warning \"-Wpadded\""
#else
	#define ASSERT_DIAG_MODE	"GCC diagnostic ignored \"-Wpadded\""
#endif

// the following code can be used to provide assert with an optional message string
// ATTENTION: calling assert() with additional arguments is not ISO-C compliant
// #undef assert
// #define assert(c)	assert_msg(c)
#ifdef NDEBUG
	#define assert_msg(condition, ...)		((void)0)
#else
	#define assert_msg(condition, ...)										\
		if (!(condition)) do {												\
			static const void* msg_[] = { #condition, ##__VA_ARGS__ }; 		\
			__assert(msg_[NUM_ELEMENTS(msg_) - 1], __FILE__, __LINE__);		\
		} while (0)
#endif

//**************************************************************************************************
// array and struct field handling

/// @brief get length of an array (i.e., number of elements, not sizeof())
/// @internal the name ARRAY_SIZE stems from the Linux kernel
/// @internal http://zubplot.blogspot.com/2015/01/gcc-is-wonderful-better-arraysize-macro.html
/// helps to understand the type issues
#define ARRAY_SIZE(a)																\
	(																				\
		(sizeof(a) / sizeof((a)[0])) +												\
		/* check if a is really the array (itself), not a pointer to the array;		\
		 * otherwise the result would be wrong */									\
		ASSERT_CT_EVAL(!__builtin_types_compatible_p(typeof(a), typeof(&(a)[0])), 	\
			invalid_ARRAY_SIZE_usage)												\
	)

/// @copybrief ARRAY_SIZE().
/// This is just another name for ARRAY_SIZE(), it avoids confusion with sizeof(array).
#define NUM_ELEMENTS(a)			ARRAY_SIZE(a)

/// @brief get size of a struct field or class member
/// @internal the name FIELD_SIZEOF stems from the Linux kernel
#define FIELD_SIZEOF(T, m)		sizeof(((const T*)0)->m)

/// @copybrief FIELD_SIZEOF().
/// This is just another name for FIELD_SIZEOF() in the style of sizeof() and offsetof().
#define sizeof_field(T, m)		FIELD_SIZEOF(T, m)
#define sizeof_member(T, m)		FIELD_SIZEOF(T, m)	///< @copybrief sizeof_field

/// @brief This is just another name for offsetof() (from stddef.h) in the style of sizeof_field().
/// The use is deprecated in favor of offsetof() since the latter is ISO standard.
#define offsetof_field(T, m) 	offsetof(T, m)
#define offsetof_member(T, m) 	offsetof(T, m)		///< @copybrief offsetof_field

// redefine offsetof() to the clean builtin form
// NOTE: Some stddef.h define offsetof(T, m) as ((size_t)&(((T*)0)->m)), which is somewhat dirty
// (see for instance https://en.wikipedia.org/wiki/Offsetof for details) and can cause annoying
// warnings in some use cases (e.g. warning: variably modified ... at file scope). Therefore we
// redefine offsetof() to the clean form using a builtin function.
// ATTENTION: Redefining standard macros is crude in general because in consequence different
// definitions may become effective in different places of the program. However, this is not
// critical here because the result of the evaluation is consistent (we just avoid potential
// warnings)
#undef offsetof
#define offsetof(T, m) __builtin_offsetof(T, m)

/// convert a pointer to an array element to the index of that element
#define ARRAY_INDEX(p, a)		(((uintptr_t)(p) - (uintptr_t)&(a)[0]) / sizeof((a)[0]))

/// convert a pointer to an array element to the index of that element with the known size (byte) and address
#define ARRAY_INDEX_SIZE_ADD(p, a, size)		(((uintptr_t)(p) - (uintptr_t)(a)) / (size))

//**************************************************************************************************
// bit twiddling

/// @brief generate bit mask with single bit at position pos set
/// @details
/// BV ("BitValue") is often used in low-level programming when working with control register
/// fields. Its main purpose is to improve the compactness and readability of source code (that's
/// why it has this short name).
#define BV(pos)				(1u << (pos))

/// @brief test if a constant value is a power of 2
/// @details
/// A typical use case for this macro is in compile-time assertions before writing some optimized
/// code that relies on a power-of-2 assumption (e.g. when replacing mul or div by shift operations).
#define IS_POWER_OF_2(a)	( (a) && (((uintmax_t)(a) & -(uintmax_t)(a)) == (uintmax_t)(a)) )

/// extract MSB of a constant value
/// @internal if sizeof(a) assertion triggers, then extend the MSBx macros (it is straightforward)
#define MSB(a)				( MSB32(a) +														\
							  ASSERT_CT_EVAL(IS_CONST_EXPRESSION(a), non_const_expression) +	\
							  ASSERT_CT_EVAL(sizeof(a) < 8, overflow) )

/// extract LSB of a constant value
#define LSB(a)				MSB((a) & -(a))

// the following macros are for internal use only
#define MSB2(a)				( (a) & 2 ? 1 : ( (a) & 1 ? 0 : -1 ) )
#define MSB4(a)				( (uint32_t)(a) >=         0x4 ?  2 +  MSB2((uint32_t)(a) >>  2) :  MSB2(a) )
#define MSB8(a)				( (uint32_t)(a) >=        0x10 ?  4 +  MSB4((uint32_t)(a) >>  4) :  MSB4(a) )
#define MSB16(a)			( (uint32_t)(a) >=       0x100 ?  8 +  MSB8((uint32_t)(a) >>  8) :  MSB8(a) )
#define MSB32(a)			( (uint32_t)(a) >=     0x10000 ? 16 + MSB16((uint32_t)(a) >> 16) : MSB16(a) )
//#define MSB64(a)			( (uint64_t)(a) >= UINT64_C(0x100000000) ? 32 + MSB32((uint64_t)(a) >> 32) : MSB32(a) )

//**************************************************************************************************
// low-level programming and flow control

/// @brief make function always inline (incl. debug builds with optimization disabled)
#define ALWAYS_INLINE					inline __attribute__((always_inline))

/// @brief general re-order barrier
static ALWAYS_INLINE void REORDER_BARRIER()	{__asm volatile ("" : : : "memory"); }

/// @brief selective re-order barrier for specific variable
/// @details As a side effect, COMPUTE_BARRIER(p) ensures that p resides in a register.
#define COMPUTE_BARRIER(p)				__asm__ volatile ("" : : "r"(p))

//**************************************************************************************************
// miscellaneous tools

#ifndef ABS
	/// @brief
	#define ABS(a)			({ typeof(a) a_ = (a); (a_ < 0) ? -a_ : a_; })
#endif

#ifndef MIN
	/// @brief
	#define MIN(a,b)		({ typeof(a) a_ = (a); typeof(b) b_ = (b); (a_ < b_) ? a_ : b_; })
	/// @brief
	#define MAX(a,b)		({ typeof(a) a_ = (a); typeof(b) b_ = (b); (a_ > b_) ? a_ : b_; })
#endif

#define ALIGNMENT			offsetof(struct {int8_t a; intmax_t b;}, b)

//**************************************************************************************************
//***** Local (Private) Defines and Consts *********************************************************

// detect if argument (or arg list) is present by evaluating the size of its stringification
// interal: accommodate to __VA_OPT__() (ISO C++2a) after that has become common
//#define VA_IS(a...) 		(sizeof(STRINGIFY(#a)) > 3)	// "\"a\"" = {\", a, \", \0}
#define VA_IS(a...) 		(sizeof(#a) > 1)

// successively count the arguments
// note: we cannot do things like #define VA_NUM(a,b...) (VA_IS(a) + VA_NUM(b)) because
// preprocessor avoids "self-expansion". Therefore the macros have to be unrolled.
#define VA_NUM1(a...)		(VA_IS(a) ? 1 + VA_NUM2(a) : 0)
#define VA_NUM2(a,b...)		(VA_IS(b) ? 1 + VA_NUM3(b) : 0)
#define VA_NUM3(b,c...)		(VA_IS(c) ? 1 + VA_NUM4(c) : 0)
#define VA_NUM4(c,d...)		(VA_IS(d) ? 1 + VA_NUM5(d) : 0)
#define VA_NUM5(d,e...)		(VA_IS(e) ? 1 + VA_NUM6(e) : 0)
#define VA_NUM6(e,f...)		(VA_IS(f) ? 1 + VA_NUM7(f) : 0)
#define VA_NUM7(f,g...)		(VA_IS(g) ? 1 + VA_NUM8(g) : 0)
#define VA_NUM8(g,h...)		(VA_IS(h) ? 1 + VA_NUM9(h) : 0)
#define VA_NUM9(h,i...)		(VA_IS(i) ? 1 + VA_NUM10(i) : 0)
#define VA_NUM10(i,j...)	(VA_IS(j) ? 1 + VA_NUM11(j) : 0)
#define VA_NUM11(j,k...)	ASSERT_CT_EVAL(!VA_IS(k), VA_NUM_overflow)
//#define VA_NUM11(j,k...)	(VA_IS(k) ? -10 - __LINE__ : 0)

// get size of argument, 0 if there is no argument
#define VA_SIZEOF(a...)		(sizeof((void)(uint8_t)1, ##a) - 1 + VA_IS(a))

// pad size to var_args granularity (typically machine word size)
#define VA_SIZEOF_PAD(a...)	((VA_SIZEOF(a) + VA_SIZE_GRANULARITY - 1) & ~(VA_SIZE_GRANULARITY - 1))

// successively count the size of the arguments
// note: preprocessor avoids "self-expansion", therefore the macros have to be unrolled
#define VA_SIZE1(a...)		(VA_IS(a) ? VA_SIZE2(a) : 0)
#define VA_SIZE2(a,b...)	(VA_SIZEOF_PAD(a) + (VA_IS(b) ? VA_SIZE3(b) : 0))
#define VA_SIZE3(b,c...)	(VA_SIZEOF_PAD(b) + (VA_IS(c) ? VA_SIZE4(c) : 0))
#define VA_SIZE4(c,d...)	(VA_SIZEOF_PAD(c) + (VA_IS(d) ? VA_SIZE5(d) : 0))
#define VA_SIZE5(d,e...)	(VA_SIZEOF_PAD(d) + (VA_IS(e) ? VA_SIZE6(e) : 0))
#define VA_SIZE6(e,f...)	(VA_SIZEOF_PAD(e) + (VA_IS(f) ? VA_SIZE7(f) : 0))
#define VA_SIZE7(f,g...)	(VA_SIZEOF_PAD(f) + (VA_IS(g) ? VA_SIZE8(g) : 0))
#define VA_SIZE8(g,h...)	(VA_SIZEOF_PAD(g) + (VA_IS(h) ? VA_SIZE9(h) : 0))
#define VA_SIZE9(h,i...)	(VA_SIZEOF_PAD(h) + (VA_IS(i) ? VA_SIZE10(i) : 0))
#define VA_SIZE10(i,j...)	(VA_SIZEOF_PAD(i) + (VA_IS(j) ? VA_SIZE11(j) : 0))
#define VA_SIZE11(j,k...)	(VA_SIZEOF_PAD(j) + ASSERT_CT_EVAL(!VA_IS(k), VA_SIZE_overflow))
//#define VA_SIZE11(j,k...)	(VA_SIZEOF_PAD(j) + (VA_IS(k) ? -80 - __LINE__ : 0))

//**************************************************************************************************
//***** Forward Class and Struct Declarations ******************************************************



//**************************************************************************************************
//***** Global Typedefs and Class Declarations *****************************************************

typedef	union Generic32_tag
{
	uint32_t		u32;
	int32_t			s32;

#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)

	struct
	{
		uint16_t	u16_l;
		uint16_t	u16_h;
	};

	struct
	{
		int16_t		s16_l;
		int16_t		s16_h;
    };

	struct
	{
		uint8_t		u8_ll;
		uint8_t		u8_lh;
		uint8_t		u8_hl;
		uint8_t		u8_hh;
    };

	struct
	{
		int8_t		s8_ll;
		int8_t		s8_lh;
		int8_t		s8_hl;
		int8_t		s8_hh;
    };

#elif (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)

	struct
	{
		uint16_t	u16_h;
		uint16_t	u16_l;
	};

	struct
	{
		int16_t		s16_h;
		int16_t		s16_l;
    };

	struct
	{
		uint8_t		u8_hh;
		uint8_t		u8_hl;
		uint8_t		u8_lh;
		uint8_t		u8_ll;
    };

	struct
	{
		int8_t		s8_hh;
		int8_t		s8_hl;
		int8_t		s8_lh;
		int8_t		s8_ll;
    };

#else
	#error byte order is unknown
#endif

} Generic32;

//**************************************************************************************************

typedef	union Generic64_tag
{
	uint64_t		u64;
	int64_t			s64;

#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)

	struct
	{
		uint32_t	u32_l;
		uint32_t	u32_h;
	};

	struct
	{
		int32_t		s32_l;
		int32_t		s32_h;
    };

	struct
	{
		uint16_t	u16_ll;
		uint16_t	u16_lh;
		uint16_t	u16_hl;
		uint16_t	u16_hh;
    };

	struct
	{
		int16_t		s16_ll;
		int16_t		s16_lh;
		int16_t		s16_hl;
		int16_t		s16_hh;
    };

	struct
	{
		uint8_t		u8_lll;
		uint8_t		u8_llh;
		uint8_t		u8_lhl;
		uint8_t		u8_lhh;
		uint8_t		u8_hll;
		uint8_t		u8_hlh;
		uint8_t		u8_hhl;
		uint8_t		u8_hhh;
    };

	struct
	{
		int8_t		s8_lll;
		int8_t		s8_llh;
		int8_t		s8_lhl;
		int8_t		s8_lhh;
		int8_t		s8_hll;
		int8_t		s8_hlh;
		int8_t		s8_hhl;
		int8_t		s8_hhh;
    };

#elif (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)

	struct
	{
		uint32_t	u32_h;
		uint32_t	u32_l;
	};

	struct
	{
		int32_t		s32_h;
		int32_t		s32_l;
    };

	struct
	{
		uint16_t	u16_hh;
		uint16_t	u16_hl;
		uint16_t	u16_lh;
		uint16_t	u16_ll;
    };

	struct
	{
		int16_t		s16_hh;
		int16_t		s16_hl;
		int16_t		s16_lh;
		int16_t		s16_ll;
    };

	struct
	{
		uint8_t		u8_hhh;
		uint8_t		u8_hhl;
		uint8_t		u8_hlh;
		uint8_t		u8_hll;
		uint8_t		u8_lhh;
		uint8_t		u8_lhl;
		uint8_t		u8_llh;
		uint8_t		u8_lll;
    };

	struct
	{
		int8_t		s8_hhh;
		int8_t		s8_hhl;
		int8_t		s8_hlh;
		int8_t		s8_hll;
		int8_t		s8_lhh;
		int8_t		s8_lhl;
		int8_t		s8_llh;
		int8_t		s8_lll;
    };

#else
	#error byte order is unknown
#endif

} Generic64;

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
//***** Post Includes ******************************************************************************

// include files that (may) depend on current file "in order"

#include "gpi/platform_spec.h"		// VA_SIZE_GRANULARITY

//**************************************************************************************************
//**************************************************************************************************

#endif // __GPI_TOOLS_H__
