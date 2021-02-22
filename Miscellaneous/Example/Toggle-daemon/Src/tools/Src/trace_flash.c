//***** Trace Settings *****************************************************************************

#include "trace_flash.h"

#if (TRACE_MODE & TRACE_MODE_FLASH)
//**************************************************************************************************
#include <stdio.h>
#include <stdarg.h>
#include <stdarg.h>
#include <stdlib.h>
#include "stm32l476xx.h"

extern char	_estack [1];

//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************

static Trace_Msg			    s_msg_queue[TRACE_BUFFER_ELEMENTS];
static volatile unsigned int	s_msg_queue_num_written = 0;
static volatile unsigned int	s_msg_queue_num_writing = 0;
static volatile unsigned int	s_msg_queue_num_read = 0;

//**************************************************************************************************
//***** Local Functions ***************************************************************************
static inline __attribute__((always_inline)) void TRACE_REORDER_BARRIER()	{__asm volatile ("" : : : "memory"); }

static inline __attribute__((always_inline)) int trace_int_lock()
{
	register int ie;

	TRACE_REORDER_BARRIER();

	// NOTE: We do not use CMSIS functions at this point to avoid spill code in debug builds.
	// This may be a matter of taste (it is not absolutely necessary if performance is secondary).
	__ASM volatile
	(
		"mrs	%0, PRIMASK		\n"		// ie = __get_PRIMASK()
		"cpsid	i				\n"		// __set_PRIMASK(0) / __disable_irq()
		: "=r"(ie)
	);

	TRACE_REORDER_BARRIER();
	__DMB();

	return ie;
}


static inline __attribute__((always_inline)) void trace_int_unlock(int ie)
{
	TRACE_REORDER_BARRIER();
	__DMB();

	// NOTE: we expect ie as it has been returned by trace_int_lock()
	__set_PRIMASK(ie);

	TRACE_REORDER_BARRIER();
}

//**************************************************************************************************
//***** Global Functions ***************************************************************************

void trace_store_msg(const char* file_name, const int file_line, const char* fmt, ...)
{
	Trace_Msg *msg;
	unsigned int num_writing;
	int	ie;

	ie = trace_int_lock();	// implies REORDER_BARRIER() ...

    num_writing = s_msg_queue_num_writing++;

	trace_int_unlock(ie);		// implies REORDER_BARRIER() ...
    /* copy to the queue */
	msg = &s_msg_queue[num_writing % TRACE_BUFFER_ELEMENTS];
    memset((Trace_Msg *)&s_msg_queue[num_writing % TRACE_BUFFER_ELEMENTS], 0, sizeof(Trace_Msg));

    memcpy(msg->file_name, file_name, sizeof(msg->file_name));
    msg->file_line = file_line;
    memcpy(msg->arguments, fmt, sizeof(msg->arguments));

    /* save parameters */
    va_list va;
    va_start(va, fmt);
    size_t va_size_max = (uintptr_t)_estack - (uintptr_t)(va.__ap);
    if (sizeof(msg->var_args) > va_size_max)
        memcpy(msg->var_args, va.__ap, va_size_max);
    else memcpy(msg->var_args, va.__ap, sizeof(msg->var_args));
    va_end(va);

	ie = trace_int_lock();	// implies REORDER_BARRIER() ...

	if (s_msg_queue_num_written == num_writing)
		s_msg_queue_num_written = s_msg_queue_num_writing;

	trace_int_unlock(ie);		// implies REORDER_BARRIER() ...
}

void trace_to_flash(uint16_t trace_page)
{
	uint8_t	num_read_start;
    int8_t i, k = 0;
	uint32_t trace_flash_address = FLASH_START_BANK1 + trace_page * FLASH_PAGE;
	int	ie;
	ie = trace_int_lock();	// implies REORDER_BARRIER() ...
    /* scope of flash address that can be written */
	if ((trace_page <= TOPO_PAGE) && (trace_page >= 240))
	{
		// erase flash
		LL_FLASH_PageErase(trace_page);

		/* loop the queue */
		num_read_start = (uint8_t)(s_msg_queue_num_written % TRACE_BUFFER_ELEMENTS) - 1;
		if (num_read_start == 0xFF)
			num_read_start = TRACE_BUFFER_ELEMENTS - 1;
		for (i = num_read_start; i >= 0; i--, k++)
		{
			LL_FLASH_Program64s(trace_flash_address + k * sizeof(Trace_Msg), (uint32_t *)(&s_msg_queue[i]), sizeof(Trace_Msg) / sizeof(uint32_t));
		}
		for (i = TRACE_BUFFER_ELEMENTS - 1; i > num_read_start; i--, k++)
		{
			LL_FLASH_Program64s(trace_flash_address + k * sizeof(Trace_Msg), (uint32_t *)(&s_msg_queue[i]), sizeof(Trace_Msg) / sizeof(uint32_t));
		}
	}

	trace_int_unlock(ie);		// implies REORDER_BARRIER() ...
}

#endif // (TRACE_MODE & TRACE_MODE_TRACE)
