//**************************************************************************************************
//**** Includes ************************************************************************************

//**************************************************************************************************
#include "chirp_internal.h"
#include "mixer_internal.h"

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

#if MX_FLASH_FILE
	#include "flash_if.h"
#endif

#include "menu.h"
#if ENERGEST_CONF_ON
#include GPI_PLATFORM_PATH(energest.h)
#endif
//**************************************************************************************************
//***** Local Defines and Consts *******************************************************************
#if DEBUG_CHIRPBOX
#include <stdio.h>
#include <stdlib.h>

#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define BUFFER_SIZE                 200

//**************************************************************************************************
//***** Local Typedefs and Class Declarations ******************************************************

typedef enum Topology_State_tag
{
	IDLE		= 0,
	RX_RUNNING	= 16,
	TX_RUNNING	= 12,
} Topology_State;

//**************************************************************************************************
//***** Forward Declarations ***********************************************************************
void packet_prepare(uint8_t node_id);

//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************

uint8_t Tx_Buffer[BUFFER_SIZE];
uint8_t Rx_Buffer[BUFFER_SIZE];

uint16_t tx_send_num;
uint16_t rx_receive_num;
static uint8_t tx_num_max;
static uint8_t tx_payload_len;
Topology_State topology_state;

uint32_t packet_time_us;
static uint32_t round_length_us;
Topology_result *node_topology;
Topology_result_link *node_topology_link;

int8_t SnrValue;
int16_t rssi_link, RssiValue_link;
//**************************************************************************************************
//***** Global Variables ***************************************************************************

//**************************************************************************************************
//***** Local Functions ****************************************************************************

void packet_prepare(uint8_t node_id)
{
    Tx_Buffer[0] = node_id + 1;
    Tx_Buffer[1] = node_id + 2;
    uint16_t topo_hash_tx = Chirp_RSHash((uint8_t *)&(Tx_Buffer[0]), 2);
    Tx_Buffer[2] = topo_hash_tx >> 8;
    Tx_Buffer[3] = topo_hash_tx;
}

//**************************************************************************************************
//***** Global Functions ***************************************************************************
uint32_t topo_init(uint8_t nodes_num, uint8_t node_id, uint8_t sf, uint8_t payload_len)
{
    tx_num_max = 20;
    assert_reset((payload_len >= 2));
    tx_payload_len = payload_len + 2;
    assert_reset((tx_payload_len <= BUFFER_SIZE));
    packet_time_us = SX1276GetPacketTime(sf, 7, 1, 0, chirp_config.lora_plen, tx_payload_len) + 50000;
    node_topology = (Topology_result *)malloc(nodes_num * sizeof(Topology_result));
    memset(node_topology, 0, nodes_num * sizeof(Topology_result));

    node_topology_link = (Topology_result_link *)malloc(nodes_num * sizeof(Topology_result_link));
    memset(node_topology_link, 0, nodes_num * sizeof(Topology_result_link));

    round_length_us = packet_time_us * (tx_num_max + 3) + 2000000;

    packet_prepare(node_id);

    return packet_time_us;
}

