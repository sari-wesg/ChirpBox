//**************************************************************************************************
//**** Includes ************************************************************************************
#include "chirp_internal.h"
//**************************************************************************************************
#include "gpi/tools.h" /* STRINGIFY(), LSB(), ASSERT_CT() */
#ifdef MX_CONFIG_FILE
#include STRINGIFY(MX_CONFIG_FILE)
#endif
#include "gpi/platform_spec.h"
#include "gpi/platform.h"
#include "gpi/clocks.h"
#include GPI_PLATFORM_PATH(radio.h)
#include GPI_PLATFORM_PATH(sx1276Regs_Fsk.h)
#include GPI_PLATFORM_PATH(sx1276Regs_LoRa.h)

#include "ds3231.h"
#include "mixer_internal.h"

#if MX_FLASH_FILE
	#include "flash_if.h"
#endif

#include "menu.h"


//**************************************************************************************************
//***** Local Defines and Consts *******************************************************************
#if DEBUG_CHIRPBOX
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif
/* LED indications */
#define RX_DONE_LED LED1
#define CAD_LED LED2
#define HEADER_LED LED3
#define TX_DONE_LED LED4
#define TIMER_LED LED5
#define CAD_DETECTED LED6
//**************************************************************************************************
//***** Local Typedefs and Class Declarations ******************************************************

//**************************************************************************************************
//***** Forward Declarations ***********************************************************************

//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************
Sniff sniff;               /* For indicate the state, radio configurations */
uint8_t node_id_offset;    /* Offset of the packet to obtain the node id */
uint8_t node_id_len;       /* The byte length to present the node id */
uint32_t TX_Node_Id;       /* For tx node (test use) */

//***** Auto assigned *******
static Sniff_stat node_list_head = {NULL, NULL, NULL, NULL, NULL}; /* header of the node list for statistic */
volatile uint8_t node_expired;                                              /* flag that there is expired */

uint32_t cad_detected_time; /* to calculate the radio on time */
//**************************************************************************************************
//***** Global Variables ***************************************************************************
Sniff_stat expired_node; /* node that radio on time is expired */
//**************************************************************************************************
//***** Local Functions ****************************************************************************

/**
 * @description: return that if the node is expired in one hour
 * @param p: pointer of the wanted know node
 * @return: true: the node is expired in one hour
 *          false: the node is not expired in one hour
 */
bool sniff_time_expired(Sniff_stat *p)
{
    uint32_t time_diff = (DS3231.Hour - p->begin_stat_time.Hour) * 3600 + (DS3231.Minute - p->begin_stat_time.Minute) * 60 + (DS3231.Second - p->begin_stat_time.Second);
    if (time_diff >= 3600)
        return true;
    else
        return false;
}

/**
 * @description: return that if the radio on time is expired at the duty cycle in one hour
 * @param p: pointer of the wanted know node
 * @return: true: the node's radio on time is expired
 *          false: the node's radio on time is expired
 */
bool sniff_radio_on_expired(Sniff_stat *p, uint8_t duty_cycle) /* percentage of 1 hour time */
{
    if (p->radio_on_time > (36000000 * duty_cycle)) /* 36000000 us is 1 % */
    // if (p->radio_on_time > (360000 * duty_cycle)) /* 36000000 us is 1 % */
        return true;
    else
        return false;
}

void sniff_stat_write(Sniff_stat *head)
{
    Sniff_stat *p1 = head;
    uint8_t i = 0;

    /* | node_id | radio_on_time | */
    uint32_t sniff_node[2];

    menu_preSend(0);
    FLASH_If_Erase_Pages(0, 255);

    while (p1->next != NULL)
    {
        memset(sniff_node, 0, sizeof(sniff_node));
        sniff_node[0] = p1->next->node_id;
        sniff_node[1] = p1->next->radio_on_time;
        PRINTF("sniff_node:%lu, %lu\n", sniff_node[0], sniff_node[1]);
        FLASH_If_Write(USER_FLASH_ADDRESS + i * sizeof(sniff_node), (uint32_t *)(sniff_node), sizeof(sniff_node) / sizeof(uint32_t));
        p1 = p1->next;
        i++;
    }
}

