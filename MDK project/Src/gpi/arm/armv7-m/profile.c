/***************************************************************************************************
 ***************************************************************************************************
 *
 *	Copyright (c) 2018, Networked Embedded Systems Lab, TU Dresden
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
 *	@file					gpi/arm/armv7-m/profile.c
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
//***** Trace Settings *****************************************************************************



//**************************************************************************************************
//**** Includes ************************************************************************************

#include "gpi/profile.h"
#include "gpi/interrupts.h"

//**************************************************************************************************
//***** Local Defines and Consts *******************************************************************



//**************************************************************************************************
//***** Local Typedefs and Class Declarations ******************************************************



//**************************************************************************************************
//***** Forward Declarations ***********************************************************************



//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************

static const __attribute__((section("gpi_profile_info @"), used)) struct __attribute__((packed))
{
	uint8_t		marker;
	uint16_t	size;
	uint8_t		type;

	uint32_t	timestamp_frequency;
	uint32_t	timestamp_range;

} gpi_profile_info_setup = {0x81, 12, 2, 64000000, 0x00ffffff};

//**************************************************************************************************
//***** Global Variables ***************************************************************************

Gpi_Profile_Desc	gpi_profile_desc_anchor
						= {&gpi_profile_desc_anchor, &gpi_profile_desc_anchor, 0, 0, 0};

//**************************************************************************************************
//***** Local Functions ****************************************************************************



//**************************************************************************************************
//***** Global Functions ***************************************************************************

// read next valid timestamp and update ticket
int_fast8_t gpi_profile_read(Gpi_Profile_Ticket *ticket, const char **module_name, uint16_t *line, uint32_t *timestamp)
{
	if (0 == ticket->desc)
	{
		ticket->desc = gpi_profile_desc_anchor.next;
		ticket->index = 0;
	}

	while (ticket->desc != &gpi_profile_desc_anchor)
	{
		Gpi_Profile_Buffer_Entry 	*p = &(ticket->desc->buffer[ticket->index]);
		Gpi_Profile_Buffer_Entry 	*p_end = &(ticket->desc->buffer[ticket->desc->buffer_length]);

		for (; p < p_end; ++p)
		{
			if (0 != p->line)
			{
				*module_name = ticket->desc->module_name;

				register int a, b;
				register int ie = gpi_int_lock();

				a = p->line;
				b = p->timestamp;

				gpi_int_unlock(ie);

				*line = a;
				*timestamp = 0x00ffffff - b;

				ticket->index = ARRAY_INDEX(p, ticket->desc->buffer) + 1;
				return 1;
			}
		}

		ticket->desc = ticket->desc->next;
		ticket->index = 0;
    }

	ticket->desc = 0;
	return 0;
}

//**************************************************************************************************
//**************************************************************************************************
