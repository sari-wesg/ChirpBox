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
 *	@file					gpi/trace.h
 *
 *	@brief					macros for (debug) log messages, facilitating a consistent handling
 *							w.r.t. the output channel, format, and (de-)activation of message types
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

#ifndef __GPI_TRACE_H__
#define __GPI_TRACE_H__

//**************************************************************************************************
//***** Includes ***********************************************************************************

// helper macros for stringification
#define GPI_TRACE_STRINGIFY2(x)		#x
#define GPI_TRACE_STRINGIFY(x)		GPI_TRACE_STRINGIFY2(x)

// include project-specific setup file (if defined)
// GPI_TRACE_SETUP_FILE can be used to manage the current project's trace settings at a central
// place. Including it here ensures that all settings (in particular GPI_TRACE_MODE) are valid
// before they get evaluated in preprocessor conditions below.
#ifdef GPI_TRACE_SETUP_FILE
	#include GPI_TRACE_STRINGIFY(GPI_TRACE_SETUP_FILE)
#elif defined(GPI_SETUP_FILE)
	#include GPI_TRACE_STRINGIFY(GPI_SETUP_FILE)
#endif

// check and complete settings
#ifndef GPI_TRACE_MODE
	#define GPI_TRACE_MODE			GPI_TRACE_MODE_NO_TRACE
#endif

#include "gpi/platform_spec.h"			// GPI_PLATFORM_PATH
#include GPI_PLATFORM_PATH(trace.h)		// GPI_TRACE_VA_SIZE_MAX
#include "gpi/tools.h"					// VA_NUM, ASSERT_CT, LSB, ...

#include <inttypes.h>					// (u)int..._t, PRI...

//**************************************************************************************************
//***** Local (Private) Defines and Consts *********************************************************

#if ((UINTPTR_MAX > UINT32_MAX) || (GPI_TRACE_PRINT_VALUE_SIZE > 32))
	#define GPI_TRACE_PRId	PRIdMAX
	#define GPI_TRACE_PRIu	PRIuMAX
	#define GPI_TRACE_PRIx	PRIxMAX
	typedef uintmax_t 		Gpi_Trace_Uint_t;
#else
	#define GPI_TRACE_PRId	PRId32
	#define GPI_TRACE_PRIu	PRIu32
	#define GPI_TRACE_PRIx	PRIx32
	typedef uint32_t		Gpi_Trace_Uint_t;
#endif	

//**************************************************************************************************

#define _GPI_TRACE_TYPE_INDEX(group)	(		\
	!(group & GPI_TRACE_MSG_TYPE_MASK) ? 0 :	\
	1 + LSB(group & GPI_TRACE_MSG_TYPE_MASK) - LSB(GPI_TRACE_MSG_TYPE_MASK))

//**************************************************************************************************

// The GPI_TRACE_FILTER_PATH mechanism has to be designed with care because it is important that
// the evaluation takes place completely at compile time (that is the main purpose of the concept).
// To reach this goal we use a tree of inline functions that get optimized away during compilation.
// The reason for building an explicit tree is that using a loop or recursive calls does not lead
// to the desired effect (maybe we didn't find the right options to improve loop unrolling, or the
// situation is just too complex).
// ATTENTION: For some reason GCC (cc1) consumes a huge amount of memory when resolving the tree.

// ATTENTION: For some reason the optimization attributes are ignored (maybe inlining inherits
// caller's attributes). Therefore we fall back to runtime mode if optimization is disabled.
// NOTE: #pragma GCC optimize doesn't help as well. Even more, it seems that it defines __OPTIMIZE__
// without undefining it on #pragma GCC pop_options. Since it is not meant for production code
// anyway, we don't use it here.
#if (GPI_TRACE_FILTER_PATH > 1)
	#if __OPTIMIZE__
	// ok
	#else
//		#pragma message "relaxing GPI_TRACE_FILTER_PATH to 1 because values > 1 are critical without optimization"
		#undef GPI_TRACE_FILTER_PATH
		#define GPI_TRACE_FILTER_PATH	1
	#endif
#endif

