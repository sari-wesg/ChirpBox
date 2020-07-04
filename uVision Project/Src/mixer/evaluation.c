//**************************************************************************************************
//**** Includes ************************************************************************************

#include "mixer_internal.h"

#if MX_PACKET_TABLE

#ifdef MX_CONFIG_FILE
	#include STRINGIFY(MX_CONFIG_FILE)
#endif

#include "evaluation.h"

#if ENERGEST_CONF_ON
#include GPI_PLATFORM_PATH(energest.h)
#endif

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif
//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************
extern uint8_t node_id_allocate;
static uint8_t real_packet_group[GROUP_NUM][MAX_GENERATE_GROUP_LENGTH];
static uint16_t real_action_time[GROUP_NUM][MAX_ACTION_LENGTH];


#if TEST_ROUND
    static uint8_t test_num;
	static uint32_t sensor_reliability[TEST_ROUND_NUM];
	static uint32_t sensor_latency[TEST_ROUND_NUM];
	static uint32_t energy[TEST_ROUND_NUM];
	static uint32_t action_reliability[TEST_ROUND_NUM];
	static uint32_t action_latency[TEST_ROUND_NUM];

    Sta_result test_sensor_reliability;
    Sta_result test_sensor_latency;
    Sta_result test_energy;
    Sta_result test_action_reliability;
    Sta_result test_action_latency;
#endif
//**************************************************************************************************
//***** Local Functions ****************************************************************************

//**************************************************************************************************
//***** Global  Variables *******************************************************************
extern uint8_t test_round;
extern int32_t the_given_second;

//**************************************************************************************************
//***** Global Functions ***************************************************************************

//**************************************************************************************************
void clear_real_packet()
{
    memset(&real_packet_group, 0, sizeof(real_packet_group));
    memset(&real_action_time, 0, sizeof(real_action_time));
}

void update_packet_table()
{
    int i = 0;
	uint8_t	        data[5];
    uint16_t        beat_number;
    uint16_t        packet_id;
    for (i = 0; i < MX_GENERATION_SIZE; i++)
    {
        void *p = mixer_read(i);
        if (NULL != p)
        {
            memcpy(data, p, 5);
            beat_number = data[1] >> 8 | data[2];
            packet_id = node_generate[data[3]][beat_number - 1];
            // PRINTF("packet_id: %d, %d, %d, %d\n", packet_id, data[3], beat_number - 1, new_packets_time[packet_id]);
            if ((!evaluation.packet_table[packet_id].flag) && (data[3] == i) && (data[0] == payload_distribution[i] + 1) && (data[4] < 2))
            {
                PRINTF("table: %d, %d, %d, %d, %d, %d\n", packet_id, data[1], data[2], data[3], beat_number - 1, new_packets_time[packet_id]);
                evaluation.packet_table[packet_id].flag = 1;
                #if GPS_DATA
                    // evaluation.packet_table[packet_id].latency = now_pps_count() - new_packets_time[packet_id];
                    if (gpi_tick_compare_slow_native(now_pps_count(), new_packets_time[packet_id]) >= 0)
                        evaluation.packet_table[packet_id].latency = now_pps_count() - new_packets_time[packet_id];
                    else
                        evaluation.packet_table[packet_id].latency = 0;
                #else
                    evaluation.packet_table[packet_id].latency = mx.slot_number - new_packets_time[packet_id];
                #endif
                evaluation.packet_table[packet_id].node_id = i;
                #if GPS_DATA
                    evaluation.packet_table[packet_id].gps_time = now_pps_count();
                #else
                    evaluation.packet_table[packet_id].gps_time = mx.slot_number;
                #endif
                evaluation.packet_table[packet_id].set = data[4];
            }
		}
	}
}

//**************************************************************************************************
void read_packet_table()
{
    int i = 0;
    for (i = 0; i < MX_PACKET_TABLE_SIZE; i++)
    {
        PRINTF("%d, %d, %d, %d, %d, %d\n", i, evaluation.packet_table[i].flag, evaluation.packet_table[i].latency,
        evaluation.packet_table[i].node_id, evaluation.packet_table[i].gps_time, evaluation.packet_table[i].set);
	}
}

void get_real_packet_group()
{
    uint8_t group_id, i, counter;
    for ( group_id = 0; group_id < GROUP_NUM; group_id++)
    {
        counter = 0;
        for ( i = 0; i < MX_PACKET_TABLE_SIZE; i++)
        {
            if ((evaluation.packet_table[i].flag) && ((evaluation.packet_table[i].node_id / SENSOR_NUM) == group_id))
            {
                real_packet_group[group_id][counter] = i;
                counter++;
            }
        }
    }

    for (group_id = 0; group_id < GROUP_NUM; group_id++)
    {
        for (i = 0; i < MAX_GENERATE_GROUP_LENGTH; i++)
        {
            PRINTF("%d ", real_packet_group[group_id][i]);
        }
        PRINTF("\n");
    }
}