/**
 * @description: insert the node to the list, examine if old nodes are expired
 * @param head: header of the list
 * @param node_id: node_id, if LoRa network is STATE_LORA_FORM; else it's not used
 * @param radio_on_time: radio_on_time in us
 * @return: head: header of the list
 */
void sniff_stat_insert(Sniff_stat *head, uint32_t node_id, uint32_t radio_on_time)
{
    /* examine if long time no update */
    Sniff_stat *p1, *p;
    p1 = head;

    while (p1->next != NULL)
    {
        if (sniff_time_expired(p1->next)) /* Long time no update, delete the node */
        {
            Sniff_stat *q;
            q = p1->next;
            p1->next = p1->next->next;
            free(q);
        }
        else
        {
            p1 = p1->next;
        }
    }

    /* add the node to the list */
    p = (Sniff_stat *)malloc(sizeof(Sniff_stat));
    p->node_id = node_id;
    p->radio_on_time = radio_on_time;
    p->last_active_time.Hour = p->begin_stat_time.Hour = DS3231.Hour;
    p->last_active_time.Minute = p->begin_stat_time.Minute = DS3231.Minute;
    p->last_active_time.Second = p->begin_stat_time.Second = DS3231.Second;
    if (head->next == NULL) /* create the list */
    {
        head->next = p;
        p->next = NULL;
    }
    else
    {
        p1 = head;
        while ((p1->next->next != NULL) && (p->node_id > p1->next->node_id)) /* To find the node id that bigger than the new node */
            p1 = p1->next;

        if (p->node_id < p1->next->node_id) /* smaller node */
        {
            if (head == p1) /* node id is the min */
                p->next = p1->next, head->next = p;
            else
            {
                p->next = p1->next;
                p1->next = p;
            }
        }
        else if (p->node_id == p1->next->node_id)
        {
            p1->next->radio_on_time += radio_on_time;
            p1->next->last_active_time.Hour = DS3231.Hour; /* update last active time */
            p1->next->last_active_time.Minute = DS3231.Minute;
            p1->next->last_active_time.Second = DS3231.Second;
        }
        else
        {
            p1->next->next = p, p->next = NULL;
        }
    }

    uint8_t i = 0;
    while (head->next != NULL)
    {
        PRINTF("Node_id %d: %x, ", i + 1, head->next->node_id);
        PRINTF("Radio time %d: %d\n", i + 1, head->next->radio_on_time);
        PRINTF("begin_stat_time %d: %d: %d: %d\n", i + 1, head->next->begin_stat_time.Hour, head->next->begin_stat_time.Minute, head->next->begin_stat_time.Second);
        PRINTF("last_active_time %d: %d: %d: %d\n", i + 1, head->next->last_active_time.Hour, head->next->last_active_time.Minute, head->next->last_active_time.Second);
        head = head->next;
        i++;
    }
}

/**
 * @description: configure the radio as defined
 * @param {type}
 * @return:
 */
void sniff_radio_set(uint8_t lora_spreading_factor, uint8_t lora_bandwidth, uint8_t lora_codingrate, uint8_t lora_preamble_length, uint8_t tx_output_power, uint32_t lora_frequency)
{
    sniff.rf.sf = lora_spreading_factor;
    sniff.rf.bw = lora_bandwidth;
    sniff.rf.cr = lora_codingrate;
    sniff.rf.preamble = lora_preamble_length;
    sniff.rf.tx_power = tx_output_power;
    sniff.rf.frequency = lora_frequency;
    gpi_radio_set_spreading_factor(sniff.rf.sf);
    gpi_radio_set_bandwidth(sniff.rf.bw);
    gpi_radio_set_coding_rate(sniff.rf.cr);
    gpi_radio_set_preamble_len(sniff.rf.preamble);
    gpi_radio_set_tx_power(sniff.rf.tx_power);
    SX1276SetChannel(sniff.rf.frequency);
    if (sniff.net == STATE_LORAWAN)
    {
        SX1276Write(REG_LR_SYNCWORD, LORA_MAC_PUBLIC_SYNCWORD); /* Value 0x34 is reserved for LoRaWAN networks */
    }
}

