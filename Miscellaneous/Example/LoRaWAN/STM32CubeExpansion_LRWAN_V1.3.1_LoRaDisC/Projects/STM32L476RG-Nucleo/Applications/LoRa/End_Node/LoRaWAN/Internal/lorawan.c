//**************************************************************************************************
//**** Includes ************************************************************************************
#include "lorawan.h"
#include "low_power_manager.h"
#include "Commissioning.h"
#include "version.h"
#include "lora.h"
#if CHIRPBOX_LORAWAN
    #include "loradisc.h"
    #include "gpi/olf.h"
#endif
//**************************************************************************************************
//***** Local Defines and Consts *******************************************************************

//**************************************************************************************************
//***** Local Typedefs and Class Declarations ******************************************************

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

static int sensor_send(void);

/* tx timer callback function*/
static void LoraMacProcessNotify(void);

//**************************************************************************************************
//***** Forward Declarations ***********************************************************************

//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************
static uint16_t send_count = 0;

/* !
 *Initialises the Lora Parameters
 */
static LoRaParam_t LoRaParamInit = {LORAWAN_ADR_STATE,
                                    LORAWAN_DEFAULT_DATA_RATE,
                                    LORAWAN_PUBLIC_NETWORK};

/* load Main call backs structure*/
static LoRaMainCallback_t LoRaMainCallbacks = {LORA_GetBatteryLevel,
                                               HW_GetTemperatureLevel,
                                               node_id_restore,
                                               HW_GetRandomSeed,
                                               LORA_RxData,
                                               LORA_HasJoined,
                                               LORA_ConfirmClass,
                                               LORA_TxNeeded,
                                               LoraMacProcessNotify};

LoraFlagStatus LoraMacProcessRequest = LORA_RESET;
LoraFlagStatus AppProcessRequest = LORA_RESET;

/*!
 * Specifies the state of the application LED
 */
static uint8_t AppLedStateOn = RESET;

static TimerEvent_t TxTimer;

//**************************************************************************************************
//***** Global Variables ***************************************************************************
uint32_t lora_rx_count_rece;
volatile uint32_t device_id[3];

uint32_t __attribute__((section(".data"))) TOS_NODE_ID = 0;

/*!
 * User application data buffer size
 */
#define LORAWAN_APP_DATA_BUFF_SIZE 64

/*!
 * User application data
 */
static uint8_t AppDataBuff[LORAWAN_APP_DATA_BUFF_SIZE];

/*!
 * User application data structure
 */
// static lora_AppData_t AppData={ AppDataBuff,  0 ,0 };
lora_AppData_t AppData = {AppDataBuff, 0, 0};

volatile chirpbox_fut_config __attribute((section (".FUTSettingSection"))) fut_config ={0xFFFF, 5, 0, 5, 0, 0x00000001, 0x14, 0x03, 0x19, 0x07, 0x0C, 0x0A};

extern LoRaDisC_Discover_Config loradisc_discover_config;
extern LoRaDisC_Energy energy_stats;

//**************************************************************************************************
//***** Local Functions ****************************************************************************

