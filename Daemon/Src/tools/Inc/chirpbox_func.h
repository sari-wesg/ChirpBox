

#ifndef __CHIRPBOX_FUNC_H__
#define __CHIRPBOX_FUNC_H__

//**************************************************************************************************
//***** Includes ***********************************************************************************
#include <stdbool.h>

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif
#include "API_ChirpBox.h"

#include "API_DaemonParam.h"
#include "API_FUTParam.h"

//**************************************************************************************************
//***** Global Typedefs and Class Declarations *****************************************************

//**************************************************************************************************
//***** Global (Public) Defines and Consts *********************************************************

extern volatile chirpbox_daemon_config __attribute((section (".ChirpBoxSettingSection"))) daemon_config;

extern volatile chirpbox_fut_config __attribute((section (".FUTSettingSection"))) fut_config;
//**************************************************************************************************
//***** Global Variables ***************************************************************************

extern ADC_HandleTypeDef hadc1;

//**************************************************************************************************
//***** Prototypes of Global Functions *************************************************************
/* Bank manager */
uint32_t Bank1_WRP(uint32_t strtA_offset, uint32_t endA_offset);
uint32_t Bank1_nWRP(void);
void Bank_WRT_Check( void );
/* Patch */
Flash_FILE Filepatch(uint8_t originalBank, uint32_t originalPage, uint32_t originalSize, uint8_t patchBank, uint32_t patchPage, uint32_t patchSize, uint8_t newBank, uint32_t newPage);
bool FirmwareUpgrade(uint8_t patch_update, uint8_t originalBank, uint32_t originalPage, uint32_t originalSize, uint8_t patchBank, uint32_t patchPage, uint32_t patchSize, uint8_t *md5_code, uint8_t file_compression);
/* Compression with lzss */
uint32_t LZSS_encode(Flash_FILE *pbReadFileName, Flash_FILE *pbWriteFileName);  // File compression
uint32_t LZSS_decode(Flash_FILE *pbReadFileName, Flash_FILE *pbWriteFileName);  // File decompression
/* ADC read voltage */
uint16_t ADC_GetVoltage(void);
void ADC_CheckVoltage(void);

#endif  /* __CHIRPBOX_FUNC_H__ */
