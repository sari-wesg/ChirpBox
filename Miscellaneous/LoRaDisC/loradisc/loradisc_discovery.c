//**************************************************************************************************
//**** Includes ************************************************************************************
#include "mixer_internal.h"

#include "loradisc.h"
#include "stm32l4xx_hal.h"
#include "gpi/olf.h"
#include "gpi/clocks.h"

/* for relay loradisc with lorawan */
#include "lora.h"
#include "lorawan.h"


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


// TODO:
uint32_t lastest_loradisc_time;
Gpi_Slow_Tick_Extended max_loradisc_start_time, min_loradisc_end_time, min_max_loradisc_start_time;

extern lora_AppData_t AppData;
//**************************************************************************************************
//***** Local Functions ****************************************************************************

//**************************************************************************************************
//***** Global Functions ***************************************************************************

/* Calculate time for LoRaDisC */

int check_combination_groups(int a[], int b[], const int N, const int M)
{
    int max, min;
    int result = 1;

    /* numbers are listed from largest to smallest */
    for (int i = M - 1; i >= 0; i--)
    {
        max = a[i*N/M + N/M-1];
        min = a[i*N/M];

        if ((a[b[i]] < min) || (a[b[i]] > max))
        {
            result = 0;
            break;
        }
    }

    return result;
}

void generate_combination(int n, int m, int a[], int b[], const int N, const int M, Gpi_Slow_Tick_Extended loradisc_start_time[], Gpi_Slow_Tick_Extended loradisc_end_time[], Gpi_Slow_Tick_Extended loradisc_duration)
{

    for (int j = n; j >= m; j--)

    {

        b[m - 1] = j - 1;

        if (m > 1)
            generate_combination(j - 1, m - 1, a, b, N, M, loradisc_start_time, loradisc_end_time, loradisc_duration); // Recursion
        else
        {
            /* Combination requirements: each number belongs to a group, i.e, each number does not exceed the maximum minimum value of the group */
            if (check_combination_groups(a, b, N, M))
            {
                max_loradisc_start_time = 0;
                min_loradisc_end_time = 0xFFFFFFFF;
                for (int i = M - 1; i >= 0; i--)
                {
                    max_loradisc_start_time = MAX(max_loradisc_start_time, loradisc_start_time[a[b[i]]]);
                    min_loradisc_end_time = MIN(min_loradisc_end_time, loradisc_end_time[a[b[i]]]);

                    // printf("combine: %d, %d, %d, %d ", a[b[i]], loradisc_start_time[a[b[i]]], loradisc_end_time[a[b[i]]], max_loradisc_start_time, min_loradisc_end_time);
                }
                // printf("\n");

                if((int32_t)(min_loradisc_end_time - max_loradisc_start_time) > (int32_t)loradisc_duration)
                    min_max_loradisc_start_time = MIN(min_max_loradisc_start_time, max_loradisc_start_time);
                // printf("time: %d, %d\n", (int32_t)(min_loradisc_end_time - max_loradisc_start_time), min_max_loradisc_start_time);
            }
        }
    }
}
void select_LoRaDisC_time(uint8_t node_number, uint8_t time_length, Gpi_Slow_Tick_Extended loradisc_start_time[], Gpi_Slow_Tick_Extended loradisc_end_time[], Gpi_Slow_Tick_Extended loradisc_duration)
{
    uint8_t list_length;
    list_length = time_length * node_number;

    int a[list_length];
    int b[node_number];

    for (int i = 0; i < list_length; i++)

        a[i] = i;

    const int N = list_length;
    const int M = node_number;

    min_max_loradisc_start_time = 0xFFFFFFFF;
    generate_combination(list_length, node_number, a, b, N, M, loradisc_start_time, loradisc_end_time, loradisc_duration);
}

