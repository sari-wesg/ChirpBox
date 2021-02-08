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

//**************************************************************************************************
//***** Local (Private) Defines and Consts *********************************************************
#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

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

//**************************************************************************************************
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

//**************************************************************************************************
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

//**************************************************************************************************
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

//**************************************************************************************************
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

//**************************************************************************************************
//***** Global Functions ***************************************************************************
//**************************************************************************************************

/**
  * @brief  Erase the Flash bank and clear the protection
  * @param  None
  * @retval None
  */
void menu_preSend(void)
{
	/* Get the current configuration */
  HAL_FLASHEx_OBGetConfig( &OBConfig );

  FLASH_If_WriteProtectionClear();

  /* Test from which bank the program runs */
  BankActive = READ_BIT(SYSCFG->MEMRMP, SYSCFG_MEMRMP_FB_MODE);

  Serial_PutString((uint8_t *)"\r\n======== Patch ============\r\n\n");
  if (BankActive == 0)
    Serial_PutString((uint8_t *)"  System running from STM32L476 Bank 1  \r\n\n");
  else
    Serial_PutString((uint8_t *)"  System running from STM32L476 Bank 2  \r\n\n");

  if ( OBConfig.USERConfig & OB_BFB2_ENABLE ) /* BANK2 active for boot */
    Serial_PutString((uint8_t *)"  System ROM bank selection active  \r\n\n");
  else
    Serial_PutString((uint8_t *)"  System ROM bank selection deactivated \r\n\n");

  /* Clean the input path */
  __HAL_UART_FLUSH_DRREGISTER(&UART_Handle);
  __HAL_UART_CLEAR_IT(&UART_Handle, UART_CLEAR_OREF);
  FLASH_If_Erase(BankActive);
}

/**
  * @brief  Download a file via serial port. Should be used after menu_preSend.
  * @param  offset_page: The download offset page of the flash (0-255)
  * @retval size: The file size
  */
uint32_t menu_serialDownload(uint32_t offset_page)
{
  uint8_t number[11] = {0};
  uint32_t size = 0;
  COM_StatusTypeDef result;

  Serial_PutString((uint8_t *)"Waiting for the file to be sent ... (press 'a' to abort)\n\r");
  result = menu_ymodem_receive( &size, BankActive, FLASH_PAGE * offset_page);
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
  else if (result == COM_LIMIT)
    Serial_PutString((uint8_t *)"\n\n\rThe image size is higher than the bank size!\n\r");
  else if (result == COM_DATA)
    Serial_PutString((uint8_t *)"\n\n\rVerification failed!\n\r");
  else if (result == COM_ABORT)
    Serial_PutString((uint8_t *)"\n\rAborted by user.\n\r");
  else
    Serial_PutString((uint8_t *)"\n\rFailed to receive the file!\n\r");
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
