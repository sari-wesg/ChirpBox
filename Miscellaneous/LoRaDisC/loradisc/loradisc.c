//**************************************************************************************************
//**** Includes ************************************************************************************

//**************************************************************************************************
#include "loradisc.h"
// Mixer
#include "mixer_internal.h"
// DEBUG
#include "gpi/tools.h"

//**************************************************************************************************
//***** Local Defines and Consts *******************************************************************

//**************************************************************************************************
//***** Local Typedefs and Class Declarations ******************************************************

//**************************************************************************************************
//***** Forward Declarations ***********************************************************************

//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************
uint32_t dev_id_list[NODE_LENGTH] = {0x004a0022, 0x00350017}; // TODO: oead
// uint32_t dev_id_list[NODE_LENGTH] = {0x00440034, 0x0027002d}; // TODO: tu graz

uint8_t MX_NUM_NODES_CONF;

//**************************************************************************************************
//***** Global Variables ***************************************************************************
extern LoRaDisC_Config loradisc_config;

/* device id */
extern uint32_t __attribute__((section(".data"))) TOS_NODE_ID;
/* node id */
uint8_t node_id_allocate;
//**************************************************************************************************
//***** Local Functions ****************************************************************************

static uint8_t logical_node_id(uint32_t dev_id, uint32_t *UID_list)
{
    uint8_t node_id;

    /* translate dev_id to logical node id used with mixer */
    for (node_id = 0; node_id < NODE_LENGTH; ++node_id)
    {
        if (UID_list[node_id] == 0)
            break;
    }
    MX_NUM_NODES_CONF = node_id;
    PRINTF("MX_NUM_NODES_CONF:%d\n", MX_NUM_NODES_CONF);

    for (node_id = 0; node_id < MX_NUM_NODES_CONF; ++node_id)
    {
        if (UID_list[node_id] == dev_id)
            break;
    }

    if (node_id >= MX_NUM_NODES_CONF)
    {
        PRINTF("Warning: node mapping not found for node 0x%x !!!\n", dev_id);
        while (1)
            ;
    }
    PRINTF("Running with node ID: %d\n", node_id);

    return node_id;
}

//**************************************************************************************************
//***** Global Functions ***************************************************************************
void loradisc_write(uint8_t i, uint8_t *data)
{
    if (loradisc_config.primitive == FLOODING)
    {
        /* Divide the payload into two parts: one part is reused with the packet header and one part is after the packet header. */
        memcpy((uint8_t *)(loradisc_config.flooding_packet_header), (uint8_t *)data, FLOODING_SURPLUS_LENGTH);
        if (loradisc_config.phy_payload_size > LORADISC_HEADER_LEN)
            memcpy((uint8_t *)(loradisc_config.flooding_packet_payload), (uint8_t *)&(data[FLOODING_SURPLUS_LENGTH]), loradisc_config.phy_payload_size - LORADISC_HEADER_LEN);
    }
    else
    {
        mixer_write(i, (uint8_t *)data, loradisc_config.mx_payload_size - HASH_TAIL);
    }
}

void loradisc_read(uint8_t *data)
{
    if (loradisc_config.primitive == FLOODING)
    {
        memcpy(data, (uint8_t *)(loradisc_config.flooding_packet_header), FLOODING_SURPLUS_LENGTH);
        if (loradisc_config.phy_payload_size > LORADISC_HEADER_LEN)
            memcpy((uint8_t *)&(data[FLOODING_SURPLUS_LENGTH]), (uint8_t *)(loradisc_config.flooding_packet_payload), loradisc_config.phy_payload_size - LORADISC_HEADER_LEN);
    }
}

void loradisc_init()
{
    // loradisc flooding clear data
    memset(loradisc_config.flooding_packet_header, 0xFF, sizeof(loradisc_config.flooding_packet_header));
    memset(loradisc_config.flooding_packet_payload, 0xFF, sizeof(loradisc_config.flooding_packet_payload));

    /* used in mixer_write, and revalue before mixer round */
    loradisc_config.full_rank = 0;
    loradisc_config.full_column = UINT8_MAX;
    // rece_dissem_index = UINT16_MAX;
}

