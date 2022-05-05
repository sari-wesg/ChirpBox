//**************************************************************************************************
//**** Includes ************************************************************************************
#include "mixer_internal.h"

#include "loradisc.h"
#include "stm32l4xx_hal.h"
#include "gpi/olf.h"

//**************************************************************************************************
//***** Local Defines and Consts *******************************************************************

//**************************************************************************************************
//***** Local Typedefs and Class Declarations ******************************************************

//**************************************************************************************************
//***** Forward Declarations ***********************************************************************

//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************

//**************************************************************************************************
//***** Global Variables ***************************************************************************

//**************************************************************************************************
//***** Local Functions ****************************************************************************

//**************************************************************************************************
//***** Global Functions ***************************************************************************
void lbt_init()
{
    memset(&loradisc_config.lbt_init_time, 0, sizeof(loradisc_config.lbt_init_time));
    loradisc_config.lbt_channel_total = LBT_CHANNEL_NUM;
    int32_t mask = 1 << (sizeof(uint_fast_t) * 8 - 1);
    uint32_t m;
    for (m = sizeof(uint32_t) * 8; m-- > loradisc_config.lbt_channel_total;)
        mask >>= 1;
    loradisc_config.lbt_channel_mask = ~(mask << 1);
    loradisc_config.lbt_channel_primary = 0;
    LoRaDS_SX1276SetChannel(loradisc_config.lora_freq + loradisc_config.lbt_channel_primary * CHANNEL_STEP);
}

void lbt_update()
{
    lbt_check_time();
    if (loradisc_config.primitive != FLOODING)
        loradisc_config.lbt_channel_primary = (loradisc_config.lbt_channel_primary + 1) % LBT_CHANNEL_NUM;
    LoRaDS_SX1276SetChannel(loradisc_config.lora_freq + loradisc_config.lbt_channel_primary * CHANNEL_STEP);
}


uint8_t lbt_pesudo_channel(uint8_t channel_total, uint8_t last_channel, uint16_t pesudo_value, uint32_t lbt_available)
{
    /* make sure the total number of channel is less than 32 */
    assert_reset((channel_total <= sizeof(uint32_t) * 8));

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
        help_bitmask = lbt & -lbt; // isolate LSB
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
    loradisc_config.lbt_channel_time_us[tx_channel] += tx_us;
    loradisc_config.lbt_channel_time_stats_us[tx_channel] += tx_us;
    if (tx_us)

        /* not enough for the next tx */
        // TODO:
        if ((loradisc_config.lbt_channel_time_us[tx_channel]) > LBT_TX_TIME_S * 1e6 - tx_us)
            loradisc_config.lbt_channel_available &= ~(1 << tx_channel);
    return loradisc_config.lbt_channel_available;
}

void lbt_check_time()
{
    Chirp_Time gps_time = RTC_GetTime();
    time_t diff = GPS_Diff(&gps_time, loradisc_config.lbt_init_time.chirp_year, loradisc_config.lbt_init_time.chirp_month, loradisc_config.lbt_init_time.chirp_date, loradisc_config.lbt_init_time.chirp_hour, loradisc_config.lbt_init_time.chirp_min, loradisc_config.lbt_init_time.chirp_sec);
    if (ABS(diff) >= 3600)
    {
        memcpy(&loradisc_config.lbt_init_time, &gps_time, sizeof(Chirp_Time));
        memset(&loradisc_config.lbt_channel_time_us[0], 0, sizeof(loradisc_config.lbt_channel_time_us));
        int32_t mask = 1 << (sizeof(uint_fast_t) * 8 - 1);
        uint32_t i;
        for (i = sizeof(uint32_t) * 8; i-- > loradisc_config.lbt_channel_total;)
            mask >>= 1;
        loradisc_config.lbt_channel_available = ~(mask << 1);
    }
}

//**************************************************************************************************
