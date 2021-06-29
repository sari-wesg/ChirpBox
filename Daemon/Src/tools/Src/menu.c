/**
  ******************************************************************************
  * @file    STM32L476G_EVAL/DualBank/src/menu.c
  * @author  MCD Application Team
  * @version 1.0.0
  * @date    15-September-2016
  * @brief   This file provides the software which contains the main menu routine.
  *          The main menu gives the options of:
  *             - downloading a new binary file,
  *             - uploading internal flash memory,
  *             - executing the binary file already loaded
  *             - configuring the write protection of the Flash sectors where the
  *               user loads his binary file.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2016 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/** @addtogroup STM32L4xx_DB
  * @{
  */

//**************************************************************************************************
//***** Includes ***********************************************************************************

#include "menu.h"
#include "string.h"
#include "mixer/mixer_internal.h"
#include "md5.h"
#include "chirpbox_func.h"
#include "loradisc.h"
#include <math.h>
//**************************************************************************************************
//***** Global Variables ****************************************************************************
volatile uint8_t uart_read_done;
uint8_t gps_time_str[4];

//**************************************************************************************************
//***** Local (Private) Defines and Consts *********************************************************
#if DEBUG_CHIRPBOX
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif
#include "gpi/olf.h"

//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************
uint32_t BankActive = 0;
FLASH_OBProgramInitTypeDef OBConfig;
uint8_t aFileName[FILE_NAME_LENGTH];

/* @note ATTENTION - please keep this variable 32bit alligned */
#if defined ( __ICCARM__ )    /* IAR Compiler */
#pragma data_alignment=4
#elif defined ( __CC_ARM )   /* ARM Compiler */
__align(4)
#elif defined ( __GNUC__ ) /* GCC Compiler */
__attribute__ ((aligned (4)))
#endif
uint8_t aPacketData[PACKET_1K_SIZE + PACKET_DATA_INDEX + PACKET_TRAILER_SIZE];
#ifdef ENCRYPT
#if defined ( __ICCARM__ )    /* IAR Compiler */
#pragma data_alignment=4
#elif defined ( __CC_ARM )   /* ARM Compiler */
__align(4)
#elif defined ( __GNUC__ ) /* GCC Compiler */
__attribute__ ((aligned (4)))
#endif
uint8_t aDecryptData[PACKET_1K_SIZE];
#endif

//**************************************************************************************************
//***** Prototypes of Local Functions **************************************************************
static void Int2Str(uint8_t *p_str, uint32_t intnum);
static uint32_t Str2Int(uint8_t *p_inputstr, uint32_t *p_intnum);
static void Serial_PutString(uint8_t *p_string);
static HAL_StatusTypeDef Serial_PutByte( uint8_t param );
static HAL_StatusTypeDef ReceivePacket(uint8_t *p_data, uint32_t *p_length, uint32_t timeout);
static uint32_t CheckOtherBank( void );

//**************************************************************************************************
//***** Local Functions ***************************************************************************
//**************************************************************************************************
/**
  * @brief  Convert an Integer to a string
  * @param  p_str: The string output pointer
  * @param  intnum: The integer to be converted
  * @retval None
  */
static void Int2Str(uint8_t *p_str, uint32_t intnum)
{
  uint32_t i, divider = 1000000000, pos = 0, status = 0;

  for (i = 0; i < 10; i++)
  {
    p_str[pos++] = (intnum / divider) + 48;

    intnum = intnum % divider;
    divider /= 10;
    if ((p_str[pos-1] == '0') & (status == 0))
    {
      pos = 0;
    }
    else
    {
      status++;
    }
  }
}

/**
  * @brief  Convert a string to an integer
  * @param  p_inputstr: The string to be converted
  * @param  p_intnum: The integer value
  * @retval 1: Correct
  *         0: Error
  */
static uint32_t Str2Int(uint8_t *p_inputstr, uint32_t *p_intnum)
{
  uint32_t i = 0, res = 0;
  uint32_t val = 0;

  if ((p_inputstr[0] == '0') && ((p_inputstr[1] == 'x') || (p_inputstr[1] == 'X')))
  {
    i = 2;
    while ( ( i < 11 ) && ( p_inputstr[i] != '\0' ) )
    {
      if (ISVALIDHEX(p_inputstr[i]))
      {
        val = (val << 4) + CONVERTHEX(p_inputstr[i]);
      }
      else
      {
        /* Return 0, Invalid input */
        res = 0;
        break;
      }
      i++;
    }

    /* valid result */
    if (p_inputstr[i] == '\0')
    {
      *p_intnum = val;
      res = 1;
    }
  }
  else /* max 10-digit decimal input */
  {
    while ( ( i < 11 ) && ( res != 1 ) )
    {
      if (p_inputstr[i] == '\0')
      {
        *p_intnum = val;
        /* return 1 */
        res = 1;
      }
      else if (((p_inputstr[i] == 'k') || (p_inputstr[i] == 'K')) && (i > 0))
      {
        val = val << 10;
        *p_intnum = val;
        res = 1;
      }
      else if (((p_inputstr[i] == 'm') || (p_inputstr[i] == 'M')) && (i > 0))
      {
        val = val << 20;
        *p_intnum = val;
        res = 1;
      }
      else if (ISVALIDDEC(p_inputstr[i]))
      {
        val = val * 10 + CONVERTDEC(p_inputstr[i]);
      }
      else
      {
        /* return 0, Invalid input */
        res = 0;
        break;
      }
      i++;
    }
  }

  return res;
}

/**
  * @brief  Print a string on the HyperTerminal
  * @param  p_string: The string to be printed
  * @retval None
  */
static void Serial_PutString(uint8_t *p_string)
{
  uint16_t length = 0;

  while (p_string[length] != '\0')
  {
    length++;
  }
  HAL_UART_Transmit(&UART_Handle, p_string, length, TX_TIMEOUT);
}

/**
  * @brief  Transmit a byte to the HyperTerminal
  * @param  param The byte to be sent
  * @retval HAL_StatusTypeDef HAL_OK if OK
  */
static HAL_StatusTypeDef Serial_PutByte( uint8_t param )
{
  /* May be timeouted... */
  if ( UART_Handle.gState == HAL_UART_STATE_TIMEOUT )
  {
    UART_Handle.gState = HAL_UART_STATE_READY;
  }
  return HAL_UART_Transmit(&UART_Handle, &param, 1, TX_TIMEOUT);
}

/**
  * @brief  Receive a packet from sender
  * @param  p_data
  * @param  p_length
  *     0: end of transmission
  *     2: abort by sender
  *    >0: packet length
  * @param  timeout
  * @retval HAL_OK: normally return
  *         HAL_BUSY: abort by user
  */
static HAL_StatusTypeDef ReceivePacket(uint8_t *p_data, uint32_t *p_length, uint32_t timeout)
{
  uint32_t crc;
  uint32_t packet_size = 0;
  HAL_StatusTypeDef status;
  uint8_t char1;

  *p_length = 0;
  status = HAL_UART_Receive(&UART_Handle, &char1, 1, timeout);
  while (UART_Handle.RxState == HAL_UART_STATE_BUSY_RX);

  if (status == HAL_OK)
  {
    switch (char1)
    {
      case SOH:
        packet_size = PACKET_SIZE;
        break;
      case STX:
        packet_size = PACKET_1K_SIZE;
        break;
      case EOT:
        break;
      case CA:
        if ((HAL_UART_Receive(&UART_Handle, &char1, 1, timeout) == HAL_OK) && (char1 == CA))
        {
          packet_size = 2;
        }
        else
        {
          status = HAL_ERROR;
        }
        break;
      case ABORT1:
      case ABORT2:
        status = HAL_BUSY;
        break;
      default:
        status = HAL_ERROR;
        break;
    }
    *p_data = char1;

    if (packet_size >= PACKET_SIZE )//data frame
    {
      status = HAL_UART_Receive(&UART_Handle, &p_data[PACKET_NUMBER_INDEX], packet_size + PACKET_OVERHEAD_SIZE, timeout);

      /* Simple packet sanity check */
      if (status == HAL_OK )
      {
        if (p_data[PACKET_NUMBER_INDEX] != ((p_data[PACKET_CNUMBER_INDEX]) ^ NEGATIVE_BYTE))
        {
          packet_size = 0;
          status = HAL_ERROR;
        }
        else
        {
          /* Check packet CRC */
          crc = p_data[ packet_size + PACKET_DATA_INDEX ] << 8;
          crc += p_data[ packet_size + PACKET_DATA_INDEX + 1 ];

          if (HAL_CRC_Calculate(&CRC_Handle, (uint32_t*)&p_data[PACKET_DATA_INDEX], packet_size) != crc )
          {
            packet_size = 0;
            status = HAL_ERROR;
          }
        }
      }
      else
      {
        packet_size = 0;
      }
    }
  }
  *p_length = packet_size;
  return status;
}

/**
 * @description: verify if the file is STM32 boot file (begin with 0x20)
 * @param
 * @return: state of the verification
 */
static uint32_t CheckOtherBank( void )
{
  uint32_t result;
  result = FLASH_If_Check_old((BankActive == 1) ? FLASH_START_BANK1 : FLASH_START_BANK2);
  if (result == FLASHIF_OK)
    Serial_PutString((uint8_t *)"Success!\r\n\n");
  else
    Serial_PutString((uint8_t *)"Failure!\r\n\n");
  return result;
}

/**
  * @brief  Copy firmware to another bank
  * @param  None
  * @retval None
  */