void loradisc_data_init(uint8_t data_length, uint8_t **data)
{
    if (*data != NULL)
        free(*data);

    assert_reset(data_length >= FLOODING_SURPLUS_LENGTH);
    *data = (uint8_t *)malloc(data_length);

    uint8_t i;
    memcpy(*data, &TOS_NODE_ID, sizeof(TOS_NODE_ID));
    for (i = sizeof(TOS_NODE_ID); i < data_length; i++)
        *data[i] = i;
}

void loradisc_reconfig(uint8_t nodes_num, uint8_t generation_size, uint8_t data_length, Disc_Primitive primitive, uint8_t sf, uint8_t tp, uint32_t lora_frequency)
{
    // radio config
    loradisc_radio_config(sf, 1, tp, lora_frequency / 1000);

    // lbt config
#if MX_LBT_ACCESS
    lbt_init();
#endif

    // packet config
    if (primitive == FLOODING)
    {
        loradisc_packet_config(nodes_num, 0, 0, primitive);
        loradisc_config.phy_payload_size = LORADISC_HEADER_LEN + (data_length > FLOODING_SURPLUS_LENGTH ? data_length - FLOODING_SURPLUS_LENGTH : 0);
    }
    else
        loradisc_packet_config(nodes_num, generation_size, data_length, primitive);

    uint32_t packet_time = SX1276GetPacketTime(loradisc_config.lora_sf, loradisc_config.lora_bw, 1, 0, 8, LORADISC_HEADER_LEN);
    uint8_t hop_count; // TODO: calculate
    if (primitive == FLOODING)
        hop_count = nodes_num > 10 ? 6 * 2 : 4 * 2;
    else
        hop_count = nodes_num > 10 ? (nodes_num * generation_size + 5) / 5 : (nodes_num * generation_size + 4) / 4;

    loradisc_slot_config(packet_time + 100000, hop_count, 1500000);

    if (loradisc_config.primitive != FLOODING)
        loradisc_config.packet_hash = DISC_HEADER;
    else
        loradisc_config.packet_hash = FLOODING_HEADER;
}

void loradisc_start()
{
    // init
    node_id_allocate = logical_node_id(TOS_NODE_ID, dev_id_list); // node id
    uint8_t *data = NULL;
    uint8_t data_length;

    // config
    loradisc_reconfig(MX_NUM_NODES_CONF, MX_NUM_NODES_CONF, sizeof(data), DISSEMINATION, 7, 14, CN470_FREQUENCY);
    loradisc_reconfig(MX_NUM_NODES_CONF, MX_NUM_NODES_CONF, sizeof(data), COLLECTION, 7, 14, CN470_FREQUENCY);
    loradisc_reconfig(MX_NUM_NODES_CONF, NULL, sizeof(data), FLOODING, 7, 14, CN470_FREQUENCY);

    // round:
    chirp_isr.state = ISR_MIXER;

    Gpi_Fast_Tick_Extended deadline;
    Gpi_Fast_Tick_Native update_period = GPI_TICK_MS_TO_FAST2(((loradisc_config.mx_period_time_s * 1000) / 1) - loradisc_config.mx_round_length * (loradisc_config.mx_slot_length_in_us / 1000));

    deadline = gpi_tick_fast_extended();

    while (1)
    {
        data_length = 10;
        loradisc_data_init(data_length, &data);
        loradisc_init();

        gpi_radio_init();

        /* init mixer */
        mixer_init(node_id_allocate);
#if ENERGEST_CONF_ON
        ENERGEST_ON(ENERGEST_TYPE_CPU);
#endif

        // /* except these two task that all nodes need to upload data, others only initiator transmit data */
        // if (loradisc_config.primitive == DISSEMINATION)
        // {
        //     if (!node_id_allocate)
        //         chirp_write(node_id_allocate, chirp_outl);
        // }
        // else
        //     chirp_write(node_id_allocate, chirp_outl);

        loradisc_packet_write(node_id_allocate, data);

        /* arm mixer, node 0 = initiator
        start first round with infinite scan
        -> nodes join next available round, does not require simultaneous boot-up */
        mixer_arm(((!node_id_allocate) ? MX_ARM_INITIATOR : 0) | ((1 == 0) ? MX_ARM_INFINITE_SCAN : 0));

        /* delay initiator a bit
        -> increase probability that all nodes are ready when initiator starts the round
        -> avoid problems in view of limited deadline accuracy */
        if (!node_id_allocate)
            deadline += (Gpi_Fast_Tick_Extended)1 * loradisc_config.mx_slot_length;

#if MX_LBT_ACCESS
        lbt_update();
#endif

        /* start when deadline reached
        ATTENTION: don't delay after the polling loop (-> print before) */
        while (gpi_tick_compare_fast_extended(gpi_tick_fast_extended(), deadline) < 0)
            ;

#if ENERGEST_CONF_ON
        ENERGEST_OFF(ENERGEST_TYPE_CPU);
#endif

        deadline = mixer_start();

        if (loradisc_config.primitive == FLOODING)
        {
            loradisc_read(data);
            uint8_t i = 0;
            PRINTF_DISC("receiving data:0x%x\n", data[i++] << 24 | data[i++] << 16 | data[i++] << 8 | data[i++]);
            uint8_t recv_result = 0;
            if (data[0] != 0xFF)
                recv_result++;

            Gpi_Fast_Tick_Native resync_plus = GPI_TICK_MS_TO_FAST2(((loradisc_config.mx_slot_length_in_us * 5 / 2) * (loradisc_config.mx_round_length / 2 - 1) / 1000) - loradisc_config.mx_round_length * (loradisc_config.mx_slot_length_in_us / 1000));

            /* haven't received any synchronization packet, always on reception mode, leading to end a round later than synchronized node */
            if (!recv_result)
                deadline += (Gpi_Fast_Tick_Extended)(update_period - resync_plus);
            /* have synchronized to a node */
            else
                deadline += (Gpi_Fast_Tick_Extended)(update_period);
        }

        while (gpi_tick_compare_fast_extended(gpi_tick_fast_extended(), deadline) < 0)
            ;
    }
}

