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

#include "flash_if.h"
#include "menu.h"
#include "string.h"
#include "mixer/mixer_internal.h"
#include "ds3231.h"
// #include "gpi/interrupts.h"
#include "md5.h"

//**************************************************************************************************
//***** Global Variables ****************************************************************************
volatile uint8_t uart_read_done;
extern uint8_t VERSION_MAJOR, VERSION_NODE;
uint8_t gps_time_str[4];

//**************************************************************************************************
//***** Local (Private) Defines and Consts *********************************************************
#define DEBUG 1
#if DEBUG
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
  printf("copy round:%lu, %lu\n", round, firmware_size);
  for (n = round; n > 0; n--)
  {
    printf("%lu, ", (n - 1));
    memcpy(firmware_file_buffer, (__IO uint32_t*)(FLASH_SRC + (n - 1) * sizeof(firmware_file_buffer)), sizeof(firmware_file_buffer));

    FLASH_If_Write(FLASH_DEST + (n - 1) * sizeof(firmware_file_buffer), (uint32_t *)(firmware_file_buffer), sizeof(firmware_file_buffer) / sizeof(uint32_t));
  }
  printf("\n");

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
	/* Get the current configuration */
  HAL_FLASHEx_OBGetConfig( &OBConfig );

  FLASH_If_WriteProtectionClear();

  /* Test from which bank the program runs */
  BankActive = READ_BIT(SYSCFG->MEMRMP, SYSCFG_MEMRMP_FB_MODE);

  Serial_PutString((uint8_t *)"\r\n========__DATE__ __TIME__ = " __DATE__ " " __TIME__ " ============\r\n\n");
  printf("\r\n========version: %x-%x ========\r\n\n", VERSION_MAJOR, VERSION_NODE);
	if (BankActive == 0)
  {
    Serial_PutString((uint8_t *)"\tSystem running from STM32L476 *Bank 1*  \r\n\n");
  }
  else
  {
    Serial_PutString((uint8_t *)"\tSystem running from STM32L476 *Bank 2*  \r\n\n");
    uint32_t firmware_size = *(__IO uint32_t*)(FIRMWARE_FLASH_ADDRESS_1);
    printf("firmware_size:%lu\n", firmware_size);
    if ((firmware_size < 0x100000) && (firmware_size))
      Flash_Bank_Copy_Bank(FLASH_START_BANK1, FLASH_START_BANK2, firmware_size, 1);
    DS3231_GetTime();
    DS3231_ShowTime();
    while(1);
  }

  if ( OBConfig.USERConfig & OB_BFB2_ENABLE ) /* BANK2 active for boot */
    Serial_PutString((uint8_t *)"\tSystem ROM bank selection active  \r\n\n");
  else
    Serial_PutString((uint8_t *)"\tSystem ROM bank selection deactivated \r\n\n");
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
  uint8_t patch_page = (old_firmware_size + FLASH_PAGE - 1) / FLASH_PAGE + 1;
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
  CheckOtherBank();
  return size;
}

#if CHIRP_OUTLINE

/**
 * @description: To chose the task according through serial input
 * @param
 * @return: None
 */
