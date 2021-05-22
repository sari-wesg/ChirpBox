/**
  ******************************************************************************
  * @file    main.c
  * @author  MCD Application Team
  * @brief   this is the main!
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "hw.h"
#include "low_power_manager.h"
#include "lora.h"
#include "bsp.h"
#include "timeServer.h"
#include "vcom.h"
#include "version.h"
#include "API_trace_flash.h"
#include <stdlib.h>
#include "ll_flash.h"
#include "flash_if.h"
// #include <stdint.h>

#include "fut-param-settings.h"
#include "App_FUT.h"
volatile chirpbox_fut_config __attribute((section (".FUTSettingSection"))) fut_config ={5, 5};

//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
#define DEVICE_ID_REG0 (*((volatile uint32_t *)0x1FFF7590))
#define DEVICE_ID_REG1 (*((volatile uint32_t *)0x1FFF7594))
#define DEVICE_ID_REG2 (*((volatile uint32_t *)0x1FFF7598))
/*---------------------------------------------------------------------------*/
// unsigned short stm_node_id = 0;
uint32_t stm_node_id = 0;

unsigned char node_mac[8];
volatile uint32_t device_id[3];
uint32_t lora_rx_count_rece;
/*---------------------------------------------------------------------------*/
void node_id_restore(uint8_t *id)
{
    device_id[0] = DEVICE_ID_REG0;
    device_id[1] = DEVICE_ID_REG1;
    device_id[2] = DEVICE_ID_REG2;

    (*(uint32_t *)node_mac) = DEVICE_ID_REG1;
    (*(((uint32_t *)node_mac) + 1)) = DEVICE_ID_REG2 + DEVICE_ID_REG0;
    stm_node_id = (uint32_t)(DEVICE_ID_REG0);
    id[7] = DEVICE_ID_REG0;
    id[6] = DEVICE_ID_REG0 >> 8;
    id[5] = DEVICE_ID_REG0 >> 16;
    id[4] = DEVICE_ID_REG0 >> 24;
    id[3] = 0;
    id[2] = 0;
    id[1] = 0;
    id[0] = 0;
}

uint32_t __attribute__((section(".data")))	TOS_NODE_ID = 0;

uint8_t node_id_allocate;
volatile uint16_t send_count = 0;
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define LORAWAN_MAX_BAT   254

/*!
 * CAYENNE_LPP is myDevices Application server.
 */
//#define CAYENNE_LPP
#define LPP_DATATYPE_DIGITAL_INPUT  0x0
#define LPP_DATATYPE_DIGITAL_OUTPUT 0x1
#define LPP_DATATYPE_HUMIDITY       0x68
#define LPP_DATATYPE_TEMPERATURE    0x67
#define LPP_DATATYPE_BAROMETER      0x73
#define LPP_APP_PORT 99
/*!
 * Defines the application data transmission duty cycle. 5s, value in [ms].
 */
#define JOIN_TX_DUTYCYCLE                           5000
#define APP_TX_DUTYCYCLE                            10000
/*!
 * LoRaWAN Adaptive Data Rate
 * @note Please note that when ADR is enabled the end-device should be static
 */
// #define LORAWAN_ADR_STATE LORAWAN_ADR_ON
#define LORAWAN_ADR_STATE LORAWAN_ADR_OFF

/*!
 * LoRaWAN Default data Rate Data Rate
 * @note Please note that LORAWAN_DEFAULT_DATA_RATE is used only when ADR is disabled
 */
#define LORAWAN_DEFAULT_DATA_RATE                   DR_5
/*!
 * LoRaWAN application port
 * @note do not use 224. It is reserved for certification
 */
#define LORAWAN_APP_PORT                            2
/*!
 * LoRaWAN default endNode class port
 */
#define LORAWAN_DEFAULT_CLASS                       CLASS_A
// #define LORAWAN_DEFAULT_CLASS                       CLASS_C
/*!
 * LoRaWAN default confirm state
 */
#define LORAWAN_DEFAULT_CONFIRM_MSG_STATE           LORAWAN_UNCONFIRMED_MSG
/*!
 * User application data buffer size
 */
#define LORAWAN_APP_DATA_BUFF_SIZE                           64
/*!
 * User application data
 */
static uint8_t AppDataBuff[LORAWAN_APP_DATA_BUFF_SIZE];

/*!
 * User application data structure
 */
