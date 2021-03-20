

#ifndef __CHIRPBOX_FUNC_H__
#define __CHIRPBOX_FUNC_H__

//**************************************************************************************************
//***** Includes ***********************************************************************************
#include <stdbool.h>
#include <stddef.h>
#include "flash_if.h"

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#include "chirpbox-param-settings.h"
#include "fut-param-settings.h"

//**************************************************************************************************
//***** Global Typedefs and Class Declarations *****************************************************
#ifndef JANPATCH_STREAM
#define JANPATCH_STREAM Flash_FILE // use POSIX FILE
#endif

typedef struct Flash_FILE_tag
{
    uint8_t  bank;
    uint32_t origin_page;
    uint32_t now_page;
    uint32_t file_size;
} Flash_FILE;

#include "janpatch.h"

//**************************************************************************************************
//***** Global (Public) Defines and Consts *********************************************************

extern volatile chirpbox_daemon_config __attribute((section (".ChirpBoxSettingSection"))) daemon_config;

extern volatile chirpbox_fut_config __attribute((section (".FUTSettingSection"))) fut_config;
//**************************************************************************************************
//***** Global Variables ***************************************************************************

extern ADC_HandleTypeDef hadc1;

//**************************************************************************************************
//***** Prototypes of Global Functions *************************************************************
/* bank manager */
uint32_t Bank1_WRP(uint32_t strtA_offset, uint32_t endA_offset);
uint32_t Bank1_nWRP(void);
void Bank_WRT_Check( void );
/* file manager in flash */
size_t the_fwrite(const void *ptr, size_t size, size_t count, Flash_FILE *file);
size_t the_fread(void *ptr, size_t size, size_t count, Flash_FILE *file);
int the_fseek(Flash_FILE *file, long int offset, int origin);
Flash_FILE Filepatch(uint8_t originalBank, uint32_t originalPage, uint32_t originalSize, uint8_t patchBank, uint32_t patchPage, uint32_t patchSize, uint8_t newBank, uint32_t newPage);
bool FirmwareUpgrade(uint8_t patch_update, uint8_t originalBank, uint32_t originalPage, uint32_t originalSize, uint8_t patchBank, uint32_t patchPage, uint32_t patchSize, uint8_t *md5_code, uint8_t file_compression);
/* compression with lzss */
uint32_t LZSS_encode(Flash_FILE *pbReadFileName, Flash_FILE *pbWriteFileName);  //文件压缩
uint32_t LZSS_decode(Flash_FILE *pbReadFileName, Flash_FILE *pbWriteFileName);  //文件解压
/* adc read voltage */
uint16_t ADC_GetVoltage(void);
void ADC_CheckVoltage(void);

#endif  /* __CHIRPBOX_FUNC_H__ */