#if (GPI_TRACE_FILTER_PATH > 1)

	// leaf function
	static inline __attribute__((always_inline, optimize("O3")))
		size_t _gpi_trace_filter_path_0(const char *s, size_t a, size_t b)
	{
//		ASSERT_CT(a == b);
		return ((s[a] == '/') || (s[a] == '\\')) ? a + 1 : 0;
	}

	// middle layer functions
	#define _GPI_TRACE_FILTER_PATH_x(x,y)											\
		static inline __attribute__((always_inline, optimize("O3")))				\
		size_t _gpi_trace_filter_path_ ## y (const char *s, size_t a, size_t b) {	\
			if (a == b)																\
				return ((s[a] == '/') || (s[a] == '\\')) ? a + 1 : 0;				\
			size_t c = _gpi_trace_filter_path_ ## x (s, a + (b - a) / 2 + 1, b);	\
			if (c)																	\
				return c;															\
			return _gpi_trace_filter_path_ ## x (s, a, a + (b - a) / 2);			\
		}
		
	_GPI_TRACE_FILTER_PATH_x(0,1)
	_GPI_TRACE_FILTER_PATH_x(1,2)
	_GPI_TRACE_FILTER_PATH_x(2,3)
	_GPI_TRACE_FILTER_PATH_x(3,4)
	_GPI_TRACE_FILTER_PATH_x(4,5)
	_GPI_TRACE_FILTER_PATH_x(5,6)
	_GPI_TRACE_FILTER_PATH_x(6,7)
	_GPI_TRACE_FILTER_PATH_x(7,8)
	_GPI_TRACE_FILTER_PATH_x(8,9)
	_GPI_TRACE_FILTER_PATH_x(9,10)

	// entry point
	#if (GPI_TRACE_FILTER_PATH <= 16)
		#define _GPI_TRACE_FILTER_PATH(s)	\
			&s[_gpi_trace_filter_path_4(s, 0, sizeof(s) - 1) +	\
			ASSERT_CT_EVAL(sizeof(s) <= (1 << 4), _GPI_TRACE_FILTER_PATH__size_coverage_exceeded)]
	#elif (GPI_TRACE_FILTER_PATH <= 32)
		#define _GPI_TRACE_FILTER_PATH(s)	\
			&s[_gpi_trace_filter_path_5(s, 0, sizeof(s) - 1) +	\
			ASSERT_CT_EVAL(sizeof(s) <= (1 << 5), _GPI_TRACE_FILTER_PATH__size_coverage_exceeded)]
	#elif (GPI_TRACE_FILTER_PATH <= 64)
		#define _GPI_TRACE_FILTER_PATH(s)	\
			&s[_gpi_trace_filter_path_6(s, 0, sizeof(s) - 1) +	\
			ASSERT_CT_EVAL(sizeof(s) <= (1 << 6), _GPI_TRACE_FILTER_PATH__size_coverage_exceeded)]
	#elif (GPI_TRACE_FILTER_PATH <= 128)
		#define _GPI_TRACE_FILTER_PATH(s)	\
			&s[_gpi_trace_filter_path_7(s, 0, sizeof(s) - 1) +	\
			ASSERT_CT_EVAL(sizeof(s) <= (1 << 7), _GPI_TRACE_FILTER_PATH__size_coverage_exceeded)]
	#elif (GPI_TRACE_FILTER_PATH <= 256)
		#define _GPI_TRACE_FILTER_PATH(s)	\
			&s[_gpi_trace_filter_path_8(s, 0, sizeof(s) - 1) +	\
			ASSERT_CT_EVAL(sizeof(s) <= (1 << 8), _GPI_TRACE_FILTER_PATH__size_coverage_exceeded)]
	#elif (GPI_TRACE_FILTER_PATH <= 512)
		#define _GPI_TRACE_FILTER_PATH(s)	\
			&s[_gpi_trace_filter_path_9(s, 0, sizeof(s) - 1) +	\
			ASSERT_CT_EVAL(sizeof(s) <= (1 << 9), _GPI_TRACE_FILTER_PATH__size_coverage_exceeded)]
	#elif (GPI_TRACE_FILTER_PATH <= 1024)
		#define _GPI_TRACE_FILTER_PATH(s)	\
			&s[_gpi_trace_filter_path_10(s, 0, sizeof(s) - 1) +	\
			ASSERT_CT_EVAL(sizeof(s) <= (1 << 10), _GPI_TRACE_FILTER_PATH__size_coverage_exceeded)]
	#else
		#error _GPI_TRACE_FILTER_PATH__size_coverage_exceeded
	#endif

#else	// GPI_TRACE_FILTER_PATH
	#define _GPI_TRACE_FILTER_PATH(s)	s
#endif	

//**************************************************************************************************
//***** Global Defines and Consts ******************************************************************

/// @name possible TRACE modes
/// @{
#define GPI_TRACE_MODE_NO_TRACE                 0		///< enables log output. see GPI_TRACE_MODE for details
#define GPI_TRACE_MODE_TRACE                    1		///< disables log output. see GPI_TRACE_MODE for details
/// @}