//static lora_AppData_t AppData={ AppDataBuff,  0 ,0 };
lora_AppData_t AppData = { AppDataBuff,  0, 0 };

/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* call back when LoRa endNode has received a frame*/
static void LORA_RxData(lora_AppData_t *AppData);

/* call back when LoRa endNode has just joined*/
static void LORA_HasJoined(void);

/* call back when LoRa endNode has just switch the class*/
static void LORA_ConfirmClass(DeviceClass_t Class);

/* call back when server needs endNode to send a frame*/
static void LORA_TxNeeded(void);

/* callback to get the battery level in % of full charge (254 full charge, 0 no charge)*/
static uint8_t LORA_GetBatteryLevel(void);

/* LoRa endNode send request*/
static void Send(void *context);

/* start the tx process*/
static void LoraStartTx(TxEventType_t EventType);

/* tx timer callback function*/
static void OnTxTimerEvent(void *context);


static void OnTxTimerAndTxNumEvent(void *context);
static void OnTxTimerAndTxNumEvent_Action(void *context);
static int sensor_send(void);

/* tx timer callback function*/
static void LoraMacProcessNotify(void);

/* Private variables ---------------------------------------------------------*/
/* load Main call backs structure*/
static LoRaMainCallback_t LoRaMainCallbacks = { LORA_GetBatteryLevel,
                                                HW_GetTemperatureLevel,
                                                node_id_restore,
                                                HW_GetRandomSeed,
                                                LORA_RxData,
                                                LORA_HasJoined,
                                                LORA_ConfirmClass,
                                                LORA_TxNeeded,
                                                LoraMacProcessNotify
                                              };
LoraFlagStatus LoraMacProcessRequest = LORA_RESET;
LoraFlagStatus AppProcessRequest = LORA_RESET;
LoraFlagStatus AppGPSRequest = LORA_RESET;


/*!
 * Specifies the state of the application LED
 */
static uint8_t AppLedStateOn = RESET;

static TimerEvent_t TxTimer;

#ifdef USE_B_L072Z_LRWAN1
/*!
 * Timer to handle the application Tx Led to toggle
 */
static TimerEvent_t TxLedTimer;
static void OnTimerLedEvent(void *context);
#endif
/* !
 *Initialises the Lora Parameters
 */
static  LoRaParam_t LoRaParamInit = {LORAWAN_ADR_STATE,
                                      LORAWAN_DEFAULT_DATA_RATE,
                                      LORAWAN_PUBLIC_NETWORK
                                    };

/* Private functions ---------------------------------------------------------*/
extern int32_t the_given_second;

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
	uint8_t				node_id;

  /* STM32 HAL library initialization*/
  HAL_Init();
#if !CHIRPBOX_LORAWAN
  /* Configure the system clock*/
  SystemClock_Config();
#else
	gpi_platform_init();
#endif

  /* Configure the debug mode*/
  DBG_Init();

  /* Configure the hardware*/
  HW_Init();
#if !CHIRPBOX_LORAWAN
  MX_GPIO_Init();
#endif

#if GPS_DATA
	GPS_Init();
	GPS_On();
	GPS_Waiting_PPS(10);
#endif

  lora_rx_count_rece = 0;
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /*Disbale Stand-by mode*/
  LPM_SetOffMode(LPM_APPLI_Id, LPM_Disable);

	PRINTF("LoRaWAN System started\n");

  uint8_t node_id_buffer[8];
	node_id_restore(node_id_buffer);
	TOS_NODE_ID = (uint32_t)stm_node_id;

	PRINTF("starting node %x ...\n", TOS_NODE_ID);
  srand(TOS_NODE_ID);

	node_id_allocate = node_id;

  PRINTF("APP_VERSION= %02X.%02X.%02X.%02X\r\n", (uint8_t)(__APP_VERSION >> 24), (uint8_t)(__APP_VERSION >> 16), (uint8_t)(__APP_VERSION >> 8), (uint8_t)__APP_VERSION);
  PRINTF("MAC_VERSION= %02X.%02X.%02X.%02X\r\n", (uint8_t)(__LORA_MAC_VERSION >> 24), (uint8_t)(__LORA_MAC_VERSION >> 16), (uint8_t)(__LORA_MAC_VERSION >> 8), (uint8_t)__LORA_MAC_VERSION);

  /* Configure the Lora Stack*/
  LORA_Init(&LoRaMainCallbacks, &LoRaParamInit);

  LORA_Join();

  LoraStartTx(TX_ON_TIMER);
  while(1)
  {
      if (AppProcessRequest == LORA_SET)
      {
        /*reset notification flag*/
        AppProcessRequest = LORA_RESET;
        /*Send*/
        Send(NULL);
      }
      if (LoraMacProcessRequest == LORA_SET)
      {
        /*reset notification flag*/
        LoraMacProcessRequest = LORA_RESET;
        LoRaMacProcess();
      }
      /*If a flag is set at this point, mcu must not enter low power and must loop*/
      DISABLE_IRQ();

      /* if an interrupt has occurred after DISABLE_IRQ, it is kept pending
        * and cortex will not enter low power anyway  */
      if ((LoraMacProcessRequest != LORA_SET) && (AppProcessRequest != LORA_SET) )
      {
    #ifndef LOW_POWER_DISABLE
        LPM_EnterLowPower();
    #endif
      }
      ENABLE_IRQ();
  }
  // TRACE_MSG("send:%lu\n", send_count);
}


