//**************************************************************************************************
//**** Includes ************************************************************************************

#include "mixer_internal.h"
#include "chirp_internal.h"

#ifdef MX_CONFIG_FILE
	#include STRINGIFY(MX_CONFIG_FILE)
#endif

#include "gpi/olf.h"
#include GPI_PLATFORM_PATH(radio.h)

#if MX_FLASH_FILE
	#include "flash_if.h"
#endif

#if ENERGEST_CONF_ON
#include GPI_PLATFORM_PATH(energest.h)
#endif

#include "loradisc.h"
#include <stdlib.h>
#include "chirpbox_func.h"
//**************************************************************************************************
//***** Local Defines and Consts *******************************************************************
#if DEBUG_CHIRPBOX
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

extern uint32_t __attribute__((section(".data"))) TOS_NODE_ID;
//**************************************************************************************************
//***** Local Typedefs and Class Declarations ******************************************************

//**************************************************************************************************
//***** Forward Declarations ***********************************************************************

//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************
// static uint32_t			round;
static uint8_t	        data[DATA_HEADER_LENGTH];

static uint16_t    deadline_dog, count_dog;

static uint16_t    rece_dissem_index;
uint16_t    calu_payload_hash, rece_hash;
//**************************************************************************************************
//***** Global Variables ***************************************************************************

#if MX_FLASH_FILE
    uint8_t                     uartRxBuffer[128];

    Mixer_Task                  task;

    /* indicate whether uart finished receiving */
    extern volatile uint8_t     uart_read_done;

    extern UART_HandleTypeDef   huart2;

#endif

//**************************************************************************************************
//***** Local Functions ****************************************************************************
void PRINT_PACKET(uint8_t *p, uint8_t len, uint8_t Packet)
{
    uint8_t i;
    if (Packet)
        PRINTF("r ");
    else
        PRINTF("f ");
    for (i = 0; i < len; i++)
        PRINTF("%02x ", ((uint8_t *)p)[i]);
    PRINTF("\n");
}

//**************************************************************************************************
//***** Global Functions ***************************************************************************

void clear_data()
{
    memset(data, 0, sizeof(data));
}

//**************************************************************************************************

void uart_read_data(uint8_t uart_isr_flag, uint8_t buffer_len)
{
    if (!uart_isr_flag)
    {
        /* executed to open uart receive interrupt */
        __HAL_UART_DISABLE(&huart2);
        __HAL_UART_ENABLE(&huart2);
        memset(&uartRxBuffer, 0, sizeof(uartRxBuffer));
        HAL_UART_Receive_IT(&huart2, (uint8_t*)uartRxBuffer, buffer_len);
        uart_read_done = 0;
    }
    else
    {
        /* executed from uart interrupt */
        uart_read_done = 1;
        __HAL_UART_DISABLE_IT(&huart2, UART_IT_RXNE);
    }
}

//**************************************************************************************************

void uart_read_command(uint8_t *p, uint8_t rxbuffer_len)
{
    memcpy((uint8_t *)p, (uint8_t *)uartRxBuffer, rxbuffer_len);
}

//**************************************************************************************************

/**
 * @description: mixer configurations
 * @param mx_num_nodes: number of nodes in the mixer network (NUM_ELEMENTS(nodes))
 * @param mx_generation_size: number of packets (NUM_ELEMENTS(payload_distribution))
 * @param mx_payload_size: length of payload (MX_PAYLOAD_CONF_SIZE)
 * @return: None
 */
void chirp_packet_config(uint8_t mx_num_nodes, uint8_t mx_generation_size, uint8_t mx_payload_size, Disc_Primitive primitive)
{
    memset(&chirp_config, 0, offsetof(Chirp_Config, mx_slot_length_in_us));
    chirp_config.primitive = primitive;
    // chirp_config
    chirp_config.mx_num_nodes = mx_num_nodes;
    chirp_config.mx_generation_size = mx_generation_size;
    chirp_config.mx_payload_size = mx_payload_size;

    chirp_config.coding_vector.pos = 0;
    chirp_config.coding_vector.len = (chirp_config.mx_generation_size + 7) / 8;
    chirp_config.payload.pos = chirp_config.coding_vector.pos + chirp_config.coding_vector.len;
    chirp_config.payload.len = chirp_config.mx_payload_size;
    chirp_config.info_vector.pos = chirp_config.payload.pos + chirp_config.payload.len;
    chirp_config.info_vector.len = (chirp_config.mx_generation_size + 7) / 8;
    chirp_config._padding_2.pos = chirp_config.info_vector.pos + chirp_config.info_vector.len;
    chirp_config._padding_2.len = PADDING_MAX(0,
                            PADDING_SIZE((chirp_config.mx_generation_size + 7) / 8)
                            + PADDING_SIZE(chirp_config.mx_payload_size)
            #if (MX_REQUEST || MX_SMART_SHUTDOWN_MAP)
                            - ((chirp_config.mx_generation_size + 7) / 8)
            #endif
                            );
    chirp_config.rand.pos = chirp_config._padding_2.pos + chirp_config._padding_2.len;
    chirp_config.rand.len = 1;
    chirp_config._padding_3.pos = chirp_config.rand.pos + chirp_config.rand.len;
    chirp_config._padding_3.len = PADDING_SIZE(
                                ((chirp_config.mx_generation_size + 7) / 8) +	// coding_vector
                                chirp_config.mx_payload_size +					// payload
#if (MX_REQUEST || MX_SMART_SHUTDOWN_MAP)
                                ((chirp_config.mx_generation_size + 7) / 8) +	// info_vector
    #if !GPI_ARCH_IS_BOARD(TMOTE)
                                PADDING_MAX(0,						// _padding_2
                                    PADDING_SIZE((chirp_config.mx_generation_size + 7) / 8)
                                    + PADDING_SIZE(chirp_config.mx_payload_size)
                                    - ((chirp_config.mx_generation_size + 7) / 8)
                                    ) +
    #endif
#else
    #if !GPI_ARCH_IS_BOARD(TMOTE)
                                PADDING_MAX(0,						// _padding_2
                                    PADDING_SIZE((chirp_config.mx_generation_size + 7) / 8)
                                    + PADDING_SIZE(chirp_config.mx_payload_size)) +
    #endif
#endif
                                1);
    chirp_config.packet_chunk_len = chirp_config.coding_vector.len + chirp_config.payload.len + chirp_config.info_vector.len + chirp_config._padding_2.len + chirp_config.rand.len + chirp_config._padding_3.len;
    chirp_config.phy_payload_size = offsetof(Packet, packet_chunk) - offsetof(Packet, phy_payload_begin) + chirp_config.coding_vector.len + chirp_config.payload.len + chirp_config.info_vector.len;
    chirp_config.packet_len = offsetof(Packet, packet_chunk) - offsetof(Packet, phy_payload_begin) + chirp_config.packet_chunk_len;
    assert_reset(!(chirp_config.packet_len % sizeof(uint_fast_t)));

    chirp_config.matrix_coding_vector_8.pos = 0;
    chirp_config.matrix_coding_vector_8.len = chirp_config.coding_vector.len;
    chirp_config.matrix_payload_8.pos = chirp_config.matrix_coding_vector_8.pos + chirp_config.matrix_coding_vector_8.len;
    chirp_config.matrix_payload_8.len = chirp_config.payload.len;

    chirp_config.matrix_coding_vector.pos = 0;
    chirp_config.matrix_coding_vector.len = (chirp_config.mx_generation_size + (sizeof(uint_fast_t) * 8) - 1) / (sizeof(uint_fast_t) * 8);
    chirp_config.matrix_payload.pos = chirp_config.matrix_coding_vector.pos + chirp_config.matrix_coding_vector.len;
    chirp_config.matrix_payload.len = (chirp_config.mx_payload_size + sizeof(uint_fast_t) - 1) / sizeof(uint_fast_t);

    chirp_config.matrix_chunk_8_len = chirp_config.matrix_coding_vector_8.len + chirp_config.matrix_payload_8.len;
    chirp_config.matrix_chunk_32_len = chirp_config.matrix_coding_vector.len + chirp_config.matrix_payload.len;
    chirp_config.matrix_size_32 = chirp_config.matrix_chunk_32_len + 1;

    chirp_config.history_len_8 = offsetof(Node, row_map_chunk) + chirp_config.matrix_coding_vector.len * sizeof(uint_fast_t);

    uint8_t hash_factor = (((chirp_config.mx_num_nodes + 7) / 8 + chirp_config.info_vector.len - 1) / chirp_config.info_vector.len);
    chirp_config.map.pos = 0;
    chirp_config.map.len = hash_factor * chirp_config.info_vector.len;
    chirp_config.hash.pos = chirp_config.map.pos + chirp_config.map.len;
    chirp_config.hash.len = chirp_config.info_vector.len;

    chirp_config.row_all_mask.pos = 0;
    chirp_config.row_all_mask.len = chirp_config.matrix_coding_vector.len;
    chirp_config.row_any_mask.pos = chirp_config.row_all_mask.pos + chirp_config.row_all_mask.len;
    chirp_config.row_any_mask.len = chirp_config.matrix_coding_vector.len;
    chirp_config.column_all_mask.pos = chirp_config.row_any_mask.pos + chirp_config.row_any_mask.len;
    chirp_config.column_all_mask.len = chirp_config.matrix_coding_vector.len;
    chirp_config.column_any_mask.pos = chirp_config.column_all_mask.pos + chirp_config.column_all_mask.len;
    chirp_config.column_any_mask.len = chirp_config.matrix_coding_vector.len;
    chirp_config.my_row_mask.pos = chirp_config.column_any_mask.pos + chirp_config.column_any_mask.len;
    chirp_config.my_row_mask.len = chirp_config.matrix_coding_vector.len;
    chirp_config.my_column_mask.pos = chirp_config.my_row_mask.pos + chirp_config.my_row_mask.len;
    chirp_config.my_column_mask.len = chirp_config.matrix_coding_vector.len;
}
/* slot length is mx_slot_length_in_us microseconds,
needed slot number is mx_round_length,
round is last for mx_period_time_us seconds */
void chirp_slot_config(uint32_t mx_slot_length_in_us, uint16_t mx_round_length, uint32_t period_time_us_plus)
{
    uint32_t mx_period_time_us;
    memset(&chirp_config + offsetof(Chirp_Config, mx_slot_length_in_us), 0, offsetof(Chirp_Config, lora_sf) - offsetof(Chirp_Config, mx_slot_length_in_us));
    chirp_config.mx_slot_length_in_us = mx_slot_length_in_us;
    #if MX_LBT_ACCESS
    chirp_config.mx_slot_length_in_us += chirp_config.lbt_detect_duration_us * CHANNEL_ALTER;
    #endif
    chirp_config.mx_slot_length = GPI_TICK_US_TO_FAST2(chirp_config.mx_slot_length_in_us);
    chirp_config.mx_round_length = mx_round_length;
    mx_period_time_us =  chirp_config.mx_slot_length_in_us * mx_round_length + period_time_us_plus;
    chirp_config.mx_period_time_s = (mx_period_time_us + 1000000 - 1) / 1000000;
}