//**************************************************************************************************
void calculate_action_time()
{
    uint8_t group_id, i, counter, action_counter;
    for ( group_id = 0; group_id < GROUP_NUM; group_id++)
    {
        counter = 0;
        action_counter = 0;
        for ( i = 0; i < MAX_GENERATE_GROUP_LENGTH; i++)
        {
            if (!evaluation.packet_table[real_packet_group[group_id][i]].set)
                counter = 0;
            else if (evaluation.packet_table[real_packet_group[group_id][i]].set)
            {
                counter++;
                if (counter >= CONSECUTIVE_NUM)
                {
                    if (gpi_tick_compare_slow_native(evaluation.packet_table[real_packet_group[group_id][i]].gps_time,
                    nominal_action_time[group_id][action_counter]) >= 0)
                    {
                        counter = 0;
                        real_action_time[group_id][action_counter] = evaluation.packet_table[real_packet_group[group_id][i]].gps_time;
                        action_counter++;
                    }
                }
            }
        }
    }

    for (group_id = 0; group_id < GROUP_NUM; group_id++)
    {
        for (i = 0; i < MAX_ACTION_LENGTH; i++)
        {
            PRINTF("%d ", real_action_time[group_id][i]);
        }
        PRINTF("\n");
    }
}

//**************************************************************************************************
void evaluation_results()
{
    uint16_t i;
    uint32_t sensor_packet_num = 0, sensor_packet_latency = 0;
    // sensor reliability and sensor latency:
    for ( i = 1; i < MX_PACKET_TABLE_SIZE; i++)
    {
        if (evaluation.packet_table[i].flag)
        {
            sensor_packet_num ++;
            sensor_packet_latency += evaluation.packet_table[i].latency;
        }
    }
    evaluation.result_packet.sensor_reliability = (uint32_t)((sensor_packet_num * 1e4) / (uint32_t)(MX_PACKET_TABLE_SIZE - 1));
    evaluation.result_packet.sensor_latency = (uint32_t)((sensor_packet_latency * 1e2) / (uint32_t)(sensor_packet_num));
    // sensor energy:
    evaluation.result_packet.energy = ((((uint32_t)gpi_tick_hybrid_to_us(energest_type_time(ENERGEST_TYPE_LISTEN) +
								energest_type_time(ENERGEST_TYPE_TRANSMIT))) * 1e3) / (uint32_t)(ECHO_PERIOD));

    // action reliability and action latency:
    if (node_id_allocate >= SENSOR_NUM * GROUP_NUM)
    {
        uint8_t group_id = (node_id_allocate -  SENSOR_NUM * GROUP_NUM + 1) / GROUP_NUM;
        uint32_t real_action_num = 0, action_latency = 0, nominal_action_num = 0;
        for ( i = 0; i < MAX_ACTION_LENGTH; i++)
        {
            if (real_action_time[group_id][i])
            {
                real_action_num ++;
                action_latency += real_action_time[group_id][i] - nominal_action_time[group_id][i];
                // printf("real_action:%lu, %lu, %lu, %lu\n", real_action_num, real_action_time[group_id][i], 
                // nominal_action_time[group_id][i], action_latency);
            }
            if (nominal_action_time[group_id][i])
            {
                nominal_action_num ++;
                // printf("nom_action:%lu\n", nominal_action_num);
            }
            else
                break;
        }
        evaluation.result_packet.action_reliability = (uint32_t)((real_action_num * 1e4) / nominal_action_num);
        evaluation.result_packet.action_latency = (uint32_t)((action_latency * 1e2) / real_action_num);
    }

    PRINTF("results:\n");
	PRINTF("sensor_reliability:%lu.%02lu%%\n", evaluation.result_packet.sensor_reliability / 100, evaluation.result_packet.sensor_reliability % 100);
	PRINTF("sensor_latency (slot/second):%lu.%02lu\n", evaluation.result_packet.sensor_latency / 100, evaluation.result_packet.sensor_latency % 100);
	PRINTF("energy:%lu.%03lu\n", evaluation.result_packet.energy / 1000, evaluation.result_packet.energy % 1000);
    if (node_id_allocate >= SENSOR_NUM * GROUP_NUM)
    {
        PRINTF("action_reliability:%lu.%02lu%%\n", evaluation.result_packet.action_reliability / 100, evaluation.result_packet.action_reliability % 100);
        PRINTF("action_latency (slot/second):%lu.%02lu\n", evaluation.result_packet.action_latency / 100, evaluation.result_packet.action_latency % 100);
    }

    #if TEST_ROUND
        sensor_reliability[test_num] = evaluation.result_packet.sensor_reliability;
        sensor_latency[test_num] = evaluation.result_packet.sensor_latency;
        energy[test_num] = evaluation.result_packet.energy;
        if (node_id_allocate >= SENSOR_NUM * GROUP_NUM)
        {
            action_reliability[test_num] = evaluation.result_packet.action_reliability;
            action_latency[test_num] = evaluation.result_packet.action_latency;
        }
        test_num++;
    #endif
}

