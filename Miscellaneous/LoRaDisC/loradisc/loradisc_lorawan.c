//**************************************************************************************************
//**** Includes ************************************************************************************

//**************************************************************************************************
#include "chirp_internal.h"
#include "mixer_config.h"

#include GPI_PLATFORM_PATH(sx1276Regs_Fsk.h)
#include GPI_PLATFORM_PATH(sx1276Regs_LoRa.h)
uint8_t send_count = 0;

//**************************************************************************************************
//***** Local Defines and Consts *******************************************************************
#if DEBUG_DISC
#include <stdio.h>
#include <stdlib.h>

#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define BUFFER_SIZE                 255
#define MAX_SEND                 	100

//**************************************************************************************************
//***** Local Typedefs and Class Declarations ******************************************************

typedef enum LoRaWAN_State_tag
{
	IDLE		= 0,
	RX_RUNNING	= 16,
	TX_RUNNING	= 12,
} LoRaWAN_State;

//**************************************************************************************************
//***** Forward Declarations ***********************************************************************

//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************
uint8_t Tx_Buffer[BUFFER_SIZE];

uint8_t Rx_Buffer[BUFFER_SIZE];
LoRaWAN_State lorawan_state;

//**************************************************************************************************
//***** Global Variables ***************************************************************************

//**************************************************************************************************
//***** Local Functions ****************************************************************************

//**************************************************************************************************
//***** Global Functions ***************************************************************************
void lorawan_listen_init(uint8_t node_id)
{
    SX1276SetOpMode( RFLR_OPMODE_SLEEP );
	chirp_radio_config(7, 1, 14, 486300);
	chirp_isr.state = ISR_LORAWAN;
	lorawan_state = IDLE;
	SX1276SetPublicNetwork(true);
	if(node_id == 0)
		lorawan_listen();
	else
		lorawan_transmission();
}

void lorawan_listen()
{
	printf("lorawan_listen\n");
	// rx config
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
	lorawan_state = RX_RUNNING;

	// int i;
	// for (i = 0; i <= 0x39; i++)
	// {
	// 	PRINTF("%02x, %02x\n", i, SX1276Read(i));
	// }

	__HAL_TIM_CLEAR_IT(&htim2, TIM_IT_CC1);
	__HAL_TIM_DISABLE_IT(&htim2, TIM_IT_CC1);
	MAIN_TIMER_CC_REG = MAIN_TIMER_CNT_REG + GPI_TICK_US_TO_FAST(1000000);
	__HAL_TIM_ENABLE_IT(&htim2, TIM_IT_CC1);
	PRINTF("lorawan_listen\n");
}

void lorawan_transmission()
{
	uint8_t tx_payload_len = 10;
	uint8_t data[21] = {0x80, 0x37, 0x00, 0x1e, 0x00, 0x00, 0x01, 0x00, 0x02, 0x79, 0x91, 0x05, 0x55, 0x29, 0x3b, 0x6b, 0xab, 0xd7, 0x47, 0xc0, 0x49};

	tx_payload_len = sizeof(data);
    memset(Tx_Buffer, 0xff, tx_payload_len);
	memcpy(Tx_Buffer, data, tx_payload_len);
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

	SX1276SetOpMode( RFLR_OPMODE_TRANSMITTER );
	lorawan_state = TX_RUNNING;
}

void lorawan_main_timer_isr()
{
    gpi_watchdog_periodic();
    __HAL_TIM_CLEAR_IT(&htim2, TIM_IT_CC1);
    __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_CC1);
    MAIN_TIMER_CC_REG = MAIN_TIMER_CNT_REG + GPI_TICK_US_TO_FAST(1000000);
    __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_CC1);
}


void lorawan_dio0_isr()
{
    gpi_watchdog_periodic();
    if (lorawan_state == RX_RUNNING)
	{
		SX1276Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXDONE );
		volatile uint8_t packet_len = (uint8_t)SX1276Read( REG_LR_RXNBBYTES );
		volatile uint8_t irqFlags = SX1276Read( REG_LR_IRQFLAGS );
		if(( irqFlags & RFLR_IRQFLAGS_PAYLOADCRCERROR_MASK ) != RFLR_IRQFLAGS_PAYLOADCRCERROR )
		{
			memset(Rx_Buffer, 0, BUFFER_SIZE);
			// read rx packet from start address (in data buffer) of last packet received
			SX1276Write(REG_LR_FIFOADDRPTR, SX1276Read( REG_LR_FIFORXCURRENTADDR ) );
			SX1276ReadFifo(Rx_Buffer, packet_len );
			int i;
			PRINTF("RX:%d\n", packet_len);
			for (i = 0; i < packet_len; i++)
			{
				PRINTF("%02x ", Rx_Buffer[i]);
			}
			PRINTF("\n");
		}
		else
		{
			SX1276Write(REG_LR_IRQFLAGS, RFLR_IRQFLAGS_PAYLOADCRCERROR);
			PRINTF("RX wrong\n");
		}
		SX1276SetOpMode( RFLR_OPMODE_SLEEP );
		SX1276SetOpMode( RFLR_OPMODE_RECEIVER );
	}
	else if (lorawan_state == TX_RUNNING)
	{
		if (send_count < MAX_SEND)
		{
			send_count++;
			PRINTF("TXDONE:%d\n", send_count);
			SX1276Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_TXDONE );

			Gpi_Fast_Tick_Native tx_interval = gpi_tick_fast_native() + GPI_TICK_US_TO_FAST2(1000000);
			while (gpi_tick_compare_fast_native(gpi_tick_fast_native(), tx_interval) < 0);
			SX1276SetOpMode( RFLR_OPMODE_TRANSMITTER );
			lorawan_state = TX_RUNNING;
		}
	}
}


void lorawan_dio3_isr()
{

}

