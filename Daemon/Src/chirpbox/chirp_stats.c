//**************************************************************************************************
//**** Includes ************************************************************************************
#include "mixer_internal.h"
#include "chirp_internal.h"
#include "stm32l4xx_hal.h"

#ifdef MX_CONFIG_FILE
#include STRINGIFY(MX_CONFIG_FILE)
#endif

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

#if MX_FLASH_FILE
	#include "flash_if.h"
#endif
#include "API_ChirpBox.h"
//**************************************************************************************************
//***** Local Typedefs and Class Declarations ******************************************************



//**************************************************************************************************
//***** Forward Declarations ***********************************************************************



//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************


//**************************************************************************************************
//***** Global Variables ***************************************************************************
/* As events come in, buffer them completely, and calculate a running sum, count, min, max. */
/* statistics */
Chirp_Stats_All chirp_stats_all;
Chirp_Energy chirp_stats_all_debug;
//**************************************************************************************************
//***** Global Functions ***************************************************************************

void Stats_value(uint8_t stats_type, uint32_t value)
{
    Chirp_Stats *chirp_stats_temp;
    switch (stats_type)
    {
    case SLOT_STATS:
        chirp_stats_temp = &(chirp_stats_all.slot);
        break;
    case RX_STATS:
        chirp_stats_temp = &(chirp_stats_all.rx_on);
        break;
    case TX_STATS:
        chirp_stats_temp = &(chirp_stats_all.tx_on);
        break;
    default:
        break;
    }

    if (value)
    {
        if (!chirp_stats_temp->stats_count)
        {
            chirp_stats_temp->stats_min = value;
            chirp_stats_temp->stats_max = value;
        }
        chirp_stats_temp->stats_sum += value;
        chirp_stats_temp->stats_count ++;
        chirp_stats_temp->stats_min = (chirp_stats_temp->stats_min <= value)? chirp_stats_temp->stats_min : value;
        chirp_stats_temp->stats_max = (chirp_stats_temp->stats_max >= value)? chirp_stats_temp->stats_max : value;
    }
    else
    {
        chirp_stats_temp->stats_none ++;
    }
}

void Stats_value_debug(uint8_t energy_type, uint32_t value)
{
    // printf("value:%lu, %lu\n", energy_type, (uint32_t)gpi_tick_fast_to_us(value));
    uint32_t value_s = (uint32_t)gpi_tick_fast_to_us(value);
    switch (energy_type)
    {
        case ENERGEST_TYPE_CPU:
            chirp_stats_all_debug.CPU += value_s;
            break;
        case ENERGEST_TYPE_LPM:
            chirp_stats_all_debug.LPM += value_s;
            break;
        case ENERGEST_TYPE_STOP:
            chirp_stats_all_debug.STOP += value_s;
            break;
        case ENERGEST_TYPE_FLASH_WRITE_BANK1:
            chirp_stats_all_debug.FLASH_WRITE_BANK1 += value_s;
            break;
        case ENERGEST_TYPE_FLASH_WRITE_BANK2:
            chirp_stats_all_debug.FLASH_WRITE_BANK2 += value_s;
            break;
        case ENERGEST_TYPE_FLASH_ERASE:
            chirp_stats_all_debug.FLASH_ERASE += value_s;
            break;
        case ENERGEST_TYPE_FLASH_VERIFY:
            chirp_stats_all_debug.FLASH_VERIFY += value_s;
            break;
        case ENERGEST_TYPE_TRANSMIT:
            chirp_stats_all_debug.TRANSMIT += value_s;
            break;
        case ENERGEST_TYPE_LISTEN:
            chirp_stats_all_debug.LISTEN += value_s;
            break;
        case ENERGEST_TYPE_GPS:
            chirp_stats_all_debug.GPS += value_s;
            break;
        default:
            break;
    }
}

void Stats_to_Flash(Mixer_Task task)
{
    uint16_t stats_len = 2 * ((sizeof(chirp_stats_all) + sizeof(uint64_t) - 1) / sizeof(uint64_t));
    uint32_t stats_array[stats_len];
    #if MX_LBT_ACCESS
        uint16_t stats_lbt_len = (LBT_CHANNEL_NUM + 1) / 2;
    #endif

    assert_reset((sizeof(stats_array) >= sizeof(chirp_stats_all)));
    memset((uint32_t *)stats_array, 0, sizeof(stats_array));
    memcpy((uint32_t *)stats_array, (uint32_t *)&chirp_stats_all.slot.stats_sum, sizeof(chirp_stats_all));

    if ((task != MX_ARRANGE) && (task != MX_COLLECT))
    {
        FLASH_If_Erase_Pages(1, DAEMON_PAGE);
        FLASH_If_Write(DAEMON_FLASH_ADDRESS, (uint32_t *)stats_array, sizeof(stats_array) / sizeof(uint32_t));
        #if MX_LBT_ACCESS
        stats_lbt_len = (LBT_CHANNEL_NUM + 1) / 2;
        FLASH_If_Write(DAEMON_FLASH_ADDRESS + sizeof(stats_array) * 2, (uint32_t *)&chirp_config.lbt_channel_time_stats_us[0], stats_lbt_len * sizeof(uint64_t) / sizeof(uint32_t));
        #endif
    }
    else if (task == MX_COLLECT)
    {
        uint32_t flash_data = *(__IO uint32_t*)(DAEMON_FLASH_ADDRESS + sizeof(stats_array));
        if (flash_data == 0xFFFFFFFF)
        {
            FLASH_If_Write(DAEMON_FLASH_ADDRESS + sizeof(stats_array), (uint32_t *)stats_array, sizeof(stats_array) / sizeof(uint32_t));
            #if MX_LBT_ACCESS
            stats_lbt_len = (LBT_CHANNEL_NUM + 1) / 2;
            FLASH_If_Write(DAEMON_FLASH_ADDRESS + sizeof(stats_array) * 2 + stats_lbt_len * sizeof(uint64_t), (uint32_t *)&chirp_config.lbt_channel_time_stats_us[0], stats_lbt_len * sizeof(uint64_t) / sizeof(uint32_t));
            #endif
        }
    }
}