//**************************************************************************************************


//**************************************************************************************************
#if TEST_ROUND

static void statistics_array_results(uint32_t *array, uint8_t result_case, uint8_t worst_case)
{
    uint32_t i, sum = 0, worst = 0, avg = 0, Spow = 0, std = 0;
    if(!worst_case)
        worst = 0xFFFFFFFFU;

    for ( i = 0; i < TEST_ROUND_NUM; i++)
    {
        sum += array[i];
        if (!worst_case)
        {
            // the lowest is worst
            if (worst > array[i])
                worst = array[i];
        }
        else
        {
            // the highest is worst
            if (worst < array[i])
                worst = array[i];
        }
    }

    avg = sum / TEST_ROUND_NUM;
    for( i = 0 ; i < TEST_ROUND_NUM; i++)
        Spow += (array[i] - avg) * (array[i] - avg);
    std = sqrt(Spow / (TEST_ROUND_NUM - 1));

    switch (result_case)
    {
    case 1:
        {
            test_sensor_reliability.mean = avg;
            test_sensor_reliability.std = std;
            test_sensor_reliability.worst = worst;
        }
        break;
    case 2:
        {
            test_sensor_latency.mean = avg;
            test_sensor_latency.std = std;
            test_sensor_latency.worst = worst;
        }
        break;
    case 3:
        {
            test_energy.mean = avg;
            test_energy.std = std;
            test_energy.worst = worst;
        }
        break;
    case 4:
        {
            test_action_reliability.mean = avg;
            test_action_reliability.std = std;
            test_action_reliability.worst = worst;
        }
        break;
    case 5:
        {
            test_action_latency.mean = avg;
            test_action_latency.std = std;
            test_action_latency.worst = worst;
        }
        break;
    default:
        break;
    }
}

void statistics_results()
{
    statistics_array_results(sensor_reliability, 1, 0);
    statistics_array_results(sensor_latency, 2, 1);
    statistics_array_results(energy, 3, 1);
    if (node_id_allocate >= SENSOR_NUM * GROUP_NUM)
    {
        statistics_array_results(action_reliability, 4, 0);
        statistics_array_results(action_latency, 5, 1);
    }

    PRINTF("all test results:-----------\n");
	PRINTF("sensor_reliability:%lu.%02lu%%, %lu.%02lu%%, %lu.%02lu%%\n", test_sensor_reliability.mean / 100, test_sensor_reliability.mean % 100,
                                            test_sensor_reliability.std / 100, test_sensor_reliability.std % 100,
                                            test_sensor_reliability.worst / 100, test_sensor_reliability.worst % 100);
	PRINTF("sensor_latency (slot/second):%lu.%02lu, %lu.%02lu, %lu.%02lu\n", test_sensor_latency.mean / 100, test_sensor_latency.mean % 100,
                                            test_sensor_latency.std / 100, test_sensor_latency.std % 100,
                                            test_sensor_latency.worst / 100, test_sensor_latency.worst % 100);
	PRINTF("energy:%lu.%03lu, %lu.%03lu, %lu.%03lu\n", test_energy.mean / 1000, test_energy.mean % 1000,
                                            test_energy.std / 1000, test_energy.std % 1000,
                                            test_energy.worst / 1000, test_energy.worst % 1000);
    if (node_id_allocate >= SENSOR_NUM * GROUP_NUM)
    {
        PRINTF("action_reliability:%lu.%02lu%%, %lu.%02lu%%, %lu.%02lu%%\n", test_action_reliability.mean / 100, test_action_reliability.mean % 100,
                                            test_action_reliability.std / 100, test_action_reliability.std % 100,
                                            test_action_reliability.worst / 100, test_action_reliability.worst % 100);
        PRINTF("action_latency (slot/second):%lu.%02lu, %lu.%02lu, %lu.%02lu\n", test_action_latency.mean / 100, test_action_latency.mean % 100,
                                            test_action_latency.std / 100, test_action_latency.std % 100,
                                            test_action_latency.worst / 100, test_action_latency.worst % 100);
    }
}

#endif

//**************************************************************************************************
//**************************************************************************************************

#endif	// MX_PACKET_TABLE