void Flash_Bank_Copy_Bank(uint32_t FLASH_SRC, uint32_t FLASH_DEST, uint32_t firmware_size, uint8_t bank)
{
  // uint32_t firmware_size;
  uint32_t n, round;
  // uint32_t FLASH_SRC, FLASH_DEST, FIRMWARE_SIZE;
  uint32_t firmware_file_buffer[64];

  /* erase another bank */
  if (bank)
    menu_preSend(1);
  // else
  //   menu_preSend(0);

  round = (firmware_size + sizeof(firmware_file_buffer) - 1) / sizeof(firmware_file_buffer);
  PRINTF("copy round:%lu, %lu\n", round, firmware_size);
  for (n = round; n > 0; n--)
  {
    PRINTF("%lu, ", (n - 1));
    memcpy(firmware_file_buffer, (__IO uint32_t*)(FLASH_SRC + (n - 1) * sizeof(firmware_file_buffer)), sizeof(firmware_file_buffer));

    FLASH_If_Write(FLASH_DEST + (n - 1) * sizeof(firmware_file_buffer), (uint32_t *)(firmware_file_buffer), sizeof(firmware_file_buffer) / sizeof(uint32_t));
  }
  PRINTF("\n");

  if (bank)
  {
    uint32_t firmware_size_buffer[1];
    firmware_size_buffer[0] = firmware_size;
    FLASH_If_Write(FIRMWARE_FLASH_ADDRESS_2, (uint32_t *)firmware_size_buffer, 2);
  }
}
//**************************************************************************************************
//***** Global Functions ***************************************************************************
//**************************************************************************************************
/**
  * @brief  Indicate current bank
  * @param  None
  * @retval None
  */
void menu_bank(void)
{
  /* Test from which bank the program runs */
  BankActive = READ_BIT(SYSCFG->MEMRMP, SYSCFG_MEMRMP_FB_MODE);

  Serial_PutString((uint8_t *)"\r\n========__DATE__ __TIME__ = " __DATE__ " " __TIME__ " ============\r\n\n");
  printf("\r\n========version: 0x%04x ========\r\n\n", daemon_config.DAEMON_version);
	if (BankActive == 0)
  {
    Serial_PutString((uint8_t *)"\tSystem running from STM32L476 *Bank 1*  \r\n\n");
  }
  else
  {
    Serial_PutString((uint8_t *)"\tSystem running from STM32L476 *Bank 2*  \r\n\n");
    #if BANK_1_RUN
    uint32_t firmware_size = *(__IO uint32_t*)(FIRMWARE_FLASH_ADDRESS_1);
    PRINTF("firmware_size:%lu\n", firmware_size);
    if ((firmware_size < 0x100000) && (firmware_size))
      Flash_Bank_Copy_Bank(FLASH_START_BANK1, FLASH_START_BANK2, firmware_size, 1);
    DS3231_GetTime();
    DS3231_ShowTime();
    while(1);
    #endif
  }

  // if ( OBConfig.USERConfig & OB_BFB2_ENABLE ) /* BANK2 active for boot */
  //   Serial_PutString((uint8_t *)"\tSystem ROM bank selection active  \r\n\n");
  // else
  //   Serial_PutString((uint8_t *)"\tSystem ROM bank selection deactivated \r\n\n");
}

/**
  * @brief  Erase the Flash bank and clear the protection
  * @param  None
  * @retval None
  */
void menu_preSend(uint8_t bank)
{
  /* Clean the input path */
  __HAL_UART_FLUSH_DRREGISTER(&UART_Handle);
  __HAL_UART_CLEAR_IT(&UART_Handle, UART_CLEAR_OREF);
  if (bank)
    FLASH_If_Erase(BankActive);
}

/**
  * @brief  Download a file via serial port. Should be used after menu_preSend.
  * @param  offset_page: The download offset page of the flash (0-255)
  * @retval size: The file size
  */
uint32_t menu_serialDownload(uint32_t offset_page, uint8_t bank_update)
{
  gpi_watchdog_periodic();
  uint8_t number[11] = {0};
  uint32_t size = 0;
  COM_StatusTypeDef result;

  uint32_t bank_active;

  /* if update bank1 ("0"), set bank_active as "1" to write file to FLASH_START_BANK1 */
  if (!bank_update)
    bank_active = 1;
  else
    bank_active = 0;

  Serial_PutString((uint8_t *)"Waiting for the file to be sent ... (press 'a' to abort)\n\r");
  result = COM_ABORT;
  do
  {
    result = menu_ymodem_receive( &size, bank_active, FLASH_PAGE * offset_page);
  } while (result != COM_OK);
  // result = menu_ymodem_receive( &size, bank_active, FLASH_PAGE * offset_page);
  if (result == COM_OK)
  {
    /* Reporting */
    Serial_PutString((uint8_t *) "\n\n\r Programming Completed Successfully!\n\r--------------------------------\r\n Name: ");
    Serial_PutString(aFileName);
    Int2Str(number, size);
    Serial_PutString((uint8_t *)"\n\r Size: ");
    Serial_PutString(number);
    Serial_PutString((uint8_t *)" Bytes\r\n");
    Serial_PutString((uint8_t *)"-------------------\n");
  }
  // else if (result == COM_LIMIT)
  //   Serial_PutString((uint8_t *)"\n\n\rThe image size is higher than the bank size!\n\r");
  // else if (result == COM_DATA)
  //   Serial_PutString((uint8_t *)"\n\n\rVerification failed!\n\r");
  // else if (result == COM_ABORT)
  //   Serial_PutString((uint8_t *)"\n\rAborted by user.\n\r");
  // else
  //   Serial_PutString((uint8_t *)"\n\rFailed to receive the file!\n\r");
  return size;
}

/**
  * @brief  Receive a file using the ymodem protocol with CRC16.
  * @param  p_size The size of the file.
  * @param  bank The actual active bank
  * @param  offset The offset of the flash
  * @retval COM_StatusTypeDef result of reception/programming
  */
COM_StatusTypeDef menu_ymodem_receive( uint32_t *p_size, uint32_t bank, uint32_t offset)
{
  gpi_watchdog_periodic();
  uint32_t i, packet_length, session_done = 0, file_done, errors = 0, session_begin = 0;
  uint32_t flashdestination, ramsource, filesize;
  uint8_t *file_ptr;
  uint8_t file_size[FILE_SIZE_LENGTH], tmp, packets_received;
  COM_StatusTypeDef result = COM_OK;

  SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;

  /* Initialize flashdestination variable */
  if (bank == 1 )
  {
    flashdestination = FLASH_START_BANK1 + offset;
  }
  else
  {
    flashdestination = FLASH_START_BANK2 + offset;
  }

  while ((session_done == 0) && (result == COM_OK))
  {
    packets_received = 0;
    file_done = 0;
    while ((file_done == 0) && (result == COM_OK))
    {
      switch (ReceivePacket(aPacketData, &packet_length, DOWNLOAD_TIMEOUT))
      {
        case HAL_OK:
          errors = 0;
          switch (packet_length)
          {
            case 2:
              /* Abort by sender */
              Serial_PutByte(ACK);
              result = COM_ABORT;
              break;
            case 0:
              /* End of transmission */
              Serial_PutByte(ACK);
              file_done = 1;
              break;
            default:
              /* Normal packet */
              if (aPacketData[PACKET_NUMBER_INDEX] != packets_received)
              {
                Serial_PutByte(NAK);
              }
              else
              {
                if (packets_received == 0)
                {
                  /* File name packet */
                  if (aPacketData[PACKET_DATA_INDEX] != 0)
                  {
                    /* File name extraction */
                    i = 0;
                    file_ptr = aPacketData + PACKET_DATA_INDEX;
                    while ( (*file_ptr != 0) && (i < FILE_NAME_LENGTH))
                    {
                      aFileName[i++] = *file_ptr++;
                    }

                    /* File size extraction */
                    aFileName[i++] = '\0';
                    i = 0;
                    file_ptr ++;
                    while ( (*file_ptr != ' ') && (i < FILE_SIZE_LENGTH))
                    {
                      file_size[i++] = *file_ptr++;
                    }
                    file_size[i++] = '\0';
                    Str2Int(file_size, &filesize);

                    /* Test the size of the image to be sent */
                    /* Image size is greater than Flash size */
                    *p_size = filesize;
                    if (*p_size > (FLASH_START_BANK2 - FLASH_START_BANK1))
                    {
                      /* End session */
                      tmp = CA;
                      HAL_UART_Transmit(&UART_Handle, &tmp, 1, NAK_TIMEOUT);
                      HAL_UART_Transmit(&UART_Handle, &tmp, 1, NAK_TIMEOUT);
                      result = COM_LIMIT;
                    }
                    else
                    {
                      /* erase destination area - always the other bank mapped on 0x08018000*/
                      // FLASH_If_Erase(bank);
                      Serial_PutByte(ACK);
                      Serial_PutByte(CRC16);
                    }
                  }
                  /* File header packet is empty, end session */
                  else
                  {
                    Serial_PutByte(ACK);
                    file_done = 1;
                    session_done = 1;
                    break;
                  }
                }
                else /* Data packet */
                {
#ifdef ENCRYPT
                  if (HAL_CRYPEx_AES( &DecHandle, &aPacketData[PACKET_DATA_INDEX], packet_length, &aDecryptData[0], NAK_TIMEOUT) != HAL_OK)
                  {
                    /* End session */
                    Serial_PutByte(CA);
                    Serial_PutByte(CA);
                    result = COM_DATA;
                    break;
                  }
                  ramsource = (uint32_t) & aDecryptData;
#else
                  ramsource = (uint32_t) & aPacketData[PACKET_DATA_INDEX];
#endif
                  /* Write received data in Flash */
                  if (FLASH_If_Write(flashdestination, (uint32_t*) ramsource, packet_length / 4) == FLASHIF_OK)
                  {
                    flashdestination += packet_length;
                    Serial_PutByte(ACK);
                  }
                  else /* An error occurred while writing to Flash memory */
                  {
                    /* End session */
                    Serial_PutByte(CA);
                    Serial_PutByte(CA);
                    result = COM_DATA;
                  }
                }
                packets_received ++;
                session_begin = 1;
              }
              break;
          }
          break;
        case HAL_BUSY: /* Abort actually */
          Serial_PutByte(CA);
          Serial_PutByte(CA);
          result = COM_ABORT;
          break;
        default:
          if (session_begin > 0)
          {
            errors ++;
          }
          if (errors > MAX_ERRORS)
          {
            /* Abort communication */
            Serial_PutByte(CA);
            Serial_PutByte(CA);
            result = COM_ABORT;
          }
          else
          {
            Serial_PutByte(CRC16); /* Ask for a packet */
          }
          break;
      }
    }
  }
  SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;

  return result;
}

