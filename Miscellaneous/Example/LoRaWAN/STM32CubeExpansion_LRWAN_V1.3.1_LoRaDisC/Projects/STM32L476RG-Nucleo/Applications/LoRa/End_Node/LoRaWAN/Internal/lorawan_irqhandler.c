
//**************************************************************************************************
//**** Includes ************************************************************************************
#include "lorawan_internal.h"

//**************************************************************************************************
//***** Local Defines and Consts *******************************************************************

//**************************************************************************************************
//***** Local Typedefs and Class Declarations ******************************************************

//**************************************************************************************************
//***** Forward Declarations ***********************************************************************

//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************

//**************************************************************************************************
//***** Global Variables ***************************************************************************
Chirpbox_ISR chirp_isr;

//**************************************************************************************************
//***** Local Functions ****************************************************************************

//**************************************************************************************************
//***** Global Functions ***************************************************************************

//Timer2 IRQ dispatcher
void __attribute__((naked)) MAIN_TIMER_ISR_NAME()
{
	__asm__ volatile
	(
		"ldr	r0, 1f						\n"		// r0 = chirp_isr.state
		"ldrb	r0, [r0]					\n"
		"add	pc, r0						\n"		// jump into vector table (see ARM DUI 0553A for details)
		".align 2							\n"		// ensure alignment and correct offset
		"b.w	mixer_main_timer_isr 		\n"		// 0: mixer_main_timer_isr (don't return to here)
		"nop								\n"
		"nop								\n"
		"b.w	lorawan_main_timer_isr	    \n"		// 8: lorawan_main_timer_isr (don't return to here)
		"1:									\n"
		".word	%c0							\n"
		: : "i"(&chirp_isr.state)
	);
}

//LP_Timer IRQ dispatcher
void __attribute__((naked)) LP_TIMER_ISR_NAME()
{
	__asm__ volatile
	(
		"ldr	r0, 1f						\n"		// r0 = chirp_isr.state
		"ldrb	r0, [r0]					\n"
		"add	pc, r0						\n"		// jump into vector table (see ARM DUI 0553A for details)
		".align 2							\n"		// ensure alignment and correct offset
		"b.w	mixer_lp_timer_isr	 		\n"		// 0: mixer_lp_timer_isr (don't return to here)
		"b.w	lpwan_lp_timer_isr	    	\n"		// 4: lpwan_lp_timer_isr (don't return to here)
		"1:									\n"
		".word	%c0							\n"
		: : "i"(&chirp_isr.state)
	);
}

//SX1276DIO0 IRQ dispatcher
void __attribute__((naked)) SX1276OnDio0Irq()
{
	__asm__ volatile
	(
		"ldr	r0, 1f						\n"		// r0 = chirp_isr.state
		"ldrb	r0, [r0]					\n"
		"add	pc, r0						\n"		// jump into vector table (see ARM DUI 0553A for details)
		".align 2							\n"		// ensure alignment and correct offset
		"b.w	mixer_dio0_isr		 		\n"		// 0: mixer_dio0_isr (don't return to here)
		"b.w	lpwan_dio0_isr				\n"		// 4: lpwan_dio0_isr (don't return to here)
		"b.w	lorawan_dio0_isr	    	\n"		// 8: lorawan_dio0_isr (don't return to here)
		"1:									\n"
		".word	%c0							\n"
		: : "i"(&chirp_isr.state)
	);
}

//SX1276DIO3 IRQ dispatcher
void __attribute__((naked)) SX1276OnDio3Irq()
{
	__asm__ volatile
	(
		"ldr	r0, 1f						\n"		// r0 = chirp_isr.state
		"ldrb	r0, [r0]					\n"
		"add	pc, r0						\n"		// jump into vector table (see ARM DUI 0553A for details)
		".align 2							\n"		// ensure alignment and correct offset
		"b.w	mixer_dio3_isr		 		\n"		// 0: mixer_dio3_isr (don't return to here)
		"b.w	lpwan_dio3_isr				\n"		// 4: lpwan_dio3_isr (don't return to here)
		"b.w	lorawan_dio3_isr		 	\n"		// 8: lorawan_dio3_isr (don't return to here)
		"1:									\n"
		".word	%c0							\n"
		: : "i"(&chirp_isr.state)
	);
}

// //SX1276DIO3 IRQ dispatcher
// void __attribute__((naked)) SX1276OnDio3Irq()
// {
// 	__asm__ volatile
// 	(
// 		"ldr	r0, 1f						\n"		// r0 = chirp_isr.state
// 		"ldrb	r0, [r0]					\n"
// 		"add	pc, r0						\n"		// jump into vector table (see ARM DUI 0553A for details)
// 		".align 2							\n"		// ensure alignment and correct offset
// 		"b.w	mixer_dio3_isr		 		\n"		// 0: mixer_dio3_isr (don't return to here)
// 		"1:									\n"
// 		".word	%c0							\n"
// 		: : "i"(&chirp_isr.state)
// 	);
// }

// //SX1276DIO3 IRQ dispatcher
// void __attribute__((naked)) SX1276OnDio3Irq()
// {
// 	__asm__ volatile
// 	(
// 		"b.w	mixer_dio3_isr		 		\n"		// 0: mixer_dio3_isr (don't return to here)
// 	);
// }
