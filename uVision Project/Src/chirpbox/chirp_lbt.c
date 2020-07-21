//**************************************************************************************************
//**** Includes ************************************************************************************
#include "mixer_internal.h"
#include "chirp_internal.h"
#include "stm32l4xx_hal.h"
#include <stdlib.h>
#include "gpi/olf.h"

#ifdef MX_CONFIG_FILE
#include STRINGIFY(MX_CONFIG_FILE)
#endif

#if MX_LBT_ACCESS

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

//**************************************************************************************************
//***** Local Typedefs and Class Declarations ******************************************************


//**************************************************************************************************
//***** Forward Declarations ***********************************************************************



//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************
static uint16_t random_number = 0U;


//**************************************************************************************************
//***** Local Functions ***************************************************************************


//**************************************************************************************************
//***** Global Variables ***************************************************************************


//**************************************************************************************************
//***** Global Functions ***************************************************************************
uint8_t lbt_pesudo_channel(uint8_t channel_total, uint8_t last_channel, uint16_t pesudo_value, uint32_t lbt_available)
{
    /* make sure the total number of channel is less than 32 */
    assert_reset(channel_total <= sizeof(uint32_t) * 8);

    /* init seed */
    srand(pesudo_value);
    rand();

    uint32_t help_bitmask = 0;

    uint32_t lbt = lbt_available & (~(1 << last_channel));

    uint8_t lbt_len = gpi_popcnt_32(lbt);
    assert_reset((lbt_len <= channel_total) && (lbt_len > 0));

    uint8_t lookupTable[lbt_len];
    uint8_t i = 0;
    while (lbt)
    {
        // isolate first set bit
        #if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
            help_bitmask = lbt & -lbt;			// isolate LSB
        #else
            #error TODO						// isolate MSB
        #endif

        lookupTable[i++] = gpi_get_lsb_32(lbt);
        lbt &= ~help_bitmask;
    }
    uint8_t value = lookupTable[rand() % lbt_len];

    return value;
}

uint32_t lbt_update_channel(uint32_t tx_us, uint8_t tx_channel)
{
    chirp_config.lbt_channel_time_us[tx_channel] += tx_us;
    chirp_config.lbt_channel_time_stats_us[tx_channel] += tx_us;
    if(tx_us)

    /* not enough for the next tx */
    // TODO:
    if ((chirp_config.lbt_channel_time_us[tx_channel]) > LBT_TX_TIME_S * 1e6 - tx_us)
        chirp_config.lbt_channel_available &= ~(1 << tx_channel);
    return chirp_config.lbt_channel_available;
}

void lbt_check_time()
{
	Chirp_Time gps_time = GPS_Get_Time();
    time_t diff = GPS_Diff(&gps_time, chirp_config.lbt_init_time.chirp_year, chirp_config.lbt_init_time.chirp_month, chirp_config.lbt_init_time.chirp_date, chirp_config.lbt_init_time.chirp_hour, chirp_config.lbt_init_time.chirp_min, chirp_config.lbt_init_time.chirp_sec);
    if (ABS(diff) >= 3600)
    {
        memcpy(&chirp_config.lbt_init_time, &gps_time, sizeof(Chirp_Time));
        memset((uint32_t)&chirp_config.lbt_channel_time_us[0], 0, sizeof(chirp_config.lbt_channel_time_us));
        int32_t mask = 1 << (sizeof(uint_fast_t) * 8 - 1);
        uint32_t i;
        for (i = sizeof(uint32_t) * 8; i-- > chirp_config.lbt_channel_total;)
            mask >>= 1;
        chirp_config.lbt_channel_available = ~(mask << 1);
    }
}

#endif
