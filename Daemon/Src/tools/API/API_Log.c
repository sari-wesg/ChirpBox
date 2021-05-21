//***** Trace Settings *****************************************************************************

#include "API_ChirpBox.h"

//**************************************************************************************************
#include <stdio.h>
#include <stdarg.h>
#include <stdarg.h>
#include <stdlib.h>
#include "stm32l476xx.h"

extern char	_estack [1];

//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************

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
//***** Global Variables ***************************************************************************
/* size of TRACE buffer (number of elements) */
#ifndef API_TRACE_BUFFER_ELEMENTS
	#define API_TRACE_BUFFER_ELEMENTS			32
#endif

/* buffer length of arguments */
#ifndef API_TRACE_LENGTH_ARGUMENT
	#define API_TRACE_LENGTH_ARGUMENT			32
#endif

/* TRACE buffer entry size
implicitly determines number/size of possible var_args */
#ifndef API_TRACE_BUFFER_ENTRY_SIZE
	#define API_TRACE_BUFFER_ENTRY_SIZE			FLASH_PAGE / API_TRACE_BUFFER_ELEMENTS
#endif

typedef struct API_Trace_Msg_tag
{
	uint8_t			arguments[API_TRACE_LENGTH_ARGUMENT];
	/* 32 bytes */
	int32_t			var_args[(API_TRACE_BUFFER_ENTRY_SIZE - (API_TRACE_LENGTH_ARGUMENT)) / sizeof(int32_t)];
	/* 64 bytes */
} API_Trace_Msg;

static API_Trace_Msg		    s_msg_queue[API_TRACE_BUFFER_ELEMENTS];
static volatile unsigned int	s_msg_queue_num_written = 0;
static volatile unsigned int	s_msg_queue_num_writing = 0;
//**************************************************************************************************
//***** Global Functions ***************************************************************************
void api_trace_store_msg(const char* fmt, ...)
{
	API_Trace_Msg *msg;
	unsigned int num_writing;
	int	ie;

	ie = trace_int_lock();	// implies REORDER_BARRIER() ...

    num_writing = s_msg_queue_num_writing++;

	trace_int_unlock(ie);		// implies REORDER_BARRIER() ...
    /* copy to the queue */
	msg = &s_msg_queue[num_writing % API_TRACE_BUFFER_ELEMENTS];
    memset((API_Trace_Msg *)&s_msg_queue[num_writing % API_TRACE_BUFFER_ELEMENTS], 0, sizeof(API_Trace_Msg));

	/* save arguments */
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

void api_trace_to_flash()
{
	uint8_t	num_read_start;
    int8_t i, k = 0;
	uint32_t trace_flash_address = FLASH_START_BANK1 + TRACE_PAGE * FLASH_PAGE;
	int	ie;
	ie = trace_int_lock();	// implies REORDER_BARRIER() ...

    /* scope of flash address that can be written */
	if ((TRACE_PAGE <= TOPO_PAGE) && (TRACE_PAGE >= 240))
	{
		// erase flash
		LL_FLASH_PageErase(TRACE_PAGE);

		/* loop the queue */
		num_read_start = (uint8_t)(s_msg_queue_num_written % API_TRACE_BUFFER_ELEMENTS) - 1;
		if (num_read_start == 0xFF)
			num_read_start = API_TRACE_BUFFER_ELEMENTS - 1;
		for (i = num_read_start; i >= 0; i--, k++)
		{
			LL_FLASH_Program64s(trace_flash_address + k * sizeof(API_Trace_Msg), (uint32_t *)(&s_msg_queue[i]), sizeof(API_Trace_Msg) / sizeof(uint32_t));
		}
		for (i = API_TRACE_BUFFER_ELEMENTS - 1; i > num_read_start; i--, k++)
		{
			LL_FLASH_Program64s(trace_flash_address + k * sizeof(API_Trace_Msg), (uint32_t *)(&s_msg_queue[i]), sizeof(API_Trace_Msg) / sizeof(uint32_t));
		}
	}

	trace_int_unlock(ie);		// implies REORDER_BARRIER() ...
}