void loradisc_packet_write(uint8_t node_id, uint8_t *data)
{
    // uint8_t loradisc_data_length;
    // uint8_t *loradisc_data;
    // if (loradisc_config.primitive != FLOODING)
    //     loradisc_data_length = loradisc_config.phy_payload_size - LORADISC_HEADER_LEN;
    // loradisc_data = (uint8_t *)malloc(loradisc_data_length);

    if (loradisc_config.primitive == FLOODING)
    {
        loradisc_write(NULL, data);
    }
}

void loradisc_radio_config(uint8_t lora_spreading_factor, uint8_t lora_codingrate, int8_t tx_output_power, uint32_t lora_frequency)
{
    memset(&loradisc_config + offsetof(LoRaDisC_Config, lora_sf), 0, sizeof(loradisc_config) - offsetof(LoRaDisC_Config, lora_sf));

    // default:
    loradisc_config.lora_bw = LoRaDisC_DEFAULT_BW;
    loradisc_config.lora_plen = LoRaDisC_PREAMBLE_LENGTH;
    // costom:
    loradisc_config.lora_sf = lora_spreading_factor;
    loradisc_config.lora_cr = lora_codingrate;
    loradisc_config.lora_tx_pwr = tx_output_power;
    loradisc_config.lora_freq = lora_frequency * 1e3; /* kHz -> Hz */
    gpi_radio_init();
#if MX_LBT_ACCESS
    uint32_t symbol_time_us = SX1276GetSymbolTime(loradisc_config.lora_sf, loradisc_config.lora_bw);
    loradisc_config.lbt_detect_duration_us = (6 * symbol_time_us >= LBT_DELAY_IN_US) ? 6 * symbol_time_us : LBT_DELAY_IN_US;
#endif
}

/**
 * @description: mixer configurations
 * @param mx_num_nodes: number of nodes in the mixer network (NUM_ELEMENTS(nodes))
 * @param mx_generation_size: number of packets (NUM_ELEMENTS(payload_distribution))
 * @param mx_payload_size: length of payload (MX_PAYLOAD_CONF_SIZE)
 * @return: None
 */