void chirp_radio_config(uint8_t lora_spreading_factor, uint8_t lora_bandwidth, uint8_t lora_codingrate, uint8_t lora_preamble_length, int8_t tx_output_power, uint32_t lora_frequency)
{
    memset(&chirp_config + offsetof(Chirp_Config, lora_sf), 0, sizeof(chirp_config) - offsetof(Chirp_Config, lora_sf));
    chirp_config.lora_sf = lora_spreading_factor;
    chirp_config.lora_bw = lora_bandwidth;
    chirp_config.lora_cr = lora_codingrate;
    chirp_config.lora_plen = lora_preamble_length;
    chirp_config.lora_tx_pwr = tx_output_power;
    chirp_config.lora_freq = lora_frequency * 1e3; /* kHz -> Hz */
    gpi_radio_init();
	#if MX_LBT_ACCESS
    uint32_t symbol_time_us = SX1276GetSymbolTime(chirp_config.lora_sf, chirp_config.lora_bw);
    chirp_config.lbt_detect_duration_us = (6 * symbol_time_us >= LBT_DELAY_IN_US) ? 6 * symbol_time_us : LBT_DELAY_IN_US;
    #endif
}

/**
 * @description: To allocate payload among nodes according to the type of mixer (dissemination / collection)
 * @param mx_task: MX_DISSEMINATE / MX_COLLECT
 * @return: None
 */
void chirp_payload_distribution(Mixer_Task mx_task)
{
    uint8_t i;
    chirp_config.disem_copy = 0;
    if ((mx_task == MX_DISSEMINATE))
    {
        payload_distribution = (uint8_t *)malloc(chirp_config.mx_generation_size);
        /* Only the initiator has packets */
        for (i = 0; i < chirp_config.mx_generation_size; i++)
            payload_distribution[i] = 0;
    }
    else
    {
        /* Each node has a packet */
        assert_reset((chirp_config.mx_num_nodes == chirp_config.mx_generation_size));
        payload_distribution = (uint8_t *)malloc(chirp_config.mx_num_nodes);

        for (i = 0; i < chirp_config.mx_num_nodes; i++)
            payload_distribution[i] = i;

        if ((mx_task == MX_ARRANGE) || (mx_task == CHIRP_START) || (mx_task == CHIRP_CONNECTIVITY))
            chirp_config.disem_copy = 1;
    }
}