//**************************************************************************************************
//***** Global Functions ***************************************************************************
void lorawan_start()
{
	chirp_isr.state = ISR_LPWAN;

    lora_rx_count_rece = 0;

    /*Disbale Stand-by mode*/
    LPM_SetOffMode(LPM_APPLI_Id, LPM_Disable);

    PRINTF("LoRaWAN System started\n");

    DS3231_GetTime();
    /* Set alarm */
    Chirp_Time ds3231_time = DS3231_ShowTime();
    log_to_flash("starting time %d, %d\n", ds3231_time.chirp_hour, ds3231_time.chirp_min);

    log_to_flash("starting node %x ...\n", TOS_NODE_ID);
    log_flush();
    srand(TOS_NODE_ID);
    rand();

    PRINTF("APP_VERSION= %02X.%02X.%02X.%02X\r\n", (uint8_t)(__APP_VERSION >> 24), (uint8_t)(__APP_VERSION >> 16), (uint8_t)(__APP_VERSION >> 8), (uint8_t)__APP_VERSION);
    PRINTF("MAC_VERSION= %02X.%02X.%02X.%02X\r\n", (uint8_t)(__LORA_MAC_VERSION >> 24), (uint8_t)(__LORA_MAC_VERSION >> 16), (uint8_t)(__LORA_MAC_VERSION >> 8), (uint8_t)__LORA_MAC_VERSION);

    /* Configure the Lora Stack*/
    if (fut_config.CUSTOM[FUT_UPLINK_RATE] != 0xFFFFFFFF)
        LoRaParamInit.TxDatarate = fut_config.CUSTOM[FUT_UPLINK_RATE];
    else
        LoRaParamInit.TxDatarate = DR_0;

    LORA_Init(&LoRaMainCallbacks, &LoRaParamInit);

    LORA_Join();

    #if USE_FOR_LORAWAN && LORADISC
        /* discover config */
        loradisc_discover_init();
        /* lorawan config */
        if ((loradisc_discover_config.lorawan_bitmap & (1 << (node_id_allocate % 32))))
            LoraStartTx(TX_ON_TIMER);
        else
            AppProcessRequest = LORA_SET;
    #else
        LoraStartTx(TX_ON_TIMER);
    #endif

    while (1)
    {
        if (AppProcessRequest == LORA_SET)
        {
            /*reset notification flag*/
            AppProcessRequest = LORA_RESET;
            #if !(USE_FOR_LORAWAN && LORADISC)
                /*Send*/
                Send(NULL);
            #else
                /* lorawan nodes */
                if ((loradisc_discover_config.lorawan_bitmap & (1 << (node_id_allocate % 32))))
                {
                    /* lorawan is on */
                    if(loradisc_discover_config.lorawan_on)
                    {
                        if(loradisc_discover_config.discover_on)
                        {
                            /* wakeup after the lorawan packet ends */
                            lpwan_grid_timer_init(loradisc_discover_config.lorawan_duration);
                        }
                        else
                        {
                            /* next loradisc */
                            loradisc_discover_config.next_loradisc_gap = calculate_next_loradisc();
                            printf("--- next:%lu, now:%lu\n", loradisc_discover_config.next_loradisc_gap, gpi_tick_slow_extended());
                            if (loradisc_discover_config.next_loradisc_gap != 0xFFFFFFFF)
                                lpwan_grid_timer_init(loradisc_discover_config.next_loradisc_gap - gpi_tick_slow_extended());
                            else
                                lpwan_grid_timer_init(GPI_TICK_S_TO_SLOW(LPM_TIMER_UPDATE_S));
                        }
                        loradisc_discover_config.lorawan_begin[node_id_allocate] = gpi_tick_slow_extended() + GPI_TICK_S_TO_SLOW(loradisc_discover_config.lorawan_interval_s[node_id_allocate]);

                        /* LoRaWAN */
                        LORA_Init_node_id(TOS_NODE_ID);
                        LORA_ReInit();
                        LORA_Join();
                        /*Send*/
                        Send(NULL);
                        loradisc_discover_config.lorawan_on ^= 1;
                    }
                    /* loradisc is on */
                    else
                    {
                        if(loradisc_discover_config.discover_on)
                        {
                            /* continue discovery until ends */
                            while ((loradisc_discover_config.discover_on) && (!loradisc_discover_config.lorawan_on))
                            {
                                if (compare_discover_initiator_expired())
                                {
                                    loradisc_discover(loradisc_discover_config.lorawan_interval_s[node_id_allocate]);
                                }
                                else
                                {
                                    lpwan_grid_timer_init(GPI_TICK_S_TO_SLOW(LPM_TIMER_UPDATE_S));
                                    break;
                                }
                            }
                            /* when discovery ends, immediately start LoRaDisC collect */
                            if (!loradisc_discover_config.discover_on)
                            {
                                log_to_flash("discovery_stats: %lu, %lu, %lu, %lu\n", gpi_tick_slow_to_us(energy_stats.CPU), gpi_tick_slow_to_us(energy_stats.LPM), gpi_tick_slow_to_us(energy_stats.TRANSMIT), gpi_tick_slow_to_us(energy_stats.LISTEN));
                                log_flush();
                                loradisc_discover_config.collect_on = 1;
                                loradisc_discover_config.next_loradisc_gap = calculate_next_loradisc();
                                /* next is collection */
                                if (loradisc_discover_config.next_loradisc_gap != 0xFFFFFFFF)
                                {
                                    // if(gpi_tick_compare_slow_extended(gpi_tick_slow_extended(), loradisc_discover_config.next_loradisc_gap) >= 0)
                                    // {
                                    //     lpwan_grid_timer_init(GPI_TICK_MS_TO_SLOW(1));
                                    // }
                                    // else
                                    // {
                                        lpwan_grid_timer_init(loradisc_discover_config.next_loradisc_gap - gpi_tick_slow_extended());
                                    // }
                                }
                                else
                                    lpwan_grid_timer_init(GPI_TICK_S_TO_SLOW(LPM_TIMER_UPDATE_S));
                            }
                        }
                        /* LoRaDisC collection */
                        else if ((loradisc_discover_config.collect_on) && (!loradisc_discover_config.loradisc_lorawan_on))
                        {
                            uint8_t collect_end = loradisc_collect();
                            if (collect_end)
                            {
                                /* next is lorawan relay */
                                loradisc_discover_config.next_loradisc_gap = calculate_next_loradisc();
                                if (loradisc_discover_config.next_loradisc_gap != 0xFFFFFFFF)
                                    lpwan_grid_timer_init(loradisc_discover_config.next_loradisc_gap - gpi_tick_slow_extended());
                                else
                                    lpwan_grid_timer_init(GPI_TICK_S_TO_SLOW(LPM_TIMER_UPDATE_S));
                            }
                            else
                            {
                                log_to_flash("energy_stats: %lu, %lu, %lu, %lu\n", gpi_tick_slow_to_us(energy_stats.CPU), gpi_tick_slow_to_us(energy_stats.LPM), gpi_tick_slow_to_us(energy_stats.TRANSMIT), gpi_tick_slow_to_us(energy_stats.LISTEN));
                                log_flush();
                                lpwan_grid_timer_init(GPI_TICK_S_TO_SLOW(LPM_TIMER_UPDATE_S));
                            }
                        }
                        /* start LoRaWAN relay LoRaDisC packets */
                        else if ((loradisc_discover_config.collect_on) && (loradisc_discover_config.loradisc_lorawan_on))
                        {
                            if(loradisc_discover_config.loradisc_on)
                            {
                                /* wait until all lorawan nodes relay lorawan packet */
                                /* next will return back and then is dissem */
                                if(gpi_tick_compare_slow_extended(gpi_tick_slow_extended(), loradisc_discover_config.next_loradisc_gap + loradisc_discover_config.lorawan_duration_slow) < 0)
                                {
                                    lpwan_grid_timer_init(loradisc_discover_config.next_loradisc_gap + loradisc_discover_config.lorawan_duration_slow - gpi_tick_slow_extended());
                                }
                                else
                                {
                                    /* change the state */
                                    if (loradisc_discover_config.dissem_on)
                                        loradisc_discover_config.collect_on = 0;

                                    loradisc_discover_config.loradisc_lorawan_on = 0;
                                    /* next is dissem or collect, depends on dissem_on */
                                    loradisc_discover_config.next_loradisc_gap = calculate_next_loradisc();
                                    if (loradisc_discover_config.next_loradisc_gap != 0xFFFFFFFF)
                                    {
                                        // if(gpi_tick_compare_slow_extended(gpi_tick_slow_extended(), loradisc_discover_config.next_loradisc_gap) >= 0)
                                        // {
                                        //     lpwan_grid_timer_init(GPI_TICK_MS_TO_SLOW(1));
                                        // }
                                        // else
                                        // {
                                            lpwan_grid_timer_init(loradisc_discover_config.next_loradisc_gap - gpi_tick_slow_extended());
                                        // }
                                    }
                                    else
                                        lpwan_grid_timer_init(GPI_TICK_S_TO_SLOW(LPM_TIMER_UPDATE_S));
                                }
                            }
                            else
                            {
                                printf("start relay!!!!\n");
                                /* start to relay LoRaDisC packets with LoRaWAN */
                                /* LoRaWAN */
                                uint32_t node_id = lorawan_relay_node_id_allocate(loradisc_discover_config.lorawan_relay_id);
                                printf("node_id:%x\n", node_id);

                                LORA_Init_node_id(node_id);
                                LORA_ReInit();
                                LORA_Join();
                                /*Send*/
                                uint8_t relay_ends = lorawan_relay_collect(loradisc_discover_config.lorawan_relay_id);
                                loradisc_discover_config.lorawan_relay_id++;
                                /* if all relay packets end, then switch to loradisc (for flag) */
                                if (relay_ends)
                                {
                                    loradisc_discover_config.loradisc_on = 1;
                                }
                                lpwan_grid_timer_init(loradisc_discover_config.lorawan_duration);
                            }
                        }
                        else if (loradisc_discover_config.dissem_on)
                        {
                            // TODO: start dissem
                            // loradisc_dissem();

                            /* change the state */
                            loradisc_discover_config.dissem_on = 0;
                            loradisc_discover_config.collect_on = 1;
                        }
                    }
                }
                /* LoRaDisC nodes */
                else
                {
                    if(loradisc_discover_config.discover_on)
                    {
                        while (loradisc_discover_config.discover_on)
                        {
                            loradisc_discover(NULL);
                        }
                        /* when discovery ends, immediately start LoRaDisC collect */
                        if (!loradisc_discover_config.discover_on)
                        {
                            log_to_flash("discovery_stats: %lu, %lu, %lu, %lu\n", gpi_tick_slow_to_us(energy_stats.CPU), gpi_tick_slow_to_us(energy_stats.LPM), gpi_tick_slow_to_us(energy_stats.TRANSMIT), gpi_tick_slow_to_us(energy_stats.LISTEN));
                            log_flush();
                            loradisc_discover_config.collect_on = 1;
                            loradisc_discover_config.next_loradisc_gap = calculate_next_loradisc();
                            if (loradisc_discover_config.next_loradisc_gap != 0xFFFFFFFF)
                            {
                                // if(gpi_tick_compare_slow_extended(gpi_tick_slow_extended(), loradisc_discover_config.next_loradisc_gap) >= 0)
                                // {
                                //     lpwan_grid_timer_init(GPI_TICK_MS_TO_SLOW(1));
                                // }
                                // else
                                // {
                                    lpwan_grid_timer_init(loradisc_discover_config.next_loradisc_gap - gpi_tick_slow_extended());
                                // }
                            }
                            else
                                lpwan_grid_timer_init(GPI_TICK_S_TO_SLOW(LPM_TIMER_UPDATE_S));
                        }
                    }
                    /* LoRaDisC collection */
                    else if ((loradisc_discover_config.collect_on) && (!loradisc_discover_config.loradisc_lorawan_on))
                    {
                        uint8_t collect_end = loradisc_collect();
                        /* next is lorawan relay, will wake up to calculate dissem time */
                        loradisc_discover_config.next_loradisc_gap = calculate_next_loradisc();
                        if (loradisc_discover_config.next_loradisc_gap != 0xFFFFFFFF)
                            lpwan_grid_timer_init(loradisc_discover_config.next_loradisc_gap - gpi_tick_slow_extended());
                        else
                            lpwan_grid_timer_init(GPI_TICK_S_TO_SLOW(LPM_TIMER_UPDATE_S));
                        if (!collect_end)
                        {
                            log_to_flash("energy_stats: %lu, %lu, %lu, %lu\n", gpi_tick_slow_to_us(energy_stats.CPU), gpi_tick_slow_to_us(energy_stats.LPM), gpi_tick_slow_to_us(energy_stats.TRANSMIT) - send_count * 32000, gpi_tick_slow_to_us(energy_stats.LISTEN) - send_count * 2000000);
                            log_flush();
                        }
                    }
                    /* start LoRaWAN relay LoRaDisC packets */
                    else if ((loradisc_discover_config.collect_on) && (loradisc_discover_config.loradisc_lorawan_on))
                    {
                        /* wait until all lorawan nodes relay lorawan packet */
                        /* next will return back and then is dissem */
                        if (gpi_tick_compare_slow_extended(gpi_tick_slow_extended(), loradisc_discover_config.next_loradisc_gap + loradisc_discover_config.lorawan_duration_slow) < 0)
                        {
                            lpwan_grid_timer_init(loradisc_discover_config.next_loradisc_gap + loradisc_discover_config.lorawan_duration_slow - gpi_tick_slow_extended());
                        }
                        else
                        {
                            /* change the state */
                            if (loradisc_discover_config.dissem_on)
                                loradisc_discover_config.collect_on = 0;

                            loradisc_discover_config.loradisc_lorawan_on = 0;
                            /* next is dissem */
                            loradisc_discover_config.next_loradisc_gap = calculate_next_loradisc();
                            if (loradisc_discover_config.next_loradisc_gap != 0xFFFFFFFF)
                            {
                                // if(gpi_tick_compare_slow_extended(gpi_tick_slow_extended(), loradisc_discover_config.next_loradisc_gap) >= 0)
                                // {
                                //     lpwan_grid_timer_init(GPI_TICK_MS_TO_SLOW(1));
                                // }
                                // else
                                // {
                                    lpwan_grid_timer_init(loradisc_discover_config.next_loradisc_gap - gpi_tick_slow_extended());
                                // }
                            }
                            else
                                lpwan_grid_timer_init(GPI_TICK_S_TO_SLOW(LPM_TIMER_UPDATE_S));
                        }
                    }
                    else if (loradisc_discover_config.dissem_on)
                    {
                        // TODO: start dissem
                        // loradisc_dissem();

                        /* change the state */
                        loradisc_discover_config.dissem_on = 0;
                        loradisc_discover_config.collect_on = 1;
                    }
                }
                // calculate_next_lorawan();
                chirp_isr.state = ISR_LPWAN;
            #endif
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
        if ((LoraMacProcessRequest != LORA_SET) && (AppProcessRequest != LORA_SET))
        {
            #ifndef LOW_POWER_DISABLE
                LPM_EnterLowPower();
            #endif
        }
        ENABLE_IRQ();
    }
}

void node_id_restore(uint8_t *id)
{
    id[7] = DEVICE_ID_REG0;
    id[6] = DEVICE_ID_REG0 >> 8;
    id[5] = DEVICE_ID_REG0 >> 16;
    id[4] = DEVICE_ID_REG0 >> 24;
    id[3] = 0;
    id[2] = 0;
    id[1] = 0;
    id[0] = 0;
	TOS_NODE_ID = (uint32_t)(DEVICE_ID_REG0);
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
                LED_Off(LED_BLUE);
            }
            else
            {
                PRINTF("LED ON\n\r");
                LED_On(LED_BLUE);
            }
        }
        break;
    default:
        break;
    }
    /* USER CODE END 4 */
}