void loradisc_packet_config(uint8_t mx_num_nodes, uint8_t mx_generation_size, uint8_t mx_payload_size, Disc_Primitive primitive)
{
    memset(&loradisc_config, 0, offsetof(LoRaDisC_Config, mx_slot_length_in_us));
    loradisc_config.primitive = primitive;
    // loradisc_config
    loradisc_config.mx_num_nodes = mx_num_nodes;
    loradisc_config.mx_generation_size = mx_generation_size;
    loradisc_config.mx_payload_size = mx_payload_size;

    loradisc_config.coding_vector.pos = 0;
    loradisc_config.coding_vector.len = (loradisc_config.mx_generation_size + 7) / 8;
    loradisc_config.payload.pos = loradisc_config.coding_vector.pos + loradisc_config.coding_vector.len;
    loradisc_config.payload.len = loradisc_config.mx_payload_size;
    loradisc_config.info_vector.pos = loradisc_config.payload.pos + loradisc_config.payload.len;
    loradisc_config.info_vector.len = (loradisc_config.mx_generation_size + 7) / 8;
    loradisc_config._padding_2.pos = loradisc_config.info_vector.pos + loradisc_config.info_vector.len;
    loradisc_config._padding_2.len = PADDING_MAX(0,
                                                 PADDING_SIZE((loradisc_config.mx_generation_size + 7) / 8) + PADDING_SIZE(loradisc_config.mx_payload_size)
#if (MX_REQUEST || MX_SMART_SHUTDOWN_MAP)
                                                     - ((loradisc_config.mx_generation_size + 7) / 8)
#endif
    );
    loradisc_config.rand.pos = loradisc_config._padding_2.pos + loradisc_config._padding_2.len;
    loradisc_config.rand.len = 1;
    loradisc_config._padding_3.pos = loradisc_config.rand.pos + loradisc_config.rand.len;
    loradisc_config._padding_3.len = PADDING_SIZE(
        ((loradisc_config.mx_generation_size + 7) / 8) + // coding_vector
        loradisc_config.mx_payload_size +                // payload
#if (MX_REQUEST || MX_SMART_SHUTDOWN_MAP)
        ((loradisc_config.mx_generation_size + 7) / 8) + // info_vector
#if !GPI_ARCH_IS_BOARD(TMOTE)
        PADDING_MAX(0, // _padding_2
                    PADDING_SIZE((loradisc_config.mx_generation_size + 7) / 8) + PADDING_SIZE(loradisc_config.mx_payload_size) - ((loradisc_config.mx_generation_size + 7) / 8)) +
#endif
#else
#if !GPI_ARCH_IS_BOARD(TMOTE)
        PADDING_MAX(0, // _padding_2
                    PADDING_SIZE((loradisc_config.mx_generation_size + 7) / 8) + PADDING_SIZE(loradisc_config.mx_payload_size)) +
#endif
#endif
        1);
    loradisc_config.packet_chunk_len = loradisc_config.coding_vector.len + loradisc_config.payload.len + loradisc_config.info_vector.len + loradisc_config._padding_2.len + loradisc_config.rand.len + loradisc_config._padding_3.len;
    loradisc_config.phy_payload_size = offsetof(Packet, packet_chunk) - offsetof(Packet, phy_payload_begin) + loradisc_config.coding_vector.len + loradisc_config.payload.len + loradisc_config.info_vector.len;
    loradisc_config.packet_len = offsetof(Packet, packet_chunk) - offsetof(Packet, phy_payload_begin) + loradisc_config.packet_chunk_len;
    assert_reset(!(loradisc_config.packet_len % sizeof(uint_fast_t)));

    loradisc_config.matrix_coding_vector_8.pos = 0;
    loradisc_config.matrix_coding_vector_8.len = loradisc_config.coding_vector.len;
    loradisc_config.matrix_payload_8.pos = loradisc_config.matrix_coding_vector_8.pos + loradisc_config.matrix_coding_vector_8.len;
    loradisc_config.matrix_payload_8.len = loradisc_config.payload.len;

    loradisc_config.matrix_coding_vector.pos = 0;
    loradisc_config.matrix_coding_vector.len = (loradisc_config.mx_generation_size + (sizeof(uint_fast_t) * 8) - 1) / (sizeof(uint_fast_t) * 8);
    loradisc_config.matrix_payload.pos = loradisc_config.matrix_coding_vector.pos + loradisc_config.matrix_coding_vector.len;
    loradisc_config.matrix_payload.len = (loradisc_config.mx_payload_size + sizeof(uint_fast_t) - 1) / sizeof(uint_fast_t);

    loradisc_config.matrix_chunk_8_len = loradisc_config.matrix_coding_vector_8.len + loradisc_config.matrix_payload_8.len;
    loradisc_config.matrix_chunk_32_len = loradisc_config.matrix_coding_vector.len + loradisc_config.matrix_payload.len;
    loradisc_config.matrix_size_32 = loradisc_config.matrix_chunk_32_len + 1;

    loradisc_config.history_len_8 = offsetof(Node, row_map_chunk) + loradisc_config.matrix_coding_vector.len * sizeof(uint_fast_t);

    uint8_t hash_factor = (((loradisc_config.mx_num_nodes + 7) / 8 + loradisc_config.info_vector.len - 1) / loradisc_config.info_vector.len);
    loradisc_config.map.pos = 0;
    loradisc_config.map.len = hash_factor * loradisc_config.info_vector.len;
    loradisc_config.hash.pos = loradisc_config.map.pos + loradisc_config.map.len;
    loradisc_config.hash.len = loradisc_config.info_vector.len;

    loradisc_config.row_all_mask.pos = 0;
    loradisc_config.row_all_mask.len = loradisc_config.matrix_coding_vector.len;
    loradisc_config.row_any_mask.pos = loradisc_config.row_all_mask.pos + loradisc_config.row_all_mask.len;
    loradisc_config.row_any_mask.len = loradisc_config.matrix_coding_vector.len;
    loradisc_config.column_all_mask.pos = loradisc_config.row_any_mask.pos + loradisc_config.row_any_mask.len;
    loradisc_config.column_all_mask.len = loradisc_config.matrix_coding_vector.len;
    loradisc_config.column_any_mask.pos = loradisc_config.column_all_mask.pos + loradisc_config.column_all_mask.len;
    loradisc_config.column_any_mask.len = loradisc_config.matrix_coding_vector.len;
    loradisc_config.my_row_mask.pos = loradisc_config.column_any_mask.pos + loradisc_config.column_any_mask.len;
    loradisc_config.my_row_mask.len = loradisc_config.matrix_coding_vector.len;
    loradisc_config.my_column_mask.pos = loradisc_config.my_row_mask.pos + loradisc_config.my_row_mask.len;
    loradisc_config.my_column_mask.len = loradisc_config.matrix_coding_vector.len;
}

