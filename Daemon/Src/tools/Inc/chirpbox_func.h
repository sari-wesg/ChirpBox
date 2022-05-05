

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
#include "flash_if.h"
#include "stm32l4xx.h"
#include "mixer_internal.h"

//**************************************************************************************************
//***** Global Typedefs and Class Declarations *****************************************************
/**
  * @brief  Comm status structures definition
  */
typedef enum
{
  COM_OK       = 0x00,
  COM_ERROR    = 0x01,
  COM_ABORT    = 0x02,
  COM_TIMEOUT  = 0x03,
  COM_DATA     = 0x04,
  COM_LIMIT    = 0x05
} COM_StatusTypeDef;

//**************************************************************************************************
//***** Global (Public) Defines and Consts *********************************************************

extern volatile chirpbox_daemon_config __attribute((section (".ChirpBoxSettingSection"))) daemon_config;

extern volatile chirpbox_fut_config __attribute((section (".FUTSettingSection"))) fut_config;

//***** serial download ***************************************************************************************
#define PACKET_HEADER_SIZE      ((uint32_t)3)
#define PACKET_DATA_INDEX       ((uint32_t)4)
#define PACKET_START_INDEX      ((uint32_t)1)
#define PACKET_NUMBER_INDEX     ((uint32_t)2)
#define PACKET_CNUMBER_INDEX    ((uint32_t)3)
#define PACKET_TRAILER_SIZE     ((uint32_t)2)
#define PACKET_OVERHEAD_SIZE    (PACKET_HEADER_SIZE + PACKET_TRAILER_SIZE - 1)
#define PACKET_SIZE             ((uint32_t)128)
#define PACKET_1K_SIZE          ((uint32_t)1024)

/* /-------- Packet in data buffer -----------------------------------------\
 * | 0      |  1    |  2     |  3   |  4      | ... | n+4     | n+5  | n+6  |
 * |------------------------------------------------------------------------|
 * | unused | start | number | !num | data[0] | ... | data[n] | crc0 | crc1 |
 * \------------------------------------------------------------------------/
 * the first byte is left unused for memory alignment reasons                 */

#define FILE_NAME_LENGTH        ((uint32_t)64)
#define FILE_SIZE_LENGTH        ((uint32_t)16)

#define SOH                     ((uint8_t)0x01)  /* start of 128-byte data packet */
#define STX                     ((uint8_t)0x02)  /* start of 1024-byte data packet */
#define EOT                     ((uint8_t)0x04)  /* end of transmission */
#define ACK                     ((uint8_t)0x06)  /* acknowledge */
#define NAK                     ((uint8_t)0x15)  /* negative acknowledge */
#define CA                      ((uint32_t)0x18) /* two of these in succession aborts transfer */
#define CRC16                   ((uint8_t)0x43)  /* 'C' == 0x43, request 16-bit CRC */
#define NEGATIVE_BYTE           ((uint8_t)0xFF)

#define ABORT1                  ((uint8_t)0x41)  /* 'A' == 0x41, abort by user */
#define ABORT2                  ((uint8_t)0x61)  /* 'a' == 0x61, abort by user */

#define NAK_TIMEOUT             ((uint32_t)10000)
#define DOWNLOAD_TIMEOUT        ((uint32_t)1000) /* Ten second retry delay */
#define MAX_ERRORS              ((uint32_t)10)
//***** common ***************************************************************************************
#define TX_TIMEOUT          ((uint32_t)100)
#define RX_TIMEOUT          ((uint32_t)0x08001BC2)

#define IS_CAP_LETTER(c)    (((c) >= 'A') && ((c) <= 'F'))
#define IS_LC_LETTER(c)     (((c) >= 'a') && ((c) <= 'f'))
#define IS_09(c)            (((c) >= '0') && ((c) <= '9'))
#define ISVALIDHEX(c)       (IS_CAP_LETTER(c) || IS_LC_LETTER(c) || IS_09(c))
#define ISVALIDDEC(c)       IS_09(c)
#define CONVERTDEC(c)       (c - '0')

#define CONVERTHEX_ALPHA(c) (IS_CAP_LETTER(c) ? ((c) - 'A'+10) : ((c) - 'a'+10))
#define CONVERTHEX(c)       (IS_09(c) ? ((c) - '0') : CONVERTHEX_ALPHA(c))

//**************************************************************************************************
//***** Global Variables ***************************************************************************
extern UART_HandleTypeDef huart2;
extern CRC_HandleTypeDef CrcHandle;

#define UART_Handle       huart2
#define CRC_Handle        CrcHandle

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
/**************************************************************************************************/
void Flash_Bank_Copy_Bank(uint32_t FLASH_SRC, uint32_t FLASH_DEST, uint32_t firmware_size, uint8_t bank);

void menu_bank(void);
void menu_preSend(uint8_t bank);
uint32_t menu_serialDownload(uint32_t offset_page, uint8_t bank_update);
COM_StatusTypeDef menu_ymodem_receive(uint32_t *p_size, uint32_t bank, uint32_t offset);
uint8_t menu_pre_patch(uint8_t patch_bank, uint32_t old_firmware_size, uint32_t patch_firmware_size);
uint32_t menu_initiator_read_file(void);

/* chirpbox */
uint8_t menu_wait_task(Chirp_Outl *chirp_outl);
void chirp_controller_read_command(Chirp_Outl *chirp_outl);
void chirpbox_packet_write(Chirp_Outl *chirp_outl, uint8_t node_id);
void chirpbox_start(uint8_t node_id, uint8_t network_num_nodes);

#endif  /* __CHIRPBOX_FUNC_H__ */
