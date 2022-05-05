//**************************************************************************************************
//**** Includes ************************************************************************************

#include "mixer_internal.h"
#if USE_FOR_CHIRPBOX
	#include "chirp_internal.h"
#endif
#if USE_FOR_LORAWAN
	#include "lorawan_internal.h"
#endif
#include "mixer_config.h"

#include "gpi/olf.h"
#include GPI_PLATFORM_PATH(radio.h)

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

static uint16_t    rece_dissem_index;
uint16_t    calu_payload_hash, rece_hash;
//**************************************************************************************************
//***** Global Variables ***************************************************************************

uint8_t                     uartRxBuffer[128];

ChirpBox_Task                  task;

/* indicate whether uart finished receiving */
extern volatile uint8_t     uart_read_done;

extern UART_HandleTypeDef   huart2;

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

void chirp_write(uint8_t node_id, Chirp_Outl *chirp_outl)
{
    PRINTF("chirp_write:%d, %d\n", node_id, chirp_outl->round_max);

	uint8_t i;
    uint16_t k = 0;
    uint32_t flash_addr;

    if ((chirp_outl->task == CB_DISSEMINATE) || (chirp_outl->task == CB_COLLECT))
    {
        assert_reset(!(chirp_outl->file_chunk_len % sizeof(uint64_t)));
        assert_reset(!((chirp_outl->payload_len - DATA_HEADER_LENGTH) % sizeof(uint64_t)));
    }
    /* file data is read from flash on the other bank */
    uint32_t flash_data[chirp_outl->file_chunk_len / sizeof(uint32_t)];
    #if LORADISC
        uint8_t loradisc_data_length;
        uint8_t *loradisc_data;
        if (loradisc_config.primitive == FLOODING)
            loradisc_data_length = chirp_outl->payload_len + FLOODING_SURPLUS_LENGTH;
        else
            loradisc_data_length = chirp_outl->payload_len;
        loradisc_data = (uint8_t *)malloc(loradisc_data_length);
    #endif
    memset(flash_data, 0, sizeof(flash_data));
    memset(data, 0, DATA_HEADER_LENGTH);

    /* CB_DISSEMINATE / CB_COLLECT: read file data from flash */
    if (((chirp_outl->task == CB_DISSEMINATE) || (chirp_outl->task == CB_COLLECT)))
    {
        memset(flash_data, 0, sizeof(flash_data));
        if ((chirp_outl->disem_file_index) && (chirp_outl->task == CB_DISSEMINATE))
        {
            if ((!chirp_outl->patch_update))
                flash_addr = FLASH_START_BANK2 + chirp_outl->file_chunk_len * (chirp_outl->disem_file_index - 1);
            else if ((chirp_outl->patch_update) && (!chirp_outl->patch_bank))
                flash_addr = FLASH_START_BANK1 + chirp_outl->patch_page * FLASH_PAGE + chirp_outl->file_chunk_len * (chirp_outl->disem_file_index - 1);
            else if ((chirp_outl->patch_update) && (chirp_outl->patch_bank))
                flash_addr = FLASH_START_BANK2 + chirp_outl->patch_page * FLASH_PAGE + chirp_outl->file_chunk_len * (chirp_outl->disem_file_index - 1);
        }
        else if (((chirp_outl->task == CB_COLLECT) && (chirp_outl->round <= chirp_outl->round_max)))
        {
            flash_addr = chirp_outl->collect_addr_start + chirp_outl->file_chunk_len * (chirp_outl->round - 1);
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
        case CB_START:
        {
            k = 0;
            loradisc_data[k++] = chirp_outl->flash_protection;
            loradisc_data[k++] = chirp_outl->version_hash >> 8;
            loradisc_data[k++] = chirp_outl->version_hash;

            loradisc_data[k++] = chirp_outl->start_year >> 8;
            loradisc_data[k++] = chirp_outl->start_year;
            loradisc_data[k++] = chirp_outl->start_month;
            loradisc_data[k++] = chirp_outl->start_date;
            loradisc_data[k++] = chirp_outl->start_hour;
            loradisc_data[k++] = chirp_outl->start_min;
            loradisc_data[k++] = chirp_outl->start_sec;

            loradisc_data[k++] = chirp_outl->end_year >> 8;
            loradisc_data[k++] = chirp_outl->end_year;
            loradisc_data[k++] = chirp_outl->end_month;
            loradisc_data[k++] = chirp_outl->end_date;
            loradisc_data[k++] = chirp_outl->end_hour;
            loradisc_data[k++] = chirp_outl->end_min;
            loradisc_data[k++] = chirp_outl->end_sec;

            loradisc_data[k++] = chirp_outl->firmware_bitmap[0] >> 24;
            loradisc_data[k++] = chirp_outl->firmware_bitmap[0] >> 16;
            loradisc_data[k++] = chirp_outl->firmware_bitmap[0] >> 8;
            loradisc_data[k++] = chirp_outl->firmware_bitmap[0];
            break;
        }
        case CB_DISSEMINATE:
        {
            /* initiator in dissemination setup: file size, patch config, and old file size (if patch) */
            /* in dissemination, only initiator sends packets */
            if (chirp_outl->disem_flag)
            {
                data[k++] = chirp_outl->disem_file_index >> 8;
                data[k++] = chirp_outl->disem_file_index;
            }
            else
            {
                data[6] = chirp_outl->disem_flag_full_rank;
            }
            break;
        }
        case CB_CONNECTIVITY:
        {
            k = 0;
            loradisc_data[k++] = chirp_outl->sf_bitmap;
            loradisc_data[k++] = chirp_outl->tx_power;
            loradisc_data[k++] = chirp_outl->topo_payload_len;
            loradisc_data[k++] = chirp_outl->freq >> 24;
            loradisc_data[k++] = chirp_outl->freq >> 16;
            loradisc_data[k++] = chirp_outl->freq >> 8;
            loradisc_data[k++] = chirp_outl->freq;
            break;
        }
        case CB_VERSION:
        {
            data[k++] = daemon_config.DAEMON_version >> 8;
            data[k++] = (uint8_t)(daemon_config.DAEMON_version);
            memcpy(loradisc_data, data, DATA_HEADER_LENGTH);
            loradisc_data[DATA_HEADER_LENGTH] = TOS_NODE_ID >> 16;
            loradisc_data[DATA_HEADER_LENGTH + 1] = TOS_NODE_ID >> 8;
            loradisc_data[DATA_HEADER_LENGTH + 2] = TOS_NODE_ID;
            k = ADC_GetVoltage();
            loradisc_data[DATA_HEADER_LENGTH + 3] = k >> 8;
            loradisc_data[DATA_HEADER_LENGTH + 4] = (uint8_t)k;
            break;
        }
        case CB_GLOSSY_ARRANGE:
        {
            /* dissem: payload_len = 35; collect: payload_len = 13 */
            k = 0;
            loradisc_data[k++] = chirp_outl->task_bitmap[0] >> 24;
            loradisc_data[k++] = chirp_outl->task_bitmap[0] >> 16;
            loradisc_data[k++] = chirp_outl->task_bitmap[0] >> 8;
            loradisc_data[k++] = chirp_outl->task_bitmap[0];
            if (chirp_outl->arrange_task == CB_DISSEMINATE)
            {
                loradisc_data[k++] = chirp_outl->dissem_back_sf;
                loradisc_data[k++] = chirp_outl->dissem_back_slot_num;
                loradisc_data[k++] = chirp_outl->default_payload_len;
                loradisc_data[k++] = chirp_outl->default_generate_size;

                loradisc_data[k++] = chirp_outl->patch_update;
                loradisc_data[k++] = chirp_outl->patch_bank;
                loradisc_data[k++] = chirp_outl->file_compression;
                loradisc_data[k++] = chirp_outl->old_firmware_size >> 24;
                loradisc_data[k++] = chirp_outl->old_firmware_size >> 16;
                loradisc_data[k++] = chirp_outl->old_firmware_size >> 8;
                loradisc_data[k++] = chirp_outl->old_firmware_size;
                loradisc_data[k++] = chirp_outl->firmware_size >> 24;
                loradisc_data[k++] = chirp_outl->firmware_size >> 16;
                loradisc_data[k++] = chirp_outl->firmware_size >> 8;
                loradisc_data[k++] = chirp_outl->firmware_size;
                memcpy(&(loradisc_data[k++]), &(chirp_outl->firmware_md5[0]), 16);
            }
            else if (chirp_outl->arrange_task == CB_COLLECT)
            {
                loradisc_data[k++] = chirp_outl->default_payload_len;
                PRINTF("collect_addr_start:%x, %x\n", chirp_outl->collect_addr_start, chirp_outl->collect_addr_end);
                loradisc_data[k++] = chirp_outl->collect_addr_start >> 24;
                loradisc_data[k++] = chirp_outl->collect_addr_start >> 16;
                loradisc_data[k++] = chirp_outl->collect_addr_start >> 8;
                loradisc_data[k++] = chirp_outl->collect_addr_start;
                loradisc_data[k++] = chirp_outl->collect_addr_end >> 24;
                loradisc_data[k++] = chirp_outl->collect_addr_end >> 16;
                loradisc_data[k++] = chirp_outl->collect_addr_end >> 8;
                loradisc_data[k++] = chirp_outl->collect_addr_end;
            }
            break;
        }
        case CB_GLOSSY:
        {
            k = 0;
            loradisc_data[k++] = chirp_outl->arrange_task << 4 | chirp_outl->default_sf;
            loradisc_data[k++] = chirp_outl->default_tp;
            loradisc_data[k++] = chirp_outl->default_slot_num;
            break;
        }
        default:
            break;
    }

    #if LORADISC
        if (loradisc_config.primitive == FLOODING)
            loradisc_write(k, loradisc_data);
    #endif

    for (i = 0; i < loradisc_config.mx_generation_size; i++)
    {
        if (payload_distribution[i] == node_id)
        {
            switch (chirp_outl->task)
            {
                case CB_DISSEMINATE:
                {
                    gpi_memcpy_dma((uint8_t *)(loradisc_data), data, DATA_HEADER_LENGTH);
                    if (chirp_outl->disem_flag)
                        gpi_memcpy_dma((uint32_t *)&(loradisc_data[DATA_HEADER_LENGTH]), flash_data + i * (chirp_outl->payload_len - DATA_HEADER_LENGTH) / sizeof(uint32_t), (chirp_outl->payload_len - DATA_HEADER_LENGTH));
                    loradisc_write(i, (uint8_t *)loradisc_data);
                    break;
                }
                case CB_COLLECT:
                {
                    gpi_memcpy_dma((uint8_t *)(loradisc_data), data, DATA_HEADER_LENGTH);
                    gpi_memcpy_dma((uint32_t *)&(loradisc_data[DATA_HEADER_LENGTH]), flash_data, (chirp_outl->payload_len - DATA_HEADER_LENGTH));
                    loradisc_write(i, (uint8_t *)loradisc_data);
                    break;
                }
                case CB_VERSION:
                {
                    loradisc_write(i, (uint8_t *)loradisc_data);
                    break;
                }
                default:
                    break;
            }
        }
    }
    #if LORADISC
        free(loradisc_data);
    #endif
}

uint8_t chirp_recv(uint8_t node_id, Chirp_Outl *chirp_outl)
{
	unsigned int		i;
    uint8_t round_inc = 0;
    uint8_t round_hash = 0;
    uint8_t k = 0;
    uint8_t packet_correct = 0;
    uint32_t mask_negative[loradisc_config.my_column_mask.len];
    uint32_t firmware_bitmap_temp[DISSEM_BITMAP_32];
    uint16_t pending;

    if ((chirp_outl->task == CB_DISSEMINATE) || (chirp_outl->task == CB_COLLECT))
    {
        assert_reset(!((chirp_outl->payload_len - DATA_HEADER_LENGTH) % sizeof(uint64_t)));
        assert_reset((chirp_outl->payload_len > DATA_HEADER_LENGTH + 28));
    }
    uint32_t file_data[(chirp_outl->payload_len - DATA_HEADER_LENGTH) / sizeof(uint32_t)];
    uint8_t task_data[chirp_outl->payload_len - DATA_HEADER_LENGTH];
    uint8_t real_data[chirp_outl->payload_len];
    uint8_t receive_payload[chirp_outl->payload_len];

    #if LORADISC
        uint8_t loradisc_data_length;
        uint8_t *loradisc_data;
        if (loradisc_config.primitive == FLOODING)
        {
            loradisc_data_length = chirp_outl->payload_len + FLOODING_SURPLUS_LENGTH;
            loradisc_data = (uint8_t *)malloc(loradisc_data_length);
        }
    #endif

    if (!node_id)
    {
        PRINTF("-----column_pending = %d-----\n", mx.request->my_column_pending);
        if ((chirp_outl->task == CB_COLLECT) && (chirp_outl->round > ROUND_SETUP))
            PRINTF("output from initiator (collect):\n");
        else if (chirp_outl->task == CB_VERSION)
            PRINTF("output from initiator (version):\n");
    }

    if  (chirp_outl->task == CB_DISSEMINATE)
    {
        if ((!node_id) && (!chirp_outl->disem_flag))
        {
            memcpy((uint32_t *)&(firmware_bitmap_temp[0]), (uint32_t *)&(chirp_outl->firmware_bitmap[0]), DISSEM_BITMAP_32 * sizeof(uint32_t));
            for (i = 0; i < loradisc_config.my_column_mask.len; i++)
                mask_negative[i] = ~mx.request->mask[loradisc_config.my_column_mask.pos + i];
            pending = mx_request_clear((uint32_t *)&(firmware_bitmap_temp[0]), (uint_fast_t *)&(mask_negative[0]), DISSEM_BITMAP_32 * sizeof(uint32_t));
            if (pending == 0)
                loradisc_config.full_column = 0;
        }

        for (i = 0; i < loradisc_config.mx_generation_size; i++)
        {
            void *p = mixer_read(i);
            if (NULL != p)
            {
                memcpy(receive_payload, p, loradisc_config.matrix_payload_8.len);
                calu_payload_hash = Chirp_RSHash((uint8_t *)receive_payload, loradisc_config.matrix_payload_8.len - 2);
                rece_hash = receive_payload[loradisc_config.matrix_payload_8.len - 2] << 8 | receive_payload[loradisc_config.matrix_payload_8.len - 1];
                PRINTF("rece_hash:%d, %x, %x, %d\n", i, rece_hash, (uint16_t)calu_payload_hash, loradisc_config.matrix_payload_8.len);
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

        if (round_hash == loradisc_config.mx_generation_size)
            loradisc_config.full_rank = 1;
        else
        {
            mx.stat_counter.slot_decoded = 0;
        }
    }
	if (loradisc_config.primitive != FLOODING)
	{
        free(mx.request);
    }

    #if LORADISC
        if (loradisc_config.primitive == FLOODING)
            loradisc_read(loradisc_data);
    #endif

    if (((loradisc_config.full_rank) && (loradisc_config.primitive == DISSEMINATION)) || (loradisc_config.primitive == COLLECTION))
    {
        for (i = 0; i < loradisc_config.mx_generation_size; i++)
        {
            void *p = mixer_read(i);
            if (NULL != p)
            {
                memcpy(data, p, sizeof(data));
                packet_correct = 0;
                if (loradisc_config.primitive == COLLECTION)
                {
                    memcpy(receive_payload, p, loradisc_config.matrix_payload_8.len);
                    calu_payload_hash = Chirp_RSHash((uint8_t *)receive_payload, loradisc_config.matrix_payload_8.len - 2);
                    rece_hash = receive_payload[loradisc_config.matrix_payload_8.len - 2] << 8 | receive_payload[loradisc_config.matrix_payload_8.len - 1];
                    if (((uint16_t)calu_payload_hash == rece_hash) && (rece_hash))
                        packet_correct++;
                    PRINTF("rece_hash:%d, %x, %x, %d\n", i, rece_hash, (uint16_t)calu_payload_hash, loradisc_config.matrix_payload_8.len);
                }
                if ((packet_correct) || ((chirp_outl->task == CB_DISSEMINATE) && (loradisc_config.full_rank)))
                {
                    /* print packet */
                    PRINT_PACKET(data, DATA_HEADER_LENGTH, 1);
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
                    if (chirp_outl->task != CB_DISSEMINATE)
                        k = ROUND_HEADER_LENGTH;
                    switch (chirp_outl->task)
                    {
                        case CB_DISSEMINATE:
                        {
                            /* CB_DISSEMINATE */
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
                            break;
                        }
                        case CB_COLLECT:
                        {
                            memcpy(file_data, p + DATA_HEADER_LENGTH, sizeof(file_data));
                            PRINT_PACKET(file_data, sizeof(file_data), 0);
                            break;
                        }
                        case CB_VERSION:
                        {
                            memcpy(data, p + DATA_HEADER_LENGTH, chirp_outl->payload_len - DATA_HEADER_LENGTH);
                            PRINT_PACKET(data, chirp_outl->payload_len - DATA_HEADER_LENGTH, 0);
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
    else if (chirp_outl->task == CB_GLOSSY)
    {
        if (loradisc_config.flooding_packet_header[0] != 0xFF)
            round_inc++;

        k = 0;
        if (chirp_outl->arrange_task == CB_GLOSSY)
        {
            i = loradisc_data[k++];
            /* update arrange_task */
            chirp_outl->arrange_task = i >> 4;
            chirp_outl->default_sf = i & 0x0F;
            chirp_outl->default_tp = loradisc_data[k++];
            chirp_outl->default_slot_num = loradisc_data[k++];
        }
    }
    else if (chirp_outl->task == CB_GLOSSY_ARRANGE)
    {
        /* When flooding is valid, the packet header content is no longer the initial value: 0xFF */
        if (loradisc_config.flooding_packet_header[0] != 0xFF)
            round_inc++;

        k = 0;
        chirp_outl->task_bitmap[0] = (loradisc_data[k++] << 24) | (loradisc_data[k++] << 16) | (loradisc_data[k++] << 8) | (loradisc_data[k++]);
        if (chirp_outl->arrange_task == CB_DISSEMINATE)
        {
            chirp_outl->dissem_back_sf = loradisc_data[k++];
            chirp_outl->dissem_back_slot_num = loradisc_data[k++];
            chirp_outl->default_payload_len = loradisc_data[k++];
            chirp_outl->default_generate_size = loradisc_data[k++];

            chirp_outl->patch_update = loradisc_data[k++];
            chirp_outl->patch_bank = loradisc_data[k++];
            chirp_outl->file_compression = loradisc_data[k++];
            chirp_outl->old_firmware_size = (loradisc_data[k++] << 24) | (loradisc_data[k++] << 16) | (loradisc_data[k++] << 8) | (loradisc_data[k++]);
            chirp_outl->firmware_size = (loradisc_data[k++] << 24) | (loradisc_data[k++] << 16) | (loradisc_data[k++] << 8) | (loradisc_data[k++]);
            memcpy(&(chirp_outl->firmware_md5[0]), (uint8_t *)&(loradisc_data[k++]), 16);

            PRINTF("CB_DISSEMINATE:%d, %d, %lu, %lu, %lu\n", chirp_outl->patch_update, chirp_outl->patch_bank, chirp_outl->old_firmware_size, chirp_outl->firmware_size, chirp_outl->file_compression);

            for (i = 0; i < 16; i++)
                PRINTF("%02X", chirp_outl->firmware_md5[i]);

            if (node_id)
            {
                /* update whole firmware */
                if ((!chirp_outl->patch_update))
                {
                    menu_preSend(1);
                    file_data[0] = chirp_outl->firmware_size;
                    FLASH_If_Write(FIRMWARE_FLASH_ADDRESS_2, (uint32_t *)file_data, 2);
                }
                /* patch firmware */
                else if ((chirp_outl->patch_update))
                {
                    chirp_outl->patch_page = menu_pre_patch(chirp_outl->patch_bank, chirp_outl->old_firmware_size, chirp_outl->firmware_size);
                    PRINTF("patch:%lu, %d\n", chirp_outl->old_firmware_size, chirp_outl->patch_page);
                }
            }
        }
        else if (chirp_outl->arrange_task == CB_COLLECT)
        {
            chirp_outl->default_payload_len = loradisc_data[k++];
            chirp_outl->collect_addr_start = (loradisc_data[k++] << 24) | (loradisc_data[k++] << 16) | (loradisc_data[k++] << 8) | (loradisc_data[k++]);
            chirp_outl->collect_addr_end = (loradisc_data[k++] << 24) | (loradisc_data[k++] << 16) | (loradisc_data[k++] << 8) | (loradisc_data[k++]);
            PRINTF("collect_addr_start:%x, %x\n", chirp_outl->collect_addr_start, chirp_outl->collect_addr_end);
        }
    }
    else if (chirp_outl->task == CB_START)
    {
        /* When flooding is valid, the packet header content is no longer the initial value: 0xFF */
        if (loradisc_config.flooding_packet_header[0] != 0xFF)
            round_inc++;

        k = 0;
        chirp_outl->flash_protection = loradisc_data[k++];
        chirp_outl->version_hash = (loradisc_data[k++] << 8) | (loradisc_data[k++]);
        chirp_outl->start_year = (loradisc_data[k++] << 8) | (loradisc_data[k++]);
        chirp_outl->start_month = loradisc_data[k++];
        chirp_outl->start_date = loradisc_data[k++];
        chirp_outl->start_hour = loradisc_data[k++];
        chirp_outl->start_min = loradisc_data[k++];
        chirp_outl->start_sec = loradisc_data[k++];

        chirp_outl->end_year = (loradisc_data[k++] << 8) | (loradisc_data[k++]);
        chirp_outl->end_month = loradisc_data[k++];
        chirp_outl->end_date = loradisc_data[k++];
        chirp_outl->end_hour = loradisc_data[k++];
        chirp_outl->end_min = loradisc_data[k++];
        chirp_outl->end_sec = loradisc_data[k++];

        chirp_outl->firmware_bitmap[0] = (loradisc_data[k++] << 24) | (loradisc_data[k++] << 16) | (loradisc_data[k++] << 8) | (loradisc_data[k++]);

        PRINTF("\t receive, START at %d-%d-%d, %d:%d:%d\n\tEnd at %d-%d-%d, %d:%d:%d\n, flash_protection:%d, v:%x, bitmap:%x\n", chirp_outl->start_year, chirp_outl->start_month, chirp_outl->start_date, chirp_outl->start_hour, chirp_outl->start_min, chirp_outl->start_sec, chirp_outl->end_year, chirp_outl->end_month, chirp_outl->end_date, chirp_outl->end_hour, chirp_outl->end_min, chirp_outl->end_sec, chirp_outl->flash_protection, chirp_outl->version_hash, chirp_outl->firmware_bitmap[0]);
    }
    else if (chirp_outl->task == CB_CONNECTIVITY)
    {
        /* When flooding is valid, the packet header content is no longer the initial value: 0xFF */
        if (loradisc_config.flooding_packet_header[0] != 0xFF)
            round_inc++;

        k = 0;
        chirp_outl->sf_bitmap = loradisc_data[k++];
        chirp_outl->tx_power = loradisc_data[k++];
        chirp_outl->topo_payload_len = loradisc_data[k++];
        chirp_outl->freq = (loradisc_data[k++] << 24) | (loradisc_data[k++] << 16) | (loradisc_data[k++] << 8) | (loradisc_data[k++]);

        PRINTF("chirp_outl->sf_bitmap: %d, %d, %d, %ld\n", chirp_outl->sf_bitmap, chirp_outl->tx_power, chirp_outl->topo_payload_len, chirp_outl->freq);
    }

	if (loradisc_config.primitive != FLOODING)
    {
        free(mx.matrix[0]);
        mx.matrix[0] = NULL;
    }

    #if LORADISC
        if (loradisc_config.primitive == FLOODING)
            free(loradisc_data);
    #endif

    /* have received at least a packet */
    if ((chirp_outl->task == CB_COLLECT) || (chirp_outl->task == CB_VERSION))
    {
        if (round_inc > 1)
        {
            chirp_outl->round++;
            return 1;
        }
        else
            return 0;

    }
    else
    {
        if (chirp_outl->task == CB_DISSEMINATE)
        {
            if (round_hash)
            {
                chirp_outl->round++;
                return 1;
            }
            else
                return 0;
        }
        else
        {
            if (round_inc)
                return 1;
            /* not received any packet */
            else
                return 0;
        }
    }
}

uint8_t chirp_round(uint8_t node_id, Chirp_Outl *chirp_outl)
{
    gpi_watchdog_periodic();
	// Gpi_Fast_Tick_Native deadline;
    Gpi_Fast_Tick_Extended deadline;
    Gpi_Fast_Tick_Native update_period = GPI_TICK_MS_TO_FAST2(((loradisc_config.mx_period_time_s * 1000) / 1) - loradisc_config.mx_round_length * (loradisc_config.mx_slot_length_in_us / 1000));

    uint8_t failed_round = 0;

    chirp_outl->round = ROUND_SETUP;

    /* set current state as mixer */
	chirp_isr.state = ISR_MIXER;

	// deadline = gpi_tick_fast_native() + 2 * loradisc_config.mx_slot_length;
	// deadline = gpi_tick_fast_native();
	deadline = gpi_tick_fast_extended();

    clear_data();

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
        if (loradisc_config.primitive == DISSEMINATION)
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
            if ((chirp_outl->task == CB_DISSEMINATE) && (chirp_outl->round == 1))
                // deadline += GPI_TICK_MS_TO_FAST2(8000);
                deadline += (Gpi_Fast_Tick_Extended)1 * loradisc_config.mx_slot_length;
            else
                deadline += (Gpi_Fast_Tick_Extended)1 * loradisc_config.mx_slot_length;
        }

		/* start when deadline reached
		ATTENTION: don't delay after the polling loop (-> print before) */
		// while (gpi_tick_compare_fast_native(gpi_tick_fast_native(), deadline) < 0);
        #if MX_LBT_ACCESS
            lbt_check_time();
            chirp_isr.state = ISR_MIXER;
            if (loradisc_config.primitive != FLOODING)
            {
            loradisc_config.lbt_channel_primary = (loradisc_config.lbt_channel_primary + 1) % LBT_CHANNEL_NUM;
            if ((!chirp_outl->disem_flag) && (chirp_outl->task == CB_DISSEMINATE) && (chirp_outl->round >= 2))
            {
                loradisc_config.lbt_channel_primary = (loradisc_config.lbt_channel_primary + LBT_CHANNEL_NUM - 1) % LBT_CHANNEL_NUM;
            }
            }
            LoRaDS_SX1276SetChannel(loradisc_config.lora_freq + loradisc_config.lbt_channel_primary * CHANNEL_STEP);
            PRINTF("-------lbt_channel_primary:%d\n", loradisc_config.lbt_channel_primary);
        #endif
		while (gpi_tick_compare_fast_extended(gpi_tick_fast_extended(), deadline) < 0);
        #if ENERGEST_CONF_ON
            ENERGEST_OFF(ENERGEST_TYPE_CPU);
        #endif
        /* used in mixer_write, and revalue before mixer round */
        loradisc_config.full_rank = 0;
        loradisc_config.full_column = UINT8_MAX;
        rece_dissem_index = UINT16_MAX;

		deadline = mixer_start();

        if (loradisc_config.primitive != FLOODING)
        {
            if (chirp_outl->task != CB_DISSEMINATE)
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
                failed_round++;
                PRINTF("failed:%d\n", failed_round);
                if (chirp_outl->task == CB_DISSEMINATE)
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
                else if ((failed_round >= 1) && (chirp_outl->task != CB_DISSEMINATE))
                {
                    Stats_value(SLOT_STATS, mx.stat_counter.slot_decoded);
                    Stats_to_Flash(chirp_outl->task);
                    return 0;
                }
            }
            else
                failed_round = 0;

            if (chirp_outl->task != CB_DISSEMINATE)
                Stats_value(SLOT_STATS, mx.stat_counter.slot_decoded);

            while (gpi_tick_compare_fast_extended(gpi_tick_fast_extended(), deadline) < 0);

            /* in dissemination, nodes have to send back the results, so switch the configuration between rounds */
            if (chirp_outl->task == CB_DISSEMINATE)
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
                    /* next is dissemination session: disseminate files to all nodes */
                    if (!chirp_outl->disem_flag)
                    {
                        Stats_value_debug(ENERGEST_TYPE_CPU, energest_type_time(ENERGEST_TYPE_CPU));
                        Stats_value_debug(ENERGEST_TYPE_LPM, energest_type_time(ENERGEST_TYPE_LPM) - energest_type_time(ENERGEST_TYPE_TRANSMIT) - energest_type_time(ENERGEST_TYPE_LISTEN));
                        Stats_value_debug(ENERGEST_TYPE_TRANSMIT, energest_type_time(ENERGEST_TYPE_TRANSMIT));
                        Stats_value_debug(ENERGEST_TYPE_LISTEN, energest_type_time(ENERGEST_TYPE_LISTEN));
                        free(payload_distribution);
                        loradisc_radio_config(chirp_outl->default_sf, 1, chirp_outl->default_tp, chirp_outl->default_freq);
                        /* If now is confirm, the initiator collect all nodes information about whether they are full rank last round, if so, then send the next file chunk, file index++, else do not increase file index */
                        if ((!node_id) && (loradisc_config.full_column == 0))
                        {
                            chirp_outl->disem_file_index++;
                            chirp_outl->disem_file_index_stay = 0;
                            PRINTF("full receive\n");
                        }
                        PRINTF("next: disem_flag: %d, %d\n", chirp_outl->disem_file_index, chirp_outl->disem_file_max);
                        loradisc_packet_config(chirp_outl->num_nodes, chirp_outl->generation_size, chirp_outl->payload_len + HASH_TAIL, DISSEMINATION);
                        chirp_outl->packet_time = SX1276GetPacketTime(loradisc_config.lora_sf, loradisc_config.lora_bw, 1, 0, 8, loradisc_config.phy_payload_size);
                        loradisc_slot_config(chirp_outl->packet_time + 100000, chirp_outl->default_slot_num, 2000000);
                        loradisc_payload_distribution();
                        chirp_outl->disem_flag = 1;
                    }
                    /* next is confirm session: collect all nodes condition (if full rank in last mixer round) */
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
                            loradisc_radio_config(chirp_outl->dissem_back_sf, 1, 14, chirp_outl->default_freq);
                        PRINTF("next: collect disem_flag: %d, %d\n", chirp_outl->disem_file_index, chirp_outl->disem_file_max);
                        // chirp_outl->payload_len = DATA_HEADER_LENGTH;
                        loradisc_packet_config(chirp_outl->num_nodes, chirp_outl->num_nodes, DATA_HEADER_LENGTH + HASH_TAIL, COLLECTION);
                        chirp_outl->packet_time = SX1276GetPacketTime(loradisc_config.lora_sf, loradisc_config.lora_bw, 1, 0, 8, loradisc_config.phy_payload_size);
                        if (chirp_outl->dissem_back_slot_num == 0)
                            chirp_outl->dissem_back_slot_num = chirp_outl->num_nodes * 8;
                        loradisc_slot_config(chirp_outl->packet_time + 100000, chirp_outl->dissem_back_slot_num, 1500000);
                        loradisc_payload_distribution();
                        chirp_outl->disem_flag = 0;
                        /* in confirm, all nodes sends packets */
                        PRINTF("rece_dissem_index:%x\n", rece_dissem_index);

                        if (chirp_outl->disem_file_index > rece_dissem_index)
                        {
                            PRINTF("full disem copy\n");
                            chirp_outl->disem_flag_full_rank = mx.stat_counter.slot_full_rank;
                        }
                    }
            }

            /* once the round num expired, quit loop */
            if ((chirp_outl->round > chirp_outl->round_max) && (chirp_outl->task != CB_DISSEMINATE))
            {
                Stats_to_Flash(chirp_outl->task);
                return 1;
            }
            /* During the collection status that belongs to the dissemination, the dissemination is interrupted when the file is completed */
            else if ((chirp_outl->task == CB_DISSEMINATE) && (!chirp_outl->disem_flag))
            {
                if ((node_id) && (chirp_outl->disem_file_index >= chirp_outl->disem_file_max + 2))
                    return 1;
                else if ((!node_id) && (chirp_outl->disem_file_index >= chirp_outl->disem_file_max + 1))
                    return 1;
            }

            deadline += (Gpi_Fast_Tick_Extended)update_period;
        }
        else /* synchronization with flooding */
        {
            uint8_t recv_result = chirp_recv(node_id, chirp_outl);

            Gpi_Fast_Tick_Native resync_plus =  GPI_TICK_MS_TO_FAST2(((loradisc_config.mx_slot_length_in_us * 5 / 2) * (loradisc_config.mx_round_length / 2 - 1) / 1000) - loradisc_config.mx_round_length * (loradisc_config.mx_slot_length_in_us / 1000));
            /* haven't received any synchronization packet, always on reception mode, leading to end a round later than synchronized node */
            if (!recv_result)
                deadline += (Gpi_Fast_Tick_Extended)(update_period - resync_plus);
            /* have synchronized to a node */
            else
                deadline += (Gpi_Fast_Tick_Extended)(update_period);
            while (gpi_tick_compare_fast_extended(gpi_tick_fast_extended(), deadline) < 0);
            if (chirp_outl->task == CB_GLOSSY)
                return chirp_outl->arrange_task;
            else if ((chirp_outl->task != CB_GLOSSY) & (recv_result == 1))
                return 1;
            else if ((chirp_outl->task != CB_GLOSSY) & (recv_result == 0))
                return 0;
        }
	}
}

//**************************************************************************************************
//**************************************************************************************************