void menu_wait_task(Chirp_Outl *chirp_outl)
{
  Mixer_Task mx_task;
  uint8_t default_sf;
  uint8_t default_payload_len;
  uint8_t default_generate_size;
  uint8_t task_wait = 0;

  uint8_t task[12];
  PRINTF("\nTask list:\n%lu: CHIRP_START\n%lu: MX_DISSEMINATE\n%lu: MX_COLLECT\n%lu: CHIRP_CONNECTIVITY\n%lu: CHIRP_TOPO\n%lu: CHIRP_SNIFF\n%lu: CHIRP_VERSION\n", CHIRP_START, MX_DISSEMINATE, MX_COLLECT, CHIRP_CONNECTIVITY, CHIRP_TOPO, CHIRP_SNIFF, CHIRP_VERSION);

  HAL_StatusTypeDef status;

  SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
  do
  {
    status = HAL_TIMEOUT;
    while(status != HAL_OK)
    {
      #if GPS_DATA
      /* initiator sleep for 60 s after 5 seconds not receiving any task */
      task_wait++;
      if (task_wait > 5)
      {
        __HAL_UART_DISABLE(&huart2);
        SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
        GPS_Sleep(60);
        task_wait = 0;
        SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
        __HAL_UART_ENABLE(&huart2);
      }
      #endif
      PRINTF("Input initiator task:\n");
      // 0,07,100
      status = HAL_UART_Receive(&UART_Handle, &task, 12, DOWNLOAD_TIMEOUT);
      while (UART_Handle.RxState == HAL_UART_STATE_BUSY_RX);
    }
    mx_task = task[0] - '0';
    default_sf = (task[2] - '0') * 10 + task[3] - '0';
    default_payload_len = (task[5] - '0') * 100 + (task[6] - '0') * 10 + task[7] - '0';
    default_generate_size = (task[9] - '0') * 100 + (task[10] - '0') * 10 + task[11] - '0';
  } while ((mx_task > MX_TASK_LAST) || (mx_task < MX_TASK_FIRST));

  SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;

  PRINTF("Select: ");

  chirp_outl->arrange_task = (Mixer_Task )mx_task;
  chirp_outl->default_sf = default_sf;
  chirp_outl->default_payload_len = default_payload_len;
  chirp_outl->default_generate_size = default_generate_size;
  PRINTF("default sf:%lu, %lu, %lu\n", chirp_outl->default_sf, chirp_outl->default_payload_len, chirp_outl->default_generate_size);
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
    case CHIRP_TOPO:
    {
      PRINTF("CHIRP_TOPO\n");
      break;
    }
    case CHIRP_SNIFF:
    {
      PRINTF("CHIRP_SNIFF\n");
      menu_initiator_read_command(chirp_outl);
      #if GPS_DATA
      GPS_Sleep(60);
      #endif
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
}

/**
  * @brief  Initiator wait for task parameters
  * @param  chirp_outl: config outline, related to each task
  * @retval none
  */
void menu_initiator_read_command(Chirp_Outl *chirp_outl)
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
          rxbuffer_len = 53;
          break;
      }
      case MX_COLLECT:
      {
          rxbuffer_len = 17;
          break;
      }
      case CHIRP_CONNECTIVITY:
      {
          rxbuffer_len = 13;
          break;
      }
      case CHIRP_SNIFF:
      {
          rxbuffer_len = 5;
          break;
      }
      default:
          break;
  }

  uart_read_data(0, rxbuffer_len);
  printf("\nWaiting for parameter(s)...\n");

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

        printf("\tSTART at %lu-%lu-%lu, %lu:%lu:%lu\n\tEnd at %lu-%lu-%lu, %lu:%lu:%lu\n, start user:%lu, ver:%x\n", chirp_outl->start_year, chirp_outl->start_month, chirp_outl->start_date, chirp_outl->start_hour, chirp_outl->start_min, chirp_outl->start_sec, chirp_outl->end_year, chirp_outl->end_month, chirp_outl->end_date, chirp_outl->end_hour, chirp_outl->end_min, chirp_outl->end_sec, chirp_outl->flash_protection, chirp_outl->version_hash);
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
            else if (i < 9)
            {
              pow_num = 8;
              if ((data >= '0') && (data <= '9'))
                data = data - '0';
              else
                data = 10 + data - 'A';
              chirp_outl->old_firmware_size += data * pow(16,(pow_num-i));
            }
            else if (i < 15)
            {
              pow_num = 14;
              if ((data >= '0') && (data <= '9'))
                data = data - '0';
              else
                data = 10 + data - 'A';
              chirp_outl->firmware_size += data * pow(16,(pow_num-i));
            }
            else if (i < 20)
            {
              pow_num = 19;
              if ((data >= '0') && (data <= '9'))
                data = data - '0';
              else
                data = 10 + data - 'A';
              chirp_outl->version_hash += data * pow(16,(pow_num-i));
            }
            else if (i < 53)
            {
              if ((data >= '0') && (data <= '9'))
                data = data - '0';
              else
                data = 10 + data - 'A';
              chirp_outl->firmware_md5[(i - 21) / 2] += data * pow(16,((i - 20) % 2));
            }
          }
        }
        printf("MX_DISSEMINATE:%lu, %lu, %lu, %lu, %lu\n", chirp_outl->patch_update, chirp_outl->patch_bank, chirp_outl->old_firmware_size, chirp_outl->firmware_size, chirp_outl->version_hash);
        for (i = 0; i < 16; i++)
        {
          printf("%02X", chirp_outl->firmware_md5[i]);
        }
        printf("\n");
        if (!chirp_outl->patch_update)
        {
          menu_preSend(1);
        }
        break;
      }
      case MX_COLLECT:
      {
        // "0807F800,08080000"
        memset(&(chirp_outl->collect_addr_start), 0, offsetof(Chirp_Outl, sf) - offsetof(Chirp_Outl, collect_addr_start));
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

        printf("Start address: 0x%x, End address: 0x%x\n", chirp_outl->collect_addr_start, chirp_outl->collect_addr_end);
        break;
      }
      case CHIRP_CONNECTIVITY:
      {
        // "09,478600,-01"
        uint8_t tx_sign = 0;
        memset(&(chirp_outl->sf), 0, offsetof(Chirp_Outl, sniff_nodes_num) - offsetof(Chirp_Outl, sf));
        for (i = 0; i < rxbuffer_len; i++)
        {
          data = (uint8_t)command_buffer[k++] - '0';
          if (((data >= 0) && (data <= 9)) || (data == (uint8_t)('-' - '0')) || (data == (uint8_t)('+' - '0')))
          {
            if (i < 2) /* SF */
            {
              pow_num = 1;
              chirp_outl->sf += data * pow(10,(pow_num-i));
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
          }
        }

        if (!tx_sign)
          chirp_outl->tx_power = 0 - chirp_outl->tx_power;

        printf("Spreading factor: %lu, Frequency at: %lu kHz, Tx power: %d\n", chirp_outl->sf, chirp_outl->freq, chirp_outl->tx_power);
        break;
      }
      case CHIRP_SNIFF:
      {
        // 0/1,004(LoRaWAN / LORA_FORM),(number of sniffer nodes)
        // 001,470000 (sniffer config: node_id, radio frequency in kHz)
        // 003,486300
        // 056,486500
        // 012,486300
        memset(&(chirp_outl->sniff_net), 0, offsetof(Chirp_Outl, sniff_node) - offsetof(Chirp_Outl, sniff_nodes_num));
        for (i = 0; i < rxbuffer_len; i++)
        {
          data = (uint8_t)command_buffer[k++];
          if (((data >= '0') && (data <= '9')) || ((data >= 'A') && (data <= 'F')))
          {
            if (i < 1)
            {
              data = data - '0';
              pow_num = 0;
              chirp_outl->sniff_net += data * pow(10,(pow_num-i));
            }
            else if (i < 5)
            {
              pow_num = 4;
              data = data - '0';
              chirp_outl->sniff_nodes_num += data * pow(10,(pow_num-i));
            }
          }
        }
        free(command_buffer);
        printf("Sniffer num:%lu\n", chirp_outl->sniff_nodes_num);

        chirp_outl->sniff_node[0] = (Sniff_Config *)malloc(sizeof(Sniff_Config) * chirp_outl->sniff_nodes_num);
        /* allocate space for sniffer nodes */
        for (i = 1; i < chirp_outl->sniff_nodes_num; i++)
          chirp_outl->sniff_node[i] = (Sniff_Config *)(chirp_outl->sniff_node[i-1] + 1);
        memset(chirp_outl->sniff_node[0], 0, (sizeof(Sniff_Config) * chirp_outl->sniff_nodes_num));

        rxbuffer_len = 10;
        command_buffer = (uint8_t *)malloc(rxbuffer_len);
        uint8_t p;
        for (i = 0; i < chirp_outl->sniff_nodes_num; i++)
        {
          printf("Sniffer config...\n");
          k = 0;
          uart_read_data(0, rxbuffer_len);
          while(!uart_read_done);

          uart_read_command(command_buffer, rxbuffer_len);

          for (p = 0; p < rxbuffer_len; p++)
          {
            data = (uint8_t)command_buffer[k++] - '0';
            if ((data >= 0) && (data <= 9))
            {
              if (p < 3)
              {
                pow_num = 2;
                chirp_outl->sniff_node[i]->sniff_id += data * pow(10,(pow_num-p));
              }
              else if (i < 10)
              {
                pow_num = 9;
                chirp_outl->sniff_node[i]->sniff_freq_khz += data * pow(10,(pow_num-p));
              }
            }
          }
        }

        for (i = 0; i < chirp_outl->sniff_nodes_num; i++)
        {
          printf("sniffer:%lu, %lu kHz\n", chirp_outl->sniff_node[i]->sniff_id, chirp_outl->sniff_node[i]->sniff_freq_khz);
        }

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

void chirp_start(uint8_t node_id, uint8_t network_num_nodes)
{
	Chirp_Outl chirp_outl;
  memset(&chirp_outl, 0, sizeof(Chirp_Outl));
  chirp_outl.default_freq = 440000;

	while (1)
	{
		PRINTF("---------Chirpbox---------\n");
    memset(gps_time_str, 1, sizeof(gps_time_str));

    #if GPS_DATA
    Chirp_Time gps_time;
    memset(&gps_time, 0, sizeof(gps_time));
    while(!gps_time.chirp_year)
    {
      gps_time = GPS_Get_Time();
    }
    uint8_t gps_time_str[4];
    memcpy(gps_time_str, (uint8_t *)&(gps_time.chirp_year), sizeof(gps_time_str));
    #endif

    chirp_outl.hash_header = Chirp_RSHash(gps_time_str, sizeof(gps_time_str));
    chirp_config.packet_hash = chirp_outl.hash_header;

		/* default mode is MX_ARRANGE (task arrangement) */
		chirp_outl.task = MX_ARRANGE;
		chirp_outl.arrange_task = MX_ARRANGE;

		PRINTF("---------MX_ARRANGE---------\n");
		// TODO: tune those parameters
		chirp_outl.num_nodes = network_num_nodes;
		chirp_outl.generation_size = network_num_nodes;
		chirp_outl.payload_len = DATA_HEADER_LENGTH;
		chirp_outl.round_setup = 1;
		chirp_outl.round_max = 1;
		chirp_outl.file_chunk_len = 0;

		chirp_mx_radio_config(12, 7, 1, 8, 14, chirp_outl.default_freq);
		chirp_mx_packet_config(chirp_outl.num_nodes, chirp_outl.generation_size, chirp_outl.payload_len + HASH_TAIL);
    chirp_outl.packet_time = SX1276GetPacketTime(chirp_config.lora_sf, chirp_config.lora_bw, 1, 0, 8, chirp_config.phy_payload_size + HASH_TAIL_CODE);
    chirp_mx_slot_config(chirp_outl.packet_time + 100000, chirp_outl.num_nodes * 8, ((chirp_outl.packet_time + 100000) * (chirp_outl.num_nodes * 8)) + 500000);
		chirp_mx_payload_distribution(chirp_outl.task);

    if (!node_id)
			menu_wait_task(&chirp_outl);

		if (!chirp_mx_round(node_id, &chirp_outl))
		{
      #if GPS_DATA
      GPS_Sleep(60);
      #endif
    }

		free(payload_distribution);

		/* into the assigned task */
		chirp_outl.task = chirp_outl.arrange_task;
    memset(&chirp_stats_all, 0, sizeof(chirp_stats_all));

		Gpi_Fast_Tick_Native deadline;
    if (chirp_outl.task == MX_DISSEMINATE)
      deadline = gpi_tick_fast_native() + GPI_TICK_MS_TO_FAST(20000);
    else
      deadline = gpi_tick_fast_native() + GPI_TICK_MS_TO_FAST(5000);
		switch (chirp_outl.task)
		{
			case CHIRP_START:
			{
				chirp_mx_radio_config(chirp_outl.default_sf, 7, 1, 8, 14, chirp_outl.default_freq);

				PRINTF("---------CHIRP_START---------\n");
				chirp_outl.num_nodes = network_num_nodes;
				chirp_outl.generation_size = network_num_nodes;
				chirp_outl.payload_len = offsetof(Chirp_Outl, num_nodes) - offsetof(Chirp_Outl, start_year) + DATA_HEADER_LENGTH + 2;
				chirp_outl.round_setup = 1;
				chirp_outl.round_max = chirp_outl.round_setup;
        chirp_outl.version_hash = 0;
				if (!node_id)
				{
					menu_initiator_read_command(&chirp_outl);
				}
				chirp_mx_packet_config(chirp_outl.num_nodes, chirp_outl.generation_size, chirp_outl.payload_len+ HASH_TAIL);
        chirp_outl.packet_time = SX1276GetPacketTime(chirp_config.lora_sf, chirp_config.lora_bw, 1, 0, 8, chirp_config.phy_payload_size + HASH_TAIL_CODE);
        chirp_mx_slot_config(chirp_outl.packet_time + 100000, chirp_outl.num_nodes * 8, ((chirp_outl.packet_time + 100000) * (chirp_outl.num_nodes * 8)) + 500000);
				chirp_mx_payload_distribution(chirp_outl.task);
        while (gpi_tick_compare_fast_native(gpi_tick_fast_native(), deadline) < 0);
				// chirp_mx_round(node_id, &chirp_outl);
        if (!chirp_mx_round(node_id, &chirp_outl))
        {
          free(payload_distribution);
          break;
        }
				free(payload_distribution);

				#if GPS_DATA
          gps_time = GPS_Get_Time();
          time_t diff = GPS_Diff(&gps_time, chirp_outl.start_year, chirp_outl.start_month, chirp_outl.start_date, chirp_outl.start_hour, chirp_outl.start_min, chirp_outl.start_sec);
          assert_reset(diff > 5);
					if (!chirp_outl.sniff_flag)
					{
            if ((chirp_outl.version_hash == ((VERSION_MAJOR << 8) | (VERSION_NODE))))
            {
              /* erase the user flash page */
              FLASH_If_Erase_Pages(0, 255);

              DS3231_GetTime();
              /* Set alarm */
              printf("date:%lu, %lu, %lu, %lu\n", chirp_outl.end_date, chirp_outl.end_hour, chirp_outl.end_min, chirp_outl.end_sec);
              DS3231_ShowTime();
              DS3231_SetAlarm1_Time(chirp_outl.end_date, chirp_outl.end_hour, chirp_outl.end_min, chirp_outl.end_sec);
              /* Waiting for bank switch */
              GPS_Waiting(chirp_outl.start_year, chirp_outl.start_month, chirp_outl.start_date, chirp_outl.start_hour, chirp_outl.start_min, chirp_outl.start_sec);
              PRINTF("---------CHIRP_BANK---------\n");
              /* flash protect */
              if (chirp_outl.flash_protection)
                Bank1_WRP(0, 255);
              else
                Bank1_nWRP();
              /* switch to bank2 */
              STMFLASH_BankSwitch();
            }
					}
					else
          {
            /* Waiting for bank switch */
            GPS_Waiting(chirp_outl.start_year, chirp_outl.start_month, chirp_outl.start_date, chirp_outl.start_hour, chirp_outl.start_min, chirp_outl.start_sec);
						PRINTF("---------sniff---------\n");
						sniff_init(chirp_outl.sniff_net, chirp_outl.sniff_freq, chirp_outl.end_year, chirp_outl.end_month, chirp_outl.end_date, chirp_outl.end_hour, chirp_outl.end_min, chirp_outl.end_sec);
          }
				#endif
				break;
			}
			case MX_DISSEMINATE:
			{
				chirp_mx_radio_config(chirp_outl.default_sf, 7, 1, 8, 14, chirp_outl.default_freq);
        chirp_outl.disem_file_index = 0;
        chirp_outl.disem_file_max = UINT16_MAX / 2;
        chirp_outl.disem_file_index_stay = 0;
        chirp_outl.version_hash = 0;
        memset(chirp_outl.firmware_md5, 0, sizeof(chirp_outl.firmware_md5));
				PRINTF("---------MX_DISSEMINATE---------\n");
				// TODO: tune those parameters
				chirp_outl.num_nodes = network_num_nodes;
				chirp_outl.generation_size = chirp_outl.default_generate_size;
				chirp_outl.payload_len = chirp_outl.default_payload_len;
        assert_reset(chirp_outl.payload_len > DATA_HEADER_LENGTH + 28);
				assert_reset(!((chirp_outl.payload_len - DATA_HEADER_LENGTH) % sizeof(uint64_t)));
				chirp_outl.round_setup = 1;
				chirp_outl.round_max = UINT16_MAX;
				chirp_outl.file_chunk_len = chirp_outl.generation_size * (chirp_outl.payload_len - DATA_HEADER_LENGTH);
        chirp_outl.disem_file_memory = (uint32_t *)malloc(chirp_outl.file_chunk_len);
        if (!node_id)
				{
					menu_initiator_read_command(&chirp_outl);
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
					PRINTF("file size:%lu, %lu, %lu, %lu\n", flash_length, chirp_outl.disem_file_max, chirp_outl.file_chunk_len, chirp_outl.payload_len - DATA_HEADER_LENGTH );
				}
        chirp_outl.disem_flag = 1;
				chirp_mx_packet_config(chirp_outl.num_nodes, chirp_outl.generation_size, chirp_outl.payload_len + HASH_TAIL);
        chirp_outl.packet_time = SX1276GetPacketTime(chirp_config.lora_sf, chirp_config.lora_bw, 1, 0, 8, chirp_config.phy_payload_size + HASH_TAIL_CODE);
        chirp_mx_slot_config(chirp_outl.packet_time + 100000, chirp_outl.generation_size * 4 + chirp_outl.num_nodes, ((chirp_outl.packet_time + 100000) * (chirp_outl.generation_size * 4 + chirp_outl.num_nodes)) + 2000000);
				chirp_mx_payload_distribution(chirp_outl.task);
        while (gpi_tick_compare_fast_native(gpi_tick_fast_native(), deadline) < 0);
				// chirp_mx_round(node_id, &chirp_outl);
        if (!chirp_mx_round(node_id, &chirp_outl))
        {
          free(payload_distribution);
          FLASH_If_Erase(0);
          break;
        }
				free(payload_distribution);
        free(chirp_outl.disem_file_memory);
				if (chirp_outl.patch_update)
				{
					printf("size: %lu, %lu\n", chirp_outl.old_firmware_size, chirp_outl.firmware_size);

					uint8_t source_bank = chirp_outl.patch_bank;
					uint8_t patch_bank = chirp_outl.patch_bank;
					uint8_t dest_bank = 1;
					uint32_t new_firmware_size = Filepatch(source_bank, 0, chirp_outl.old_firmware_size, patch_bank, chirp_outl.patch_page, chirp_outl.firmware_size, dest_bank, 0);
					if (new_firmware_size)
						printf("Patch success!:%lu\n", new_firmware_size);
					else
          {
            printf("Patch failed!\n");
            FLASH_If_Erase(0);
            break;
          }
          uint32_t firmware_size_buffer[1];
          firmware_size_buffer[0] = new_firmware_size;
          FLASH_If_Erase_Pages(0, 253);
          FLASH_If_Write(FIRMWARE_FLASH_ADDRESS_2, (uint32_t *)firmware_size_buffer, 2);
          chirp_outl.firmware_size = new_firmware_size;
				}
        uint8_t i;
        printf("Md5 check: %lu\n", chirp_outl.firmware_size);
        for (i = 0; i < 16; i++)
        {
          printf("%02X", chirp_outl.firmware_md5[i]);
        }
        printf("\n");
        if (!MD5_File(1, 0, chirp_outl.firmware_size, chirp_outl.firmware_md5))
        {
          printf("md5 error\n");
          FLASH_If_Erase(0);
          break;
        }
        printf("version_hash:%lu, %lu\n", ((VERSION_MAJOR << 8) | (VERSION_NODE)), chirp_outl.version_hash);
        if (chirp_outl.version_hash != ((VERSION_MAJOR << 8) | (VERSION_NODE)))
        {
          printf("version wrong\n");
          FLASH_If_Erase(0);
        }
        else
          printf("version right\n");
        Stats_to_Flash(chirp_outl.task);
				break;
			}
			case MX_COLLECT:
			{
				chirp_mx_radio_config(chirp_outl.default_sf, 7, 1, 8, 14, chirp_outl.default_freq);

				PRINTF("---------MX_COLLECT---------\n");
				// TODO: tune those parameters
				chirp_outl.num_nodes = network_num_nodes;
				chirp_outl.generation_size = chirp_outl.num_nodes;
				chirp_outl.payload_len = chirp_outl.default_payload_len;
				chirp_outl.round_max = UINT16_MAX;
				chirp_outl.round_setup = 1;
				chirp_outl.file_chunk_len = chirp_outl.payload_len - DATA_HEADER_LENGTH;
        assert_reset(chirp_outl.payload_len > DATA_HEADER_LENGTH + 8);
				assert_reset(!(chirp_outl.file_chunk_len % sizeof(uint64_t)));
				if (!node_id)
				{
					menu_initiator_read_command(&chirp_outl);
					chirp_outl.collect_length = ((chirp_outl.collect_addr_end - chirp_outl.collect_addr_start + sizeof(uint64_t) - 1) / sizeof(uint64_t)) * sizeof(uint64_t);
					chirp_outl.round_max = chirp_outl.round_setup + (chirp_outl.collect_length + chirp_outl.file_chunk_len - 1) / chirp_outl.file_chunk_len;
					printf("set:%lu\n", chirp_outl.round_max);
				}
				chirp_mx_packet_config(chirp_outl.num_nodes, chirp_outl.generation_size, chirp_outl.payload_len+ HASH_TAIL);
        chirp_outl.packet_time = SX1276GetPacketTime(chirp_config.lora_sf, chirp_config.lora_bw, 1, 0, 8, chirp_config.phy_payload_size + HASH_TAIL_CODE);
        chirp_mx_slot_config(chirp_outl.packet_time + 100000, chirp_outl.num_nodes * 8, ((chirp_outl.packet_time + 100000) * (chirp_outl.num_nodes * 8)) + 500000);
				chirp_mx_payload_distribution(chirp_outl.task);
        while (gpi_tick_compare_fast_native(gpi_tick_fast_native(), deadline) < 0);
        printf("set88:%lu\n", chirp_outl.round_max);

				// chirp_mx_round(node_id, &chirp_outl);
        if (!chirp_mx_round(node_id, &chirp_outl))
        {
          free(payload_distribution);
          break;
        }
				free(payload_distribution);
				break;
			}
			case CHIRP_CONNECTIVITY:
			{
				chirp_mx_radio_config(chirp_outl.default_sf, 7, 1, 8, 14, chirp_outl.default_freq);

				PRINTF("---------CHIRP_CONNECTIVITY---------\n");
				chirp_outl.num_nodes = network_num_nodes;
				chirp_outl.generation_size = network_num_nodes;
				chirp_outl.payload_len = DATA_HEADER_LENGTH + 6;
				chirp_outl.round_setup = 1;
				chirp_outl.round_max = chirp_outl.round_setup;
				if (!node_id)
					menu_initiator_read_command(&chirp_outl);
				chirp_mx_packet_config(chirp_outl.num_nodes, chirp_outl.generation_size, chirp_outl.payload_len+ HASH_TAIL);
        chirp_outl.packet_time = SX1276GetPacketTime(chirp_config.lora_sf, chirp_config.lora_bw, 1, 0, 8, chirp_config.phy_payload_size + HASH_TAIL_CODE);
        chirp_mx_slot_config(chirp_outl.packet_time + 100000, chirp_outl.num_nodes * 8, ((chirp_outl.packet_time + 100000) * (chirp_outl.num_nodes * 8)) + 500000);
				chirp_mx_payload_distribution(chirp_outl.task);
        while (gpi_tick_compare_fast_native(gpi_tick_fast_native(), deadline) < 0);
				// chirp_mx_round(node_id, &chirp_outl);
        if (!chirp_mx_round(node_id, &chirp_outl))
        {
          free(payload_distribution);
          break;
        }
        chirp_mx_radio_config(chirp_outl.sf, 7, 1, 8, chirp_outl.tx_power, chirp_outl.freq);
        topo_init(network_num_nodes, node_id, chirp_outl.sf);
        uint8_t i;
        for (i = 0; i < network_num_nodes; i++)
        {
          #if GPS_DATA
          GPS_Sleep(10);
          #endif
          topo_round_robin(node_id, chirp_outl.num_nodes, i, deadline);
        }
				topo_result(chirp_outl.num_nodes);
				free(payload_distribution);
				break;
			}
			case CHIRP_TOPO:
			{
				chirp_mx_radio_config(chirp_outl.default_sf, 7, 1, 8, 14, chirp_outl.default_freq);

				PRINTF("---------CHIRP_TOPO---------\n");
				// TODO: tune those parameters
				chirp_outl.num_nodes = network_num_nodes;
				chirp_outl.generation_size = chirp_outl.num_nodes;
				chirp_outl.payload_len = chirp_outl.default_payload_len;
				chirp_outl.round_setup = 0;
				chirp_outl.file_chunk_len = chirp_outl.payload_len - DATA_HEADER_LENGTH;
        assert_reset(chirp_outl.payload_len > DATA_HEADER_LENGTH);
				assert_reset(!(chirp_outl.file_chunk_len % sizeof(uint64_t)));

				uint16_t file_size = (((chirp_outl.num_nodes + 1) / 2) * 2) * sizeof(uint32_t);
				chirp_outl.round_max = chirp_outl.round_setup + (file_size + chirp_outl.file_chunk_len - 1)/ chirp_outl.file_chunk_len;

				chirp_mx_packet_config(chirp_outl.num_nodes, chirp_outl.generation_size, chirp_outl.payload_len+ HASH_TAIL);
        chirp_outl.packet_time = SX1276GetPacketTime(chirp_config.lora_sf, chirp_config.lora_bw, 1, 0, 8, chirp_config.phy_payload_size + HASH_TAIL_CODE);
        chirp_mx_slot_config(chirp_outl.packet_time + 100000, chirp_outl.num_nodes * 8, ((chirp_outl.packet_time + 100000) * (chirp_outl.num_nodes * 8)) + 500000);
				chirp_mx_payload_distribution(chirp_outl.task);
        while (gpi_tick_compare_fast_native(gpi_tick_fast_native(), deadline) < 0);
				// chirp_mx_round(node_id, &chirp_outl);
        if (!chirp_mx_round(node_id, &chirp_outl))
        {
          free(payload_distribution);
          break;
        }
				free(payload_distribution);
				break;
			}
			case CHIRP_SNIFF:
			{
				chirp_mx_radio_config(chirp_outl.default_sf, 7, 1, 8, 14, chirp_outl.default_freq);
				PRINTF("---------CHIRP_SNIFF---------\n");
				chirp_outl.num_nodes = network_num_nodes;
				chirp_outl.generation_size = network_num_nodes;
        /* payload is composed of: data_header and config of each sniffer (1 byte node id and 4 bytes sniff frequency) */
				chirp_outl.payload_len = DATA_HEADER_LENGTH + chirp_outl.sniff_nodes_num * (sizeof(uint32_t) + sizeof(uint8_t));
				chirp_outl.round_setup = 1;
				chirp_outl.round_max = chirp_outl.round_setup;

				chirp_mx_packet_config(chirp_outl.num_nodes, chirp_outl.generation_size, chirp_outl.payload_len+ HASH_TAIL);

        /* config the slot according to the payload length */
        chirp_outl.packet_time = SX1276GetPacketTime(chirp_config.lora_sf, chirp_config.lora_bw, 1, 0, 8, chirp_config.phy_payload_size + HASH_TAIL_CODE);
        chirp_mx_slot_config(chirp_outl.packet_time + 100000, chirp_outl.num_nodes * 8, ((chirp_outl.packet_time + 100000) * (chirp_outl.num_nodes * 8)) + 500000);
				chirp_mx_payload_distribution(chirp_outl.task);
        while (gpi_tick_compare_fast_native(gpi_tick_fast_native(), deadline) < 0);
				// chirp_mx_round(node_id, &chirp_outl);
        if (!chirp_mx_round(node_id, &chirp_outl))
        {
          free(payload_distribution);
          break;
        }
				free(payload_distribution);
				break;
			}
			case CHIRP_VERSION:
			{
				chirp_mx_radio_config(chirp_outl.default_sf, 7, 1, 8, 14, chirp_outl.default_freq);

				PRINTF("---------CHIRP_VERSION---------\n");
				// TODO: tune those parameters
				chirp_outl.num_nodes = network_num_nodes;
				chirp_outl.generation_size = chirp_outl.num_nodes;
				chirp_outl.payload_len = DATA_HEADER_LENGTH;
				chirp_outl.round_setup = 1;

				uint16_t file_size = (((chirp_outl.num_nodes + 1) / 2) * 2) * sizeof(uint32_t);
				chirp_outl.round_max = chirp_outl.round_setup;

				chirp_mx_packet_config(chirp_outl.num_nodes, chirp_outl.generation_size, chirp_outl.payload_len+ HASH_TAIL);
        chirp_outl.packet_time = SX1276GetPacketTime(chirp_config.lora_sf, chirp_config.lora_bw, 1, 0, 8, chirp_config.phy_payload_size + HASH_TAIL_CODE);
        chirp_mx_slot_config(chirp_outl.packet_time + 100000, chirp_outl.num_nodes * 6, ((chirp_outl.packet_time + 100000) * (chirp_outl.num_nodes * 6)) + 500000);
				chirp_mx_payload_distribution(chirp_outl.task);
        while (gpi_tick_compare_fast_native(gpi_tick_fast_native(), deadline) < 0);
				// chirp_mx_round(node_id, &chirp_outl);
        if (!chirp_mx_round(node_id, &chirp_outl))
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

#endif
//**************************************************************************************************
//**************************************************************************************************