static void LORA_HasJoined(void)
{
#if (OVER_THE_AIR_ACTIVATION != 0)
    Chirp_Time time = obtain_rtc_time();
    log_to_flash("JOINED, time:%d-%d:%d:%d\n", time.chirp_date, time.chirp_hour, time.chirp_min, time.chirp_sec);
    log_flush();
    LL_FLASH_PageErase(RESET_PAGE);
#endif
    LORA_RequestClass(LORAWAN_DEFAULT_CLASS);
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

void LoraMacProcessNotify(void)
{
    LoraMacProcessRequest = LORA_SET;
}

static void Send(void *context)
{
    if (LORA_JoinStatus() != LORA_SET)
    {
        /*Not joined, try again later*/
        if (fut_config.CUSTOM[FUT_UPLINK_RATE] != 0xFFFFFFFF)
            lora_tx_rate(fut_config.CUSTOM[FUT_UPLINK_RATE]);
        else
            lora_tx_rate(DR_0);
        LORA_Join();
        return;
    }
    else if (LORA_JoinStatus() == LORA_SET)
    {
        if ((fut_config.CUSTOM[FUT_UPLINK_RATE] != 0xFFFFFFFF) && (send_count < fut_config.CUSTOM[FUT_MAX_SEND]))
        {
            lora_tx_rate(fut_config.CUSTOM[FUT_UPLINK_RATE]);
            sensor_send();
        }
        else if ((fut_config.CUSTOM[FUT_UPLINK_RATE] == 0xFFFFFFFF) && (send_count < fut_config.CUSTOM[FUT_MAX_SEND] * 6))
        {
            lora_tx_rate(send_count / fut_config.CUSTOM[FUT_MAX_SEND]);
            if ((send_count % fut_config.CUSTOM[FUT_MAX_SEND] == 0) && (send_count > 0))
            {
                // log_to_flash("rx_time:%lu, tx_time: %lu\n", gpi_tick_slow_to_us(energest_type_time(ENERGEST_TYPE_LISTEN)), gpi_tick_slow_to_us(energest_type_time(ENERGEST_TYPE_TRANSMIT)));
                // log_to_flash("cpu: %lu, stop: %lu\n", gpi_tick_slow_to_us(energest_type_time(ENERGEST_TYPE_CPU)), gpi_tick_slow_to_us(energest_type_time(ENERGEST_TYPE_STOP)));
                // log_flush();
            }
            sensor_send();
        }
        else
        {
            TimerStop(&TxTimer);
            // log_to_flash("rx_time:%lu, tx_time: %lu\n", gpi_tick_slow_to_us(energest_type_time(ENERGEST_TYPE_LISTEN)), gpi_tick_slow_to_us(energest_type_time(ENERGEST_TYPE_TRANSMIT)));
            // log_to_flash("cpu: %lu, stop: %lu\n", gpi_tick_slow_to_us(energest_type_time(ENERGEST_TYPE_CPU)), gpi_tick_slow_to_us(energest_type_time(ENERGEST_TYPE_STOP)));
            // log_flush();
        }
        return;
    }
}

static void OnTxTimerEvent(void *context)
{
    uint32_t time_value;
    #if USE_FOR_LORAWAN && LORADISC
        time_value = loradisc_discover_config.lorawan_interval_s[node_id_allocate] * 1000;
        loradisc_discover_config.lorawan_on = 1;
        loradisc_discover_config.loradisc_on = 0;
    #else
        if (fut_config.CUSTOM[FUT_RANDOM_INTERVAL] == 1)
        {
            time_value = rand() % (fut_config.CUSTOM[FUT_DATA_SEND_INTERVAL] * 1000) + fut_config.CUSTOM[FUT_DATA_SEND_INTERVAL] * 1000;
        }
        else
            time_value = fut_config.CUSTOM[FUT_DATA_SEND_INTERVAL] * 1000;
    #endif
    TimerSetValue(&TxTimer, time_value);
    TimerStart(&TxTimer);
    AppProcessRequest = LORA_SET;
#if ENERGEST_CONF_ON
    // PRINTF("rx_time:%lu, tx_time: %lu, cpu: %lu, stop: %lu\n", gpi_tick_slow_to_us(energest_type_time(ENERGEST_TYPE_LISTEN)), gpi_tick_slow_to_us(energest_type_time(ENERGEST_TYPE_TRANSMIT)), gpi_tick_slow_to_us(energest_type_time(ENERGEST_TYPE_CPU)), gpi_tick_slow_to_us(energest_type_time(ENERGEST_TYPE_STOP)));
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

    AppData.BuffSize = i;
    for (i = 0; i < AppData.BuffSize; i++)
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
        uint32_t time_value;
        #if USE_FOR_LORAWAN && LORADISC
            time_value = loradisc_discover_config.lorawan_interval_s[node_id_allocate];
        #else
            if (fut_config.CUSTOM[FUT_RANDOM_INTERVAL] == 1)
            {
                time_value = rand() % (fut_config.CUSTOM[FUT_DATA_SEND_INTERVAL] * 1000) + fut_config.CUSTOM[FUT_DATA_SEND_INTERVAL] * 1000;
            }
            else
                time_value = fut_config.CUSTOM[FUT_DATA_SEND_INTERVAL] * 1000;
        #endif
        TimerSetValue(&TxTimer, time_value);
        OnTxTimerEvent(NULL);
    }
}
