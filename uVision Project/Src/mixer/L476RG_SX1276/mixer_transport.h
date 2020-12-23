
//**************************************************************************************************
//**** Includes ************************************************************************************
// #include "../mixer_internal.h"

#include <math.h>
//**************************************************************************************************
//***** Global Defines and Consts *******************************************************************
//SX1276*******************************************************************************************
// #define SYMBOL_BANDWIDTH 		( LORA_BANDWIDTH > 8) ? 500000 : ( ( LORA_BANDWIDTH - 6) * 125000 )
// #define SYMBOL_RATE				( ( SYMBOL_BANDWIDTH ) / ( 1 << LORA_SPREADING_FACTOR ) )
// #define SYMBOL_TIME				((uint32_t)1e6 / ( SYMBOL_RATE ))					//us
// #define PREAMBLE_TIME			(( LORA_PREAMBLE_LENGTH + 4 ) * ( SYMBOL_TIME ) + ( SYMBOL_TIME ) / 4)

// // payload with explict header
// #define LOWDATA_OPTIMIZE       	((((SYMBOL_BANDWIDTH == 7) && ((LORA_SPREADING_FACTOR == 11) || (LORA_SPREADING_FACTOR == 12))) || \
// 								((SYMBOL_BANDWIDTH == 8 ) && ( LORA_SPREADING_FACTOR == 12)))? 1 : 0)
// #define PAYLOAD_TMP			    (uint32_t)(ceil( (int32_t)( 8 * PHY_PAYLOAD_SIZE - 4 * LORA_SPREADING_FACTOR + 28 + 16 ) / \
//                                 (double)( 4 * (LORA_SPREADING_FACTOR - 2 * LOWDATA_OPTIMIZE)) ) * ( LORA_CODINGRATE + 4 ))
// #define PAYLOAD_NUM         	( 8 + ( ( (PAYLOAD_TMP) > 0 ) ? (PAYLOAD_TMP) : 0 ) )
// #define PAYLOAD_TIME        	(PAYLOAD_NUM) * (SYMBOL_TIME)
// #define PAYLOAD_AIR_TIME 		( ( PREAMBLE_TIME ) + ( PAYLOAD_TIME ))

// // explict header mode
// #define HEADER_LEN          	2												//explict header length
// #define HEADER_TMP			    (uint32_t)(ceil( (int32_t)( 8 * HEADER_LEN - 4 * LORA_SPREADING_FACTOR + 28 - 20) / \
//                                 (double)( 4 * LORA_SPREADING_FACTOR ) ) * ( 4 + 4 ))
// #define HEADER_NUM          	8 + ( ( (HEADER_TMP) > 0 ) ? (HEADER_TMP) : 0 )
// #define HEADER_TIME         	( ( HEADER_NUM ) * ( SYMBOL_TIME ) + PREAMBLE_TIME )
// #define HEADER_TIME_SHORT       ( ( HEADER_NUM ) * ( SYMBOL_TIME ) )

// #define AFTER_HEADER_TIME   	( ( PAYLOAD_AIR_TIME ) - ( HEADER_TIME ) )			//expected rxdone time after a valid header detection

#if MX_DOUBLE_BITMAP
	// #define BITMAP_BYTE   			( (MX_GENERATION_SIZE + 7) / 8 ) * 2
	#define BITMAP_BYTE   			(offsetof(Packet, payload) - offsetof(Packet, phy_payload_begin))
	#define BITMAP_TMP			    (uint32_t)(ceil( (int32_t)( 8 * BITMAP_BYTE - 4 * LORA_SPREADING_FACTOR + 28) / \
									(double)( 4 * LORA_SPREADING_FACTOR ) ) * ( 1 + 4 ))
	#define BITMAP_NUM          	8 + ( ( (BITMAP_TMP) > 0 ) ? (BITMAP_TMP) : 0 )
	#define BITMAP_TIME         	( ( BITMAP_NUM ) * ( SYMBOL_TIME ) + PREAMBLE_TIME )
	#define AFTER_HEADER_BITMAP   	( ( BITMAP_TIME ) - ( HEADER_TIME ) )			//expected rxdone time after a valid header detection
#endif

//**************************************************************************************************
ASSERT_CT_STATIC(IS_POWER_OF_2(FAST_HYBRID_RATIO), unefficient_FAST_HYBRID_RATIO);

// timing parameters

// NOTE: a drift tolerance of 300 ppm (150 ppm on each side) should be a comfortable choice
// (typical clock crystals have < 20...50 ppm at 25�C and temperature coefficient < 0.04 ppm/K)
// note: MIN(2500, ...) is effective in case of very long slots (up to seconds). It reduces the
// tolerance to avoid that RX_WINDOW overflows too fast (or even immediately).
// #define DRIFT_TOLERANCE			MIN(2500, MAX((MX_SLOT_LENGTH + 999) / 1000, 1))	// +/- 1000 ppm

// #define MAX_PROPAGATION_DELAY	GPI_TICK_US_TO_HYBRID(2)

// #define PACKET_AIR_TIME			GPI_TICK_US_TO_HYBRID2(PAYLOAD_AIR_TIME + 712)
// // #define GRID_TO_EVENT_OFFSET	GPI_TICK_US_TO_HYBRID2(HEADER_TIME)	// preamble + explict header + event signaling latency
// #define RX_TO_GRID_OFFSET		(0 + GPI_TICK_US_TO_HYBRID(37))		// software latency + RX ramp up time
// #define TX_TO_GRID_OFFSET		(0 + GPI_TICK_US_TO_HYBRID(130))		// software latency + TX ramp up time

// #define RX_WINDOW_INCREMENT		(2 * DRIFT_TOLERANCE)			// times 2 is important to widen the window in next slot (times 1 would follow only)
// #define RX_WINDOW_MAX			MIN(0x7FFFFFFF, MIN(15 * RX_WINDOW_INCREMENT, (MX_SLOT_LENGTH - PACKET_AIR_TIME - RX_TO_GRID_OFFSET) / 2))
// #define RX_WINDOW_MIN			MIN(RX_WINDOW_MAX / 2, MAX(2 * RX_WINDOW_INCREMENT, GPI_TICK_US_TO_HYBRID(1)))		// minimum must cover variations in execution time from timer polling to RX on

// #define GRID_DRIFT_FILTER_DIV	4
// #define GRID_TICK_UPDATE_DIV	2
// #define GRID_DRIFT_MAX			MIN(3 * DRIFT_TOLERANCE * GRID_TICK_UPDATE_DIV * GRID_DRIFT_FILTER_DIV, 0x7FFFFF)

// #define TX_OFFSET_FILTER_DIV	2
// #define TX_OFFSET_MAX			(2 * MAX_PROPAGATION_DELAY + GPI_TICK_US_TO_HYBRID(2))

// #define ISR_LATENCY_BUFFER		122		// in microseconds
// #define ISR_LATENCY_SLOW		4 * HYBRID_SLOW_RATIO		// in fast tick

// #define GRID_DRIFT_OFFSET		GPI_TICK_US_TO_HYBRID2(5 * SYMBOL_TIME)

// // #define SLOT_INTERVAL			((((PACKET_AIR_TIME) * MX_DUTY_CYCLE_PERCENT) / MX_SLOT_LENGTH) + 1)

// #define MAX_TB_INTERVAL			GPI_TICK_US_TO_HYBRID(2000)