//**************************************************************************************************

uint8_t menu_pre_patch(uint8_t patch_bank, uint32_t old_firmware_size, uint32_t patch_firmware_size)
{
  /* Prepare for download files */
  menu_preSend(0);

  /* chose the place to write patch file */
  uint8_t patch_page = (old_firmware_size + FLASH_PAGE - 1) / FLASH_PAGE + 5;
  uint8_t patch_used_page = (patch_firmware_size + FLASH_PAGE - 1) / FLASH_PAGE;

  /* erase flash pages that may be used */
  uint8_t i;

  if (!patch_bank)
  {
    for (i = patch_page; i < patch_page + patch_used_page; i++)
      FLASH_If_Erase_Pages(1, i);
  }
  else
  {
    for (i = patch_page; i < patch_page + patch_used_page; i++)
      FLASH_If_Erase_Pages(0, i);
  }

  return patch_page;
}

/**
 * @description: read file through Ymodem and set the file at the other bank 0 page
 * @param
 * @return: size of the downloaded file
 */
uint32_t menu_initiator_read_file(void)
{
  /* initiator needs to erase bank2 */
  /* erase bank2 */
  FLASH_If_Erase(0);
  /* write file to bank2 */
  uint32_t size = menu_serialDownload(0, 1);
  // CheckOtherBank();
  return size;
}

/**
 * @description: To chose the task according through serial input
 * @param
 * @return: None
 */
uint8_t menu_wait_task(Chirp_Outl *chirp_outl)
{
  Mixer_Task mx_task;
  uint8_t default_sf;
  uint8_t default_payload_len;
  uint8_t default_generate_size;
  int8_t default_tp;
  uint16_t default_slot_num;
  uint8_t dissem_back_sf;
  uint8_t dissem_back_slot_num;
  uint8_t task_wait = 0;

  uint8_t task[28 + DISSEM_BITMAP_32 * 8 + DISSEM_BITMAP_32 * 8 + 1];
  PRINTF("\nTask list:\n%d: CHIRP_START\n%d: MX_DISSEMINATE\n%d: MX_COLLECT\n%d: CHIRP_CONNECTIVITY\n%d: CHIRP_VERSION\n", CHIRP_START, MX_DISSEMINATE, MX_COLLECT, CHIRP_CONNECTIVITY, CHIRP_VERSION);

  HAL_StatusTypeDef status;

  SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
  do
  {
    status = HAL_TIMEOUT;
    while(status != HAL_OK)
    {
      gpi_watchdog_periodic();
      /* initiator sleep for 60 s after 1 seconds not receiving any task */
      task_wait++;
      if (task_wait > 1)
      {
        __HAL_UART_DISABLE(&huart2);
        SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
        task_wait = 0;
        SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
        __HAL_UART_ENABLE(&huart2);
        return 0;
      }
      PRINTF("Input initiator task:\n");
      // 0,07,100
      status = HAL_UART_Receive(&UART_Handle, &task, sizeof(task), DOWNLOAD_TIMEOUT);
      while (UART_Handle.RxState == HAL_UART_STATE_BUSY_RX);
    }
    mx_task = task[0] - '0';
    default_sf = (task[2] - '0') * 10 + task[3] - '0';
    default_payload_len = (task[5] - '0') * 100 + (task[6] - '0') * 10 + task[7] - '0';
    default_generate_size = (task[9] - '0') * 100 + (task[10] - '0') * 10 + task[11] - '0';
    default_slot_num = (task[13] - '0') * 1000 + (task[14] - '0') * 100 + (task[15] - '0') * 10 + (task[16] - '0');
    dissem_back_sf = (task[18] - '0') * 10 + task[19] - '0';
    dissem_back_slot_num = (task[21] - '0') * 100 + (task[22] - '0') * 10 + task[23] - '0';
    default_tp = (task[25] - '0') * 10 + (task[26] - '0');
    uint8_t i, data;
    memset((uint32_t *)&(chirp_outl->firmware_bitmap[0]), 0, DISSEM_BITMAP_32 * sizeof(uint32_t));
    memset((uint32_t *)&(chirp_outl->task_bitmap[0]), 0, DISSEM_BITMAP_32 * sizeof(uint32_t));
    for (i = 28; i < 28 + DISSEM_BITMAP_32 * sizeof(uint32_t) * 2; i++)
    {
      data = task[i];
      if ((data >= '0') && (data <= '9'))
        data = data - '0';
      else
        data = 10 + data - 'A';
      chirp_outl->firmware_bitmap[(i - 28) / 8] += data * pow(0x10, sizeof(uint32_t) * 2 - 1 - ((i - 28) % (sizeof(uint32_t) * 2)));
    }
    for (i = 37; i < 37 + DISSEM_BITMAP_32 * sizeof(uint32_t) * 2; i++)
    {
      data = task[i];
      if ((data >= '0') && (data <= '9'))
        data = data - '0';
      else
        data = 10 + data - 'A';
      chirp_outl->task_bitmap[(i - 37) / 8] += data * pow(0x10, sizeof(uint32_t) * 2 - 1 - ((i - 37) % (sizeof(uint32_t) * 2)));
    }
  } while ((mx_task > MX_TASK_LAST) || (mx_task < MX_TASK_FIRST));

  SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;

  PRINTF("Select: ");

  chirp_outl->arrange_task = (Mixer_Task )mx_task;
  chirp_outl->default_sf = default_sf;
  chirp_outl->default_tp = default_tp;
  chirp_outl->default_payload_len = default_payload_len;
  chirp_outl->default_generate_size = default_generate_size;
  chirp_outl->default_slot_num = default_slot_num;
  chirp_outl->dissem_back_sf = dissem_back_sf;
  chirp_outl->dissem_back_slot_num = dissem_back_slot_num;
  PRINTF("default sf:%lu, %d, %d, %d, %d, %d, %d, %02x, %02x\n", chirp_outl->default_sf, chirp_outl->default_tp, chirp_outl->default_payload_len, chirp_outl->default_generate_size, chirp_outl->default_slot_num, chirp_outl->dissem_back_sf, chirp_outl->dissem_back_slot_num, chirp_outl->firmware_bitmap[0], chirp_outl->task_bitmap[0]);
  switch (mx_task)
  {
    case CHIRP_START:
    {
      PRINTF("CHIRP_START\n");
      break;
    }
    case MX_DISSEMINATE:
    {
      PRINTF("MX_DISSEMINATE\n");
      break;
    }
    case MX_COLLECT:
    {
      PRINTF("MX_COLLECT\n");
      break;
    }
    case CHIRP_CONNECTIVITY:
    {
      PRINTF("CHIRP_CONNECTIVITY\n");
      break;
    }
    case CHIRP_VERSION:
    {
      PRINTF("CHIRP_VERSION\n");
      break;
    }
    default:
      PRINTF("WRONG TASK\n");
      break;
  }
  return 1;
}

/**
  * @brief  Controller node waits for task parameters
  * @param  chirp_outl: config outline, related to each task
  * @retval none
  */