/* slot length is mx_slot_length_in_us microseconds,
needed slot number is mx_round_length,
round is last for mx_period_time_us seconds */
void loradisc_slot_config(uint32_t mx_slot_length_in_us, uint16_t mx_round_length, uint32_t period_time_us_plus)
{
    uint32_t mx_period_time_us;
    memset(&loradisc_config + offsetof(LoRaDisC_Config, mx_slot_length_in_us), 0, offsetof(LoRaDisC_Config, lora_sf) - offsetof(LoRaDisC_Config, mx_slot_length_in_us));
    loradisc_config.mx_slot_length_in_us = mx_slot_length_in_us;
#if MX_LBT_ACCESS
    loradisc_config.mx_slot_length_in_us += loradisc_config.lbt_detect_duration_us * CHANNEL_ALTER;
#endif
    loradisc_config.mx_slot_length = GPI_TICK_US_TO_FAST2(loradisc_config.mx_slot_length_in_us);
    loradisc_config.mx_round_length = mx_round_length;
    mx_period_time_us = loradisc_config.mx_slot_length_in_us * mx_round_length + period_time_us_plus;
    loradisc_config.mx_period_time_s = (mx_period_time_us + 1000000 - 1) / 1000000;
}

uint32_t Chirp_RSHash(uint8_t *str, uint32_t len)
{
    uint32_t b = 378551;
    uint32_t a = 63689;
    uint32_t hash = 0;
    uint32_t i = 0;

    for (i = 0; i < len; str++, i++)
    {
        hash = hash * a + (*str);
        a = a * b;
    }

    return hash;
}

uint8_t randInt(uint8_t begin, uint8_t end)
{
    uint8_t temp;
    temp = begin + (rand() % (end - begin + 1));
    return temp;
}

void randomPermutation1(uint8_t channel_sync, uint8_t n)
{
    bool boollist[n];
    uint8_t *a = channel_sync;
    bool *used = boollist;
    for (uint8_t i = 0; i < n; i++)
        used[i] = true;
    uint8_t k = 0;
    for (uint8_t i = 0; i < n; ++i)
    {
        k = randInt(1, n);
        while (used[k] == false)
        {
            k = randInt(1, n);
        }

        used[k] = false;
        a[i] = k;
        // printf("a[i]:%lu, %lu\n", a[i], i);
    }
}
