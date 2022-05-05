//**************************************************************************************************
//**** Includes ************************************************************************************
#include "mixer_internal.h"

#include "loradisc.h"
#include "stm32l4xx_hal.h"
#include "gpi/olf.h"
#include "gpi/clocks.h"

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
LoRaDisC_Discover_Config loradisc_discover_config;
extern LoRaDisC_Config loradisc_config;
extern uint8_t MX_NUM_NODES_CONF;

//**************************************************************************************************
//***** Local Functions ****************************************************************************

//**************************************************************************************************
//***** Global Functions ***************************************************************************
void calculate_next_loradisc()
{
    // loradisc_discover_config.next_loradisc_gap =
}

void calculate_next_lorawan()
{
    uint8_t discovered_node_num = gpi_popcnt_32(loradisc_discover_config.node_id_bitmap);
    uint8_t i;
    for (i = 0; i < discovered_node_num; i++)
    {
        /* lorawan time is behind now */
        if (gpi_tick_compare_slow_extended(gpi_tick_slow_extended(), loradisc_discover_config.lorawan_begin[i]) >= 0)
        {
            uint8_t multiple_lorawan_interval = (gpi_tick_slow_extended() - loradisc_discover_config.lorawan_begin[i] + GPI_TICK_S_TO_SLOW(loradisc_discover_config.lorawan_interval_s[i]) - 1) / GPI_TICK_S_TO_SLOW(loradisc_discover_config.lorawan_interval_s[i]);
            loradisc_discover_config.lorawan_begin[i] += multiple_lorawan_interval * GPI_TICK_S_TO_SLOW(loradisc_discover_config.lorawan_interval_s[i]);
        }
        else
        {
            uint8_t multiple_lorawan_interval = (loradisc_discover_config.lorawan_begin[i] - gpi_tick_slow_extended() + GPI_TICK_S_TO_SLOW(loradisc_discover_config.lorawan_interval_s[i]) - 1) / GPI_TICK_S_TO_SLOW(loradisc_discover_config.lorawan_interval_s[i]);
            if (multiple_lorawan_interval)
                loradisc_discover_config.lorawan_begin[i] = loradisc_discover_config.lorawan_begin[i] - (multiple_lorawan_interval - 1) * GPI_TICK_S_TO_SLOW(loradisc_discover_config.lorawan_interval_s[i]);
        }
    }
}

uint8_t compare_discover_initiator_expired()
{
    uint8_t data_length = sizeof(uint8_t) + sizeof(uint16_t) + sizeof(Gpi_Slow_Tick_Extended);

    loradisc_reconfig(MX_NUM_NODES_CONF, MX_NUM_NODES_CONF, data_length, FLOODING, 7, 14, CN470_FREQUENCY);
    loradisc_discover_update_initiator();

    loradisc_discover_config.discover_duration_slow = GPI_TICK_MS_TO_SLOW((uint16_t)((loradisc_config.mx_slot_length_in_us * DISCOVER_SLOT_DEFAULT + 177320) / 1000));

    Gpi_Slow_Tick_Extended discover_end = gpi_tick_slow_extended() + loradisc_discover_config.discover_duration_slow + GPI_TICK_MS_TO_SLOW(100);
    printf("discover_end:%lu, %lu, %lu, %lu\n", discover_end, loradisc_discover_config.lorawan_begin[node_id_allocate], (loradisc_config.initiator == node_id_allocate), (gpi_tick_compare_slow_extended(discover_end, loradisc_discover_config.lorawan_begin[node_id_allocate]) >= 0));

    /* if the node id is not 0, and the initiator is the node itself, and the discover can not end before lorawan begin */
    if ((loradisc_config.initiator == node_id_allocate) && (gpi_tick_compare_slow_extended(discover_end, loradisc_discover_config.lorawan_begin[node_id_allocate]) >= 0) && (node_id_allocate))
        return 0;
    else
        return 1;
}

uint8_t reset_discover_slot_num()
{
    uint8_t slot_num;
    loradisc_discover_config.discover_duration_slow = GPI_TICK_MS_TO_SLOW((uint16_t)((loradisc_config.mx_slot_length_in_us * DISCOVER_SLOT_DEFAULT + 177320) / 1000));

    if (loradisc_config.initiator != node_id_allocate)
    {
        /* calculate the discover end time */
        Gpi_Slow_Tick_Extended discover_end = gpi_tick_slow_extended() + loradisc_discover_config.discover_duration_slow + GPI_TICK_MS_TO_SLOW(100);

        if (gpi_tick_compare_slow_extended(discover_end, loradisc_discover_config.lorawan_begin[node_id_allocate]) >= 0)
        {
            Gpi_Slow_Tick_Extended rest_time;
            rest_time = loradisc_discover_config.lorawan_begin[node_id_allocate] - gpi_tick_slow_extended() - GPI_TICK_MS_TO_SLOW(1000);

            /* slot_num = (LoRaWAN start time - discover end time) / slot length */
            if ((int32_t)(rest_time) > 0)
                slot_num = (rest_time) / GPI_TICK_MS_TO_SLOW((uint16_t)(loradisc_config.mx_slot_length_in_us / 1000));
            else
                slot_num = 0;

            /* update round length */
            loradisc_config.mx_round_length = slot_num;
            printf("slot_num:%lu, %lu, %lu, %lu, %d, %d\n", slot_num, loradisc_discover_config.lorawan_begin[node_id_allocate], gpi_tick_slow_extended(), GPI_TICK_MS_TO_SLOW((uint16_t)(loradisc_config.mx_slot_length_in_us / 1000)), (int32_t)(rest_time), ((int32_t)(rest_time) > 0));

            /* update the discover_duration_slow with new slot num */
            loradisc_discover_config.discover_duration_slow = GPI_TICK_MS_TO_SLOW((uint16_t)((loradisc_config.mx_slot_length_in_us * slot_num + 177320) / 1000));
        }
        else
            slot_num = 1;
    }
    else
        slot_num = 1;
    return slot_num;
}

//**************************************************************************************************