void chirp_controller_read_command(Chirp_Outl *chirp_outl)
{
  uint8_t rxbuffer_len;
  uint8_t k = 0;
  uint8_t i, data;
  uint8_t pow_num;

  switch (chirp_outl->arrange_task)
  {
      case CHIRP_START:
      {
          rxbuffer_len = 46;
          break;
      }
      case MX_DISSEMINATE:
      {
          rxbuffer_len = 55;
          break;
      }
      case MX_COLLECT:
      {
          rxbuffer_len = 17;
          break;
      }
      case CHIRP_CONNECTIVITY:
      {
          rxbuffer_len = 17;
          break;
      }
      default:
          break;
  }

  uart_read_data(0, rxbuffer_len);
  PRINTF("\nWaiting for parameter(s)...\n");

  while(!uart_read_done);

  uint8_t *command_buffer = (uint8_t *)malloc(rxbuffer_len);
  uart_read_command(command_buffer, rxbuffer_len);

  switch (chirp_outl->arrange_task)
  {
      case CHIRP_START:
      {
        chirp_outl->version_hash = 0;
        // "2020,12,31,15,18,20,2020,12,31,16,18,20,0,6A75" 0: upgrade, 1: user, 6A75:version
        memset(&(chirp_outl->start_year), 0, offsetof(Chirp_Outl, flash_protection) - offsetof(Chirp_Outl, start_year));
        for (i = 0; i < rxbuffer_len; i++)
        {
          data = (uint8_t)command_buffer[k++];
          if (((data >= '0') && (data <= '9')) || ((data >= 'A') && (data <= 'F')))
          {
            if (i < 4)
            {
              data =  data - '0';
              pow_num = 3;
              chirp_outl->start_year += data * pow(10,(pow_num-i));
            }
            else if (i < 7)
            {
              data =  data - '0';
              pow_num = 6;
              chirp_outl->start_month += data * pow(10,(pow_num-i));
            }
            else if (i < 10)
            {
              data =  data - '0';
              pow_num = 9;
              chirp_outl->start_date += data * pow(10,(pow_num-i));
            }
            else if (i < 13)
            {
              data =  data - '0';
              pow_num = 12;
              chirp_outl->start_hour += data * pow(10,(pow_num-i));
            }
            else if (i < 16)
            {
              data =  data - '0';
              pow_num = 15;
              chirp_outl->start_min += data * pow(10,(pow_num-i));
            }
            else if (i < 19)
            {
              data =  data - '0';
              pow_num = 18;
              chirp_outl->start_sec += data * pow(10,(pow_num-i));
            }
            else if (i < 24)
            {
              data =  data - '0';
              pow_num = 23;
              chirp_outl->end_year += data * pow(10,(pow_num-i));
            }
            else if (i < 27)
            {
              data =  data - '0';
              pow_num = 26;
              chirp_outl->end_month += data * pow(10,(pow_num-i));
            }
            else if (i < 30)
            {
              data =  data - '0';
              pow_num = 29;
              chirp_outl->end_date += data * pow(10,(pow_num-i));
            }
            else if (i < 33)
            {
              data =  data - '0';
              pow_num = 32;
              chirp_outl->end_hour += data * pow(10,(pow_num-i));
            }
            else if (i < 36)
            {
              data =  data - '0';
              pow_num = 35;
              chirp_outl->end_min += data * pow(10,(pow_num-i));
            }
            else if (i < 39)
            {
              data =  data - '0';
              pow_num = 38;
              chirp_outl->end_sec += data * pow(10,(pow_num-i));
            }
            else if (i < 41)
            {
              data =  data - '0';
              pow_num = 40;
              chirp_outl->flash_protection += data * pow(10,(pow_num-i));
            }
            else if (i < 46)
            {
              if ((data >= '0') && (data <= '9'))
                data = data - '0';
              else
                data = 10 + data - 'A';
              pow_num = 45;
              chirp_outl->version_hash += data * pow(16,(pow_num-i));
            }
          }
        }

        PRINTF("\tSTART at %d-%d-%d, %d:%d:%d\n\tEnd at %d-%d-%d, %d:%d:%d\n, start user:%d, ver:%x\n", chirp_outl->start_year, chirp_outl->start_month, chirp_outl->start_date, chirp_outl->start_hour, chirp_outl->start_min, chirp_outl->start_sec, chirp_outl->end_year, chirp_outl->end_month, chirp_outl->end_date, chirp_outl->end_hour, chirp_outl->end_min, chirp_outl->end_sec, chirp_outl->flash_protection, chirp_outl->version_hash);
        break;
      }
      case MX_DISSEMINATE:
      {
        /* ("0,0,00000,00000,6A75": update whole firmware, "1,0,7f800,7f500,6A75": patch firmware of bank1, "1,1,7f800,7f500,6A75": patch firmware of bank2) */
        /* hash code is 0x6A75 */
        /* 87BA9B1B68BFE39666AAB255A1049CC7, md5 of the new file */
        memset(&(chirp_outl->firmware_size), 0, offsetof(Chirp_Outl, collect_addr_start) - offsetof(Chirp_Outl, firmware_size));
        for (i = 0; i < rxbuffer_len; i++)
        {
          data = (uint8_t)command_buffer[k++];
          if (((data >= '0') && (data <= '9')) || ((data >= 'A') && (data <= 'F')))
          {
            if (i < 1)
            {
              data =  data - '0';
              pow_num = 0;
              chirp_outl->patch_update += data * pow(10,(pow_num-i));
            }
            else if (i < 3)
            {
              data =  data - '0';
              pow_num = 2;
              chirp_outl->patch_bank += data * pow(10,(pow_num-i));
            }
            else if (i < 5)
            {
              data =  data - '0';
              pow_num = 4;
              chirp_outl->file_compression += data * pow(10,(pow_num-i));
            }
            else if (i < 11)
            {
              pow_num = 10;
              if ((data >= '0') && (data <= '9'))
                data = data - '0';
              else
                data = 10 + data - 'A';
              chirp_outl->old_firmware_size += data * pow(16,(pow_num-i));
            }
            else if (i < 17)
            {
              pow_num = 16;
              if ((data >= '0') && (data <= '9'))
                data = data - '0';
              else
                data = 10 + data - 'A';
              chirp_outl->firmware_size += data * pow(16,(pow_num-i));
            }
            else if (i < 22)
            {
              pow_num = 21;
              if ((data >= '0') && (data <= '9'))
                data = data - '0';
              else
                data = 10 + data - 'A';
              chirp_outl->version_hash += data * pow(16,(pow_num-i));
            }
            else if (i < 55)
            {
              if ((data >= '0') && (data <= '9'))
                data = data - '0';
              else
                data = 10 + data - 'A';
              chirp_outl->firmware_md5[(i - 23) / 2] += data * pow(16,((i - 22) % 2));
            }
          }
        }
        PRINTF("MX_DISSEMINATE:%d, %d, %lu, %lu, %d, %lu\n", chirp_outl->patch_update, chirp_outl->patch_bank, chirp_outl->old_firmware_size, chirp_outl->firmware_size, chirp_outl->version_hash, chirp_outl->file_compression);
        for (i = 0; i < 16; i++)
        {
          PRINTF("%02X", chirp_outl->firmware_md5[i]);
        }
        PRINTF("\n");
        if (!chirp_outl->patch_update)
        {
          menu_preSend(1);
        }
        break;
      }
      case MX_COLLECT:
      {
        // "0807F800,08080000"
        memset(&(chirp_outl->collect_addr_start), 0, offsetof(Chirp_Outl, sf_bitmap) - offsetof(Chirp_Outl, collect_addr_start));
        for (i = 0; i < rxbuffer_len; i++)
        {
          data = (uint8_t)command_buffer[k++];
          if (((data >= '0') && (data <= '9')) || ((data >= 'A') && (data <= 'F')))
          {
            if (i < 8)
            {
              pow_num = 7;
              if ((data >= '0') && (data <= '9'))
                data = data - '0';
              else
                data = 10 + data - 'A';
              chirp_outl->collect_addr_start += data * pow(16,(pow_num-i));
            }
            else if (i < 17) /* Frequency */
            {
              pow_num = 16;
              if ((data >= '0') && (data <= '9'))
                data = data - '0';
              else
                data = 10 + data - 'A';
              chirp_outl->collect_addr_end += data * pow(16,(pow_num-i));
            }
          }
        }

        PRINTF("Start address: 0x%x, End address: 0x%x\n", chirp_outl->collect_addr_start, chirp_outl->collect_addr_end);
        break;
      }
      case CHIRP_CONNECTIVITY:
      {
        // "63,478600,-01"
        uint8_t tx_sign = 0;
        memset(&(chirp_outl->sf_bitmap), 0, offsetof(Chirp_Outl, chirp_energy) - offsetof(Chirp_Outl, sf_bitmap));
        for (i = 0; i < rxbuffer_len; i++)
        {
          data = (uint8_t)command_buffer[k++] - '0';
          if (((data >= 0) && (data <= 9)) || (data == (uint8_t)('-' - '0')) || (data == (uint8_t)('+' - '0')))
          {
            if (i < 2) /* SF bitmap */
            {
              pow_num = 1;
              chirp_outl->sf_bitmap += data * pow(10,(pow_num-i));
            }
            else if (i < 9) /* Frequency */
            {
              pow_num = 8;
              chirp_outl->freq += data * pow(10,(pow_num-i));
            }
            else if (i < 13) /* Tx power */
            {
              pow_num = 12;
              if (i < 11)
              {
                if (data == (uint8_t)('-' - '0'))
                  tx_sign = 0;
                else if (data == (uint8_t)('+' - '0'))
                  tx_sign = 1;
              }
              else
                chirp_outl->tx_power += data * pow(10,(pow_num-i));
            }
            else if (i < 17) /* Frequency */
            {
              pow_num = 16;
              chirp_outl->topo_payload_len += data * pow(10,(pow_num-i));
            }
          }
        }

        if (!tx_sign)
          chirp_outl->tx_power = 0 - chirp_outl->tx_power;

        PRINTF("Spreading factor bitmap: %d, Frequency at: %lu kHz, Tx power: %d, topo_payload_len: %d\n", chirp_outl->sf_bitmap, chirp_outl->freq, chirp_outl->tx_power, chirp_outl->topo_payload_len);
        break;
      }
      default:
        break;
  }
  free(command_buffer);
}

uint32_t Chirp_RSHash(uint8_t* str, uint32_t len)
{
    uint32_t b    = 378551;
    uint32_t a    = 63689;
    uint32_t hash = 0;
    uint32_t i    = 0;

    for(i = 0; i < len; str++, i++)
    {
      hash = hash * a + (*str);
      a    = a * b;
    }

    return hash;
}

uint8_t randInt(uint8_t begin, uint8_t end)
{
	uint8_t temp;
	temp = begin + (rand() % (end - begin + 1));
	return temp;
}

void randomPermutation1(uint8_t channel_sync, uint8_t n)
{
  bool boollist[n];
  uint8_t *a = channel_sync;
  bool *used = boollist;
  for (uint8_t i = 0; i < n; i++)
    used[i] = true;
  uint8_t k = 0;
  for (uint8_t i = 0; i < n; ++i)
  {
    k = randInt(1, n);
    while (used[k] == false) {
      k = randInt(1, n);
    }

    used[k] = false;
    a[i] = k;
    // printf("a[i]:%lu, %lu\n", a[i], i);
  }
}