Gpi_Slow_Tick_Extended calculate_next_loradisc()
{
   /* collect data length */
    uint8_t data_length = LORADISC_COLLECT_LENGTH;  //TODO:

    if (loradisc_discover_config.loradisc_lorawan_on)
    {
        uint8_t packet_num = lorawan_relay_max_packetnum();
        loradisc_discover_config.lorawan_duration_slow = packet_num *         GPI_TICK_S_TO_SLOW(LORAWAN_DURATION_S + LORAWAN_GUARDTIME_S);
    }
    else
    {
        if (loradisc_discover_config.collect_on)
            loradisc_reconfig(MX_NUM_NODES_CONF, data_length, COLLECTION, fut_config.CUSTOM[FUT_LORADISC_SF], fut_config.CUSTOM[FUT_UPLINK_POWER], daemon_config.Frequency);
        else if (loradisc_discover_config.dissem_on)
            loradisc_reconfig(MX_NUM_NODES_CONF, data_length, DISSEMINATION, fut_config.CUSTOM[FUT_LORADISC_SF], fut_config.CUSTOM[FUT_UPLINK_POWER], daemon_config.Frequency);

        loradisc_discover_config.loradisc_duration_slow = GPI_TICK_MS_TO_SLOW((uint16_t)((loradisc_config.mx_slot_length_in_us * loradisc_config.mx_round_length + LORADISC_LATENCY) / 1000 + 1000));
    }

    uint8_t lorawan_node_number = gpi_popcnt_32(loradisc_discover_config.lorawan_bitmap);

    Gpi_Slow_Tick_Extended loradisc_start_time[lorawan_node_number * LORAWAN_TIME_LENGTH];
    Gpi_Slow_Tick_Extended loradisc_end_time[lorawan_node_number * LORAWAN_TIME_LENGTH];

    calculate_next_lorawan();
    for (uint8_t i = 0; i < lorawan_node_number; i++)
    {
        for (uint8_t j = 0; j < LORAWAN_TIME_LENGTH; j++)
        {
            if (j == 0)
                {
                    /* before lorawan start */
                    if (gpi_tick_compare_slow_extended(gpi_tick_slow_extended(), loradisc_discover_config.lorawan_begin[i] + GPI_TICK_S_TO_SLOW(LORAWAN_DURATION_S) - GPI_TICK_S_TO_SLOW(loradisc_discover_config.lorawan_interval_s[i])) >= 0)
                        loradisc_start_time[i * LORAWAN_TIME_LENGTH + j] = gpi_tick_slow_extended();
                    /* after a loradisc */
                    else
                        loradisc_start_time[i * LORAWAN_TIME_LENGTH + j] = loradisc_discover_config.lorawan_begin[i] + GPI_TICK_S_TO_SLOW(LORAWAN_DURATION_S) - GPI_TICK_S_TO_SLOW(loradisc_discover_config.lorawan_interval_s[i]);
                }
            else
            {
                loradisc_start_time[i * LORAWAN_TIME_LENGTH + j] = loradisc_discover_config.lorawan_begin[i] + GPI_TICK_S_TO_SLOW(LORAWAN_DURATION_S) + GPI_TICK_S_TO_SLOW(loradisc_discover_config.lorawan_interval_s[i]) * (j - 1);
            }
            loradisc_end_time[i * LORAWAN_TIME_LENGTH + j] = loradisc_discover_config.lorawan_begin[i] + GPI_TICK_S_TO_SLOW(loradisc_discover_config.lorawan_interval_s[i]) * j;
        }
    }

    printf("now:%lu, %lu\n", gpi_tick_slow_extended(), loradisc_discover_config.loradisc_duration_slow);
    for (uint8_t i = 0; i < lorawan_node_number * LORAWAN_TIME_LENGTH; i++)
    {
        printf("%lu, %lu\n", (Gpi_Slow_Tick_Extended)(loradisc_start_time[i]), (Gpi_Slow_Tick_Extended)(loradisc_end_time[i]));
    }

    if (loradisc_discover_config.loradisc_lorawan_on)
        select_LoRaDisC_time(lorawan_node_number, LORAWAN_TIME_LENGTH, loradisc_start_time, loradisc_end_time, loradisc_discover_config.lorawan_duration_slow);
    else
        select_LoRaDisC_time(lorawan_node_number, LORAWAN_TIME_LENGTH, loradisc_start_time, loradisc_end_time, loradisc_discover_config.loradisc_duration_slow);

    printf("loradisc:%lu\n", min_max_loradisc_start_time);
    return min_max_loradisc_start_time;
}
// ---------------------------------------------------------------------------------
void calculate_next_lorawan()
{
    printf("--- calculate_next_lorawan\n");
    uint8_t discovered_node_num = gpi_popcnt_32(loradisc_discover_config.node_id_bitmap);
    uint8_t i;
    for (i = 0; i < discovered_node_num; i++)
    {
        printf("--- node %d, now: %lu\n", i, gpi_tick_slow_extended());
        /* lorawan time is before now */
        if (gpi_tick_compare_slow_extended(gpi_tick_slow_extended(), loradisc_discover_config.lorawan_begin[i]) >= 0)
        {
            int8_t multiple_lorawan_interval = (int32_t)(gpi_tick_slow_extended() - loradisc_discover_config.lorawan_begin[i] + GPI_TICK_S_TO_SLOW(loradisc_discover_config.lorawan_interval_s[i]) - 1) / GPI_TICK_S_TO_SLOW(loradisc_discover_config.lorawan_interval_s[i]);
            loradisc_discover_config.lorawan_begin[i] += multiple_lorawan_interval * GPI_TICK_S_TO_SLOW(loradisc_discover_config.lorawan_interval_s[i]);
            printf("--- behind: multiple_lorawan_interval: %lu, rest_time: %d interval_in_slow: %lu, new lorawan_begin:%lu\n", multiple_lorawan_interval, (int32_t)(gpi_tick_slow_extended() - loradisc_discover_config.lorawan_begin[i]), GPI_TICK_S_TO_SLOW(loradisc_discover_config.lorawan_interval_s[i]), loradisc_discover_config.lorawan_begin[i]);
        }
        else
        {
            int8_t multiple_lorawan_interval = (int32_t)(loradisc_discover_config.lorawan_begin[i] - gpi_tick_slow_extended()) / GPI_TICK_S_TO_SLOW(loradisc_discover_config.lorawan_interval_s[i]);
            if (multiple_lorawan_interval > 0)
                loradisc_discover_config.lorawan_begin[i] = loradisc_discover_config.lorawan_begin[i] - multiple_lorawan_interval * GPI_TICK_S_TO_SLOW(loradisc_discover_config.lorawan_interval_s[i]);
            printf("--- after: multiple_lorawan_interval: %d, rest_time: %d interval_in_slow: %lu, new lorawan_begin:%lu\n", multiple_lorawan_interval, (int32_t)(loradisc_discover_config.lorawan_begin[i] - gpi_tick_slow_extended()), GPI_TICK_S_TO_SLOW(loradisc_discover_config.lorawan_interval_s[i]), loradisc_discover_config.lorawan_begin[i]);
        }
    }
}