/**
 * @description: increase SF in a circle
 * @param sf: the current SF value
 * @return:
 */
void sniff_SF_set(uint8_t *sf)
{
    SX1276SetOpMode(RFLR_OPMODE_SLEEP);
    // upper = 12, lower = 7;
    uint8_t old_sf = *sf;
    do
    {
        *sf = (mixer_rand() % (12 - 7 + 1)) + 7;
    } while (old_sf == *sf);
    // if (*sf == 12)
    // {
    //     *sf = 7;
    // }
    // else
    // {
    //     (*sf)++;
    // }
    gpi_radio_set_spreading_factor(*sf);
}

/**
 * @description: extract content from packet and statist node list
 * @param {type}
 * @return:
 */
void sniff_stat_node()
{
    uint32_t packet_time = (MAIN_TIMER_CNT_REG - cad_detected_time) * 1e6 / GPI_FAST_CLOCK_RATE;

    uint8_t *RX_TAMP_FIFO = (uint8_t *)malloc(node_id_offset + node_id_len);
    memset(RX_TAMP_FIFO, 0, node_id_offset + node_id_len);
    SX1276Write(REG_LR_FIFOADDRPTR, SX1276Read(REG_LR_FIFORXCURRENTADDR));
    SX1276ReadFifo(RX_TAMP_FIFO, node_id_offset + node_id_len); /* read the payload */
    uint32_t node_id = 0;
    uint8_t i;
    for (i = 0; i < node_id_len; i++)
        node_id |= (RX_TAMP_FIFO[node_id_offset + i - node_id_len] << ((node_id_len - i - 1) * 8));
    PRINTF("id:%x\n", RX_TAMP_FIFO[0]);

    sniff_stat_insert(&node_list_head, node_id, packet_time);
    PRINTF("packet_time:%lu\n", packet_time);
    __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_CC1);
    SX1276SetOpMode(RFLR_OPMODE_SLEEP);
}

/**
  * @brief  adjust sniff time automatically
  * @param  none
  * @retval none
  */
void sniff_grid()
{
    Gpi_Fast_Tick_Native d = sniff.sf_switch - MAIN_TIMER_CNT_REG;
    if ((int32_t)d <= 0)
    {
        sniff_SF_set(&sniff.rf.sf);
        /* rate = 125 kHz / (2 ^ sf) */
        uint32_t rs = 125000 / ( 1 << sniff.rf.sf );
        /* Symbol time : time for one symbol (us); interval time should not larger than 263 s */
        uint32_t ts = (uint32_t)1e6 / rs;
        /* for SF 12: 2 ^ 4 symbol time, for SF 7: 512 symbol time */
        sniff.sf_switch += GPI_TICK_US_TO_FAST((1 << (18 - sniff.rf.sf)) * ts);
    }
}

