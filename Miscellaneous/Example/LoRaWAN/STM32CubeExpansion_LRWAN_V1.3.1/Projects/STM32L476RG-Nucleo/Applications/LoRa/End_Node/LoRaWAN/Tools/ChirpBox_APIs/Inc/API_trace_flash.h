
#ifndef __TRACE_FLASH_H__
#define __TRACE_FLASH_H__
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "ll_flash.h"
#include "flash_if.h"

//**************************************************************************************************
//***** Includes ***********************************************************************************

/* Filename without full path */
#define __FILENAME__ (__FUNCTION__)

/* write trace data to flash */
#define TRACE_MODE_NO_FLASH                 0
#define TRACE_MODE_FLASH                    1

/* trace mode */
#ifndef TRACE_MODE
	#define TRACE_MODE						TRACE_MODE_FLASH
#endif

/* buffer length of filename */
#ifndef TRACE_LENGTH_FILENAME
	#define TRACE_LENGTH_FILENAME			14
#endif

/* buffer length of arguments */
#ifndef TRACE_LENGTH_ARGUMENT
	#define TRACE_LENGTH_ARGUMENT			64
#endif

/* size of TRACE buffer (number of elements) */
#ifndef TRACE_BUFFER_ELEMENTS
	#define TRACE_BUFFER_ELEMENTS			16
#endif

/* TRACE buffer entry size
implicitly determines number/size of possible var_args */
#ifndef TRACE_BUFFER_ENTRY_SIZE
	#define TRACE_BUFFER_ENTRY_SIZE			128
#endif

//**************************************************************************************************
//***** Global (Public) Defines and Consts *********************************************************

#define TRACE_MSG_FAST(fmt, args...)												\
		do {																		\
				printf(fmt, ##args);												\
                trace_store_msg(fmt, ##args);										\
            } while (0)

#define TRACE_FLUSH()																\
		do {																		\
                trace_to_flash();													\
            } while (0)

#define TRACE_MSG(fmt, args...)														\
		do {																		\
				printf(fmt, ##args);												\
                trace_store_msg(fmt, ##args);										\
                trace_to_flash();													\
            } while (0)

//**************************************************************************************************
//***** Local (Private) Defines and Consts *********************************************************

//**************************************************************************************************
//***** Forward Class and Struct Declarations ******************************************************

//**************************************************************************************************
//***** Global Typedefs and Class Declarations *****************************************************
/* 8-byte-alignment */
typedef struct Trace_Msg_tag
{
	uint8_t			file_name[TRACE_LENGTH_FILENAME];
	uint16_t		file_line;
	/* 16 bytes */
	uint8_t			arguments[TRACE_LENGTH_ARGUMENT];
	/* 80 bytes */
	int32_t			var_args[(TRACE_BUFFER_ENTRY_SIZE - (TRACE_LENGTH_FILENAME + sizeof(uint16_t) + TRACE_LENGTH_ARGUMENT)) / sizeof(int32_t)];
	/* 128 bytes */
} Trace_Msg;

//**************************************************************************************************
//***** Global Variables ***************************************************************************

//**************************************************************************************************
//***** Prototypes of Global Functions *************************************************************

//**************************************************************************************************
//***** Implementations of Inline Functions ********************************************************

#endif // __TRACE_FLASH_H__