/// @name output options
/// @{
#define GPI_TRACE_LOG_FILE              		UINT32_C(0x80000000)	///< print source file name and line number
#define GPI_TRACE_LOG_SCOPE             		UINT32_C(0x40000000)    ///< print class::function scope
#define GPI_TRACE_LOG_TASK              		UINT32_C(0x20000000)    ///< print task id or name or something similar
#define GPI_TRACE_LOG_TIME              		UINT32_C(0x10000000)    ///< print current timestamp
#define GPI_TRACE_LOG_TYPE						UINT32_C(0x08000000)	///< highlight message type
/// @}

/// @name standard message types
/// @{
#define GPI_TRACE_LOG_FUNCTION_ENTRY			UINT32_C(0x02000000)	// for internal use only (externally, consider GPI_TRACE_LOG_FUNCTION_CALLS instead)
#define GPI_TRACE_LOG_FUNCTION_RETURN			UINT32_C(0x02000000)	// for internal use only (externally, consider GPI_TRACE_LOG_FUNCTION_CALLS instead)
#define GPI_TRACE_LOG_FUNCTION_RETURN_MSG		UINT32_C(0x01000000)	///< trace returns with explicit message only (i.e. GPI_TRACE_RETURN_MSG())
#define GPI_TRACE_LOG_FUNCTION_CALLS			UINT32_C(0x03000000)    ///< trace function calls (includes GPI_TRACE_LOG_FUNCTION_RETURN_MSG)
#define GPI_TRACE_LOG_PROGRAM_FLOW				UINT32_C(0x03000000)    ///< another name for GPI_TRACE_LOG_FUNCTION_CALLS
//#define GPI_TRACE_LOG_STARTUP/MODULES			UINT32_C(0x04000000)

#define GPI_TRACE_MSG_TYPE_VERBOSE				UINT32_C(0x00800000)
#define GPI_TRACE_MSG_TYPE_INFO					UINT32_C(0x00400000)
#define GPI_TRACE_MSG_TYPE_WARNING				UINT32_C(0x00200000)
#define GPI_TRACE_MSG_TYPE_ERROR				UINT32_C(0x00100000)
#define GPI_TRACE_MSG_TYPE_OK					UINT32_C(0x00080000)
#define GPI_TRACE_MSG_TYPE_FAILED				UINT32_C(0x00040000)
#define GPI_TRACE_MSG_TYPE_MASK					UINT32_C(0x03FC0000)	// for internal use only

#define GPI_TRACE_LOG_ALL						UINT32_C(0xFFFFFFFF)	///< select everything
#define GPI_TRACE_LOG_STANDARD					UINT32_C(0xF8000000)	///< select all standard format components
#define GPI_TRACE_LOG_USER						UINT32_C(0x00FFFFFF)	///< select all user-defined message groups
/// @}

/// @name formatting options
/// @{
#ifndef GPI_TRACE_SIZE_FILE
	#define GPI_TRACE_SIZE_FILE					25						///< width of filename column
#endif
#ifndef GPI_TRACE_SIZE_SCOPE
	#define GPI_TRACE_SIZE_SCOPE				25						///< width of scope column
#endif
/// @}

//**************************************************************************************************
// TRACE macros

