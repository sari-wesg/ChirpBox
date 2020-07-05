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

//**************************************************************************************************
//***** Local Defines and Consts *******************************************************************
#define DEBUG 1
#if DEBUG
#include <stdio.h>
#include <stdlib.h>

#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define BUFFER_SIZE                 1

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

Topology_State topology_state;

uint32_t packet_time_us;
static uint32_t round_length_us;
Topology_result *node_topology;

//**************************************************************************************************
//***** Global Variables ***************************************************************************

//**************************************************************************************************
//***** Local Functions ****************************************************************************

void packet_prepare(uint8_t node_id)
{
    Tx_Buffer[0] = node_id;
}

//**************************************************************************************************
//***** Global Functions ***************************************************************************
uint32_t topo_init(uint8_t nodes_num, uint8_t node_id, uint8_t sf)
{
    tx_num_max = 20;
    packet_time_us = SX1276GetPacketTime(sf, 7, 1, 0, chirp_config.lora_plen, BUFFER_SIZE) + 50000;
    node_topology = (Topology_result *)malloc(nodes_num * sizeof(Topology_result));
    memset(node_topology, 0, nodes_num * sizeof(Topology_result));
    round_length_us = packet_time_us * (tx_num_max + 3) + 2000000;

    packet_prepare(node_id);

    return packet_time_us;
}

Gpi_Fast_Tick_Extended topo_round_robin(uint8_t node_id, uint8_t nodes_num, uint8_t i, Gpi_Fast_Tick_Extended deadline)
{
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

        topology_state = RX_RUNNING;

        deadline = gpi_tick_fast_extended() + GPI_TICK_US_TO_FAST2(round_length_us);

        while(1)
        {
            if (gpi_tick_compare_fast_extended(gpi_tick_fast_extended(), deadline) >= 0)
                break;
        }

        node_topology[i].rx_num = rx_receive_num;
    }
    else
    {
        /* delay more than receivers */
        deadline = gpi_tick_fast_extended() + GPI_TICK_US_TO_FAST2(packet_time_us * 3) + GPI_TICK_US_TO_FAST2(1000000);

        SX1276WriteFIFO(Tx_Buffer, BUFFER_SIZE);
		SX1276Write( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
											RFLR_IRQFLAGS_RXDONE |
											RFLR_IRQFLAGS_PAYLOADCRCERROR |
											RFLR_IRQFLAGS_VALIDHEADER |
											//RFLR_IRQFLAGS_TXDONE |
											RFLR_IRQFLAGS_CADDONE |
											RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
											RFLR_IRQFLAGS_CADDETECTED );
        SX1276Write( REG_DIOMAPPING1, ( SX1276Read( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK ) | RFLR_DIOMAPPING1_DIO0_01 );

		SX1276Write( REG_LR_PAYLOADLENGTH, BUFFER_SIZE );
		SX1276Write( REG_LR_INVERTIQ, ( ( SX1276Read( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_OFF ) );
		SX1276Write( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_OFF );
		// Full buffer used for Tx
		SX1276Write( REG_LR_FIFOTXBASEADDR, 0 );
		SX1276Write( REG_LR_FIFOADDRPTR, 0 );

        while (gpi_tick_compare_fast_extended(gpi_tick_fast_extended(), deadline) < 0);
        PRINTF("Topology---Tx\n");
        SX1276SetOpMode( RFLR_OPMODE_TRANSMITTER );

        tx_send_num++;
        topology_state = TX_RUNNING;
        deadline += GPI_TICK_US_TO_FAST2(round_length_us - packet_time_us * 3 - 1000000);

        while(1)
        {
            if (gpi_tick_compare_fast_extended(gpi_tick_fast_extended(), deadline) >= 0)
                break;
        }
    }
    SX1276SetOpMode( RFLR_OPMODE_SLEEP );
    return deadline;
}

void topo_result(uint8_t nodes_num)
{
    uint8_t i;
    /* 64 bit length */
    uint32_t topo_result[((nodes_num + 1) / 2) * 2];
    memset(topo_result, 0, sizeof(topo_result));

    for ( i = 0; i < nodes_num; i++)
    {
        uint32_t reliability = (uint32_t)((node_topology[i].rx_num * 1e4) / (uint32_t)(tx_num_max));
        node_topology[i].reliability = reliability;
        topo_result[i] = (uint8_t)i | (uint32_t)node_topology[i].reliability << 16;
        PRINTF("r:%d, %lu, %x\n", i, node_topology[i].reliability, topo_result[i]);
    }

    #if MX_FLASH_FILE
        // menu_preSend(0);
        FLASH_If_Erase_Pages(1, 255);
        FLASH_If_Write(TOPO_FLASH_ADDRESS, (uint32_t *)(topo_result), sizeof(topo_result) / sizeof(uint32_t));
    #endif

    free(node_topology);
}

void topo_dio0_isr()
{
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
            // memset(Rx_Buffer, 0, BUFFER_SIZE);
            // read rx packet from start address (in data buffer) of last packet received
            // SX1276Write(REG_LR_FIFOADDRPTR, SX1276Read( REG_LR_FIFORXCURRENTADDR ) );
            // SX1276ReadFifo(Rx_Buffer, packet_len );
            // count++;
            rx_receive_num++;
            // add_rx_topology_count();

            #if DEBUG
                PRINTF("RX: %d\n", rx_receive_num);
            #endif
            SX1276SetOpMode( RFLR_OPMODE_RECEIVER );
        }
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
