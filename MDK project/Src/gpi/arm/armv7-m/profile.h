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
 *	@file					gpi/arm/armv7-m/profile.h
 *
 *	@brief					support for program execution time profiling
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

#ifndef __GPI_ARMv7M_PROFILING_H__
#define __GPI_ARMv7M_PROFILING_H__

//**************************************************************************************************
//***** Includes ***********************************************************************************

#include "gpi/tools.h"

#include <stdint.h>
#include <string.h>

//**************************************************************************************************
//***** Global (Public) Defines and Consts *********************************************************

// use this macro once per file (on file scope, for each file with calls to GPI_PROFILE())
#define GPI_PROFILE_SETUP(_name_, _lines_, _granularity_)								\
																						\
	static const __attribute__((section("gpi_profile_info @"), used)) struct __attribute__((packed)) {	\
		uint8_t marker; uint16_t size; uint8_t type; uint8_t name[sizeof(_name_)];		\
	} gpi_profile_info_module = {														\
		0x81, 4 + sizeof(_name_), 3, _name_												\
	};																					\
																						\
	ASSERT_CT_STATIC(IS_POWER_OF_2(_granularity_), granularity_must_be_power_of_2);		\
	static const uint_fast8_t		gpi_profile_shift = MSB(_granularity_);				\
	static Gpi_Profile_Buffer_Entry	gpi_profile_buffer[(_lines_ + _granularity_ - 1) / _granularity_];	\
	static Gpi_Profile_Desc			gpi_profile_desc;									\
																						\
	static void __attribute__((constructor)) gpi_profile_setup() {						\
		gpi_profile_desc.prev = gpi_profile_desc_anchor.prev;							\
		gpi_profile_desc.next = &gpi_profile_desc_anchor;								\
		gpi_profile_desc.prev->next = &gpi_profile_desc;								\
		gpi_profile_desc.next->prev = &gpi_profile_desc;								\
		gpi_profile_desc.buffer = &gpi_profile_buffer[0];								\
		gpi_profile_desc.buffer_length = NUM_ELEMENTS(gpi_profile_buffer);				\
		gpi_profile_desc.module_name = _name_;											\
		memset(gpi_profile_buffer, 0, sizeof(gpi_profile_buffer));						\
    }

// record timestamp
#define GPI_PROFILE(priority_level, ...)										\
	do {																		\
		__asm__ volatile (														\
			".section gpi_profile_info				\n"							\
			"1:										\n"							\
			".byte 0x81								\n"	/* marker */			\
			".2byte %c0								\n"	/* size */				\
			".byte 4								\n"	/* type */				\
			".2byte gpi_profile_info_module - 1b	\n"	/* module_link */		\
			".2byte " STRINGIFY(__LINE__) "			\n"	/* line */				\
			".4byte %c1								\n"	/* function_id */		\
			".byte %c2								\n"	/* priority_level */	\
			".ascii " #__VA_ARGS__ " \"\\0\"		\n"	/* name */				\
			".previous								\n"							\
			: : "i"(13 + sizeof("" __VA_ARGS__)), "i"(__func__), "i"(priority_level)	\
		);																		\
																				\
		ASSERT_CT((__LINE__ >> gpi_profile_shift) < NUM_ELEMENTS(gpi_profile_buffer), profile_buffer_overflow);	\
																				\
		register int t1, t2;													\
		__asm__ volatile (														\
			"mov		%0, #0xE000E000				\n"							\
			"ldr		%1, =gpi_profile_buffer		\n"							\
			"ldr		%0, [%0, #0x18]				\n"							\
			"str		%0, [%1, %2]				\n"							\
			"movw		%0, " STRINGIFY(__LINE__) "	\n"							\
			"str		%0, [%1, %2 + 4]			\n"							\
			: "=&r"(t1), "=&r"(t2)												\
			: "i"((__LINE__ >> gpi_profile_shift) << 3)							\
			/*: "memory"*/														\
		);																		\
	} while (0)

//**************************************************************************************************
//***** Local (Private) Defines and Consts *********************************************************



//**************************************************************************************************
//***** Forward Class and Struct Declarations ******************************************************

struct Gpi_Profile_Desc_;

//**************************************************************************************************
//***** Global Typedefs and Class Declarations *****************************************************

typedef struct Gpi_Profile_Buffer_Entry_
{
	uint32_t	timestamp;
	uint16_t	line;
	uint16_t	_unused;

} Gpi_Profile_Buffer_Entry;

//**************************************************************************************************

typedef struct Gpi_Profile_Desc_
{
	struct Gpi_Profile_Desc_	*prev;
	struct Gpi_Profile_Desc_	*next;
	Gpi_Profile_Buffer_Entry	*buffer;
	uint32_t					buffer_length;
	const char					*module_name;
		
} Gpi_Profile_Desc;

//**************************************************************************************************

typedef struct Gpi_Profile_Ticket_
{
	Gpi_Profile_Desc	*desc;
	uint_fast32_t		index;
	
} Gpi_Profile_Ticket;

//**************************************************************************************************
//***** Global Variables ***************************************************************************

extern Gpi_Profile_Desc 	gpi_profile_desc_anchor;

//**************************************************************************************************
//***** Prototypes of Global Functions *************************************************************

#ifdef __cplusplus
	extern "C" {
#endif

// use memset(&ticket, 0, sizeof(ticket)) to (re-)initialize ticket (e.g. before first call)
int_fast8_t 	gpi_profile_read(
					Gpi_Profile_Ticket	*ticket,
					const char			**module_name,
					uint16_t			*line,
					uint32_t			*timestamp);

#ifdef __cplusplus
	}
#endif

//**************************************************************************************************
//***** Implementations of Inline Functions ********************************************************



//**************************************************************************************************
//**************************************************************************************************

#endif // __GPI_ARMv7M_PROFILING_H__
