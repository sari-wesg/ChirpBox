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
 *	@file					gpi/platform.h
 *
 *	@brief					platform interface functions
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

#ifndef __GPI_STM32L476RG_PLATFORM_H__
#define __GPI_STM32L476RG_PLATFORM_H__

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "stm32l4xx_hal.h"
#include "stm32l4xx_hw_conf.h"

/*-------------------*/
#define USART_TX_Pin GPIO_PIN_2
#define USART_TX_GPIO_Port GPIOA
#define USART_RX_Pin GPIO_PIN_3
#define USART_RX_GPIO_Port GPIOA

#define USART3_TX_Pin GPIO_PIN_4
#define USART3_TX_GPIO_Port GPIOC
#define USART3_RX_Pin GPIO_PIN_5
#define USART3_RX_GPIO_Port GPIOC
/*gps-------------------*/
#define GPS_TRIGGER_Pin GPIO_PIN_2
#define GPS_TRIGGER_Port GPIOC
#define GPS_PPS_Pin GPIO_PIN_0
#define GPS_PPS_Port GPIOC
/*-------------------*/
#define LED_GPIO_Port GPIOC
#define LED1 GPIO_PIN_8
#define LED2 GPIO_PIN_6
#define LED3 GPIO_PIN_3
#define LED4 GPIO_PIN_10
#define LED5 GPIO_PIN_12
#define LED6 GPIO_PIN_11


//**************************************************************************************************
//***** Global (Public) Defines and Consts *********************************************************

#define GPI_LED_NONE	0
#define GPI_LED_1		LED1
#define GPI_LED_2		LED2
#define GPI_LED_3		LED3
#define GPI_LED_4		LED4
#define GPI_LED_5		LED5
#define GPI_LED_6		LED6

// #define GPI_LED_NONE	0
// #define GPI_LED_1		GPI_LED_NONE
// #define GPI_LED_2		GPI_LED_NONE
// #define GPI_LED_3		GPI_LED_NONE
// #define GPI_LED_4		GPI_LED_NONE
// #define GPI_LED_5		GPI_LED_NONE
// #define GPI_LED_6		GPI_LED_NONE

#define GPI_BUTTON(x)	x
/*
// ATTENTION: x is evaluated twice, so take care with side effects
#if 1	// DEFAULT connection
	#define GPI_BUTTON(x)	((x == 1) ? 11 : ((x == 2) ? 12 : ((x == 3) ? 24 : 25)))
#else	// OPTIONAL connection
	#define GPI_BUTTON(x)	((x == 1) ? -7 : ((x == 2) ? -8 : ((x == 3) ? 24 : 25)))
#endif
*/
//**************************************************************************************************
//***** Global Variables ***************************************************************************

// mark that CPU comes from power-down
// this flag can be evaluated by the application
// NOTE: to be meaningful, the first ISR taken after power-up should clear it
extern uint_fast8_t		gpi_wakeup_event;

//**************************************************************************************************
//***** Implementations of Inline Functions ********************************************************

static ALWAYS_INLINE void gpi_led_on(int mask)
{
	if (mask)
		LED_GPIO_Port->BSRR = mask;
}

static ALWAYS_INLINE void gpi_led_off(int mask)
{
	if (mask)
		LED_GPIO_Port->BRR = mask;
}

static ALWAYS_INLINE void gpi_led_toggle(int mask)
{
	if (mask)
		LED_GPIO_Port->ODR ^= mask;
}

//**************************************************************************************************

#endif // __GPI_STM32L476RG_PLATFORM_H__