Gpi_Fast_Tick_Extended topo_round_robin(uint8_t node_id, uint8_t nodes_num, uint8_t i, Gpi_Fast_Tick_Extended deadline)
{
    #if ENERGEST_CONF_ON
        ENERGEST_ON(ENERGEST_TYPE_CPU);
    #endif

    SX1276SetOpMode( RFLR_OPMODE_SLEEP );
	chirp_isr.state = ISR_TOPO;

    topology_state = IDLE;
    tx_send_num = 0;
    rx_receive_num = 0;
    if (i != node_id)
    {
        PRINTF("Topology---Rx:%d\n", i);
		SX1276Write( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
											//RFLR_IRQFLAGS_RXDONE |
											//RFLR_IRQFLAGS_PAYLOADCRCERROR |
											RFLR_IRQFLAGS_VALIDHEADER |
											RFLR_IRQFLAGS_TXDONE |
											RFLR_IRQFLAGS_CADDONE |
											RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
											RFLR_IRQFLAGS_CADDETECTED );
		SX1276Write( REG_DIOMAPPING1, ( SX1276Read( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK) | RFLR_DIOMAPPING1_DIO0_00);

		SX1276Write( REG_LR_INVERTIQ, ( ( SX1276Read( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_OFF ) );
		SX1276Write( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_OFF );

		SX1276Write( REG_LR_DETECTOPTIMIZE, SX1276Read( REG_LR_DETECTOPTIMIZE ) & 0x7F );
		SX1276Write( REG_LR_IFFREQ2, 0x00 );
        SX1276Write( REG_LR_IFFREQ1, 0x40 );

        SX1276SetOpMode( RFLR_OPMODE_RECEIVER );
        #if ENERGEST_CONF_ON
            ENERGEST_OFF(ENERGEST_TYPE_CPU);
            ENERGEST_ON(ENERGEST_TYPE_LISTEN);
        #endif
        topology_state = RX_RUNNING;

        deadline = gpi_tick_fast_extended() + GPI_TICK_US_TO_FAST2(round_length_us);

        __HAL_TIM_CLEAR_IT(&htim2, TIM_IT_CC1);
        __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_CC1);
        MAIN_TIMER_CC_REG = MAIN_TIMER_CNT_REG + GPI_TICK_US_TO_FAST(16000000);
        __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_CC1);

        while(1)
        {
            if (gpi_tick_compare_fast_extended(gpi_tick_fast_extended(), deadline) >= 0)
                break;
        }

        node_topology[i].rx_num = rx_receive_num;
        #if ENERGEST_CONF_ON
            ENERGEST_ON(ENERGEST_TYPE_CPU);
            ENERGEST_OFF(ENERGEST_TYPE_LISTEN);
        #endif
    }
    else
    {
        /* delay more than receivers */
        deadline = gpi_tick_fast_extended() + GPI_TICK_US_TO_FAST2(packet_time_us * 3) + GPI_TICK_US_TO_FAST2(1000000);

        SX1276WriteFIFO(Tx_Buffer, tx_payload_len);
		SX1276Write( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
											RFLR_IRQFLAGS_RXDONE |
											RFLR_IRQFLAGS_PAYLOADCRCERROR |
											RFLR_IRQFLAGS_VALIDHEADER |
											//RFLR_IRQFLAGS_TXDONE |
											RFLR_IRQFLAGS_CADDONE |
											RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
											RFLR_IRQFLAGS_CADDETECTED );
        SX1276Write( REG_DIOMAPPING1, ( SX1276Read( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK ) | RFLR_DIOMAPPING1_DIO0_01 );

		SX1276Write( REG_LR_PAYLOADLENGTH, tx_payload_len );
		SX1276Write( REG_LR_INVERTIQ, ( ( SX1276Read( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_OFF ) );
		SX1276Write( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_OFF );
		// Full buffer used for Tx
		SX1276Write( REG_LR_FIFOTXBASEADDR, 0 );
		SX1276Write( REG_LR_FIFOADDRPTR, 0 );

        while (gpi_tick_compare_fast_extended(gpi_tick_fast_extended(), deadline) < 0);
        PRINTF("Topology---Tx\n");
        SX1276SetOpMode( RFLR_OPMODE_TRANSMITTER );
        #if ENERGEST_CONF_ON
            ENERGEST_OFF(ENERGEST_TYPE_CPU);
            ENERGEST_ON(ENERGEST_TYPE_TRANSMIT);
        #endif
        tx_send_num++;
        topology_state = TX_RUNNING;
        deadline += GPI_TICK_US_TO_FAST2(round_length_us - packet_time_us * 3 - 1000000);

        while(1)
        {
            if (gpi_tick_compare_fast_extended(gpi_tick_fast_extended(), deadline) >= 0)
                break;
        }
        #if ENERGEST_CONF_ON
            ENERGEST_ON(ENERGEST_TYPE_CPU);
            ENERGEST_OFF(ENERGEST_TYPE_TRANSMIT);
        #endif
    }
    __HAL_TIM_CLEAR_IT(&htim2, TIM_IT_CC1);
    __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_CC1);
    SX1276SetOpMode( RFLR_OPMODE_SLEEP );
    return deadline;
}

void topo_result(uint8_t nodes_num)
{
    gpi_watchdog_periodic();
    uint8_t i;
    /* 64 bit length */
    uint32_t topo_result[((nodes_num + 1) / 2) * 2];
    memset(topo_result, 0, sizeof(topo_result));

    for ( i = 0; i < nodes_num; i++)
    {
        uint32_t reliability = (uint32_t)((node_topology[i].rx_num * 1e4) / (uint32_t)(tx_num_max));
        node_topology[i].reliability = reliability;
        topo_result[i] = (uint8_t)i | (uint32_t)node_topology[i].reliability << 16;
        PRINTF("r:%d, %d, %x\n", i, node_topology[i].reliability, topo_result[i]);
    }

    uint8_t temp_raw = SX1276GetRawTemp();
    uint32_t temp_flash[2];
    temp_flash[0] = (uint32_t)(temp_raw);

    #if MX_FLASH_FILE
        // menu_preSend(0);
        FLASH_If_Erase_Pages(1, TOPO_PAGE);
        FLASH_If_Write(TOPO_FLASH_ADDRESS, (uint32_t *)(topo_result), sizeof(topo_result) / sizeof(uint32_t));
        FLASH_If_Write(TOPO_FLASH_ADDRESS + sizeof(topo_result) + 8, (uint32_t *)(temp_flash), sizeof(temp_flash) / sizeof(uint32_t));
        memcpy(topo_result, (uint32_t *)(node_topology_link), nodes_num * sizeof(Topology_result_link));
        FLASH_If_Write(TOPO_FLASH_ADDRESS + sizeof(topo_result) + 8 + sizeof(temp_flash), (uint32_t *)(topo_result), sizeof(topo_result) / sizeof(uint32_t));
    #endif

    free(node_topology);
    free(node_topology_link);
}

void topo_dio0_isr()
{
    gpi_watchdog_periodic();
    /* must be periodically called */
    gpi_tick_hybrid_reference();

    if (topology_state == RX_RUNNING)
    {
        gpi_led_on(GPI_LED_1);
        SX1276Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXDONE );
        volatile uint8_t packet_len = (uint8_t)SX1276Read( REG_LR_RXNBBYTES );
        volatile uint8_t irqFlags = SX1276Read( REG_LR_IRQFLAGS );
        if(( irqFlags & RFLR_IRQFLAGS_PAYLOADCRCERROR_MASK ) != RFLR_IRQFLAGS_PAYLOADCRCERROR )
        {
            memset(Rx_Buffer, 0, BUFFER_SIZE);
            // read rx packet from start address (in data buffer) of last packet received
            SX1276Write(REG_LR_FIFOADDRPTR, SX1276Read( REG_LR_FIFORXCURRENTADDR ) );
            SX1276ReadFifo(Rx_Buffer, packet_len );
            uint16_t topo_hash_rx = Chirp_RSHash((uint8_t *)&(Rx_Buffer[0]), 2);
            if ((topo_hash_rx == Rx_Buffer[2] >> 8) | (Rx_Buffer[3]))
            {
                // count++;
                rx_receive_num++;

                // Returns SNR value [dB] rounded to the nearest integer value
                SnrValue = (((int8_t)SX1276Read(REG_LR_PKTSNRVALUE)) + 2) >> 2;
                rssi_link = SX1276Read(REG_LR_PKTRSSIVALUE);

                if (SnrValue < 0)
                {
                    if (chirp_config.lora_freq > RF_MID_BAND_THRESH)
                        RssiValue_link = RSSI_OFFSET_HF + rssi_link + (rssi_link >> 4) + SnrValue;
                    else
                        RssiValue_link = RSSI_OFFSET_LF + rssi_link + (rssi_link >> 4) + SnrValue;
                }
                else
                {
                    if (chirp_config.lora_freq > RF_MID_BAND_THRESH)
                        RssiValue_link = RSSI_OFFSET_HF + rssi_link + (rssi_link >> 4);
                    else
                        RssiValue_link = RSSI_OFFSET_LF + rssi_link + (rssi_link >> 4);
                }
                node_topology_link[Rx_Buffer[0]-1].snr_link = SnrValue;
                node_topology_link[Rx_Buffer[0]-1].rssi_link = RssiValue_link;
                // printf("rx-----------:%d, %d, %lu\n", SnrValue, RssiValue_link, Rx_Buffer[0]-1);

                PRINTF("RX: %d\n", rx_receive_num);
            }
            else
            {
                PRINTF("crc: %d\n", rx_receive_num);
            }
        }
        else
        {
			SX1276Write(REG_LR_IRQFLAGS, RFLR_IRQFLAGS_PAYLOADCRCERROR);
            PRINTF("RX wrong: %d\n", rx_receive_num);
        }
        SX1276SetOpMode( RFLR_OPMODE_SLEEP );
        SX1276SetOpMode( RFLR_OPMODE_RECEIVER );

        gpi_led_off(GPI_LED_1);
    }
    else if (topology_state == TX_RUNNING)
    {
        PRINTF("TXDONE\n");
        gpi_led_on(GPI_LED_2);
        SX1276Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_TXDONE );
        topology_state = 0;

        Gpi_Fast_Tick_Native tx_interval = gpi_tick_fast_native() + GPI_TICK_US_TO_FAST2(10000);

        while (gpi_tick_compare_fast_native(gpi_tick_fast_native(), tx_interval) < 0);

        if (tx_send_num < tx_num_max)
        {
            SX1276SetOpMode( RFLR_OPMODE_TRANSMITTER );
            tx_send_num++;
            topology_state = TX_RUNNING;
        }
        gpi_led_off(GPI_LED_2);
    }
}

void topo_main_timer_isr()
{
    gpi_watchdog_periodic();
    __HAL_TIM_CLEAR_IT(&htim2, TIM_IT_CC1);
    __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_CC1);
    MAIN_TIMER_CC_REG = MAIN_TIMER_CNT_REG + GPI_TICK_US_TO_FAST(16000000);
    __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_CC1);
}