void chirp_write(uint8_t node_id, Chirp_Outl *chirp_outl)
{
    PRINTF("chirp_write:%d, %d\n", node_id, chirp_outl->round_max);

	uint8_t i;
    uint16_t k = 0;
    uint32_t flash_addr;

    if ((chirp_outl->task == MX_DISSEMINATE) || (chirp_outl->task == MX_COLLECT) || (chirp_outl->task == CHIRP_TOPO))
    {
        assert_reset(!(chirp_outl->file_chunk_len % sizeof(uint64_t)));
        assert_reset(!((chirp_outl->payload_len - DATA_HEADER_LENGTH) % sizeof(uint64_t)));
    }
    /* file data is read from flash on the other bank */
    uint32_t flash_data[chirp_outl->file_chunk_len / sizeof(uint32_t)];
    uint8_t file_data[chirp_outl->payload_len];
    memset(flash_data, 0, sizeof(flash_data));
    memset(file_data, 0, sizeof(file_data));
    memset(data, 0, DATA_HEADER_LENGTH);

    /* MX_DISSEMINATE / MX_COLLECT / CHIRP_TOPO: read file data from flash */
    if (((chirp_outl->task == MX_DISSEMINATE) || (chirp_outl->task == MX_COLLECT) || (chirp_outl->task == CHIRP_TOPO)))
    {
        memset(flash_data, 0, sizeof(flash_data));
        if ((chirp_outl->disem_file_index) && (chirp_outl->task == MX_DISSEMINATE))
        {
            if ((!chirp_outl->patch_update))
                flash_addr = FLASH_START_BANK2 + chirp_outl->file_chunk_len * (chirp_outl->disem_file_index - 1);
            else if ((chirp_outl->patch_update) && (!chirp_outl->patch_bank))
                flash_addr = FLASH_START_BANK1 + chirp_outl->patch_page * FLASH_PAGE + chirp_outl->file_chunk_len * (chirp_outl->disem_file_index - 1);
            else if ((chirp_outl->patch_update) && (chirp_outl->patch_bank))
                flash_addr = FLASH_START_BANK2 + chirp_outl->patch_page * FLASH_PAGE + chirp_outl->file_chunk_len * (chirp_outl->disem_file_index - 1);
        }
        else if (((chirp_outl->round > chirp_outl->round_setup) && (chirp_outl->round <= chirp_outl->round_max)))
        {
            if (chirp_outl->task == MX_COLLECT)
                flash_addr = chirp_outl->collect_addr_start + chirp_outl->file_chunk_len * (chirp_outl->round - chirp_outl->round_setup - 1);
            else if (chirp_outl->task == CHIRP_TOPO)
                flash_addr = TOPO_FLASH_ADDRESS + chirp_outl->file_chunk_len * (chirp_outl->round - chirp_outl->round_setup - 1);
        }

        uint16_t n;
        for (n = 0; n < chirp_outl->file_chunk_len / sizeof(uint32_t); n++)
            flash_data[n] = *(__IO uint32_t*)(flash_addr + n * sizeof(uint32_t));
    }

    /* All: config packet header including node_id, round No., current task, and proper content corresponded to task */
    data[0] = node_id;
    data[1] = chirp_outl->round >> 8;
    data[2] = chirp_outl->round;
    assert_reset(2 < ROUND_HEADER_LENGTH);
    /* write task index */
    data[DATA_HEADER_LENGTH - 1] = chirp_outl->task;
    k = ROUND_HEADER_LENGTH;

    switch (chirp_outl->task)
    {
        case CHIRP_START:
        {
            data[k++] = chirp_outl->round_max >> 8;
            data[k++] = chirp_outl->round_max;
            memcpy(file_data, data, DATA_HEADER_LENGTH);
            k = 0;
            file_data[DATA_HEADER_LENGTH + (k++)] = chirp_outl->start_year >> 8;
            file_data[DATA_HEADER_LENGTH + (k++)] = chirp_outl->start_year;
            file_data[DATA_HEADER_LENGTH + (k++)] = chirp_outl->start_month;
            file_data[DATA_HEADER_LENGTH + (k++)] = chirp_outl->start_date;
            file_data[DATA_HEADER_LENGTH + (k++)] = chirp_outl->start_hour;
            file_data[DATA_HEADER_LENGTH + (k++)] = chirp_outl->start_min;
            file_data[DATA_HEADER_LENGTH + (k++)] = chirp_outl->start_sec;

            file_data[DATA_HEADER_LENGTH + (k++)] = chirp_outl->end_year >> 8;
            file_data[DATA_HEADER_LENGTH + (k++)] = chirp_outl->end_year;
            file_data[DATA_HEADER_LENGTH + (k++)] = chirp_outl->end_month;
            file_data[DATA_HEADER_LENGTH + (k++)] = chirp_outl->end_date;
            file_data[DATA_HEADER_LENGTH + (k++)] = chirp_outl->end_hour;
            file_data[DATA_HEADER_LENGTH + (k++)] = chirp_outl->end_min;
            file_data[DATA_HEADER_LENGTH + (k++)] = chirp_outl->end_sec;
            file_data[DATA_HEADER_LENGTH + (k++)] = chirp_outl->flash_protection;
            file_data[DATA_HEADER_LENGTH + (k++)] = chirp_outl->version_hash >> 8;
            file_data[DATA_HEADER_LENGTH + (k++)] = chirp_outl->version_hash;
            k = 0;
            break;
        }
        case MX_DISSEMINATE:
        case MX_COLLECT:
        case CHIRP_TOPO:
        {
            if (chirp_outl->arrange_task == MX_DISSEMINATE)
            {
                /* initiator in dissemination setup: file size, patch config, and old file size (if patch) */
                if ((chirp_outl->disem_file_index == 0) && (!node_id))
                {
                    data[k++] = chirp_outl->disem_file_index >> 8;
                    data[k++] = chirp_outl->disem_file_index;
                    memcpy(file_data, data, DATA_HEADER_LENGTH);
                    k = 0;
                    if (chirp_outl->disem_flag)
                    {
                        file_data[DATA_HEADER_LENGTH + k++] = chirp_outl->firmware_size >> 24;
                        file_data[DATA_HEADER_LENGTH + k++] = chirp_outl->firmware_size >> 16;
                        file_data[DATA_HEADER_LENGTH + k++] = chirp_outl->firmware_size >> 8;
                        file_data[DATA_HEADER_LENGTH + k++] = chirp_outl->firmware_size;
                        file_data[DATA_HEADER_LENGTH + k++] = chirp_outl->patch_update;
                        file_data[DATA_HEADER_LENGTH + k++] = chirp_outl->patch_bank;
                        file_data[DATA_HEADER_LENGTH + k++] = chirp_outl->version_hash >> 8;
                        file_data[DATA_HEADER_LENGTH + k++] = chirp_outl->version_hash;
                        file_data[DATA_HEADER_LENGTH + k++] = chirp_outl->file_compression;
                        /* k = 9 */
                        memcpy(&(file_data[DATA_HEADER_LENGTH + 9]), &(chirp_outl->firmware_md5[0]), 16);
                        /* k = 25 */
                        if (chirp_outl->patch_update)
                        {
                            k = 28;
                            file_data[DATA_HEADER_LENGTH + k++] = chirp_outl->old_firmware_size >> 24;
                            file_data[DATA_HEADER_LENGTH + k++] = chirp_outl->old_firmware_size >> 16;
                            file_data[DATA_HEADER_LENGTH + k++] = chirp_outl->old_firmware_size >> 8;
                            file_data[DATA_HEADER_LENGTH + k++] = chirp_outl->old_firmware_size;
                            /* k = 32 */
                        }
                        k = 0;
                    }
                }
                /* if in dissemination / confirm session */
                else if (chirp_outl->disem_file_index)
                {
                    /* in dissemination, only initiator sends packets */
                    if (chirp_outl->disem_flag)
                    {
                        data[k++] = chirp_outl->disem_file_index >> 8;
                        data[k++] = chirp_outl->disem_file_index;
                    }
                    else
                    {
                        data[6] = chirp_outl->disem_flag_full_rank;
                        chirp_config.disem_copy = 0;
                    }
                }
            }
            /* collect setup: initiator sends start and end collect address in flash */
            else if ((chirp_outl->arrange_task == MX_COLLECT) && (chirp_outl->round <= chirp_outl->round_setup))
            {
                data[k++] = chirp_outl->round_max >> 8;
                data[k++] = chirp_outl->round_max;
                PRINTF("set99:%d\n", chirp_outl->round_max);

                memcpy(file_data, data, DATA_HEADER_LENGTH);
                if (!node_id)
                {
                    file_data[DATA_HEADER_LENGTH + 0] = chirp_outl->collect_addr_start >> 24;
                    file_data[DATA_HEADER_LENGTH + 1] = chirp_outl->collect_addr_start >> 16;
                    file_data[DATA_HEADER_LENGTH + 2] = chirp_outl->collect_addr_start >> 8;
                    file_data[DATA_HEADER_LENGTH + 3] = chirp_outl->collect_addr_start;
                    file_data[DATA_HEADER_LENGTH + 4] = chirp_outl->collect_addr_end >> 24;
                    file_data[DATA_HEADER_LENGTH + 5] = chirp_outl->collect_addr_end >> 16;
                    file_data[DATA_HEADER_LENGTH + 6] = chirp_outl->collect_addr_end >> 8;
                    file_data[DATA_HEADER_LENGTH + 7] = chirp_outl->collect_addr_end;
                }
            }
            break;
        }
        case CHIRP_CONNECTIVITY:
        {
            /* only initiator writes to the payload */
            data[k++] = chirp_outl->round_max >> 8;
            data[k++] = chirp_outl->round_max;
            memcpy(file_data, data, DATA_HEADER_LENGTH);
            file_data[DATA_HEADER_LENGTH] = chirp_outl->sf;
            file_data[DATA_HEADER_LENGTH + 1] = chirp_outl->freq >> 24;
            file_data[DATA_HEADER_LENGTH + 2] = chirp_outl->freq >> 16;
            file_data[DATA_HEADER_LENGTH + 3] = chirp_outl->freq >> 8;
            file_data[DATA_HEADER_LENGTH + 4] = chirp_outl->freq;
            file_data[DATA_HEADER_LENGTH + 5] = chirp_outl->tx_power;
            file_data[DATA_HEADER_LENGTH + 6] = chirp_outl->topo_payload_len;
            break;
        }
        case CHIRP_VERSION:
        {
            data[k++] = daemon_config.DAEMON_version >> 8;
            data[k++] = (uint8_t)(daemon_config.DAEMON_version);
            memcpy(file_data, data, DATA_HEADER_LENGTH);
            file_data[DATA_HEADER_LENGTH] = TOS_NODE_ID >> 16;
            file_data[DATA_HEADER_LENGTH + 1] = TOS_NODE_ID >> 8;
            file_data[DATA_HEADER_LENGTH + 2] = TOS_NODE_ID;
            k = ADC_GetVoltage();
            file_data[DATA_HEADER_LENGTH + 3] = k >> 8;
            file_data[DATA_HEADER_LENGTH + 4] = (uint8_t)k;
            k = 0;
            break;
        }
        case MX_ARRANGE:
        {
            data[1] = chirp_outl->default_slot_num >> 8;
            data[2] = chirp_outl->default_slot_num;
            if (chirp_outl->arrange_task == MX_DISSEMINATE)
            {
                data[0] = chirp_outl->dissem_back_sf;
                data[1] = chirp_outl->dissem_back_slot_num;
            }
            data[k++] = chirp_outl->default_sf;
            data[k++] = chirp_outl->default_payload_len;
            data[k++] = chirp_outl->arrange_task;
            data[ROUND_HEADER_LENGTH - 1] = chirp_outl->default_generate_size;
            memcpy(file_data, data, DATA_HEADER_LENGTH);
            k = 0;
            file_data[DATA_HEADER_LENGTH + k++] = chirp_outl->firmware_bitmap[0] >> 24;
            file_data[DATA_HEADER_LENGTH + k++] = chirp_outl->firmware_bitmap[0] >> 16;
            file_data[DATA_HEADER_LENGTH + k++] = chirp_outl->firmware_bitmap[0] >> 8;
            file_data[DATA_HEADER_LENGTH + k++] = chirp_outl->firmware_bitmap[0];
            file_data[DATA_HEADER_LENGTH + k++] = chirp_outl->default_tp;
            file_data[DATA_HEADER_LENGTH + k++] = chirp_outl->task_bitmap[0] >> 24;
            file_data[DATA_HEADER_LENGTH + k++] = chirp_outl->task_bitmap[0] >> 16;
            file_data[DATA_HEADER_LENGTH + k++] = chirp_outl->task_bitmap[0] >> 8;
            file_data[DATA_HEADER_LENGTH + k++] = chirp_outl->task_bitmap[0];
            k = 0;
            break;
        }
        default:
            break;
    }

    assert_reset((k <= DATA_HEADER_LENGTH));

    for (i = 0; i < chirp_config.mx_generation_size; i++)
    {
        if (payload_distribution[i] == node_id)
        {
            // data[ROUND_HEADER_LENGTH - 1] = i;
            // file_data[ROUND_HEADER_LENGTH - 1] = i;
            switch (chirp_outl->task)
            {
                case CHIRP_START:
                {
                    mixer_write(i, file_data, chirp_outl->payload_len);
                    break;
                }
                case MX_DISSEMINATE:
                {
                    if (!chirp_outl->disem_file_index)
                        mixer_write(i, file_data, chirp_outl->payload_len);
                    else
                    {
                        gpi_memcpy_dma((uint8_t *)(file_data), data, DATA_HEADER_LENGTH);
                        if (chirp_outl->disem_flag)
                            gpi_memcpy_dma((uint32_t *)(file_data + DATA_HEADER_LENGTH), flash_data + i * (chirp_outl->payload_len - DATA_HEADER_LENGTH) / sizeof(uint32_t), (chirp_outl->payload_len - DATA_HEADER_LENGTH));
                        mixer_write(i, (uint8_t *)file_data, chirp_outl->payload_len);
                        // PRINT_PACKET(file_data + DATA_HEADER_LENGTH, sizeof(file_data) - 8, 0);
                    }
                    break;
                }
                case MX_COLLECT:
                case CHIRP_TOPO:
                {
                    if (chirp_outl->round <= chirp_outl->round_setup)
                    {
                        if (chirp_outl->task == MX_COLLECT)
                            mixer_write(i, file_data, chirp_outl->payload_len);
                        else
                            mixer_write(i, data, MIN(sizeof(data), chirp_outl->payload_len));
                    }
                    else if (chirp_outl->round > chirp_outl->round_setup)
                    {
                        gpi_memcpy_dma((uint8_t *)(file_data), data, DATA_HEADER_LENGTH);
                        gpi_memcpy_dma((uint32_t *)(file_data + DATA_HEADER_LENGTH), flash_data, (chirp_outl->payload_len - DATA_HEADER_LENGTH));
                        mixer_write(i, (uint8_t *)file_data, chirp_outl->payload_len);
                    }
                    break;
                }
                case CHIRP_CONNECTIVITY:
                {
                    mixer_write(i, file_data, chirp_outl->payload_len);
                    break;
                }
                case CHIRP_VERSION:
                {
                    mixer_write(i, file_data, chirp_outl->payload_len);
                    // mixer_write(i, data, MIN(sizeof(data), chirp_outl->payload_len));
                    break;
                }
                case MX_ARRANGE:
                {
                    mixer_write(i, file_data, chirp_outl->payload_len);
                    break;
                }
                default:
                    break;
            }
        }
    }
}