// if TRACE enabled
#if (GPI_TRACE_MODE & GPI_TRACE_MODE_TRACE)

	/// configure log format options and select active message types/groups
	///
	/// @internal If there is a need to (de-)active message groups at runtime (-> to remove the
	/// const attribute), consider splitting format options (LOG_FILE, LOG_SCOPE, ...) into a
	/// separate desc member and keep them const to preserve the optimization potential in the
	/// TRACE macros. Otherwise (if formatting should also be variable) it may be better to move
	/// the (then dynamic) format handling into gpi_trace_store_msg().
	///
	#define GPI_TRACE_CONFIG(module, config)										\
		/* provide private dummy function needed internally */						\
		static inline int gpi_trace_dummy_function() { return 0; }					\
		/* instantiate module control block */                          			\
		static const Gpi_Trace_Module_Descriptor gpi_trace_module_desc =      		\
		{																			\
			(config)																\
			/*#module,																\
			__FILE__*/																\
		};																			\
		/* add a flat version of config because GCC's constant propagation */		\
		/* doesn't fully propagate const from struct in all cases */				\
		static const uint32_t gpi_trace_config = (config)

	/// output all buffered messages
	#define GPI_TRACE_FLUSH()														\
		gpi_trace_print_all_msgs()

	/// buffer log message for printing with next flush
	//
	// NOTE: The format string can contain an internal header that provides hints to the output
	// function about how to print the specific message. The encoding of the header allows to
	// easily skip unrelevant hints (or even ignore the full header). In detail:
	// The header content consists of printable characters. Every character in the header is
	// followed by \b. Hence, if the output function doesn't handle the header then the characters
	// get printed but immediately erased by following backspaces. Implicitly, fmt[1] = \b marks
	// that a header is present.
	// The header content is a sequence of commands where every command consists of a command code
	// (one uppercase letter) and optional arguments. The character set used for parameters is
	// disjoint from command codes.
	// Right now there are the following commands:
	//	A <i>	mark var_arg that contains __FILE__
	//			The output function can use this hint to filter the string when printing,
	//			e.g. to remove the path. <i> = '0'...'9' is the zero-based index of the
	//			corresponding argument in the var_args list.
	//	B <i>	mark the message type (see GPI_TRACE_MSG_TYPE_...)
	//			<i> = '1'...'9' or '0' if type is not specified

	// ASSERT_CT can not be used if constant propagation is inactive
	#if (__OPTIMIZE__)	
		// ATTENTION: since ASSERT_CT generates compile-time errors we must incorporate all checks
		// (including mode), we cannot rely on the position in the switch-case-block
		#define _GPI_TRACE_ASSERT_VA(m, ...)		\
			ASSERT_CT((_mode_ != (m)) || (VA_SIZE(__VA_ARGS__) <= GPI_TRACE_VA_SIZE_MAX), var_args_size_overflow)
	#else
		#define _GPI_TRACE_ASSERT_VA(m, ...)		\
			assert_reset(VA_SIZE(__VA_ARGS__) <= GPI_TRACE_VA_SIZE_MAX)
		#ifdef NDEBUG
			#warning setting NDEBUG with optimization disabled deactivates var_arg size checking in GPI_TRACE macros
		#endif
	#endif

	// use a helper macro to implement a number of case distinctions
	// NOTE: this is necessary due to limited constant propagation (at least with optimization
	// disabled). It looks painful, but optimization makes it very efficient.
	#define _GPI_TRACE_MSG_FAST_1(a, b, group, fmt)												\
		const char *_s_;																		\
		if (!(a) && !(b)) {																		\
			_s_ = fmt;																			\
		} else if ((a) && !(b)) {																\
			static const struct __attribute__((packed)) {										\
				char sc[4]; char sfmt[sizeof(fmt)]; } _s2_ = {{									\
				'A', '\b', '0', '\b'															\
				}, fmt };																		\
			_s_ = (const char*)&_s2_;															\
		} else if (!(a) && (b)) {																\
			static const struct __attribute__((packed)) {										\
				char sc[4]; char sfmt[sizeof(fmt)]; } _s2_ = {{									\
				'B', '\b', '0' + (char)_GPI_TRACE_TYPE_INDEX(group), '\b'						\
				}, fmt };																		\
			_s_ = (const char*)&_s2_;															\
		} else {																				\
			static const struct __attribute__((packed)) {										\
				char sc[8]; char sfmt[sizeof(fmt)]; } _s2_ = {{									\
				'A', '\b', '0', '\b',															\
				'B', '\b', '0' + (char)_GPI_TRACE_TYPE_INDEX(group), '\b'						\
				}, fmt };																		\
			_s_ = (const char*)&_s2_;															\
        }			

	#define _GPI_TRACE_FMT_FILE		\
		"%-" GPI_TRACE_STRINGIFY(GPI_TRACE_SIZE_FILE) "." GPI_TRACE_STRINGIFY(GPI_TRACE_SIZE_FILE) "s"

	#define _GPI_TRACE_FMT_SCOPE	\
		"%-" GPI_TRACE_STRINGIFY(GPI_TRACE_SIZE_SCOPE) "." GPI_TRACE_STRINGIFY(GPI_TRACE_SIZE_SCOPE) "s"

	#define GPI_TRACE_MSG_FAST(group, msg, ...)     											\
		do {																					\
			if (gpi_trace_module_desc.msg_config & (group)) {									\
				const uint32_t _mode_ = gpi_trace_config & (GPI_TRACE_LOG_FILE | GPI_TRACE_LOG_SCOPE);	\
				switch (_mode_) {																\
					case 0:	{																	\
						_GPI_TRACE_ASSERT_VA(0,	##__VA_ARGS__);									\
						_GPI_TRACE_MSG_FAST_1(0, gpi_trace_config & GPI_TRACE_LOG_TYPE,			\
							group, msg)															\
						gpi_trace_store_msg(_s_, ##__VA_ARGS__);								\
						break; }																\
					case GPI_TRACE_LOG_FILE: {													\
						_GPI_TRACE_ASSERT_VA(GPI_TRACE_LOG_FILE, &"x", ##__VA_ARGS__);			\
						if (1 == GPI_TRACE_FILTER_PATH) {										\
							_GPI_TRACE_MSG_FAST_1(1, gpi_trace_config & GPI_TRACE_LOG_TYPE,		\
								group, _GPI_TRACE_FMT_FILE " " msg)								\
							gpi_trace_store_msg(_s_, 											\
								__FILE__ "(" GPI_TRACE_STRINGIFY(__LINE__) ")", ##__VA_ARGS__);	\
						} else {																\
							_GPI_TRACE_MSG_FAST_1(0, gpi_trace_config & GPI_TRACE_LOG_TYPE,		\
								group, _GPI_TRACE_FMT_FILE " " msg)								\
							gpi_trace_store_msg(_s_, 											\
								_GPI_TRACE_FILTER_PATH(__FILE__ "(" GPI_TRACE_STRINGIFY(__LINE__) ")"),	\
								##__VA_ARGS__);													\
						}																		\
						break; }																\
					case GPI_TRACE_LOG_SCOPE: {													\
						_GPI_TRACE_ASSERT_VA(GPI_TRACE_LOG_SCOPE,								\
							&(__func__[0]), 0, (void*)0, ##__VA_ARGS__);						\
						_GPI_TRACE_MSG_FAST_1(0, gpi_trace_config & GPI_TRACE_LOG_TYPE,			\
							group, "%-." GPI_TRACE_STRINGIFY(GPI_TRACE_SIZE_SCOPE) "s ("		\
							GPI_TRACE_STRINGIFY(__LINE__) ")%-*.0s" msg)						\
						gpi_trace_store_msg(_s_, 												\
							__func__,															\
							(int)(GPI_TRACE_SIZE_SCOPE - MIN(GPI_TRACE_SIZE_SCOPE, sizeof(__func__) - 1)	\
								+ 6 - sizeof(GPI_TRACE_STRINGIFY(__LINE__))),					\
							(void*)0, ##__VA_ARGS__);											\
						break; }																\
					default: {																	\
						_GPI_TRACE_ASSERT_VA(GPI_TRACE_LOG_FILE | GPI_TRACE_LOG_SCOPE,			\
							&"x", &(__func__[0]), ##__VA_ARGS__);								\
						if (1 == GPI_TRACE_FILTER_PATH) {										\
							_GPI_TRACE_MSG_FAST_1(1, gpi_trace_config & GPI_TRACE_LOG_TYPE,		\
								group, _GPI_TRACE_FMT_FILE " " _GPI_TRACE_FMT_SCOPE " " msg)	\
							gpi_trace_store_msg(_s_, 											\
								__FILE__ "(" GPI_TRACE_STRINGIFY(__LINE__) ")",					\
								__func__, ##__VA_ARGS__);										\
						} else {																\
							_GPI_TRACE_MSG_FAST_1(0, gpi_trace_config & GPI_TRACE_LOG_TYPE,		\
								group, _GPI_TRACE_FMT_FILE " " _GPI_TRACE_FMT_SCOPE " " msg)	\
							gpi_trace_store_msg(_s_, 											\
								_GPI_TRACE_FILTER_PATH(__FILE__ "(" GPI_TRACE_STRINGIFY(__LINE__) ")"),	\
								__func__, ##__VA_ARGS__);										\
						}																		\
						break; }																\
                }																				\
            }																					\
        } while (0)

	/// print log message
	#define GPI_TRACE_MSG(group, msg, ...)						\
		do {													\
			GPI_TRACE_MSG_FAST(group, msg, ##__VA_ARGS__);		\
			GPI_TRACE_FLUSH();									\
		} while (0)

	/// log function call/entry
	#define GPI_TRACE_FUNCTION_FAST()     												\
		do {																			\
			if (gpi_trace_module_desc.msg_config & GPI_TRACE_LOG_SCOPE)					\
				GPI_TRACE_MSG_FAST(GPI_TRACE_LOG_FUNCTION_ENTRY, "-> entry");			\
			else 																		\
				GPI_TRACE_MSG_FAST(GPI_TRACE_LOG_FUNCTION_ENTRY, "-> %s()", __func__);	\
        } while (0)

	/// @copybrief GPI_TRACE_FUNCTION_FAST
	#define GPI_TRACE_FUNCTION()						\
		do {											\
			GPI_TRACE_FUNCTION_FAST();					\
			GPI_TRACE_FLUSH();							\
		} while (0)

	// GPI_TRACE_RETURN functionality
	//
	// attention: evaluating the return value can cause side effects (e.g. consider
	// GPI_TRACE_RETURN(a++)). Therefore we must evaluate __VA_ARGS__ exactly once.
	// We use a local variable to do so, the concept is
	// 		typeof(__VA_ARGS__) r = __VA_ARGS__;
	//		... // log r
	//		return r;
	// The tricky thing is that _VA_ARGS__ is empty when returning from a void function,
	// and we have to realize return (void) in this case. We extend the concept as follows:
	//		typeof(0, ##__VA_ARGS__) r = (0, ##__VA_ARGS__);
	// This construct uses the comma operator (if comma as an operator appears weird to you:
	// it is a well defined concept, you have probably used it without notice in variable
	// initializations and/or for-loops) and the very special ## behavior of GCC (should be
	// replaced by __VA_OPT__() after that has become common).
	// This basically solves the problem, but generates an unused-value warning in case of
	// non-void functions ("left-hand operand of comma expression has no effect"). To
	// circumvent that we replace the 0 with a dummy function.
	//
	// note: one might ask why we do not use something like that:
	// 		if (VA_NUM(__VA_ARGS__)) {
	//			...
	//			return r;
	//		} else {
	//			...
	//			return;
	//		}
	// This would not work for the following reason: After preprocessing, the function contains
	// both return statements. This causes a compilation error because one of them does not
	// match the return type of the function.
	//
	#define GPI_TRACE_RETURN_INTERNAL(flush, ...)												\
		do {																					\
			typeof(gpi_trace_dummy_function(), ##__VA_ARGS__) r_ =								\
					(gpi_trace_dummy_function(), ##__VA_ARGS__);								\
																								\
			_Pragma("GCC diagnostic push")														\
			_Pragma("GCC diagnostic ignored \"-Wpointer-to-int-cast\"")							\
			if (gpi_trace_module_desc.msg_config & GPI_TRACE_LOG_SCOPE) {						\
				if (VA_NUM(__VA_ARGS__))														\
					GPI_TRACE_MSG_FAST(GPI_TRACE_LOG_FUNCTION_RETURN, 							\
						"<- return %" GPI_TRACE_PRId " (0x%08" GPI_TRACE_PRIx ")", 				\
						(Gpi_Trace_Uint_t)r_, (Gpi_Trace_Uint_t)r_);							\
				else GPI_TRACE_MSG_FAST(GPI_TRACE_LOG_FUNCTION_RETURN, "<- return");			\
			} else {																			\
				if (VA_NUM(__VA_ARGS__))														\
					GPI_TRACE_MSG_FAST(GPI_TRACE_LOG_FUNCTION_RETURN,							\
						"<- %s() returned %" GPI_TRACE_PRId " (0x%08" GPI_TRACE_PRIx ")", 		\
						__func__, (Gpi_Trace_Uint_t)r_, (Gpi_Trace_Uint_t)r_);					\
				else GPI_TRACE_MSG_FAST(GPI_TRACE_LOG_FUNCTION_RETURN, "<- %s()", __func__);	\
			}																					\
			_Pragma("GCC diagnostic pop")														\
																								\
			/* attention: if flush is requested, we have to do it before returning				\
			 * (that is why we do it here and not outside of this macro) */						\
			if (flush)																			\
				GPI_TRACE_FLUSH();																\
																								\
			return (typeof((void)0, ##__VA_ARGS__))r_;											\
        } while (0)

	/// log function return/exit
	#define GPI_TRACE_RETURN_FAST(...)		GPI_TRACE_RETURN_INTERNAL(0, ##__VA_ARGS__)
	#define GPI_TRACE_RETURN(...)			GPI_TRACE_RETURN_INTERNAL(1, ##__VA_ARGS__)
		///< @copybrief GPI_TRACE_RETURN_FAST

	#define GPI_TRACE_RETURN_MSG_INTERNAL(msg, ...)										\
		do {																			\
			if (gpi_trace_module_desc.msg_config & GPI_TRACE_LOG_SCOPE)					\
				GPI_TRACE_MSG_FAST(GPI_TRACE_LOG_FUNCTION_RETURN_MSG,					\
					"<- return " msg, ##__VA_ARGS__);									\
			else 																		\
				GPI_TRACE_MSG_FAST(GPI_TRACE_LOG_FUNCTION_RETURN_MSG,					\
					"<- %s() returned " msg, __func__, ##__VA_ARGS__);					\
        } while (0)
		
	/// log function return/exit with specific message (e.g. return value format)
	#define GPI_TRACE_RETURN_MSG_FAST(r, msg, ...)					\
		do {														\
			GPI_TRACE_RETURN_MSG_INTERNAL(msg, ##__VA_ARGS__);		\
			return r;												\
		} while (0)
	
	/// @copybrief GPI_TRACE_RETURN_MSG_FAST
	#define GPI_TRACE_RETURN_MSG(r, msg, ...)						\
		do {														\
			GPI_TRACE_RETURN_MSG_INTERNAL(msg, ##__VA_ARGS__);		\
			GPI_TRACE_FLUSH();										\
			return r;												\
		} while (0)
	
// if TRACE disabled
#else	// GPI_TRACE_MODE

	// attention: empty macros must bind following semicolon

	#define GPI_TRACE_CONFIG(module, config)			typedef int Gpi_Trace_Dummy_Type
		// GPI_TRACE_CONFIG is used on file scope, therefore we bind the semicolon in a different way

	#define GPI_TRACE_FLUSH()							while (0)

	#define GPI_TRACE_MSG(group, msg, ...)				while (0)
	#define GPI_TRACE_MSG_FAST(group, msg, ...)	     	while (0)

	#define GPI_TRACE_FUNCTION()						while (0)
	#define GPI_TRACE_FUNCTION_FAST()					while (0)

	#define GPI_TRACE_RETURN(...)						return __VA_ARGS__
	#define GPI_TRACE_RETURN_FAST(...)					return __VA_ARGS__
	#define GPI_TRACE_RETURN_MSG(r, msg, ...)			return r
	#define GPI_TRACE_RETURN_MSG_FAST(r, msg, ...)		return r

#endif	// GPI_TRACE_MODE

//**************************************************************************************************
// GPI_TRACE_TYPE_FORMAT
// for details about escape sequences see for instance
// https://misc.flogisoft.com/bash/tip_colors_and_formatting
// https://en.wikipedia.org/wiki/ANSI_escape_code

#ifndef GPI_TRACE_TYPE_FORMAT
	#define GPI_TRACE_TYPE_FORMAT	1
#endif

// default (type not specified or unknown)
#ifndef GPI_TRACE_TYPE_FORMAT_DEFAULT
	#if (GPI_TRACE_TYPE_FORMAT == 1)
		#define GPI_TRACE_TYPE_FORMAT_DEFAULT	"    "
	#elif (GPI_TRACE_TYPE_FORMAT == 3)
		#define GPI_TRACE_TYPE_FORMAT_DEFAULT	"    "
	#else
		#define GPI_TRACE_TYPE_FORMAT_DEFAULT	""
	#endif
#endif

// reset formatting (close message)
#ifndef GPI_TRACE_TYPE_FORMAT_RESET
	#if (GPI_TRACE_TYPE_FORMAT == 1)
		#define GPI_TRACE_TYPE_FORMAT_RESET	""
	#elif (GPI_TRACE_TYPE_FORMAT == 3)
		#define GPI_TRACE_TYPE_FORMAT_RESET	"\e[0m"
	#else
		#define GPI_TRACE_TYPE_FORMAT_RESET	""
	#endif
#endif

#define GPI_TRACE_TYPE_FORMAT_PROGRAM_FLOW		GPI_TRACE_TYPE_FORMAT_VERBOSE
				
#ifndef GPI_TRACE_TYPE_FORMAT_VERBOSE
	#if (GPI_TRACE_TYPE_FORMAT == 1)
		#define GPI_TRACE_TYPE_FORMAT_VERBOSE	"    "
	#elif (GPI_TRACE_TYPE_FORMAT == 3)
		#define GPI_TRACE_TYPE_FORMAT_VERBOSE	"\e[90m    "
	#else
		#define GPI_TRACE_TYPE_FORMAT_VERBOSE	""
	#endif
#endif

#define GPI_TRACE_TYPE_FORMAT_INFO				GPI_TRACE_TYPE_FORMAT_DEFAULT

#ifndef GPI_TRACE_TYPE_FORMAT_WARNING
	#if (GPI_TRACE_TYPE_FORMAT == 1)
		#define GPI_TRACE_TYPE_FORMAT_WARNING	"[W] "
	#elif (GPI_TRACE_TYPE_FORMAT == 3)
		#define GPI_TRACE_TYPE_FORMAT_WARNING	"\e[93m[W] "
	#else
		#define GPI_TRACE_TYPE_FORMAT_WARNING	""
	#endif
#endif

#ifndef GPI_TRACE_TYPE_FORMAT_ERROR
	#if (GPI_TRACE_TYPE_FORMAT == 1)
		#define GPI_TRACE_TYPE_FORMAT_ERROR		"[E] "
	#elif (GPI_TRACE_TYPE_FORMAT == 3)
		#define GPI_TRACE_TYPE_FORMAT_ERROR		"\e[91;1m[E] "
	#else
		#define GPI_TRACE_TYPE_FORMAT_ERROR		""
	#endif
#endif

#ifndef GPI_TRACE_TYPE_FORMAT_OK
	#if (GPI_TRACE_TYPE_FORMAT == 1)
		#define GPI_TRACE_TYPE_FORMAT_OK		"[O] "
	#elif (GPI_TRACE_TYPE_FORMAT == 3)
		#define GPI_TRACE_TYPE_FORMAT_OK		"\e[92m[O] "
	#else
		#define GPI_TRACE_TYPE_FORMAT_OK		""
	#endif
#endif

#ifndef GPI_TRACE_TYPE_FORMAT_FAILED
	#if (GPI_TRACE_TYPE_FORMAT == 1)
		#define GPI_TRACE_TYPE_FORMAT_FAILED	"[F] "
	#elif (GPI_TRACE_TYPE_FORMAT == 3)
		#define GPI_TRACE_TYPE_FORMAT_FAILED	"\e[35m[F] "
	#else
		#define GPI_TRACE_TYPE_FORMAT_FAILED	""
	#endif
#endif

#define GPI_TRACE_TYPE_FORMAT_LUT			\
	{										\
		GPI_TRACE_TYPE_FORMAT_DEFAULT,		\
		GPI_TRACE_TYPE_FORMAT_FAILED,		\
		GPI_TRACE_TYPE_FORMAT_OK,			\
		GPI_TRACE_TYPE_FORMAT_ERROR,		\
		GPI_TRACE_TYPE_FORMAT_WARNING,		\
		GPI_TRACE_TYPE_FORMAT_INFO,			\
		GPI_TRACE_TYPE_FORMAT_VERBOSE,		\
		GPI_TRACE_TYPE_FORMAT_PROGRAM_FLOW,	\
		GPI_TRACE_TYPE_FORMAT_PROGRAM_FLOW	\
	}

ASSERT_CT_STATIC(1 == _GPI_TRACE_TYPE_INDEX(GPI_TRACE_MSG_TYPE_FAILED));
ASSERT_CT_STATIC(2 == _GPI_TRACE_TYPE_INDEX(GPI_TRACE_MSG_TYPE_OK));
ASSERT_CT_STATIC(3 == _GPI_TRACE_TYPE_INDEX(GPI_TRACE_MSG_TYPE_ERROR));
ASSERT_CT_STATIC(4 == _GPI_TRACE_TYPE_INDEX(GPI_TRACE_MSG_TYPE_WARNING));
ASSERT_CT_STATIC(5 == _GPI_TRACE_TYPE_INDEX(GPI_TRACE_MSG_TYPE_INFO));
ASSERT_CT_STATIC(6 == _GPI_TRACE_TYPE_INDEX(GPI_TRACE_MSG_TYPE_VERBOSE));
ASSERT_CT_STATIC(7 == _GPI_TRACE_TYPE_INDEX(GPI_TRACE_LOG_FUNCTION_RETURN_MSG));
ASSERT_CT_STATIC(8 == _GPI_TRACE_TYPE_INDEX(GPI_TRACE_LOG_FUNCTION_RETURN));

//**************************************************************************************************
//***** Forward Class and Struct Declarations ******************************************************



//**************************************************************************************************
//***** Global Typedefs and Class Declarations *****************************************************

/// @internal control block representing one source module
typedef struct Gpi_Trace_Module_Descriptor_tag
{
	uint32_t			msg_config;
//	const char*         module_name;
//	const char*         file_path;
//	unsigned long       module_version;		// take care: keep clean decoupling from version control

} Gpi_Trace_Module_Descriptor;

//**************************************************************************************************
//***** Global Variables ***************************************************************************



//**************************************************************************************************
//***** Prototypes of Global Functions *************************************************************

#ifdef __cplusplus
	extern "C" {
#endif

void gpi_trace_store_msg(
//				Gpi_Trace_Module_Descriptor*	module,
//				const char*						file,
//				int                         	line,
//				const char*                 	scope,
//				uint32_t		               	msg_group,
				const char*                 	msg,
				...);

void gpi_trace_print_all_msgs();

#ifdef __cplusplus
	}
#endif

//**************************************************************************************************
//**************************************************************************************************

#endif // __GPI_TRACE_H__