void LoraMacProcessNotify(void)
{
  LoraMacProcessRequest = LORA_SET;
}


static void LORA_HasJoined(void)
{
#if( OVER_THE_AIR_ACTIVATION != 0 )
  PRINTF("JOINED\n\r");
  LL_FLASH_PageErase(RESET_PAGE);
  // /* send everytime timer elapses */
  // TimerInit(&TxTimer, OnTxTimerEvent);
  // TimerSetValue(&TxTimer, APP_TX_DUTYCYCLE);
  // OnTxTimerEvent(NULL);
#endif
  LORA_RequestClass(LORAWAN_DEFAULT_CLASS);
}

static void Send(void *context)
{
  // uint32_t time_value = (rand() % 11) + 5;
  // printf("time_value:%lu\n");
  // TimerSetValue(&TxTimer, time_value);
  // TimerStart(&TxTimer);
  if (LORA_JoinStatus() != LORA_SET)
  {
    PRINTF("LORA_Join\n");
    /*Not joined, try again later*/
    lora_tx_rate(DR_0);
    LORA_Join();
    return;
  }
  else if(LORA_JoinStatus() == LORA_SET)
  {
    if (send_count < fut_config.CUSTOM[FUT_MAX_SEND])
    {
      lora_tx_rate(DR_5);
      sensor_send();
    }
    else
    {
      TimerStop(&TxTimer);
      TRACE_MSG("rx_time:%lu, tx_time: %lu, cpu: %lu, stop: %lu\n", gpi_tick_slow_to_us(energest_type_time(ENERGEST_TYPE_LISTEN)), gpi_tick_slow_to_us(energest_type_time(ENERGEST_TYPE_TRANSMIT)), gpi_tick_slow_to_us(energest_type_time(ENERGEST_TYPE_CPU)), gpi_tick_slow_to_us(energest_type_time(ENERGEST_TYPE_STOP)));
    }
    return;
  }
}

static void LORA_RxData(lora_AppData_t *AppData)
{
  /* USER CODE BEGIN 4 */
  PRINTF("PACKET RECEIVED ON PORT %d\n\r", AppData->Port);

  switch (AppData->Port)
  {
    case 3:
      /*this port switches the class*/
      if (AppData->BuffSize == 1)
      {
        switch (AppData->Buff[0])
        {
          case 0:
          {
            LORA_RequestClass(CLASS_A);
            break;
          }
          case 1:
          {
            LORA_RequestClass(CLASS_B);
            break;
          }
          case 2:
          {
            LORA_RequestClass(CLASS_C);
            break;
          }
          default:
            break;
        }
      }
      break;
    case LORAWAN_APP_PORT:
      if (AppData->BuffSize == 1)
      {
        AppLedStateOn = AppData->Buff[0] & 0x01;
        if (AppLedStateOn == RESET)
        {
          PRINTF("LED OFF\n\r");
          LED_Off(LED_BLUE) ;
        }
        else
        {
          PRINTF("LED ON\n\r");
          LED_On(LED_BLUE) ;
        }
      }
      break;
    case LPP_APP_PORT:
    {
      AppLedStateOn = (AppData->Buff[2] == 100) ?  0x01 : 0x00;
      if (AppLedStateOn == RESET)
      {
        PRINTF("LED OFF\n\r");
        LED_Off(LED_BLUE) ;

      }
      else
      {
        PRINTF("LED ON\n\r");
        LED_On(LED_BLUE) ;
      }
      break;
    }
    default:
      break;
  }
  /* USER CODE END 4 */
}