uint8_t chirp_recv(uint8_t node_id, Chirp_Outl *chirp_outl)
{
	unsigned int		i;
    uint8_t round_inc = 0;
    uint8_t round_hash = 0;
    uint8_t k = 0;
    uint8_t packet_correct = 0;
    uint32_t mask_negative[chirp_config.my_column_mask.len];
    uint32_t firmware_bitmap_temp[DISSEM_BITMAP_32];
    uint16_t pending;
    if ((chirp_outl->task == MX_DISSEMINATE) || (chirp_outl->task == MX_COLLECT) || (chirp_outl->task == CHIRP_TOPO))
    {
        assert_reset(!((chirp_outl->payload_len - DATA_HEADER_LENGTH) % sizeof(uint64_t)));
        assert_reset((chirp_outl->payload_len > DATA_HEADER_LENGTH + 28));
    }
    uint32_t file_data[(chirp_outl->payload_len - DATA_HEADER_LENGTH) / sizeof(uint32_t)];
    uint8_t task_data[chirp_outl->payload_len - DATA_HEADER_LENGTH];
    uint8_t receive_payload[chirp_outl->payload_len];

    if (!node_id)
    {
        PRINTF("-----column_pending = %d-----\n", mx.request->my_column_pending);
        if ((chirp_outl->task == MX_COLLECT) && (chirp_outl->round > chirp_outl->round_setup))
            PRINTF("output from initiator (collect):\n");
        else if (chirp_outl->task == CHIRP_TOPO)
            PRINTF("output from initiator (topology):\n");
        else if (chirp_outl->task == CHIRP_VERSION)
            PRINTF("output from initiator (version):\n");
    }

    if  (chirp_outl->task == MX_DISSEMINATE)
    {
        if ((!node_id) && (!chirp_outl->disem_flag))
        {
            memcpy((uint32_t *)&(firmware_bitmap_temp[0]), (uint32_t *)&(chirp_outl->firmware_bitmap[0]), DISSEM_BITMAP_32 * sizeof(uint32_t));
            for (i = 0; i < chirp_config.my_column_mask.len; i++)
                mask_negative[i] = ~mx.request->mask[chirp_config.my_column_mask.pos + i];
            pending = mx_request_clear((uint32_t *)&(firmware_bitmap_temp[0]), (uint_fast_t *)&(mask_negative[0]), DISSEM_BITMAP_32 * sizeof(uint32_t));
            if (pending == 0)
                chirp_config.full_column = 0;
        }

        for (i = 0; i < chirp_config.mx_generation_size; i++)
        {
            void *p = mixer_read(i);
            if (NULL != p)
            {
                memcpy(receive_payload, p, chirp_config.matrix_payload_8.len);
                calu_payload_hash = Chirp_RSHash((uint8_t *)receive_payload, chirp_config.matrix_payload_8.len - 2);
                rece_hash = receive_payload[chirp_config.matrix_payload_8.len - 2] << 8 | receive_payload[chirp_config.matrix_payload_8.len - 1];
                PRINTF("rece_hash:%d, %x, %x, %d\n", i, rece_hash, (uint16_t)calu_payload_hash, chirp_config.matrix_payload_8.len);
                if (((uint16_t)calu_payload_hash == rece_hash) && (rece_hash))
                {
                    rece_dissem_index = (receive_payload[ROUND_HEADER_LENGTH] << 8 | receive_payload[ROUND_HEADER_LENGTH + 1]);
                    if (rece_dissem_index >= chirp_outl->disem_file_max + 1)
                        chirp_outl->disem_file_index++;
                    PRINTF("dissem_index:%d, %d\n", rece_dissem_index, chirp_outl->disem_file_index);
                    round_hash++;
                    PRINT_PACKET(p, DATA_HEADER_LENGTH, 1);
                }
            }
        }

        if (round_hash == chirp_config.mx_generation_size)
            chirp_config.full_rank = 1;
        else
        {
            mx.stat_counter.slot_decoded = 0;
        }
    }
	if (chirp_config.primitive != FLOODING)
	{
        free(mx.request);
    }

    if (((chirp_config.full_rank) && (chirp_outl->task == MX_DISSEMINATE)) || (chirp_outl->task != MX_DISSEMINATE))
    {
        for (i = 0; i < chirp_config.mx_generation_size; i++)
        {
            void *p = mixer_read(i);
            if (NULL != p)
            {
                memcpy(data, p, sizeof(data));
                if (chirp_outl->task != MX_DISSEMINATE)
                {
                    memcpy(receive_payload, p, chirp_config.matrix_payload_8.len);
                    calu_payload_hash = Chirp_RSHash((uint8_t *)receive_payload, chirp_config.matrix_payload_8.len - 2);
                    rece_hash = receive_payload[chirp_config.matrix_payload_8.len - 2] << 8 | receive_payload[chirp_config.matrix_payload_8.len - 1];
                    PRINTF("rece_hash:%d, %x, %x, %d\n", i, rece_hash, (uint16_t)calu_payload_hash, chirp_config.matrix_payload_8.len);
                }
                // PRINT_PACKET(data, DATA_HEADER_LENGTH, 1);
                packet_correct = 0;
                if ((data[DATA_HEADER_LENGTH - 1] == chirp_outl->task))
                {
                    if ((chirp_outl->task != MX_DISSEMINATE) && ((uint16_t)calu_payload_hash == rece_hash) && (rece_hash))
                        packet_correct = 1;
                    else if (chirp_outl->task == MX_DISSEMINATE)
                        packet_correct = 1;
                }
                if (packet_correct)
                {
                    /* print packet */
                    PRINT_PACKET(data, DATA_HEADER_LENGTH, 1);
                    if (chirp_outl->task != MX_ARRANGE)
                    {
                        /* check/adapt round number by message 0 (initiator) */
                        if ((0 == i) && (chirp_outl->payload_len >= 7))
                        {
                            Generic32	r;
                            r.u8_ll = data[2];
                            r.u8_lh = data[1];
                            r.u8_hl = 0;
                            r.u8_hh = 0;
                            if (chirp_outl->round != r.u32)
                                chirp_outl->round = r.u32;
                        }
                    }
                    if (chirp_outl->task != MX_DISSEMINATE)
                        k = ROUND_HEADER_LENGTH;
                    switch (chirp_outl->task)
                    {
                        case CHIRP_START:
                        {
                            if (node_id)
                            {
                                memcpy(task_data, (uint8_t *)(p + DATA_HEADER_LENGTH), sizeof(task_data));
                                chirp_outl->start_year = (task_data[0] << 8) | (task_data[1]);
                                chirp_outl->start_month = task_data[2];
                                chirp_outl->start_date = task_data[3];
                                chirp_outl->start_hour = task_data[4];
                                chirp_outl->start_min = task_data[5];
                                chirp_outl->start_sec = task_data[6];

                                chirp_outl->end_year = (task_data[7] << 8) | (task_data[8]);
                                chirp_outl->end_month = task_data[9];
                                chirp_outl->end_date = task_data[10];
                                chirp_outl->end_hour = task_data[11];
                                chirp_outl->end_min = task_data[12];
                                chirp_outl->end_sec = task_data[13];
                                chirp_outl->flash_protection = task_data[14];
                                chirp_outl->version_hash = (task_data[15] << 8) | (task_data[16]);
                                PRINTF("\t receive, START at %d-%d-%d, %d:%d:%d\n\tEnd at %d-%d-%d, %d:%d:%d\n, flash_protection:%d, v:%x\n", chirp_outl->start_year, chirp_outl->start_month, chirp_outl->start_date, chirp_outl->start_hour, chirp_outl->start_min, chirp_outl->start_sec, chirp_outl->end_year, chirp_outl->end_month, chirp_outl->end_date, chirp_outl->end_hour, chirp_outl->end_min, chirp_outl->end_sec, chirp_outl->flash_protection, chirp_outl->version_hash);
                            }
                            break;
                        }
                        case MX_DISSEMINATE:
                        {
                            /* MX_DISSEMINATE */
                            if (!chirp_outl->disem_file_index)
                            {
                                if (node_id)
                                {
                                    /* compare / increase the index */
                                    if (chirp_outl->disem_file_index == (data[ROUND_HEADER_LENGTH] << 8 | data[ROUND_HEADER_LENGTH + 1]))
                                    {
                                        if (i == 0)
                                        {
                                            memcpy(&(chirp_outl->disem_file_memory[0]), (uint8_t *)(p + DATA_HEADER_LENGTH), sizeof(file_data));

                                            memcpy(data, &(chirp_outl->disem_file_memory[0]), DATA_HEADER_LENGTH + 1);
                                            chirp_outl->firmware_size = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | (data[3]);
                                            chirp_outl->patch_update = data[4];
                                            chirp_outl->patch_bank = data[5];
                                            chirp_outl->disem_file_max = (chirp_outl->firmware_size + chirp_outl->file_chunk_len - 1) / chirp_outl->file_chunk_len  + 1;
                                            chirp_outl->version_hash = (data[6] << 8) | (data[7]);
                                            chirp_outl->file_compression = data[8];
                                            PRINTF("version_hash:%x, %x, %x\n", chirp_outl->version_hash, data[6], data[7]);
                                            PRINTF("MX_DISSEMINATE: %lu, %d, %d, %d, %lu\n", chirp_outl->firmware_size, chirp_outl->patch_update, chirp_outl->disem_file_max, chirp_outl->file_chunk_len, chirp_outl->file_compression);

                                            memcpy(&(chirp_outl->firmware_md5[0]), (uint8_t *)(p + 17), 16);
                                            /* update whole firmware */
                                            if ((!chirp_outl->patch_update) && (i == 0))
                                            {
                                                menu_preSend(1);
                                                file_data[0] = chirp_outl->firmware_size;
                                                // PRINTF("whole firmware_size:%lu\n", chirp_outl->firmware_size);
                                                FLASH_If_Write(FIRMWARE_FLASH_ADDRESS_2, (uint32_t *)file_data, 2);
                                            }
                                            /* patch firmware */
                                            else if ((chirp_outl->patch_update) && (i == 0))
                                            {
                                                memcpy(data, &(chirp_outl->disem_file_memory[7]), 4);
                                                k = 0;

                                                chirp_outl->old_firmware_size = (data[k++] << 24) | (data[k++] << 16) | (data[k++] << 8) | (data[k++]);
                                                k = 0;
                                                chirp_outl->patch_page = menu_pre_patch(chirp_outl->patch_bank, chirp_outl->old_firmware_size, chirp_outl->firmware_size);
                                                PRINTF("patch:%lu, %d\n", chirp_outl->old_firmware_size, chirp_outl->patch_page);
                                            }
                                        }
                                        if (i == chirp_outl->generation_size - 1)
                                        {
                                            chirp_outl->disem_file_index++;
                                            chirp_outl->disem_file_index_stay = 0;
                                        }
                                    }
                                }
                            }
                            else if (chirp_outl->disem_file_index)
                            {
                                if (node_id)
                                {
                                    /* compare / increase the index */
                                    if (chirp_outl->disem_file_index == (data[ROUND_HEADER_LENGTH] << 8 | data[ROUND_HEADER_LENGTH + 1]))
                                    {
                                        PRINTF("write\n");
                                        memcpy(&(chirp_outl->disem_file_memory[i * sizeof(file_data) / sizeof(uint32_t)]), (uint8_t *)(p + DATA_HEADER_LENGTH), sizeof(file_data));

                                        if (i == chirp_outl->generation_size - 1)
                                        {
                                            if (!chirp_outl->patch_update)
                                            {
                                                FLASH_If_Write(FLASH_START_BANK2 + (chirp_outl->disem_file_index - 1) * chirp_outl->file_chunk_len, (uint32_t *)(chirp_outl->disem_file_memory), chirp_outl->file_chunk_len / sizeof(uint32_t));
                                            }
                                            else if (chirp_outl->patch_update)
                                            {
                                                if (!chirp_outl->patch_bank)
                                                {
                                                    FLASH_If_Write(FLASH_START_BANK1 + chirp_outl->patch_page * FLASH_PAGE + (chirp_outl->disem_file_index - 1) * chirp_outl->file_chunk_len, (uint32_t *)(chirp_outl->disem_file_memory), chirp_outl->file_chunk_len / sizeof(uint32_t));
                                                }
                                                else
                                                {
                                                    FLASH_If_Write(FLASH_START_BANK2 + chirp_outl->patch_page * FLASH_PAGE + (chirp_outl->disem_file_index - 1) * chirp_outl->file_chunk_len, (uint32_t *)(chirp_outl->disem_file_memory), chirp_outl->file_chunk_len / sizeof(uint32_t));
                                                }
                                            }
                                            chirp_outl->disem_file_index++;
                                            chirp_outl->disem_file_index_stay = 0;
                                        }
                                    }
                                }
                            }
                            break;
                        }
                        case MX_COLLECT:
                        case CHIRP_TOPO:
                        {
                            /* reconfig chirp_outl (except the initiator) */
                            if (chirp_outl->round <= chirp_outl->round_setup)
                            {
                                /* MX_COLLECT */
                                /* only initiator indicates the file information */
                                if ((chirp_outl->task == MX_COLLECT) && (node_id) && (i == 0))
                                {
                                    PRINTF("MX_COLLECT\n");
                                    chirp_outl->round_max = (data[k++] << 8) | (data[k++]);
                                    memcpy(data, p + DATA_HEADER_LENGTH, DATA_HEADER_LENGTH);
                                    PRINTF("col_max:%d\n", chirp_outl->round_max);

                                    chirp_outl->collect_addr_start = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | (data[3]);
                                    chirp_outl->collect_addr_end = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | (data[7]);
                                    chirp_outl->collect_length = ((chirp_outl->collect_addr_end - chirp_outl->collect_addr_start + sizeof(uint64_t) - 1) / sizeof(uint64_t)) * sizeof(uint64_t);
                                    PRINTF("addr:%lu, %lu, %lu\n", chirp_outl->collect_addr_start, chirp_outl->collect_addr_end, chirp_outl->collect_length);
                                }
                            }
                            /* round > setup_round */
                            else if ((chirp_outl->round > chirp_outl->round_setup) && (chirp_outl->round <= chirp_outl->round_max))
                            {
                                if ((chirp_outl->task == MX_COLLECT) || (chirp_outl->task == CHIRP_TOPO))
                                {
                                    memcpy(file_data, p + DATA_HEADER_LENGTH, sizeof(file_data));
                                    PRINT_PACKET(file_data, sizeof(file_data), 0);
                                }
                            }
                            break;
                        }
                        case CHIRP_CONNECTIVITY:
                        {
                            if (node_id)
                            {
                                PRINTF("CHIRP_CONNECTIVITY\n");

                                memcpy(task_data, (uint8_t *)(p + DATA_HEADER_LENGTH), sizeof(task_data));
                                chirp_outl->sf = task_data[0];
                                chirp_outl->freq = (task_data[1] << 24) | (task_data[2] << 16) | (task_data[3] << 8) | (task_data[4]);
                                chirp_outl->tx_power = task_data[5];
                                chirp_outl->topo_payload_len = task_data[6];
                            }
                            break;
                        }
                        case CHIRP_VERSION:
                        {
                            memcpy(data, p + DATA_HEADER_LENGTH, chirp_outl->payload_len - DATA_HEADER_LENGTH);
                            PRINT_PACKET(data, chirp_outl->payload_len - DATA_HEADER_LENGTH, 0);
                            break;
                        }
                        case MX_ARRANGE:
                        {
                            PRINTF("MX_ARRANGE\n");
                            chirp_outl->arrange_task = data[6];
                            if (chirp_outl->arrange_task == MX_DISSEMINATE)
                            {
                                chirp_outl->dissem_back_sf = data[0];
                                chirp_outl->dissem_back_slot_num = data[1];
                                chirp_outl->default_slot_num = data[2];
                            }
                            else
                                chirp_outl->default_slot_num = data[1] << 8 | data[2];
                            /* reconfig chirp_outl (except the initiator) */
                            chirp_outl->default_sf = data[k++];
                            // chirp_outl->round_max = data[k++];
                            chirp_outl->default_payload_len = data[k++];
                            // chirp_outl->round_max = (data[k++] << 8) | (data[k++]);
                            // chirp_outl->arrange_task = data[k++];
                            chirp_outl->default_generate_size = data[ROUND_HEADER_LENGTH - 1];
                            if (node_id)
                            {
                                memcpy(task_data, (uint8_t *)(p + DATA_HEADER_LENGTH), sizeof(task_data));
                                chirp_outl->firmware_bitmap[0] = (task_data[0] << 24) | (task_data[1] << 16) | (task_data[2] << 8) | (task_data[3]);
                                chirp_outl->default_tp = task_data[4];
                                chirp_outl->task_bitmap[0] = (task_data[5] << 24) | (task_data[6] << 16) | (task_data[7] << 8) | (task_data[8]);
                            }
                            break;
                        }
                        default:
                            break;
                    }
                    round_inc++;
                    PRINTF("roundinc %d\n", round_inc);
                }
            }
        }
    }
	if (chirp_config.primitive != FLOODING)
    {
    free(mx.matrix[0]);
    mx.matrix[0] = NULL;
    }

    /* have received at least a packet */
    if (((chirp_outl->task == MX_COLLECT) ||(chirp_outl->task == CHIRP_TOPO) ||(chirp_outl->task == CHIRP_VERSION)))
    {
        if (round_inc > 1)
        {
            chirp_outl->round++;
            return 1;
        }
        else
        {
            return 0;
        }

    }
    else
    {
        if (chirp_outl->task == MX_DISSEMINATE)
        {
            if (round_hash)
            {
                chirp_outl->round++;
                return 1;
            }
            else
            {
                return 0;
            }
        }
        else
        {
            if (round_inc)
            {
                chirp_outl->round++;
                return 1;
            }
            /* not received any packet */
            else
            {
                return 0;
            }
        }
    }
}