/* For discovery initiator: check if the initiator can complete LoRaDisC before LoRaWAN, if not, return 0 and vice versa */
uint8_t compare_discover_initiator_expired()
{
    uint8_t data_length = sizeof(uint8_t) + sizeof(uint16_t) + sizeof(Gpi_Slow_Tick_Extended);

    loradisc_reconfig(MX_NUM_NODES_CONF, data_length, FLOODING, fut_config.CUSTOM[FUT_LORADISC_SF], fut_config.CUSTOM[FUT_UPLINK_POWER], daemon_config.Frequency);
    loradisc_discover_update_initiator();

    loradisc_discover_config.discover_duration_slow = GPI_TICK_MS_TO_SLOW((uint16_t)((loradisc_config.mx_slot_length_in_us * DISCOVER_SLOT_DEFAULT + LORADISC_LATENCY) / 1000));

    Gpi_Slow_Tick_Extended discover_end = gpi_tick_slow_extended() + loradisc_discover_config.discover_duration_slow + GPI_TICK_MS_TO_SLOW(100);
    printf("check initiator discover expired:\ndiscover will end at %lu, node will begin LoRaWAN at %lu, if node is initiator %lu, if discover will end after LoRaWAN %lu\n", discover_end, loradisc_discover_config.lorawan_begin[node_id_allocate], (loradisc_config.initiator == node_id_allocate), (gpi_tick_compare_slow_extended(discover_end, loradisc_discover_config.lorawan_begin[node_id_allocate]) >= 0));

    /* if the node id is not 0, and the initiator is the node itself, and the discover can not end before lorawan begin, then return 0 */
    if ((loradisc_config.initiator == node_id_allocate) && (gpi_tick_compare_slow_extended(discover_end, loradisc_discover_config.lorawan_begin[node_id_allocate]) >= 0) && (node_id_allocate))
    {
        return 0;
        printf("--- Node %d stop initiate LoRaDisC\n", node_id_allocate);
    }
    else
        return 1;
}