static void OnTxTimerEvent(void *context)
{
  uint8_t time_value = fut_config.CUSTOM[FUT_DATA_SEND_INTERVAL];
  TimerSetValue(&TxTimer, time_value * 1000);
  TimerStart(&TxTimer);
  AppProcessRequest = LORA_SET;
  #if ENERGEST_CONF_ON
    PRINTF("rx_time:%lu, tx_time: %lu, cpu: %lu, stop: %lu\n", gpi_tick_slow_to_us(energest_type_time(ENERGEST_TYPE_LISTEN)), gpi_tick_slow_to_us(energest_type_time(ENERGEST_TYPE_TRANSMIT)), gpi_tick_slow_to_us(energest_type_time(ENERGEST_TYPE_CPU)), gpi_tick_slow_to_us(energest_type_time(ENERGEST_TYPE_STOP)));
  #endif
}

static int sensor_send(void)
{
  AppData.Port = LORAWAN_APP_PORT;
  uint32_t i = 0;
  send_count++;
  PRINTF("send:%lu\n", send_count);
  AppData.Buff[i++] = send_count >> 8;
  AppData.Buff[i++] = send_count;
  AppData.Buff[i++] = 0xff;
  AppData.Buff[i++] = 0xee;
  AppData.Buff[i++] = 0xdd;
  AppData.Buff[i++] = 0xcc;
  AppData.Buff[i++] = 0xbb;
  AppData.Buff[i++] = 0xaa;
  // AppData.Buff[i++] = TOS_NODE_ID >> 24;
  // AppData.Buff[i++] = TOS_NODE_ID >> 16;
  // AppData.Buff[i++] = TOS_NODE_ID >> 8;
  // AppData.Buff[i++] = TOS_NODE_ID;

  AppData.BuffSize = i;
  for ( i = 0; i < AppData.BuffSize; i++)
  {
    /* code */
    PRINTF("%02x ", AppData.Buff[i]);
  }
  PRINTF("\n");

  LORA_send(&AppData, LORAWAN_DEFAULT_CONFIRM_MSG_STATE);

  return 0;
}

static void LoraStartTx(TxEventType_t EventType)
{
  if (EventType == TX_ON_TIMER)
  {
    /* send everytime timer elapses */
    TimerInit(&TxTimer, OnTxTimerEvent);
    TimerSetValue(&TxTimer, JOIN_TX_DUTYCYCLE);
    OnTxTimerEvent(NULL);
  }
}

static void LORA_ConfirmClass(DeviceClass_t Class)
{
  PRINTF("switch to class %c done\n\r", "ABC"[Class]);

  /*Optionnal*/
  /*informs the server that switch has occurred ASAP*/
  AppData.BuffSize = 0;
  AppData.Port = LORAWAN_APP_PORT;

  LORA_send(&AppData, LORAWAN_UNCONFIRMED_MSG);
  TimerStop(&TxTimer);
}

static void LORA_TxNeeded(void)
{
  AppData.BuffSize = 0;
  AppData.Port = LORAWAN_APP_PORT;

  LORA_send(&AppData, LORAWAN_UNCONFIRMED_MSG);
}

/**
  * @brief This function return the battery level
  * @param none
  * @retval the battery level  1 (very low) to 254 (fully charged)
  */
uint8_t LORA_GetBatteryLevel(void)
{
  uint16_t batteryLevelmV;
  uint8_t batteryLevel = 0;

  batteryLevelmV = HW_GetBatteryLevel();


  /* Convert batterey level from mV to linea scale: 1 (very low) to 254 (fully charged) */
  if (batteryLevelmV > VDD_BAT)
  {
    batteryLevel = LORAWAN_MAX_BAT;
  }
  else if (batteryLevelmV < VDD_MIN)
  {
    batteryLevel = 0;
  }
  else
  {
    batteryLevel = (((uint32_t)(batteryLevelmV - VDD_MIN) * LORAWAN_MAX_BAT) / (VDD_BAT - VDD_MIN));
  }

  return batteryLevel;
}

#ifdef USE_B_L072Z_LRWAN1
static void OnTimerLedEvent(void *context)
{
  LED_Off(LED_RED1) ;
}
#endif
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