void chirp_start(uint8_t node_id, uint8_t network_num_nodes)
{
  gpi_watchdog_periodic();
	Chirp_Outl chirp_outl;
  memset(&chirp_outl, 0, sizeof(Chirp_Outl));
  assert_reset((daemon_config.Frequency));
  chirp_outl.default_freq = daemon_config.Frequency;

  #if MX_LBT_ACCESS
    memset(&chirp_config.lbt_init_time, 0, sizeof(chirp_config.lbt_init_time));
    chirp_config.lbt_channel_total = LBT_CHANNEL_NUM;
    int32_t mask = 1 << (sizeof(uint_fast_t) * 8 - 1);
    uint32_t m;
    for (m = sizeof(uint32_t) * 8; m-- > chirp_config.lbt_channel_total;)
        mask >>= 1;
    chirp_config.lbt_channel_mask = ~(mask << 1);
  #endif

  Chirp_Time ds3231_time;
  time_t diff;
  time_t sleep_sec;

  Chirp_Time gps_time;
  chirp_config.lbt_channel_primary = 0;
  uint8_t sync_channel_id = 0;
  sync_channel_id = gps_time.chirp_min % LBT_CHANNEL_NUM;
  chirp_outl.glossy_gps_on = 1;

  #if GPS_DATA
  GPS_Wakeup(60);
  // gps_time = GPS_Get_Time();
  // uint8_t gps_time_str[2];
  // memcpy(gps_time_str, (uint8_t *)&(gps_time.chirp_year), sizeof(gps_time_str));
  // uint32_t channel_seed;
  // uint8_t sync_channel[LBT_CHANNEL_NUM];

  // channel_seed = Chirp_RSHash(gps_time_str, sizeof(gps_time_str));
  // srand(channel_seed);
  // randomPermutation1(sync_channel, LBT_CHANNEL_NUM);

  // gps_time = GPS_Get_Time();

  #endif

  PRINTF("---------Chirpbox---------\n");
	while (1)
	{
    #if ENERGEST_CONF_ON
      energest_init();
      memset(&chirp_stats_all_debug, 0, sizeof(chirp_stats_all_debug));
      ENERGEST_ON(ENERGEST_TYPE_CPU);
    #endif
    // just finish a task
    if (chirp_config.glossy_task == 2)
    {
      #if GPS_DATA
      DS3231_GetTime();
      /* Set alarm */
      ds3231_time = DS3231_ShowTime();
      sync_channel_id = (ds3231_time.chirp_min+1) % LBT_CHANNEL_NUM;
      if (node_id)
      {
        diff = GPS_Diff(&ds3231_time, 1970, 1, 1, 0, 0, 0);
        sleep_sec = 60 - (time_t)(0 - diff) % 60;
        #if ENERGEST_CONF_ON
          ENERGEST_OFF(ENERGEST_TYPE_CPU);
        #endif
        RTC_Waiting_Count(sleep_sec);
      }
      else
      {
          GPS_Wakeup(60);
      }
    #endif
    #if ENERGEST_CONF_ON
      ENERGEST_ON(ENERGEST_TYPE_CPU);
      energest_type_set(ENERGEST_TYPE_STOP, energest_type_time(ENERGEST_TYPE_STOP) + GPI_TICK_S_TO_FAST(sleep_sec));
      Stats_value_debug(ENERGEST_TYPE_STOP, energest_type_time(ENERGEST_TYPE_STOP));
    #endif
    }

		/* MX_GLOSSY (sync) */
		chirp_outl.task = MX_GLOSSY;
		chirp_outl.arrange_task = MX_GLOSSY;

		PRINTF("---------MX_GLOSSY---------\n");
		// TODO: glossy without mixer payload
		chirp_outl.num_nodes = network_num_nodes;
		chirp_outl.generation_size = 0;
		chirp_outl.payload_len = 0;
		chirp_outl.round_setup = 0;
		chirp_outl.round_max = 0;
		chirp_outl.file_chunk_len = 0;

		chirp_radio_config(12, 7, 1, 8, 14, chirp_outl.default_freq);
		chirp_packet_config(chirp_outl.num_nodes, 0, 0, FLOODING);
    chirp_outl.packet_time = SX1276GetPacketTime(chirp_config.lora_sf, chirp_config.lora_bw, 1, 0, 8, 8);
    chirp_slot_config(chirp_outl.packet_time + 100000, 12, 10000000);

    chirp_config.glossy_task = 0;
    if (!node_id)
    {
      if (!menu_wait_task(&chirp_outl))
        chirp_config.glossy_task = 1;
      else
        chirp_config.glossy_task = 2;
    }

    PRINTF("chirp_config.glossy_task:%d\n", chirp_config.glossy_task);
    #if ENERGEST_CONF_ON
      ENERGEST_OFF(ENERGEST_TYPE_CPU);
      Stats_value_debug(ENERGEST_TYPE_CPU, energest_type_time(ENERGEST_TYPE_CPU));
    #endif
    sync_channel_id = 0;
    chirp_config.lbt_channel_primary = sync_channel_id;
    SX1276SetChannel(chirp_config.lora_freq + chirp_config.lbt_channel_primary * CHANNEL_STEP);
    // no task
		if (chirp_round(node_id, &chirp_outl) != 2)
		{
        sync_channel_id = (sync_channel_id+1) % LBT_CHANNEL_NUM;
        #if ENERGEST_CONF_ON
          ENERGEST_OFF(ENERGEST_TYPE_CPU);
          ENERGEST_OFF(ENERGEST_TYPE_LPM);
          Stats_value_debug(ENERGEST_TYPE_CPU, energest_type_time(ENERGEST_TYPE_CPU));
          Stats_value_debug(ENERGEST_TYPE_LPM, energest_type_time(ENERGEST_TYPE_LPM) - energest_type_time(ENERGEST_TYPE_TRANSMIT) - energest_type_time(ENERGEST_TYPE_LISTEN));
          Stats_value_debug(ENERGEST_TYPE_TRANSMIT, energest_type_time(ENERGEST_TYPE_TRANSMIT));
          Stats_value_debug(ENERGEST_TYPE_LISTEN, energest_type_time(ENERGEST_TYPE_LISTEN));
        #endif
        PRINTF("chirp_round:%d\n", chirp_config.glossy_task);
        if (!node_id)
        {
          #if GPS_DATA
          GPS_Wakeup(60);
          #else
          DS3231_GetTime();
          /* Set alarm */
          ds3231_time = DS3231_ShowTime();
          diff = GPS_Diff(&ds3231_time, 1970, 1, 1, 0, 0, 0);
          sleep_sec = 60 - (time_t)(0 - diff) % 60;
          RTC_Waiting_Count(sleep_sec);
          #endif
        }
        else
        {
          // this time glossy no task but sync true
          if (chirp_config.glossy_task)
          {
            chirp_outl.glossy_resync = 0;
            // close gps if on
            if (chirp_outl.glossy_gps_on)
            {
              chirp_outl.glossy_gps_on = 0;
              #if GPS_DATA
                GPS_Off();
              #endif
            }
          }
          // long time no glossy, open the gps
          if (!chirp_config.glossy_task)
          {
            chirp_outl.glossy_resync++;
            if ((chirp_outl.glossy_resync >= 5) && (!chirp_outl.glossy_gps_on))
            {
              chirp_outl.glossy_gps_on = 1;
              GPS_On();
              GPS_Waiting_PPS(10);
              // GPS_Wakeup(60);
              // gps_time = GPS_Get_Time();
              // sync_channel_id = gps_time.chirp_min % LBT_CHANNEL_NUM;
              // sync_channel_id = (sync_channel_id+1) % LBT_CHANNEL_NUM;
            }
            // else if (chirp_outl.glossy_resync >= 5)
            // {
            //   gps_time = GPS_Get_Time();
            //   sync_channel_id = gps_time.chirp_min % LBT_CHANNEL_NUM;
            //   sync_channel_id = (sync_channel_id+1) % LBT_CHANNEL_NUM;
            // }
          }

          // wait on each 60 seconds
          if (chirp_outl.glossy_gps_on)
          {
            GPS_Wakeup(60);
          }
          else
          {
            // RTC_Waiting_Count(60 - chirp_config.mx_period_time_s - 2);
            DS3231_GetTime();
            /* Set alarm */
            ds3231_time = DS3231_ShowTime();
            diff = GPS_Diff(&ds3231_time, 1970, 1, 1, 0, 0, 0);
            sleep_sec = 60 - (time_t)(0 - diff) % 60;
            RTC_Waiting_Count(sleep_sec);
          }
        }
        #if ENERGEST_CONF_ON
          energest_type_set(ENERGEST_TYPE_STOP, energest_type_time(ENERGEST_TYPE_STOP) + GPI_TICK_S_TO_FAST(60 - chirp_config.mx_period_time_s - 2));
          Stats_value_debug(ENERGEST_TYPE_STOP, energest_type_time(ENERGEST_TYPE_STOP));
          memcpy((uint32_t *)(&chirp_outl.chirp_energy[0]), (uint32_t *)(&chirp_stats_all_debug), sizeof(chirp_stats_all_debug));
        #endif
    }
    // have a task to do
    else
    {
      PRINTF("glossy_task == 2\n");
      chirp_outl.glossy_resync = 0;
      if ((chirp_outl.glossy_gps_on) && (node_id))
      {
        chirp_outl.glossy_gps_on = 0;
        #if GPS_DATA
          GPS_Off();
        #endif
      }
      #if ENERGEST_CONF_ON
        ENERGEST_OFF(ENERGEST_TYPE_CPU);
        ENERGEST_OFF(ENERGEST_TYPE_LPM);
        Stats_value_debug(ENERGEST_TYPE_LPM, energest_type_time(ENERGEST_TYPE_LPM) - energest_type_time(ENERGEST_TYPE_TRANSMIT) - energest_type_time(ENERGEST_TYPE_LISTEN));
        Stats_value_debug(ENERGEST_TYPE_TRANSMIT, energest_type_time(ENERGEST_TYPE_TRANSMIT));
        Stats_value_debug(ENERGEST_TYPE_LISTEN, energest_type_time(ENERGEST_TYPE_LISTEN));
      #endif
      // // start at minute
      // if (node_id)
      //   RTC_Waiting_Count(60 - chirp_config.mx_period_time_s - 2);
      // else
      //   GPS_Wakeup(60);

		/* default mode is MX_ARRANGE (task arrangement) */
		chirp_outl.task = MX_ARRANGE;
    if (node_id)
      chirp_outl.arrange_task = MX_ARRANGE;

      #if ENERGEST_CONF_ON
        energest_type_set(ENERGEST_TYPE_STOP, energest_type_time(ENERGEST_TYPE_STOP) + GPI_TICK_S_TO_FAST(60 - chirp_config.mx_period_time_s - 2));
        ENERGEST_ON(ENERGEST_TYPE_CPU);
      #endif
		PRINTF("---------MX_ARRANGE---------\n");
		// TODO: tune those parameters
		chirp_outl.num_nodes = network_num_nodes;
		chirp_outl.generation_size = network_num_nodes;
		chirp_outl.payload_len = DATA_HEADER_LENGTH + 5 + 4;
		chirp_outl.round_setup = 1;
		chirp_outl.round_max = 1;
		chirp_outl.file_chunk_len = 0;

		chirp_radio_config(11, 7, 1, 8, 14, chirp_outl.default_freq);
		chirp_packet_config(chirp_outl.num_nodes, chirp_outl.generation_size, chirp_outl.payload_len + HASH_TAIL, DISSEMINATION);
    chirp_outl.packet_time = SX1276GetPacketTime(chirp_config.lora_sf, chirp_config.lora_bw, 1, 0, 8, chirp_config.phy_payload_size + HASH_TAIL_CODE);
    chirp_slot_config(chirp_outl.packet_time + 100000, chirp_outl.num_nodes * 3, 1500000);
		chirp_payload_distribution(chirp_outl.task);
    #if ENERGEST_CONF_ON
      ENERGEST_OFF(ENERGEST_TYPE_CPU);
      Stats_value_debug(ENERGEST_TYPE_CPU, energest_type_time(ENERGEST_TYPE_CPU));
      Stats_value_debug(ENERGEST_TYPE_STOP, energest_type_time(ENERGEST_TYPE_STOP));
      memcpy((uint32_t *)(&chirp_outl.chirp_energy[0]), (uint32_t *)(&chirp_stats_all_debug), sizeof(chirp_stats_all_debug));
      memset(&chirp_stats_all_debug, 0, sizeof(chirp_stats_all_debug));
    #endif
		if (!chirp_round(node_id, &chirp_outl))
    {
      chirp_outl.task = MX_ARRANGE;
      chirp_outl.arrange_task = MX_ARRANGE;
    }

    #if ENERGEST_CONF_ON
      ENERGEST_OFF(ENERGEST_TYPE_CPU);
      ENERGEST_OFF(ENERGEST_TYPE_LPM);
      ENERGEST_ON(ENERGEST_TYPE_CPU);
      if (chirp_outl.arrange_task == MX_DISSEMINATE)
      {
        FLASH_If_Erase_Pages(1, DAEMON_DEBUG_PAGE);
      }
    #endif
    }
		/* into the assigned task */
		chirp_outl.task = chirp_outl.arrange_task;
    memset(&chirp_stats_all, 0, sizeof(chirp_stats_all));
    #if MX_LBT_ACCESS
      memset(&chirp_config.lbt_channel_time_stats_us, 0, sizeof(chirp_config.lbt_channel_time_stats_us));
    #endif

		Gpi_Fast_Tick_Native deadline;
    if (chirp_outl.task == MX_DISSEMINATE)
      deadline = gpi_tick_fast_native() + GPI_TICK_MS_TO_FAST(20000);
    else
      deadline = gpi_tick_fast_native() + GPI_TICK_MS_TO_FAST(5000);

    uint32_t task_bitmap_temp = chirp_outl.task_bitmap[0];
    uint8_t task_node_id = 0;
    uint8_t task_lsb;
    while(task_bitmap_temp)
    {
      task_lsb = gpi_get_lsb_32(task_bitmap_temp);
      // PRINTF("task_bitmap_temp:%02x, %lu\n", task_bitmap_temp, task_lsb);
      if (task_lsb == node_id)
        break;
      task_bitmap_temp &= ~(1 << task_lsb);
      task_node_id++;
    }
    uint32_t task_node_num = gpi_popcnt_32(chirp_outl.task_bitmap[0]);
    // PRINTF("task_node_id:%lu, %lu\n", task_node_id, task_node_num);
    gpi_watchdog_periodic();
		switch (chirp_outl.task)
		{
			case CHIRP_START:
			{
				chirp_radio_config(chirp_outl.default_sf, 7, 1, 8, chirp_outl.default_tp, chirp_outl.default_freq);

				log_to_flash("---------CHIRP_START---------\n");
				chirp_outl.num_nodes = network_num_nodes;
				chirp_outl.generation_size = network_num_nodes;
				chirp_outl.payload_len = offsetof(Chirp_Outl, num_nodes) - offsetof(Chirp_Outl, start_year) + DATA_HEADER_LENGTH + 2;
				chirp_outl.round_setup = 1;
				chirp_outl.round_max = chirp_outl.round_setup;
        chirp_outl.version_hash = 0;
				if (!node_id)
				{
					chirp_controller_read_command(&chirp_outl);
				}
				chirp_packet_config(chirp_outl.num_nodes, chirp_outl.generation_size, chirp_outl.payload_len+ HASH_TAIL, DISSEMINATION);
        chirp_outl.packet_time = SX1276GetPacketTime(chirp_config.lora_sf, chirp_config.lora_bw, 1, 0, 8, chirp_config.phy_payload_size + HASH_TAIL_CODE);
        chirp_slot_config(chirp_outl.packet_time + 100000, chirp_outl.default_slot_num, 1500000);
				chirp_payload_distribution(chirp_outl.task);
        while (gpi_tick_compare_fast_native(gpi_tick_fast_native(), deadline) < 0);
				// chirp_round(node_id, &chirp_outl);

        #if ENERGEST_CONF_ON
          ENERGEST_OFF(ENERGEST_TYPE_CPU);
          Stats_value_debug(ENERGEST_TYPE_CPU, energest_type_time(ENERGEST_TYPE_CPU));
          memcpy((uint32_t *)(&chirp_outl.chirp_energy[1]), (uint32_t *)(&chirp_stats_all_debug), sizeof(chirp_stats_all_debug));
          memset(&chirp_stats_all_debug, 0, sizeof(chirp_stats_all_debug));
        #endif
        if (!chirp_round(node_id, &chirp_outl))
        {
          free(payload_distribution);
          break;
        }
				free(payload_distribution);

        #if ENERGEST_CONF_ON
          ENERGEST_OFF(ENERGEST_TYPE_CPU);
          Stats_value_debug(ENERGEST_TYPE_CPU, energest_type_time(ENERGEST_TYPE_CPU));
          memcpy((uint32_t *)(&chirp_outl.chirp_energy[2]), (uint32_t *)(&chirp_stats_all_debug), sizeof(chirp_stats_all_debug));
          FLASH_If_Write(DAEMON_DEBUG_FLASH_ADDRESS + chirp_outl.task * DAEMON_DEBUG_ENERGY_LEN_192, (uint32_t *)(&chirp_outl.chirp_energy[0]), sizeof(chirp_outl.chirp_energy[0]) / sizeof(uint32_t));
          FLASH_If_Write(DAEMON_DEBUG_FLASH_ADDRESS + chirp_outl.task * DAEMON_DEBUG_ENERGY_LEN_192 + DAEMON_DEBUG_ENERGY_LEN_64, (uint32_t *)(&chirp_outl.chirp_energy[1]), sizeof(chirp_outl.chirp_energy[1]) / sizeof(uint32_t));
          FLASH_If_Write(DAEMON_DEBUG_FLASH_ADDRESS + chirp_outl.task * DAEMON_DEBUG_ENERGY_LEN_192 + DAEMON_DEBUG_ENERGY_LEN_64 * 2, (uint32_t *)(&chirp_outl.chirp_energy[2]), sizeof(chirp_outl.chirp_energy[2]) / sizeof(uint32_t));
          FLASH_If_Erase_Pages(1, DAEMON_LBT_PAGE);
          FLASH_If_Write(DAEMON_DEBUG_LBT_ADDRESS, (uint32_t *)&chirp_config.lbt_channel_time_us[0], ((LBT_CHANNEL_NUM + 1) / 2) * sizeof(uint64_t) / sizeof(uint32_t));
        #endif

				#if GPS_DATA
          // gps_time = GPS_Get_Time();
          // time_t diff = GPS_Diff(&gps_time, chirp_outl.start_year, chirp_outl.start_month, chirp_outl.start_date, chirp_outl.start_hour, chirp_outl.start_min, chirp_outl.start_sec);
          DS3231_GetTime();
          ds3231_time = DS3231_ShowTime();
          diff = GPS_Diff(&ds3231_time, chirp_outl.start_year, chirp_outl.start_month, chirp_outl.start_date, chirp_outl.start_hour, chirp_outl.start_min, chirp_outl.start_sec);
          assert_reset((diff > 5));
          if (((chirp_outl.version_hash == (daemon_config.DAEMON_version))) && (chirp_outl.firmware_bitmap[node_id / 32] & (1 << (node_id % 32))) && (CheckOtherBank() == FLASHIF_OK))
          {
            /* erase the user flash page */
            FLASH_If_Erase_Pages(0, 255);

            DS3231_GetTime();
            /* Set alarm */
            ds3231_time = DS3231_ShowTime();
            log_to_flash("date:%d, %d, %d, %d\n", chirp_outl.end_date, chirp_outl.end_hour, chirp_outl.end_min, chirp_outl.end_sec);
            DS3231_SetAlarm1_Time(chirp_outl.end_date, chirp_outl.end_hour, chirp_outl.end_min, chirp_outl.end_sec);
            /* Waiting for bank switch */
            // GPS_Waiting(chirp_outl.start_year, chirp_outl.start_month, chirp_outl.start_date, chirp_outl.start_hour, chirp_outl.start_min, chirp_outl.start_sec);
            diff = GPS_Diff(&ds3231_time, chirp_outl.start_year, chirp_outl.start_month, chirp_outl.start_date, chirp_outl.start_hour, chirp_outl.start_min, chirp_outl.start_sec);
            RTC_Waiting_Count(diff);
            log_to_flash("---------CHIRP_BANK---------\n");
            #if BANK_1_RUN
            /* flash protect */
            if (chirp_outl.flash_protection)
              Bank1_WRP(0, 255);
            else
              Bank1_nWRP();
            #endif
            /* switch to bank2 */
            STMFLASH_BankSwitch();
          }
				#endif
				break;
			}
			case MX_DISSEMINATE:
			{
        if ((chirp_outl.task_bitmap[node_id / 32] & (1 << (node_id % 32))))
        {
				chirp_radio_config(chirp_outl.default_sf, 7, 1, 8, chirp_outl.default_tp, chirp_outl.default_freq);
        chirp_outl.disem_file_index = 0;
        chirp_outl.disem_file_max = UINT16_MAX / 2;
        chirp_outl.disem_file_index_stay = 0;
        chirp_outl.version_hash = 0;
        memset(chirp_outl.firmware_md5, 0, sizeof(chirp_outl.firmware_md5));
				log_to_flash("---------MX_DISSEMINATE---------\n");
				// TODO: tune those parameters
				chirp_outl.num_nodes = task_node_num;
				chirp_outl.generation_size = chirp_outl.default_generate_size;
				chirp_outl.payload_len = chirp_outl.default_payload_len;
				assert_reset((chirp_outl.payload_len > DATA_HEADER_LENGTH + 28));
				assert_reset(!((chirp_outl.payload_len - DATA_HEADER_LENGTH) % sizeof(uint64_t)));
				chirp_outl.round_setup = 1;
				chirp_outl.round_max = UINT16_MAX;
				chirp_outl.file_chunk_len = chirp_outl.generation_size * (chirp_outl.payload_len - DATA_HEADER_LENGTH);
        chirp_outl.disem_file_memory = (uint32_t *)malloc(chirp_outl.file_chunk_len);
        if (!node_id)
				{
					chirp_controller_read_command(&chirp_outl);
					uint32_t flash_length;
					if (!chirp_outl.patch_update)
						flash_length = menu_initiator_read_file();
					else
					{
						/* patch bank1, means update self firmware */
						chirp_outl.patch_page = menu_pre_patch(chirp_outl.patch_bank, chirp_outl.old_firmware_size, chirp_outl.firmware_size);
						flash_length = menu_serialDownload(chirp_outl.patch_page, chirp_outl.patch_bank);
					}
					chirp_outl.firmware_size = flash_length;
					uint32_t firmware_size[1];
					firmware_size[0] = chirp_outl.firmware_size;
          FLASH_If_Erase_Pages(0, 253);
					FLASH_If_Write(FIRMWARE_FLASH_ADDRESS_2, (uint32_t *)firmware_size, 2);

          chirp_outl.disem_file_max = (chirp_outl.firmware_size + chirp_outl.file_chunk_len - 1) / chirp_outl.file_chunk_len + 1;
					log_to_flash("file size:%lu, %d, %d, %lu\n", flash_length, chirp_outl.disem_file_max, chirp_outl.file_chunk_len, chirp_outl.payload_len - DATA_HEADER_LENGTH );
				}
        chirp_outl.disem_flag = 1;
				chirp_packet_config(chirp_outl.num_nodes, chirp_outl.generation_size, chirp_outl.payload_len + HASH_TAIL, DISSEMINATION);
        chirp_outl.packet_time = SX1276GetPacketTime(chirp_config.lora_sf, chirp_config.lora_bw, 1, 0, 8, chirp_config.phy_payload_size + HASH_TAIL_CODE);
        chirp_slot_config(chirp_outl.packet_time + 100000, chirp_outl.default_slot_num, 2000000);
				chirp_payload_distribution(chirp_outl.task);
        while (gpi_tick_compare_fast_native(gpi_tick_fast_native(), deadline) < 0);
				// chirp_round(node_id, &chirp_outl);
        #if ENERGEST_CONF_ON
          ENERGEST_OFF(ENERGEST_TYPE_CPU);
          Stats_value_debug(ENERGEST_TYPE_CPU, energest_type_time(ENERGEST_TYPE_CPU));
          memcpy((uint32_t *)(&chirp_outl.chirp_energy[1]), (uint32_t *)(&chirp_stats_all_debug), sizeof(chirp_stats_all_debug));
          memset(&chirp_stats_all_debug, 0, sizeof(chirp_stats_all_debug));
        #endif
        if (!chirp_round(task_node_id, &chirp_outl))
        {
          free(payload_distribution);
          FLASH_If_Erase(0);
          break;
        }
				free(payload_distribution);
        free(chirp_outl.disem_file_memory);
        if(!FirmwareUpgrade(chirp_outl.patch_update, chirp_outl.patch_bank, 0, chirp_outl.old_firmware_size, chirp_outl.patch_bank, chirp_outl.patch_page, chirp_outl.firmware_size, chirp_outl.firmware_md5, chirp_outl.file_compression))
          break;
        Stats_to_Flash(chirp_outl.task);

        #if ENERGEST_CONF_ON
          ENERGEST_OFF(ENERGEST_TYPE_CPU);
          Stats_value_debug(ENERGEST_TYPE_CPU, energest_type_time(ENERGEST_TYPE_CPU));
          Stats_value_debug(ENERGEST_TYPE_FLASH_WRITE_BANK1, energest_type_time(ENERGEST_TYPE_FLASH_WRITE_BANK1));
          Stats_value_debug(ENERGEST_TYPE_FLASH_WRITE_BANK2, energest_type_time(ENERGEST_TYPE_FLASH_WRITE_BANK2));
          Stats_value_debug(ENERGEST_TYPE_FLASH_ERASE, energest_type_time(ENERGEST_TYPE_FLASH_ERASE));
          Stats_value_debug(ENERGEST_TYPE_FLASH_VERIFY, energest_type_time(ENERGEST_TYPE_FLASH_VERIFY));
          memcpy((uint32_t *)(&chirp_outl.chirp_energy[2]), (uint32_t *)(&chirp_stats_all_debug), sizeof(chirp_stats_all_debug));
          FLASH_If_Write(DAEMON_DEBUG_FLASH_ADDRESS + chirp_outl.task * DAEMON_DEBUG_ENERGY_LEN_192, (uint32_t *)(&chirp_outl.chirp_energy[0]), sizeof(chirp_outl.chirp_energy[0]) / sizeof(uint32_t));
          FLASH_If_Write(DAEMON_DEBUG_FLASH_ADDRESS + chirp_outl.task * DAEMON_DEBUG_ENERGY_LEN_192 + DAEMON_DEBUG_ENERGY_LEN_64, (uint32_t *)(&chirp_outl.chirp_energy[1]), sizeof(chirp_outl.chirp_energy[1]) / sizeof(uint32_t));
          FLASH_If_Write(DAEMON_DEBUG_FLASH_ADDRESS + chirp_outl.task * DAEMON_DEBUG_ENERGY_LEN_192 + DAEMON_DEBUG_ENERGY_LEN_64 * 2, (uint32_t *)(&chirp_outl.chirp_energy[2]), sizeof(chirp_outl.chirp_energy[2]) / sizeof(uint32_t));
          FLASH_If_Erase_Pages(1, DAEMON_LBT_PAGE);
          FLASH_If_Write(DAEMON_DEBUG_LBT_ADDRESS, (uint32_t *)&chirp_config.lbt_channel_time_us[0], ((LBT_CHANNEL_NUM + 1) / 2) * sizeof(uint64_t) / sizeof(uint32_t));
        #endif

        }
				break;
			}
			case MX_COLLECT:
			{
        if ((chirp_outl.task_bitmap[node_id / 32] & (1 << (node_id % 32))))
        {
				chirp_radio_config(chirp_outl.default_sf, 7, 1, 8, chirp_outl.default_tp, chirp_outl.default_freq);

				log_to_flash("---------MX_COLLECT---------\n");
				// TODO: tune those parameters
				chirp_outl.num_nodes = task_node_num;
				chirp_outl.generation_size = chirp_outl.num_nodes;
				chirp_outl.payload_len = chirp_outl.default_payload_len;
				chirp_outl.round_max = UINT16_MAX;
				chirp_outl.round_setup = 1;
				chirp_outl.file_chunk_len = chirp_outl.payload_len - DATA_HEADER_LENGTH;
        assert_reset((chirp_outl.payload_len > DATA_HEADER_LENGTH + 8));
				assert_reset(!(chirp_outl.file_chunk_len % sizeof(uint64_t)));
				if (!node_id)
				{
					chirp_controller_read_command(&chirp_outl);
					chirp_outl.collect_length = ((chirp_outl.collect_addr_end - chirp_outl.collect_addr_start + sizeof(uint64_t) - 1) / sizeof(uint64_t)) * sizeof(uint64_t);
					chirp_outl.round_max = chirp_outl.round_setup + (chirp_outl.collect_length + chirp_outl.file_chunk_len - 1) / chirp_outl.file_chunk_len;
					PRINTF("set:%d\n", chirp_outl.round_max);
				}
				chirp_packet_config(chirp_outl.num_nodes, chirp_outl.generation_size, chirp_outl.payload_len+ HASH_TAIL, COLLECTION);
        chirp_outl.packet_time = SX1276GetPacketTime(chirp_config.lora_sf, chirp_config.lora_bw, 1, 0, 8, chirp_config.phy_payload_size + HASH_TAIL_CODE);
        chirp_slot_config(chirp_outl.packet_time + 100000, chirp_outl.default_slot_num, 1500000);
				chirp_payload_distribution(chirp_outl.task);
        while (gpi_tick_compare_fast_native(gpi_tick_fast_native(), deadline) < 0);

        #if ENERGEST_CONF_ON
          ENERGEST_OFF(ENERGEST_TYPE_CPU);
          Stats_value_debug(ENERGEST_TYPE_CPU, energest_type_time(ENERGEST_TYPE_CPU));
          memcpy((uint32_t *)(&chirp_outl.chirp_energy[1]), (uint32_t *)(&chirp_stats_all_debug), sizeof(chirp_stats_all_debug));
          memset(&chirp_stats_all_debug, 0, sizeof(chirp_stats_all_debug));
        #endif
				// chirp_round(node_id, &chirp_outl);
        if (!chirp_round(task_node_id, &chirp_outl))
        {
          free(payload_distribution);
          break;
        }
				free(payload_distribution);
        }
        #if ENERGEST_CONF_ON
          ENERGEST_OFF(ENERGEST_TYPE_CPU);
          Stats_value_debug(ENERGEST_TYPE_CPU, energest_type_time(ENERGEST_TYPE_CPU));
          memcpy((uint32_t *)(&chirp_outl.chirp_energy[2]), (uint32_t *)(&chirp_stats_all_debug), sizeof(chirp_stats_all_debug));
          FLASH_If_Write(DAEMON_DEBUG_FLASH_ADDRESS + chirp_outl.task * DAEMON_DEBUG_ENERGY_LEN_192, (uint32_t *)(&chirp_outl.chirp_energy[0]), sizeof(chirp_outl.chirp_energy[0]) / sizeof(uint32_t));
          FLASH_If_Write(DAEMON_DEBUG_FLASH_ADDRESS + chirp_outl.task * DAEMON_DEBUG_ENERGY_LEN_192 + DAEMON_DEBUG_ENERGY_LEN_64, (uint32_t *)(&chirp_outl.chirp_energy[1]), sizeof(chirp_outl.chirp_energy[1]) / sizeof(uint32_t));
          FLASH_If_Write(DAEMON_DEBUG_FLASH_ADDRESS + chirp_outl.task * DAEMON_DEBUG_ENERGY_LEN_192 + DAEMON_DEBUG_ENERGY_LEN_64 * 2, (uint32_t *)(&chirp_outl.chirp_energy[2]), sizeof(chirp_outl.chirp_energy[2]) / sizeof(uint32_t));
          FLASH_If_Erase_Pages(1, DAEMON_LBT_PAGE);
          FLASH_If_Write(DAEMON_DEBUG_LBT_ADDRESS, (uint32_t *)&chirp_config.lbt_channel_time_us[0], ((LBT_CHANNEL_NUM + 1) / 2) * sizeof(uint64_t) / sizeof(uint32_t));
        #endif
				break;
			}
			case CHIRP_CONNECTIVITY:
			{
				chirp_radio_config(chirp_outl.default_sf, 7, 1, 8, chirp_outl.default_tp, chirp_outl.default_freq);

				log_to_flash("---------CHIRP_CONNECTIVITY---------\n");
				chirp_outl.num_nodes = network_num_nodes;
				chirp_outl.generation_size = network_num_nodes;
				chirp_outl.payload_len = DATA_HEADER_LENGTH + 7;
				chirp_outl.round_setup = 1;
				chirp_outl.round_max = chirp_outl.round_setup;
				if (!node_id)
					chirp_controller_read_command(&chirp_outl);
				chirp_packet_config(chirp_outl.num_nodes, chirp_outl.generation_size, chirp_outl.payload_len+ HASH_TAIL, DISSEMINATION);
        chirp_outl.packet_time = SX1276GetPacketTime(chirp_config.lora_sf, chirp_config.lora_bw, 1, 0, 8, chirp_config.phy_payload_size + HASH_TAIL_CODE);
        chirp_slot_config(chirp_outl.packet_time + 100000, chirp_outl.default_slot_num, 1500000);
				chirp_payload_distribution(chirp_outl.task);
        while (gpi_tick_compare_fast_native(gpi_tick_fast_native(), deadline) < 0);

        #if ENERGEST_CONF_ON
          ENERGEST_OFF(ENERGEST_TYPE_CPU);
          Stats_value_debug(ENERGEST_TYPE_CPU, energest_type_time(ENERGEST_TYPE_CPU));
          memcpy((uint32_t *)(&chirp_outl.chirp_energy[1]), (uint32_t *)(&chirp_stats_all_debug), sizeof(chirp_stats_all_debug));
          memset(&chirp_stats_all_debug, 0, sizeof(chirp_stats_all_debug));
        #endif

				// chirp_round(node_id, &chirp_outl);
        if (!chirp_round(node_id, &chirp_outl))
        {
          free(payload_distribution);
          break;
        }

        #if ENERGEST_CONF_ON
          ENERGEST_OFF(ENERGEST_TYPE_CPU);
          ENERGEST_OFF(ENERGEST_TYPE_LPM);
          ENERGEST_ON(ENERGEST_TYPE_CPU);
        #endif

        // initiate LoRa radio with sf 7 and testing TP and frequency.
        chirp_radio_config(7, 7, 1, 8, chirp_outl.tx_power, chirp_outl.freq);
        topo_manager(chirp_outl.num_nodes, node_id, chirp_outl.sf_bitmap, chirp_outl.topo_payload_len);

				free(payload_distribution);
        #if ENERGEST_CONF_ON
          ENERGEST_OFF(ENERGEST_TYPE_CPU);
          Stats_value_debug(ENERGEST_TYPE_CPU, energest_type_time(ENERGEST_TYPE_CPU));
          Stats_value_debug(ENERGEST_TYPE_TRANSMIT, energest_type_time(ENERGEST_TYPE_TRANSMIT));
          Stats_value_debug(ENERGEST_TYPE_LISTEN, energest_type_time(ENERGEST_TYPE_LISTEN));
          Stats_value_debug(ENERGEST_TYPE_FLASH_WRITE_BANK1, energest_type_time(ENERGEST_TYPE_FLASH_WRITE_BANK1));
          memcpy((uint32_t *)(&chirp_outl.chirp_energy[2]), (uint32_t *)(&chirp_stats_all_debug), sizeof(chirp_stats_all_debug));
          FLASH_If_Write(DAEMON_DEBUG_FLASH_ADDRESS + chirp_outl.task * DAEMON_DEBUG_ENERGY_LEN_192, (uint32_t *)(&chirp_outl.chirp_energy[0]), sizeof(chirp_outl.chirp_energy[0]) / sizeof(uint32_t));
          FLASH_If_Write(DAEMON_DEBUG_FLASH_ADDRESS + chirp_outl.task * DAEMON_DEBUG_ENERGY_LEN_192 + DAEMON_DEBUG_ENERGY_LEN_64, (uint32_t *)(&chirp_outl.chirp_energy[1]), sizeof(chirp_outl.chirp_energy[1]) / sizeof(uint32_t));
          FLASH_If_Write(DAEMON_DEBUG_FLASH_ADDRESS + chirp_outl.task * DAEMON_DEBUG_ENERGY_LEN_192 + DAEMON_DEBUG_ENERGY_LEN_64 * 2, (uint32_t *)(&chirp_outl.chirp_energy[2]), sizeof(chirp_outl.chirp_energy[2]) / sizeof(uint32_t));
          FLASH_If_Erase_Pages(1, DAEMON_LBT_PAGE);
          FLASH_If_Write(DAEMON_DEBUG_LBT_ADDRESS, (uint32_t *)&chirp_config.lbt_channel_time_us[0], ((LBT_CHANNEL_NUM + 1) / 2) * sizeof(uint64_t) / sizeof(uint32_t));
        #endif
				break;
			}
			case CHIRP_VERSION:
			{
				chirp_radio_config(chirp_outl.default_sf, 7, 1, 8, chirp_outl.default_tp, chirp_outl.default_freq);

				log_to_flash("---------CHIRP_VERSION---------\n");
				// TODO: tune those parameters
				chirp_outl.num_nodes = network_num_nodes;
				chirp_outl.generation_size = chirp_outl.num_nodes;
				chirp_outl.payload_len = DATA_HEADER_LENGTH + 3 + sizeof(uint16_t);
				chirp_outl.round_setup = 1;
				chirp_outl.round_max = chirp_outl.round_setup;

				chirp_packet_config(chirp_outl.num_nodes, chirp_outl.generation_size, chirp_outl.payload_len+ HASH_TAIL, COLLECTION);
        chirp_outl.packet_time = SX1276GetPacketTime(chirp_config.lora_sf, chirp_config.lora_bw, 1, 0, 8, chirp_config.phy_payload_size + HASH_TAIL_CODE);
        chirp_slot_config(chirp_outl.packet_time + 100000, chirp_outl.default_slot_num, 1500000);
				chirp_payload_distribution(chirp_outl.task);
        while (gpi_tick_compare_fast_native(gpi_tick_fast_native(), deadline) < 0);
				// chirp_round(node_id, &chirp_outl);
        if (!chirp_round(node_id, &chirp_outl))
        {
          free(payload_distribution);
          break;
        }
				free(payload_distribution);
				break;
			}
			default:
				break;
		}
	}
}

//**************************************************************************************************
//**************************************************************************************************