uint8_t chirp_round(uint8_t node_id, Chirp_Outl *chirp_outl)
{
    gpi_watchdog_periodic();
	// Gpi_Fast_Tick_Native deadline;
    Gpi_Fast_Tick_Extended deadline;
    Gpi_Fast_Tick_Native update_period = GPI_TICK_MS_TO_FAST2(((chirp_config.mx_period_time_s * 1000) / 1) - chirp_config.mx_round_length * (chirp_config.mx_slot_length_in_us / 1000));

    uint8_t failed_round = 0;

    chirp_outl->round = 1;

    /* set current state as mixer */
	chirp_isr.state = ISR_MIXER;

	// deadline = gpi_tick_fast_native() + 2 * chirp_config.mx_slot_length;
	// deadline = gpi_tick_fast_native();
	deadline = gpi_tick_fast_extended();

    clear_data();

    chirp_config.task = chirp_outl->task;

	if (chirp_config.primitive != FLOODING)
        chirp_config.packet_hash = DISC_HEADER;
    else
        chirp_config.packet_hash = FLOODING_HEADER;

	while (1)
	{
        PRINTF("round:%d, %d\n", chirp_outl->round, chirp_outl->round_max);

        gpi_radio_init();

        /* init mixer */
        mixer_init(node_id);
        #if ENERGEST_CONF_ON
            ENERGEST_ON(ENERGEST_TYPE_CPU);
        #endif
        /* except these two task that all nodes need to upload data, others only initiator transmit data */
        if (chirp_config.primitive == DISSEMINATION)
        {
            if (!node_id)
                chirp_write(node_id, chirp_outl);
        }
        else
            chirp_write(node_id, chirp_outl);

		/* arm mixer, node 0 = initiator
		start first round with infinite scan
		-> nodes join next available round, does not require simultaneous boot-up */
        mixer_arm(((!node_id) ? MX_ARM_INITIATOR : 0) | ((1 == 0) ? MX_ARM_INFINITE_SCAN : 0));

		/* delay initiator a bit
		-> increase probability that all nodes are ready when initiator starts the round
		-> avoid problems in view of limited deadline accuracy */
		if (!node_id)
        {
            /* because other nodes need time to erase pages */
            if ((chirp_outl->task == MX_DISSEMINATE) && (chirp_outl->round == 1))
                // deadline += GPI_TICK_MS_TO_FAST2(8000);
                deadline += (Gpi_Fast_Tick_Extended)1 * chirp_config.mx_slot_length;
            else
                deadline += (Gpi_Fast_Tick_Extended)1 * chirp_config.mx_slot_length;
        }

		/* start when deadline reached
		ATTENTION: don't delay after the polling loop (-> print before) */
		// while (gpi_tick_compare_fast_native(gpi_tick_fast_native(), deadline) < 0);
        deadline_dog = (chirp_config.mx_period_time_s + 60 - 1) / DOG_PERIOD + 60 / DOG_PERIOD;
        count_dog = 0;
        PRINTF("dg:%d\n", deadline_dog);
        #if MX_LBT_ACCESS
            lbt_check_time();
            chirp_isr.state = ISR_MIXER;
            if (chirp_config.primitive != FLOODING)
            {
            chirp_config.lbt_channel_primary = (chirp_config.lbt_channel_primary + 1) % LBT_CHANNEL_NUM;
            if ((!chirp_outl->disem_flag) && (chirp_outl->task == MX_DISSEMINATE) && (chirp_outl->round >= 2))
            {
                chirp_config.lbt_channel_primary = (chirp_config.lbt_channel_primary + LBT_CHANNEL_NUM - 1) % LBT_CHANNEL_NUM;
            }
            }
            SX1276SetChannel(chirp_config.lora_freq + chirp_config.lbt_channel_primary * CHANNEL_STEP);
            PRINTF("-------lbt_channel_primary:%d\n", chirp_config.lbt_channel_primary);
        #endif
		while (gpi_tick_compare_fast_extended(gpi_tick_fast_extended(), deadline) < 0);
        #if ENERGEST_CONF_ON
            ENERGEST_OFF(ENERGEST_TYPE_CPU);
        #endif
        /* used in mixer_write, and revalue before mixer round */
        chirp_config.full_rank = 0;
        chirp_config.full_column = UINT8_MAX;
        rece_dissem_index = UINT16_MAX;

		deadline = mixer_start();

        if (chirp_config.primitive != FLOODING)
        {
        if (chirp_outl->task != MX_DISSEMINATE)
        {
            Stats_value(RX_STATS, (uint32_t)gpi_tick_hybrid_to_us(energest_type_time(ENERGEST_TYPE_LISTEN)));
            Stats_value(TX_STATS, (uint32_t)gpi_tick_hybrid_to_us(energest_type_time(ENERGEST_TYPE_TRANSMIT)));
            Stats_value_debug(ENERGEST_TYPE_CPU, energest_type_time(ENERGEST_TYPE_CPU));
            Stats_value_debug(ENERGEST_TYPE_LPM, energest_type_time(ENERGEST_TYPE_LPM) - energest_type_time(ENERGEST_TYPE_TRANSMIT) - energest_type_time(ENERGEST_TYPE_LISTEN));
            Stats_value_debug(ENERGEST_TYPE_TRANSMIT, energest_type_time(ENERGEST_TYPE_TRANSMIT));
            Stats_value_debug(ENERGEST_TYPE_LISTEN, energest_type_time(ENERGEST_TYPE_LISTEN));
        }

        if (!chirp_recv(node_id, chirp_outl))
        {
            mx.stat_counter.slot_decoded = 0;
            /* If in arrange state, none packet has been received, break loop */
            if (chirp_outl->task == MX_ARRANGE)
            {
                return 0;
            }
            else
            {
                failed_round++;
                PRINTF("failed:%d\n", failed_round);
                if (chirp_outl->task == MX_DISSEMINATE)
                {
                    if (failed_round >= 2)
                    {
                        Stats_value(SLOT_STATS, mx.stat_counter.slot_decoded);
                        Stats_to_Flash(chirp_outl->task);
                        if (((node_id) && (chirp_outl->disem_file_index >= chirp_outl->disem_file_max)) || ((!node_id) && (chirp_outl->disem_file_index >= chirp_outl->disem_file_max + 1)))
                            return 1;
                        else
                        {
                            return 0;
                        }
                    }
                }
                else if ((failed_round >= 1) && (chirp_outl->task != MX_DISSEMINATE))
                {
                    Stats_value(SLOT_STATS, mx.stat_counter.slot_decoded);
                    Stats_to_Flash(chirp_outl->task);
                    return 0;
                }
            }
        }
        else
            failed_round = 0;

        if (chirp_outl->task != MX_DISSEMINATE)
            Stats_value(SLOT_STATS, mx.stat_counter.slot_decoded);

		while (gpi_tick_compare_fast_extended(gpi_tick_fast_extended(), deadline) < 0);

        /* in dissemination, nodes have to send back the results, so switch the configuration between rounds */
        if (chirp_outl->task == MX_DISSEMINATE)
        {
                chirp_outl->disem_file_index_stay++;
                PRINTF("disem_file_index_stay:%d\n", chirp_outl->disem_file_index_stay);
                // if (chirp_outl->disem_file_index_stay > 5 * 2)
                // {
                //     if (((node_id) && (chirp_outl->disem_file_index >= chirp_outl->disem_file_max)) || ((!node_id) && (chirp_outl->disem_file_index >= chirp_outl->disem_file_max + 1)))
                //     {
                //         PRINTF("full\n");
                //         return 1;
                //     }
                //     else
                //     {
                //         return 0;
                //     }
                // }
                /* dissemination session: disseminate files to all nodes */
                if (!chirp_outl->disem_flag)
                {
                    Stats_value_debug(ENERGEST_TYPE_CPU, energest_type_time(ENERGEST_TYPE_CPU));
                    Stats_value_debug(ENERGEST_TYPE_LPM, energest_type_time(ENERGEST_TYPE_LPM) - energest_type_time(ENERGEST_TYPE_TRANSMIT) - energest_type_time(ENERGEST_TYPE_LISTEN));
                    Stats_value_debug(ENERGEST_TYPE_TRANSMIT, energest_type_time(ENERGEST_TYPE_TRANSMIT));
                    Stats_value_debug(ENERGEST_TYPE_LISTEN, energest_type_time(ENERGEST_TYPE_LISTEN));
                    free(payload_distribution);
                    chirp_radio_config(chirp_outl->default_sf, 7, 1, 8, chirp_outl->default_tp, chirp_outl->default_freq);
                    /* If now is confirm, the initiator collect all nodes information about whether they are full rank last round, if so, then send the next file chunk, file index++, else do not increase file index */
                    if ((!node_id) && (chirp_config.full_column == 0))
                    {
                        chirp_outl->disem_file_index++;
                        chirp_outl->disem_file_index_stay = 0;
                        PRINTF("full receive\n");
                    }
                    PRINTF("next: disem_flag: %d, %d\n", chirp_outl->disem_file_index, chirp_outl->disem_file_max);
                    chirp_packet_config(chirp_outl->num_nodes, chirp_outl->generation_size, chirp_outl->payload_len + HASH_TAIL, DISSEMINATION);
                    chirp_outl->packet_time = SX1276GetPacketTime(chirp_config.lora_sf, chirp_config.lora_bw, 1, 0, 8, chirp_config.phy_payload_size + HASH_TAIL_CODE);
                    chirp_slot_config(chirp_outl->packet_time + 100000, chirp_outl->default_slot_num, 2000000);
                    chirp_payload_distribution(chirp_outl->task);
                    chirp_outl->disem_flag = 1;
                }
                /* confirm session: collect all nodes condition (if full rank in last mixer round) */
                else
                {
                    Stats_value(RX_STATS, (uint32_t)gpi_tick_hybrid_to_us(energest_type_time(ENERGEST_TYPE_LISTEN)));
                    Stats_value(TX_STATS, (uint32_t)gpi_tick_hybrid_to_us(energest_type_time(ENERGEST_TYPE_TRANSMIT)));
                    Stats_value(SLOT_STATS, mx.stat_counter.slot_decoded);

                    Stats_value_debug(ENERGEST_TYPE_CPU, energest_type_time(ENERGEST_TYPE_CPU));
                    Stats_value_debug(ENERGEST_TYPE_LPM, energest_type_time(ENERGEST_TYPE_LPM) - energest_type_time(ENERGEST_TYPE_TRANSMIT) - energest_type_time(ENERGEST_TYPE_LISTEN));
                    Stats_value_debug(ENERGEST_TYPE_TRANSMIT, energest_type_time(ENERGEST_TYPE_TRANSMIT));
                    Stats_value_debug(ENERGEST_TYPE_LISTEN, energest_type_time(ENERGEST_TYPE_LISTEN));
                    PRINTF("ENERGEST_TYPE_LPM:%lu\n", (uint32_t)gpi_tick_hybrid_to_us(energest_type_time(ENERGEST_TYPE_LPM)));

                    free(payload_distribution);
                    if (chirp_outl->dissem_back_sf)
                        chirp_radio_config(chirp_outl->dissem_back_sf, 7, 1, 8, 14, chirp_outl->default_freq);
                    PRINTF("next: collect disem_flag: %d, %d\n", chirp_outl->disem_file_index, chirp_outl->disem_file_max);
                    // chirp_outl->payload_len = DATA_HEADER_LENGTH;
                    chirp_packet_config(chirp_outl->num_nodes, chirp_outl->num_nodes, DATA_HEADER_LENGTH + HASH_TAIL, COLLECTION);
                    chirp_outl->packet_time = SX1276GetPacketTime(chirp_config.lora_sf, chirp_config.lora_bw, 1, 0, 8, chirp_config.phy_payload_size + HASH_TAIL_CODE);
                    if (chirp_outl->dissem_back_slot_num == 0)
                        chirp_outl->dissem_back_slot_num = chirp_outl->num_nodes * 8;
                    chirp_slot_config(chirp_outl->packet_time + 100000, chirp_outl->dissem_back_slot_num, 1500000);
                    chirp_payload_distribution(MX_COLLECT);
                    chirp_outl->disem_flag = 0;
                    /* in confirm, all nodes sends packets */
                    PRINTF("rece_dissem_index:%x\n", rece_dissem_index);

                    if (chirp_outl->disem_file_index > rece_dissem_index)
                    {
                        PRINTF("full disem_copy\n");
                        chirp_config.disem_copy = 1;
                        chirp_outl->disem_flag_full_rank = mx.stat_counter.slot_full_rank;
                    }
                }
        }

        /* once the round num expired, quit loop */
        if ((chirp_outl->round > chirp_outl->round_max) && (chirp_outl->task != MX_DISSEMINATE))
        {
            Stats_to_Flash(chirp_outl->task);
            return 1;
        }
        /* in collection, break when file is done */
        else if ((chirp_outl->task == MX_DISSEMINATE) && (!chirp_outl->disem_flag))
        {
            if ((node_id) && (chirp_outl->disem_file_index >= chirp_outl->disem_file_max + 2))
                return 1;
            else if ((!node_id) && (chirp_outl->disem_file_index >= chirp_outl->disem_file_max + 1))
                return 1;
        }

        deadline += (Gpi_Fast_Tick_Extended)update_period;
        }
        else
        {
            Gpi_Fast_Tick_Native resync_plus =  GPI_TICK_MS_TO_FAST2(((chirp_config.mx_slot_length_in_us * 5 / 2) * (chirp_config.mx_round_length / 2 - 1) / 1000) - chirp_config.mx_round_length * (chirp_config.mx_slot_length_in_us / 1000));
            if (!chirp_config.glossy_task)
                deadline += (Gpi_Fast_Tick_Extended)(update_period - resync_plus);
            else
                deadline += (Gpi_Fast_Tick_Extended)(update_period);
            while (gpi_tick_compare_fast_extended(gpi_tick_fast_extended(), deadline) < 0);
            return chirp_config.glossy_task;
        }
	}
}

//**************************************************************************************************
//**************************************************************************************************