//**************************************************************************************************
//***** Global Functions ***************************************************************************
void sniff_init(Sniff_Net LoRa_Net, uint32_t lora_frequency, uint16_t end_year, uint8_t end_month, uint8_t end_date, uint8_t end_hour, uint8_t end_min, uint8_t end_sec)
{
	chirp_isr.state = ISR_SNIFF;

    sniff.sniff_end.chirp_year = end_year;
    sniff.sniff_end.chirp_month = end_month;
    sniff.sniff_end.chirp_date = end_date;
    sniff.sniff_end.chirp_hour = end_hour;
    sniff.sniff_end.chirp_min = end_min;
    sniff.sniff_end.chirp_sec = end_sec;
    sniff.net = LoRa_Net;
    /* if sniff the LoRaWAN network */
    if (sniff.net == STATE_LORAWAN)
    {
        /* For MACPayload LoRaWAN packet, DevAddr is 4 bytes long after the MHDR with 1 byte, see "LoRaWAN™ 1.0.3 Specification", Page 15. */
        node_id_offset = 5;
        node_id_len = 4;
    }
    /* if sniff a network that we already know the packet format */
    else if (sniff.net == STATE_LORA_FORM)
    {
        /* | node id  | payload | */
        node_id_offset = 0;
        node_id_len = 1;
    }
    /* initial SF is 7, default bandwidth is 125 kHz, coding rate is 5 / 4, preamble length is 8-symbol. */
    sniff_radio_set(7, 7, 1, 8, 14, lora_frequency * 1e3); /* radio configuration */
    sniff.sf_switch = MAIN_TIMER_CNT_REG + GPI_TICK_US_TO_FAST(1 << (16 - sniff.rf.sf));

    sniff_cad();
    while (1)
    {
        #if GPS_DATA
        if (node_expired)
        {
            /* examine gps time */
            Chirp_Time gps_time = GPS_Get_Time();

            time_t diff = GPS_Diff(&gps_time, sniff.sniff_end.chirp_year, sniff.sniff_end.chirp_month, sniff.sniff_end.chirp_date, sniff.sniff_end.chirp_hour, sniff.sniff_end.chirp_min, sniff.sniff_end.chirp_sec);
            /* close to stop sniffer */
            if ((diff < 3) && (diff > 0))
            {
                /* write the results to flash */
                sniff_stat_write(&node_list_head);
                Gpi_Fast_Tick_Native deadline = gpi_tick_fast_native() + GPI_TICK_S_TO_FAST(diff);
                while (gpi_tick_compare_fast_native(gpi_tick_fast_native(), deadline) < 0);
                break;
            }
            else if (diff <= 0)
                break;
            node_expired = 0;
            sniff_cad();
            chirp_isr.state = ISR_SNIFF;
        }
        #endif
    }
}
//**************************************************************************************************
void sniff_cad()
{
    gpi_led_on(CAD_LED);
    sniff_grid();
    /* Use CadDone instead of CadDetected. Once the calculation is finished the modem generates the CadDone interrupt. If the correlation was successful, CadDetected is generated simultaneously, see SX1276/77/78/79 - 137 MHz to 1020 MHz, Rev. 5, Page 44 */
    SX1276Write(REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
                                         // RFLR_IRQFLAGS_RXDONE |
                                         RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                         // RFLR_IRQFLAGS_VALIDHEADER |
                                         RFLR_IRQFLAGS_TXDONE |
                                         // RFLR_IRQFLAGS_CADDONE |
                                         RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL // |
                                        //  RFLR_IRQFLAGS_CADDETECTED
    );
    SX1276Write(REG_DIOMAPPING1, (SX1276Read(REG_DIOMAPPING1) & RFLR_DIOMAPPING1_DIO0_MASK & RFLR_DIOMAPPING1_DIO3_MASK) | RFLR_DIOMAPPING1_DIO0_10 | RFLR_DIOMAPPING1_DIO3_01);
    SX1276SetOpMode(RFLR_OPMODE_CAD);
    sniff.radio = SNIFF_CAD;
    sniff.state = SNIFF_CAD_DETECT;
    gpi_led_off(CAD_LED);
}
void sniff_rx()
{
    SX1276Write(REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
                                         // RFLR_IRQFLAGS_RXDONE |
                                         // RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                         // RFLR_IRQFLAGS_VALIDHEADER |
                                         RFLR_IRQFLAGS_TXDONE |
                                         RFLR_IRQFLAGS_CADDONE |
                                         RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                         RFLR_IRQFLAGS_CADDETECTED);
    /* DIO0: RxDone, DIO3: ValidHeader */
    SX1276Write(REG_DIOMAPPING1, (SX1276Read(REG_DIOMAPPING1) & RFLR_DIOMAPPING1_DIO0_MASK & RFLR_DIOMAPPING1_DIO3_MASK) | RFLR_DIOMAPPING1_DIO0_00 | RFLR_DIOMAPPING1_DIO3_01);
    SX1276SetOpMode(RFLR_OPMODE_RECEIVER);
    sniff.radio = SNIFF_RX;
}
void sniff_tx(uint32_t node_id) /* For test sniff */
{
	chirp_isr.state = ISR_SNIFF;

    TX_Node_Id = node_id;
    uint8_t TX_buffer_len = 50;
    uint8_t *TX_buffer = (uint8_t *)malloc(TX_buffer_len); /* initiate buffer */
    // TX_buffer[0] = node_id >> 24;
    // TX_buffer[1] = node_id >> 16;
    // TX_buffer[2] = node_id >> 8;
    // TX_buffer[3] = node_id;
    // TX_buffer[4] = TX_buffer_len;
    TX_buffer[0] = node_id;
    sniff.net = STATE_LORA_FORM;
    PRINTF("tx:%x\n", TX_buffer[0]);
    if (sniff.net == STATE_LORAWAN)
    {
        SX1276Write(REG_LR_SYNCWORD, LORA_MAC_PUBLIC_SYNCWORD); /* Value 0x34 is reserved for LoRaWAN networks */
    }
    SX1276SetOpMode(RFLR_OPMODE_TRANSMITTER); /* set mode to tx */
    gpi_led_on(TX_DONE_LED);
    SX1276Write(REG_LR_PAYLOADLENGTH, TX_buffer_len); /* payload length */
    SX1276Write(REG_LR_INVERTIQ, ((SX1276Read(REG_LR_INVERTIQ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_OFF));
    SX1276Write(REG_LR_INVERTIQ2, RFLR_INVERTIQ2_OFF);
    SX1276Write(REG_LR_FIFOTXBASEADDR, 0); /* init FIFO buffer used for Tx */
    SX1276Write(REG_LR_FIFOADDRPTR, 0);
    SX1276WriteBuffer(0, TX_buffer, 5); /* write to the send buffer */
    SX1276Write(REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
                                         RFLR_IRQFLAGS_RXDONE |
                                         RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                         RFLR_IRQFLAGS_VALIDHEADER |
                                         //RFLR_IRQFLAGS_TXDONE |
                                         RFLR_IRQFLAGS_CADDONE |
                                         RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                         RFLR_IRQFLAGS_CADDETECTED);
    /* DIO0=TxDone */
    SX1276Write(REG_DIOMAPPING1, (SX1276Read(REG_DIOMAPPING1) & RFLR_DIOMAPPING1_DIO0_MASK) | RFLR_DIOMAPPING1_DIO0_01);
    sniff.radio = SNIFF_TX;
}
//**************************************************************************************************
void chirp_sniff_main_timer_isr()
{
    gpi_led_on(TIMER_LED);
    __HAL_TIM_CLEAR_IT(&htim2, TIM_IT_CC1);

    if ((sniff.radio == SNIFF_RX) | (sniff.radio == SNIFF_CAD))
    {
        switch (sniff.state)
        {
        case SNIFF_CAD_DETECT: /* Cad detect timeout */
            if (sniff.net == STATE_LORA_FORM)
            {
                /* change SF */
                sniff_SF_set(&sniff.rf.sf);
                sniff_cad();
                MAIN_TIMER_CC_REG = sniff.sf_switch;
                __HAL_TIM_CLEAR_IT(&htim2, TIM_IT_CC1);
                __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_CC1);
            }
            break;
        /* Valid header timeout, 1. at the wrong position (after header), 2. False positive at wrong SF */
        case SNIFF_VALID_HEADER:
            gpi_led_on(TX_DONE_LED);
            SX1276SetOpMode(RFLR_OPMODE_SLEEP);
            sniff_cad();
            gpi_led_off(TX_DONE_LED);
            break;
        default:
            break;
        }
    }
    else
    {
        sniff_tx(TX_Node_Id);
    }

    gpi_led_off(TIMER_LED);
}
//**************************************************************************************************
void chirp_dio0_isr()
{
    gpi_watchdog_periodic();
    if (sniff.radio == SNIFF_RX) /* RxDone */
    {
        gpi_led_on(RX_DONE_LED);
        SX1276Write(REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXDONE); /* Clear Irq */
        volatile uint8_t irqFlags = SX1276Read(REG_LR_IRQFLAGS);
        if (((irqFlags & RFLR_IRQFLAGS_PAYLOADCRCERROR_MASK) == RFLR_IRQFLAGS_PAYLOADCRCERROR))
        {
            SX1276SetOpMode(RFLR_OPMODE_SLEEP); /* if CRC not ok: regard packet as invisible */
            sniff_cad();
        }
        else
        {
            sniff_stat_node();
            sniff_cad();
        }
        gpi_led_off(RX_DONE_LED);
    }
    else if (sniff.radio == SNIFF_TX) /* TxDone */
    {
        SX1276Write(REG_LR_IRQFLAGS, RFLR_IRQFLAGS_TXDONE); /* Clear Irq */
        /* situation at this point: transmission completed, radio entering DISABLED state */
        SX1276SetOpMode(RFLR_OPMODE_SLEEP);
        MAIN_TIMER_CC_REG = MAIN_TIMER_CNT_REG + GPI_FAST_CLOCK_RATE / 4; /* set the timout time for tx */
        __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_CC1);
        gpi_led_off(TX_DONE_LED);
    }
    else if (sniff.radio == SNIFF_CAD) /* CadDone */
    {
        if ((SX1276Read(REG_LR_IRQFLAGS) & RFLR_IRQFLAGS_CADDETECTED) == RFLR_IRQFLAGS_CADDETECTED)
        {
            cad_detected_time = MAIN_TIMER_CNT_REG;
            gpi_led_on(CAD_DETECTED);
            SX1276SetOpMode(RFLR_OPMODE_RECEIVER); /* set in receive mode for the packet */
            SX1276Write(REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_CADDONE | RFLR_IRQFLAGS_CADDETECTED);
            /* CadDetected */
            SX1276Write(REG_LR_IRQFLAGS, RFLR_IRQFLAGS_CADDETECTED | RFLR_IRQFLAGS_CADDONE); /* clear flag */
            /* calculate the time for a valid header */
            uint32_t header_time = SX1276GetPacketTime(sniff.rf.sf, sniff.rf.bw, sniff.rf.cr, 0, sniff.rf.preamble, 2);
            MAIN_TIMER_CC_REG = MAIN_TIMER_CNT_REG + GPI_TICK_US_TO_FAST(header_time);
            __HAL_TIM_CLEAR_IT(&htim2, TIM_IT_CC1);
            __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_CC1);
            sniff.radio = SNIFF_RX;
            sniff.state = SNIFF_VALID_HEADER; /* wait for a valid header */
            gpi_led_off(CAD_DETECTED);
        }
        else
        {
            SX1276Write(REG_LR_IRQFLAGS, RFLR_IRQFLAGS_CADDONE);
            #if GPS_DATA
            node_expired = 1;
            #else
            sniff_cad();
            #endif
        }
    }
}

void chirp_dio3_isr()
{
    gpi_watchdog_periodic();
    if (sniff.radio == SNIFF_RX)
    {
        gpi_led_on(HEADER_LED);
        SX1276Write(REG_LR_IRQFLAGS, RFLR_IRQFLAGS_VALIDHEADER);
        SX1276Write(REG_DIOMAPPING1, (SX1276Read(REG_DIOMAPPING1) & RFLR_DIOMAPPING1_DIO0_MASK) | RFLR_DIOMAPPING1_DIO0_00); /* rx done interrupt */
        __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_CC1);
        gpi_led_off(HEADER_LED);
        DS3231_GetTime(); /* Get the rtc time for statistic */
    }
}
//**************************************************************************************************
