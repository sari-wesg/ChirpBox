//**************************************************************************************************
//**** Includes ************************************************************************************

//**************************************************************************************************
#include "loradisc.h"
// Mixer
#include "mixer_internal.h"
// DEBUG
#include "gpi/tools.h"
#include "gpi/olf.h"

//**************************************************************************************************
//***** Local Defines and Consts *******************************************************************

//**************************************************************************************************
//***** Local Typedefs and Class Declarations ******************************************************

//**************************************************************************************************
//***** Forward Declarations ***********************************************************************

//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************
// uint32_t dev_id_list[NODE_LENGTH] = {0x004a0022, 0x00350017}; // TODO:
uint32_t dev_id_list[NODE_LENGTH] = {0x00440034, 0x0027002d}; // TODO:

uint8_t MX_NUM_NODES_CONF;

//**************************************************************************************************
//***** Global Variables ***************************************************************************
extern LoRaDisC_Config loradisc_config;
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

void loradisc_start(uint32_t dev_id)
{
    // init:
    uint8_t network_num_nodes = MX_NUM_NODES_CONF;
    node_id_allocate = logical_node_id(dev_id, dev_id_list);

// radio and lbt config
#if MX_LBT_ACCESS
    memset(&loradisc_config.lbt_init_time, 0, sizeof(loradisc_config.lbt_init_time));
    loradisc_config.lbt_channel_total = LBT_CHANNEL_NUM;
    int32_t mask = 1 << (sizeof(uint_fast_t) * 8 - 1);
    uint32_t m;
    for (m = sizeof(uint32_t) * 8; m-- > loradisc_config.lbt_channel_total;)
        mask >>= 1;
    loradisc_config.lbt_channel_mask = ~(mask << 1);
#endif

    loradisc_config.lbt_channel_primary = 0;

    uint8_t hop_count = network_num_nodes > 10 ? 6 : 4;

    // loradisc flooding clear data
    memset(loradisc_config.flooding_packet_header, 0xFF, sizeof(loradisc_config.flooding_packet_header));
    memset(loradisc_config.flooding_packet_payload, 0, sizeof(loradisc_config.flooding_packet_payload));

    // data input: TODO:
    uint8_t the_data_length = 10;
    uint8_t the_data[the_data_length];
    uint8_t i;
    memset(the_data, 0xFF, sizeof(dev_id));
    if (!node_id_allocate)
        memcpy(the_data, &dev_id, sizeof(dev_id));
    for (i = sizeof(dev_id); i < the_data_length; i++)
        the_data[i] = i;
    assert_reset(the_data_length >= FLOODING_SURPLUS_LENGTH);
    i = 0;
    PRINTF_DISC("sending data: 0x%x\n", the_data[i++] << 24 | the_data[i++] << 16 | the_data[i++] << 8 | the_data[i++]);
    // radio config
    loradisc_radio_config(7, 1, 14, CN470_FREQUENCY / 1000);
    // packet config, if flooding
    loradisc_packet_config(MX_NUM_NODES_CONF, 0, 0, FLOODING);

    // if flooding:
    if (loradisc_config.primitive == FLOODING)
        loradisc_config.phy_payload_size = LORADISC_HEADER_LEN + (the_data_length > FLOODING_SURPLUS_LENGTH ? the_data_length - FLOODING_SURPLUS_LENGTH : 0);

    uint32_t packet_time = SX1276GetPacketTime(loradisc_config.lora_sf, loradisc_config.lora_bw, 1, 0, 8, LORADISC_HEADER_LEN);
    loradisc_slot_config(packet_time + 100000, hop_count * 2, 1500000);
    // initialize glossy
    memset(loradisc_config.flooding_packet_header, 0xFF, sizeof(loradisc_config.flooding_packet_header));
    // lbt channel
    uint8_t sync_channel_id = 0;
    loradisc_config.lbt_channel_primary = sync_channel_id;
    LoRaDS_SX1276SetChannel(loradisc_config.lora_freq + loradisc_config.lbt_channel_primary * CHANNEL_STEP);

    // round:
    Gpi_Fast_Tick_Extended deadline;
    Gpi_Fast_Tick_Native update_period = GPI_TICK_MS_TO_FAST2(((loradisc_config.mx_period_time_s * 1000) / 1) - loradisc_config.mx_round_length * (loradisc_config.mx_slot_length_in_us / 1000));

    /* set current state as mixer */
    chirp_isr.state = ISR_MIXER;

    deadline = gpi_tick_fast_extended();
    PRINTF_DISC("deadline:%lu\n", deadline);

    if (loradisc_config.primitive != FLOODING)
        loradisc_config.packet_hash = DISC_HEADER;
    else
        loradisc_config.packet_hash = FLOODING_HEADER;

    while (1)
    {
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

        loradisc_packet_write(node_id_allocate, the_data);

        /* arm mixer, node 0 = initiator
        start first round with infinite scan
        -> nodes join next available round, does not require simultaneous boot-up */
        mixer_arm(((!node_id_allocate) ? MX_ARM_INITIATOR : 0) | ((1 == 0) ? MX_ARM_INFINITE_SCAN : 0));

        /* delay initiator a bit
        -> increase probability that all nodes are ready when initiator starts the round
        -> avoid problems in view of limited deadline accuracy */
        if (!node_id_allocate)
            deadline += (Gpi_Fast_Tick_Extended)1 * loradisc_config.mx_slot_length;

/* start when deadline reached
ATTENTION: don't delay after the polling loop (-> print before) */
// while (gpi_tick_compare_fast_native(gpi_tick_fast_native(), deadline) < 0);
#if MX_LBT_ACCESS
        lbt_check_time();
        chirp_isr.state = ISR_MIXER;
        if (loradisc_config.primitive != FLOODING)
        {
            // loradisc_config.lbt_channel_primary = (loradisc_config.lbt_channel_primary + 1) % LBT_CHANNEL_NUM;
            // if ((!chirp_outl->disem_flag) && (chirp_outl->task == CB_DISSEMINATE) && (chirp_outl->round >= 2))
            // {
            //     loradisc_config.lbt_channel_primary = (loradisc_config.lbt_channel_primary + LBT_CHANNEL_NUM - 1) % LBT_CHANNEL_NUM;
            // }
        }
        LoRaDS_SX1276SetChannel(loradisc_config.lora_freq + loradisc_config.lbt_channel_primary * CHANNEL_STEP);
        PRINTF("-------lbt_channel_primary:%d\n", loradisc_config.lbt_channel_primary);
#endif

        while (gpi_tick_compare_fast_extended(gpi_tick_fast_extended(), deadline) < 0)
            ;
#if ENERGEST_CONF_ON
        ENERGEST_OFF(ENERGEST_TYPE_CPU);
#endif

        /* used in mixer_write, and revalue before mixer round */
        loradisc_config.full_rank = 0;
        loradisc_config.full_column = UINT8_MAX;
        // rece_dissem_index = UINT16_MAX;

        deadline = mixer_start();

        if (loradisc_config.primitive == FLOODING)
            loradisc_read(the_data);
        i = 0;
        PRINTF_DISC("receiving data:0x%x\n", the_data[i++] << 24 | the_data[i++] << 16 | the_data[i++] << 8 | the_data[i++]);
        uint8_t recv_result = 0;
        if (the_data[0] != 0xFF)
            recv_result++;

        Gpi_Fast_Tick_Native resync_plus = GPI_TICK_MS_TO_FAST2(((loradisc_config.mx_slot_length_in_us * 5 / 2) * (loradisc_config.mx_round_length / 2 - 1) / 1000) - loradisc_config.mx_round_length * (loradisc_config.mx_slot_length_in_us / 1000));

        /* haven't received any synchronization packet, always on reception mode, leading to end a round later than synchronized node */
        if (!recv_result)
            deadline += (Gpi_Fast_Tick_Extended)(update_period - resync_plus);
        /* have synchronized to a node */
        else
            deadline += (Gpi_Fast_Tick_Extended)(update_period);
        while (gpi_tick_compare_fast_extended(gpi_tick_fast_extended(), deadline) < 0)
            ;
    }
}

void loradisc_packet_write(uint8_t node_id, uint8_t *the_data)
{
    // uint8_t loradisc_data_length;
    // uint8_t *loradisc_data;
    // if (loradisc_config.primitive != FLOODING)
    //     loradisc_data_length = loradisc_config.phy_payload_size - LORADISC_HEADER_LEN;
    // loradisc_data = (uint8_t *)malloc(loradisc_data_length);

    if (loradisc_config.primitive == FLOODING)
    {
        loradisc_write(NULL, the_data);
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

#if MX_LBT_ACCESS

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

#endif

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