/* For discovery receivers: ensure the receiver ends LoRaDisC before LoRaWAN comes */
uint8_t reset_discover_slot_num()
{
    uint8_t slot_num, LoRaDisC_complete = 1;
    loradisc_discover_config.discover_duration_slow = GPI_TICK_MS_TO_SLOW((uint16_t)((loradisc_config.mx_slot_length_in_us * DISCOVER_SLOT_DEFAULT + LORADISC_LATENCY) / 1000));

    /* node is receiver and LoRaWAN nodes */
    if ((loradisc_config.initiator != node_id_allocate) && ((loradisc_discover_config.lorawan_bitmap & (1 << (node_id_allocate % 32)))))
    {
        /* calculate the discover end time */
        Gpi_Slow_Tick_Extended discover_end = gpi_tick_slow_extended() + loradisc_discover_config.discover_duration_slow + GPI_TICK_MS_TO_SLOW(100);

        /* if the LoRaDisC cannot be completed before LoRaWAN */
        if (gpi_tick_compare_slow_extended(discover_end, loradisc_discover_config.lorawan_begin[node_id_allocate]) >= 0)
        {
            Gpi_Slow_Tick_Extended rest_time;
            /* rest_time = LoRaWAN start time - now - guard time */
            rest_time = loradisc_discover_config.lorawan_begin[node_id_allocate] - gpi_tick_slow_extended() - GPI_TICK_MS_TO_SLOW(1000);

            /* if rest_time > 0, slot_num = rest_time / slot length */
            if ((int32_t)(rest_time) > 0)
                slot_num = (rest_time) / GPI_TICK_MS_TO_SLOW((uint16_t)(loradisc_config.mx_slot_length_in_us / 1000));
            else
                LoRaDisC_complete = 0;

            /* update round length */
            loradisc_config.mx_round_length = slot_num;
            if (!LoRaDisC_complete)
                printf("--- LoRaDisC receiver cannot complete discover\n");
            else
                printf("--- LoRaDisC receiver update slot number\nslot_num:%lu, LoRaWAN start at %lu, now: %lu, LoRaDisC slot length(ms): %lu, rest time: %d, if rest > 0: %d\n", slot_num, loradisc_discover_config.lorawan_begin[node_id_allocate], gpi_tick_slow_extended(), GPI_TICK_MS_TO_SLOW((uint16_t)(loradisc_config.mx_slot_length_in_us / 1000)), (int32_t)(rest_time), ((int32_t)(rest_time) > 0));

            /* update the discover_duration_slow with new slot num */
            loradisc_discover_config.discover_duration_slow = GPI_TICK_MS_TO_SLOW((uint16_t)((loradisc_config.mx_slot_length_in_us * slot_num + LORADISC_LATENCY) / 1000));
        }
    }
    return LoRaDisC_complete;
}

int lorawan_loradisc_send(uint8_t *data, uint8_t data_length)
{
    lora_tx_rate(DR_5);
    AppData.Port = LORAWAN_APP_PORT;
    // uint32_t i = 0;
    // send_count++;
    // PRINTF("send:%lu\n", send_count);
    memcpy(AppData.Buff, (uint8_t *)(data), data_length);

    // AppData.Buff[i++] = send_count >> 8;
    // AppData.Buff[i++] = send_count;
    // AppData.Buff[i++] = 0xff;
    // AppData.Buff[i++] = 0xee;
    // AppData.Buff[i++] = 0xdd;
    // AppData.Buff[i++] = 0xcc;
    // AppData.Buff[i++] = 0xbb;
    // AppData.Buff[i++] = 0xaa;

    AppData.BuffSize = data_length;
    // for (i = 0; i < AppData.BuffSize; i++)
    // {
    //     /* code */
    //     PRINTF("%02x ", AppData.Buff[i]);
    // }
    // PRINTF("\n");

    LORA_send(&AppData, LORAWAN_DEFAULT_CONFIRM_MSG_STATE);

    return 0;
}
//**************************************************************************************************
