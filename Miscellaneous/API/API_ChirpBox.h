#ifndef __API_CHIRPBOX_H__
#define __API_CHIRPBOX_H__
//**************************************************************************************************
//**** Includes ************************************************************************************
#include <stddef.h>
#include <stdint.h>
#define ENERGEST_CONF_ON    1
#include "energest.h"
#include "flash_if.h"
#include "ll_flash.h"

//**************************************************************************************************
//***** Local Defines and Consts *******************************************************************


//**************************************************************************************************
//***** Global Defines and Consts *******************************************************************

//**************************************************************************************************
//***** Local Typedefs and Class Declarations ******************************************************

//**************************************************************************************************
//***** Global Typedefs and Class Declarations ******************************************************
typedef struct Flash_FILE_tag
{
    uint8_t  bank;
    uint32_t origin_page;
    uint32_t now_page;
    uint32_t file_size;
} Flash_FILE;

// need to first define file format
#ifndef JANPATCH_STREAM
#define JANPATCH_STREAM Flash_FILE // use POSIX FILE
#endif

#include "janpatch.h"

//GPS / RTC ********************************************************************************************

typedef struct Chirp_Time_tag
{
	uint16_t		chirp_year;
	uint8_t			chirp_month;
	uint8_t			chirp_date;
	uint8_t			chirp_day;
	uint8_t			chirp_hour;
	uint8_t			chirp_min;
	uint8_t			chirp_sec;
} Chirp_Time;

#include "ds3231.h"
#include "gps.h"

//**************************************************************************************************
//***** Global (Public) Defines and Consts *********************************************************
// API: logging

#define log_to_flash(fmt, args...)													\
		do {																		\
				printf(fmt, ##args);												\
                api_trace_store_msg(fmt, ##args);									\
            } while (0)

#define log_flush()																	\
		do {																		\
                api_trace_to_flash();												\
            } while (0)

#define log_to_serial(fmt, args...)													\
		do {																		\
				printf(fmt, ##args);												\
            } while (0)

// API: time
#define obtain_rtc_time()		api_obtain_rtc_time()

//**************************************************************************************************
//***** Forward Declarations ***********************************************************************

/* file manager */
int process_fread(janpatch_ctx *ctx, janpatch_buffer *source, size_t count, uint8_t *buffer);
int process_fwrite(janpatch_ctx *ctx, janpatch_buffer *target, size_t count, uint8_t *buffer);
/* file manager in flash */
size_t the_fwrite(const void *ptr, size_t size, size_t count, Flash_FILE *file);
size_t the_fread(void *ptr, size_t size, size_t count, Flash_FILE *file);
int the_fseek(Flash_FILE *file, long int offset, int origin);
/* logging */
void api_trace_store_msg(const char* fmt, ...);
void api_trace_to_flash();
/* time */
Chirp_Time api_obtain_rtc_time();
/* loudness sensor */
void MX_ADC1_Init(void);
uint16_t ADC_GetVoltage(void);
void ADC_CheckVoltage(void);
void ADC_GetLoud(void);
//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************

//**************************************************************************************************
//***** Global Variables ***************************************************************************

//**************************************************************************************************
//***** Local Functions ****************************************************************************

//**************************************************************************************************
//***** Global Functions ***************************************************************************

#endif /* __API_CHIRPBOX_H__ */
