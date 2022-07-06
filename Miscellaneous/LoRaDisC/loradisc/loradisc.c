//**************************************************************************************************
//**** Includes ************************************************************************************
//**************************************************************************************************
#include "loradisc.h"
// Mixer
#include "mixer_internal.h"
// DEBUG
#include "gpi/tools.h"
// discover
#include "gpi/olf.h"
// patch
#include "Commissioning.h"


//
//**************************************************************************************************
//***** Local Defines and Consts *******************************************************************
//**************************************************************************************************
//***** Local Typedefs and Class Declarations ******************************************************
//**************************************************************************************************
//***** Forward Declarations ***********************************************************************
//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************
// uint32_t dev_id_list[NODE_LENGTH] = {0x004A0038, 0x00300047, 0x004a0022}; // TODO: delft

volatile chirpbox_daemon_config __attribute((section (".ChirpBoxSettingSection"))) daemon_config ={{0x0042002C, 0x004E004A}, 0, 470000}; // TODO: sari
// volatile chirpbox_daemon_config __attribute((section (".ChirpBoxSettingSection"))) daemon_config ={{0x00440034, 0x0027002d, 0x004a0022, 0x00350017}, 0, 486300}; // TODO: tu graz
// volatile chirpbox_daemon_config __attribute((section (".ChirpBoxSettingSection"))) daemon_config ={{0x00440034, 0x0027002d, 0x00350017}, 0, 486300}; // TODO: tu graz

uint8_t MX_NUM_NODES_CONF;
uint8_t *data, data_length, *collect_data;
//**************************************************************************************************
//***** Global Variables ***************************************************************************
extern LoRaDisC_Config loradisc_config;
/* device id */
extern uint32_t __attribute__((section(".data"))) TOS_NODE_ID;
/* node id */
uint8_t node_id_allocate;
extern LoRaDisC_Discover_Config loradisc_discover_config;
LoRaDisC_Energy energy_stats;

uint32_t *collect_full_rank;
uint32_t collect_success;

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
    else
    {
        uint8_t i;
        uint8_t packet_correct_num = 0;
        for (i = 0; i < loradisc_config.mx_generation_size; i++)
        {
            void *p = mixer_read(i);
            if (NULL != p)
            {
                // valid packet with correct hash
                uint8_t packet_correct = 0;
                uint16_t calu_payload_hash, rece_hash;
                uint8_t *receive_payload = (uint8_t *)malloc(loradisc_config.matrix_payload_8.len);
                memcpy(receive_payload, p, loradisc_config.matrix_payload_8.len);
                calu_payload_hash = Chirp_RSHash((uint8_t *)receive_payload, loradisc_config.matrix_payload_8.len - 2);
                rece_hash = receive_payload[loradisc_config.matrix_payload_8.len - 2] << 8 | receive_payload[loradisc_config.matrix_payload_8.len - 1];
                if (((uint16_t)calu_payload_hash == rece_hash) && (rece_hash))
                packet_correct++;
                packet_correct_num++;
                free(receive_payload);
                /* for lorawan relay */
                if ((loradisc_config.primitive == COLLECTION) && (loradisc_discover_config.lorawan_bitmap & (1 << (node_id_allocate % 32))))
                    memcpy(data + i * data_length, p, data_length);
            }
        }
        PRINTF_DISC("RX OK num:%d\n", packet_correct_num);
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
    loradisc_config.timeout_flag = 0;
}

void loradisc_reconfig(uint8_t nodes_num, uint8_t data_length, Disc_Primitive primitive, uint8_t sf, uint8_t tp, uint32_t lora_frequency)
{
    // 1. radio config
    loradisc_radio_config(sf, 1, tp, lora_frequency);
    // 2. lbt config
#if MX_LBT_ACCESS
    lbt_init();
#endif
    // 3. packet structure config
    if (primitive == FLOODING)
    {
        loradisc_packet_config(nodes_num, 0, 0, primitive);
        loradisc_config.phy_payload_size = LORADISC_HEADER_LEN + (data_length > FLOODING_SURPLUS_LENGTH ? data_length - FLOODING_SURPLUS_LENGTH : 0);
    }
    else if (primitive == COLLECTION)
        loradisc_packet_config(nodes_num, nodes_num - loradisc_discover_config.lorawan_num, data_length + HASH_TAIL, primitive);
    // 4. slot config
    uint32_t packet_time = SX1276GetPacketTime(loradisc_config.lora_sf, loradisc_config.lora_bw, 1, 0, 8, LORADISC_HEADER_LEN);
    uint8_t hop_count;
    if (primitive == FLOODING)
        hop_count = fut_config.CUSTOM[FUT_DISCOVER_SLOTS];
    else
        hop_count = fut_config.CUSTOM[FUT_COLLECT_SLOTS]; // TODO:
    loradisc_slot_config(packet_time + 100000, hop_count, 1500000);
}

void loradisc_discover_init()
{
    memset(&loradisc_discover_config, 0, sizeof(loradisc_discover_config));
    loradisc_discover_config.discover_on = 1;
    loradisc_discover_config.lorawan_on = 1;
    loradisc_discover_config.node_id_bitmap = 0x00000001;

    loradisc_discover_config.lorawan_bitmap = fut_config.CUSTOM[FUT_LORAWAN_BITMAP]; // TODO:
    loradisc_discover_config.lorawan_num = gpi_popcnt_32(loradisc_discover_config.lorawan_bitmap);
    loradisc_discover_config.lorawan_duration = GPI_TICK_S_TO_SLOW(5); //TODO:
    loradisc_discover_config.lorawan_interval_s[node_id_allocate] = fut_config.CUSTOM[FUT_LORAWAN_INTERVAL_S]; // TODO:
    loradisc_discover_config.loradisc_collect_id = 0;
    collect_full_rank = (uint32_t *)malloc(sizeof(uint32_t) * fut_config.CUSTOM[FUT_COLLECT_SLOTS_MAX]);
    memset(collect_full_rank, 0, sizeof(uint32_t) * fut_config.CUSTOM[FUT_COLLECT_SLOTS_MAX]);
    collect_success = 0;
}

void loradisc_discover_update_initiator()
{
    if (loradisc_discover_config.discover_on)
        loradisc_config.initiator = gpi_get_msb_32(loradisc_discover_config.node_id_bitmap);
    /* if collection, the initiator is the node with the smallest LoRaDisC node ID */
    else if (loradisc_discover_config.collect_on)
        loradisc_config.initiator = loradisc_discover_config.lorawan_num;
    printf("--- LoRaDisC initiator is node %d\n", loradisc_config.initiator);
}


/* one lorawan node initiates the flooding round */
void loradisc_discover(uint16_t lorawan_interval_s)
{
    Gpi_Slow_Tick_Extended discover_start = gpi_tick_slow_extended();
    loradisc_discover_config.loradisc_on = 1;

    int32_t gap_to_node_0 = loradisc_discover_config.lorawan_diff;
    // update data
    data = NULL;
    data_length = sizeof(uint8_t) + sizeof(uint16_t) + sizeof(Gpi_Slow_Tick_Extended);
    assert_reset(data_length >= FLOODING_SURPLUS_LENGTH);
    data = (uint8_t *)malloc(data_length);
    data[0] = node_id_allocate;
    uint8_t i;
    for (i = 0; i < sizeof(uint16_t); i++)
        data[sizeof(uint8_t) + i] = lorawan_interval_s >> 8 * i;
    for (i = 0; i < sizeof(Gpi_Slow_Tick_Extended); i++)
        data[sizeof(uint8_t) + sizeof(uint16_t) + i] = gap_to_node_0 >> 8 * i;

    /* start loradisc */
    loradisc_start(FLOODING);

    /* update node id bitmap */
    if (loradisc_config.recv_ok)
    {
        Gpi_Slow_Tick_Extended discover_end = gpi_tick_slow_extended();
        Gpi_Slow_Tick_Extended discover_duration = discover_end - discover_start;
        // printf("discover_duration:%lu, %lu, %lu\n", gpi_tick_slow_to_us(discover_duration), discover_duration, loradisc_discover_config.discover_duration_slow);

        /* read data */
        uint8_t discover_node = data[0];
        /* update discover data */
        if (discover_node + 1 < loradisc_discover_config.lorawan_num)
            loradisc_discover_config.node_id_bitmap |= 1 << (discover_node + 1);
        loradisc_discover_config.lorawan_interval_s[discover_node] = data[sizeof(uint8_t)] | data[sizeof(uint8_t) + 1] << 8;
        gap_to_node_0 = data[sizeof(uint8_t) + 2] | data[sizeof(uint8_t) + 3] << 8 | data[sizeof(uint8_t) + 4] << 16 | data[sizeof(uint8_t) + 5] << 24;

        /* update lorawan begin */
        if (discover_node != node_id_allocate)
        {
            if (!discover_node) // node 0
                loradisc_discover_config.lorawan_begin[discover_node] = gpi_tick_slow_extended() - loradisc_discover_config.discover_duration_slow - loradisc_discover_config.lorawan_duration;
            else // node x
                loradisc_discover_config.lorawan_begin[discover_node] = loradisc_discover_config.lorawan_begin[0] + gap_to_node_0;
        }

        /* update lorawan_diff compared to node 0 */
        if ((!discover_node) && (node_id_allocate) && ((loradisc_discover_config.lorawan_bitmap & (1 << (node_id_allocate % 32)))))
            loradisc_discover_config.lorawan_diff = (int32_t)((int32_t)loradisc_discover_config.lorawan_begin[node_id_allocate] - (int32_t)loradisc_discover_config.lorawan_begin[discover_node]);

        /* end discover when all nodes are collected */
        if (discover_node + 1 >= loradisc_discover_config.lorawan_num)
            loradisc_discover_config.discover_on = 0;

        printf("--- discover initiator is node %d, discover's lorawan_diff is %d, node's lorawan_diff is %d, node's lorawan_interval_s is %d, node_bitmap: %x\n", discover_node, (int32_t)(gap_to_node_0), (int32_t)(loradisc_discover_config.lorawan_diff), loradisc_discover_config.lorawan_interval_s[discover_node], loradisc_discover_config.node_id_bitmap);
    }
    loradisc_discover_update_initiator();

    if (data != NULL)
        free(data);
    if (payload_distribution != NULL)
        free(payload_distribution);
}

uint8_t loradisc_collect()
{
    if (loradisc_discover_config.loradisc_collect_id >= fut_config.CUSTOM[FUT_COLLECT_SLOTS_MAX])
    {
        loradisc_discover_config.collect_on = 0;
        return 0;
    }

    loradisc_discover_config.loradisc_on = 1;

    // TODO: data from sensor for gateway
    uint32_t payload_hash;
    uint8_t dissem_flag;
    // update data
    data = NULL;
    data_length = sizeof(TOS_NODE_ID) + sizeof(payload_hash) + sizeof(dissem_flag);
    assert_reset(data_length >= FLOODING_SURPLUS_LENGTH);
    /* first part of data is the device address */
    data = (uint8_t *)malloc(data_length);
    memcpy(data, &TOS_NODE_ID, sizeof(TOS_NODE_ID));
    /* currently, uplink payload is the hash data */
	payload_hash = Chirp_RSHash((uint8_t *)&(TOS_NODE_ID), sizeof(TOS_NODE_ID));
    memcpy(data + sizeof(TOS_NODE_ID), &payload_hash, sizeof(uint32_t));
    /* do the node needs lorawan ack from gateway? */
    dissem_flag = 0;
    data[data_length - sizeof(dissem_flag)] = dissem_flag;

    collect_data = NULL;
    if (loradisc_discover_config.lorawan_bitmap & (1 << (node_id_allocate % 32)))
    {
        collect_data = (uint8_t *)malloc(data_length * (MX_NUM_NODES_CONF - loradisc_discover_config.lorawan_num));
        printf("length:%d\n", data_length * (MX_NUM_NODES_CONF - loradisc_discover_config.lorawan_num));
    }

    loradisc_start(COLLECTION);

    if (data != NULL)
        free(data);
    if (payload_distribution != NULL)
        free(payload_distribution);

    printf("slot_full_rank:%x\n", mx.stat_counter.slot_full_rank);
    if(!mx.stat_counter.slot_full_rank)
        mx.stat_counter.slot_full_rank = fut_config.CUSTOM[FUT_COLLECT_SLOTS];
    else
        collect_success++;
    collect_full_rank[loradisc_discover_config.loradisc_collect_id] = mx.stat_counter.slot_full_rank;

    /* start LoRaWAN relay with LoRaDisC */
    loradisc_discover_config.loradisc_lorawan_on = 1;
    loradisc_discover_config.loradisc_on = 0;
    loradisc_discover_config.lorawan_relay_id = 0;

    // loradisc_discover_config.dissem_on = check_if_dissem(collect_data);//TODO:
    loradisc_discover_config.dissem_on = 0;
    loradisc_discover_config.loradisc_collect_id++;

    free(mx.request);

    if (!(loradisc_discover_config.lorawan_bitmap & (1 << (node_id_allocate % 32))))
    {
        free(mx.matrix[0]);
        free(collect_data);
        if (loradisc_discover_config.loradisc_collect_id >= fut_config.CUSTOM[FUT_COLLECT_SLOTS_MAX])
            return 0;
    }

    return 1;
}

/* loradisc relay with lorawan TODO: assume each loradisc node has one packet in each round */
// ---------------------------------------------------------------------------------
uint32_t lorawan_relay_node_id_allocate(uint8_t relay_id)
{
    uint8_t node_id_start = (((MX_NUM_NODES_CONF - loradisc_discover_config.lorawan_num) / loradisc_discover_config.lorawan_num) * node_id_allocate) + ((int8_t)(((MX_NUM_NODES_CONF - loradisc_discover_config.lorawan_num) % loradisc_discover_config.lorawan_num) - node_id_allocate) > 0 ? node_id_allocate : ((MX_NUM_NODES_CONF - loradisc_discover_config.lorawan_num) % loradisc_discover_config.lorawan_num));

    uint32_t node_id = 0;
    uint8_t i = 0;
    if (collect_data != NULL)
    {
        printf("collect_data:%x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x\n", collect_data[i++], collect_data[i++], collect_data[i++], collect_data[i++], collect_data[i++], collect_data[i++], collect_data[i++], collect_data[i++], collect_data[i++], collect_data[i++], collect_data[i++], collect_data[i++], collect_data[i++], collect_data[i++], collect_data[i++], collect_data[i++], collect_data[i++], collect_data[i++]);
        memcpy(&node_id, collect_data + (relay_id + node_id_start) * data_length, sizeof(node_id));
        printf("node_id:%d, %x, %d\n", relay_id + node_id_start, node_id, data_length);
    }
    return node_id;
}

uint8_t lorawan_relay_collect(uint8_t relay_id)
{
    /* allocate the LoRaDisC packets to each LoRaWAN nodes */
    uint8_t packet_num = ((MX_NUM_NODES_CONF - loradisc_discover_config.lorawan_num) / loradisc_discover_config.lorawan_num) + ((((MX_NUM_NODES_CONF - loradisc_discover_config.lorawan_num) % loradisc_discover_config.lorawan_num) > node_id_allocate) ? 1 : 0);

    uint8_t node_id_start = (((MX_NUM_NODES_CONF - loradisc_discover_config.lorawan_num) / loradisc_discover_config.lorawan_num) * node_id_allocate) + ((int8_t)(((MX_NUM_NODES_CONF - loradisc_discover_config.lorawan_num) % loradisc_discover_config.lorawan_num) - node_id_allocate) > 0 ? node_id_allocate : ((MX_NUM_NODES_CONF - loradisc_discover_config.lorawan_num) % loradisc_discover_config.lorawan_num));

    /* the collected packet is decoded correctly */
    if (mx.matrix[relay_id + node_id_start]->birth_slot != UINT16_MAX)
        lorawan_loradisc_send(collect_data + (node_id_start + relay_id) * data_length, data_length);
    printf("packet_num:%d, %d, %d\n", packet_num, relay_id, relay_id + node_id_start);
    if ((int8_t)(relay_id - packet_num + 1) >= 0)
    {
        free(collect_data);
        collect_data = NULL;
        free(mx.matrix[0]);
        mx.matrix[0] = NULL;
        return 1;
    }
    else
        return 0;
}

uint8_t lorawan_relay_max_packetnum()
{
    uint8_t packet_num = ((MX_NUM_NODES_CONF - loradisc_discover_config.lorawan_num) / loradisc_discover_config.lorawan_num) + ((((MX_NUM_NODES_CONF - loradisc_discover_config.lorawan_num) % loradisc_discover_config.lorawan_num) > 0) ? 1 : 0);
    return packet_num;
}
// ---------------------------------------------------------------------------------

void loradisc_node_id()
{
    node_id_allocate = logical_node_id(TOS_NODE_ID, daemon_config.UID_list); // node id
}

void loradisc_start(Disc_Primitive primitive)
{
    printf("--- LoRaDisC start\n");
    // TODO: config
    if (primitive == FLOODING)
        loradisc_reconfig(MX_NUM_NODES_CONF, data_length, primitive, fut_config.CUSTOM[FUT_DISCOVER_SF], fut_config.CUSTOM[FUT_UPLINK_POWER], daemon_config.Frequency);
    else
        loradisc_reconfig(MX_NUM_NODES_CONF, data_length, primitive, fut_config.CUSTOM[FUT_LORADISC_SF], fut_config.CUSTOM[FUT_UPLINK_POWER], daemon_config.Frequency);
    // payload distribution
    loradisc_lorawan_payload_distribution();
    /* initiator */
    loradisc_discover_update_initiator();

    // round start:
    chirp_isr.state = ISR_MIXER;
    Gpi_Fast_Tick_Extended deadline;
    Gpi_Fast_Tick_Native update_period = GPI_TICK_MS_TO_FAST2(((loradisc_config.mx_period_time_s * 1000) / 1) - loradisc_config.mx_round_length * (loradisc_config.mx_slot_length_in_us / 1000));
    deadline = gpi_tick_fast_extended();
    loradisc_config.recv_ok = 0;
    while ((!loradisc_config.recv_ok) && (loradisc_discover_config.loradisc_on))
    {
        /* node is the receiver, and cannot complete LoRaDisC before LoRaWAN, end LoRaDisC round immediately */
        if ((loradisc_discover_config.discover_on) && (!reset_discover_slot_num()))
        {
            /* receiver time limited, end the discover */
            loradisc_discover_config.loradisc_on = 0;
            loradisc_discover_config.lorawan_on = 1;
            break;
        }

        loradisc_init();
        // gpi_radio_init();
        /* init mixer */
        mixer_init(node_id_allocate);
        loradisc_packet_write(node_id_allocate, data);
        /* arm mixer, node 0 = initiator
        start first round with infinite scan
        -> nodes join next available round, does not require simultaneous boot-up */
        mixer_arm(((loradisc_config.initiator == node_id_allocate) ? MX_ARM_INITIATOR : 0) | ((1 == 0) ? MX_ARM_INFINITE_SCAN : 0));
        /* delay initiator a bit
        -> increase probability that all nodes are ready when initiator starts the round
        -> avoid problems in view of limited deadline accuracy */
        if (loradisc_config.initiator == node_id_allocate)
            deadline += (Gpi_Fast_Tick_Extended)1 * loradisc_config.mx_slot_length / 10;
#if MX_LBT_ACCESS
        lbt_update();
#endif
        /* start when deadline reached
        ATTENTION: don't delay after the polling loop (-> print before) */
        while (gpi_tick_compare_fast_extended(gpi_tick_fast_extended(), deadline) < 0)
            ;

        deadline = mixer_start();

        if (loradisc_discover_config.loradisc_on)
        {
            if (loradisc_config.primitive == FLOODING)
            {
                loradisc_read(data);
                uint8_t i = 0;
                if ((data[0] != 0xFF) && (!loradisc_config.timeout_flag))
                {
                    loradisc_config.recv_ok++;
                }
                if (loradisc_config.recv_ok)
                {
                    log_to_flash("FLOODING OK\n");
                    log_flush();
                }
                // PRINTF_DISC("FLOODING OK\n");
                // Gpi_Fast_Tick_Native resync_plus = GPI_TICK_MS_TO_FAST2(((loradisc_config.mx_slot_length_in_us * 5 / 2) * (loradisc_config.mx_round_length / 2 - 1) / 1000) - loradisc_config.mx_round_length * (loradisc_config.mx_slot_length_in_us / 1000));
                /* haven't received any synchronization packet, always on reception mode, leading to end a round later than synchronized node */
                // if (!loradisc_config.recv_ok)
                //     deadline += (Gpi_Fast_Tick_Extended)(update_period - resync_plus);
                // /* have synchronized to a node */
                // else
                // {
                //     deadline += (Gpi_Fast_Tick_Extended)(update_period);
                //     PRINTF_DISC("RX OK:0x%x\n", data[i++] << 24 | data[i++] << 16 | data[i++] << 8 | data[i++]);
                // }
            }
            else if (loradisc_config.primitive == COLLECTION)
            {
                // deadline += (Gpi_Fast_Tick_Extended)(update_period);
                loradisc_read(collect_data);
                loradisc_config.recv_ok++;
            }
            while (gpi_tick_compare_fast_extended(gpi_tick_fast_extended(), deadline) < 0);
        }
    }
}
void loradisc_packet_write(uint8_t node_id, uint8_t *data)
{
    if (loradisc_config.primitive == FLOODING)
    {
        if (loradisc_config.initiator == node_id_allocate)
            loradisc_write(NULL, data);
    }
    else
    {
        uint8_t i;
        for (i = 0; i < loradisc_config.mx_generation_size; i++)
        {
            if (payload_distribution[i] == node_id)
                loradisc_write(i, (uint8_t *)data);
        }
    }
}
void loradisc_radio_config(uint8_t lora_spreading_factor, uint8_t lora_codingrate, int8_t tx_output_power, uint32_t lora_frequency)
{
    memset(&loradisc_config + offsetof(LoRaDisC_Config, lora_sf), 0, sizeof(loradisc_config) - offsetof(LoRaDisC_Config, lora_sf));
    // default:
    loradisc_config.lora_bw = LoRaDisC_DEFAULT_BW;
    loradisc_config.lora_plen = LoRaDisC_PREAMBLE_LENGTH;
    // custom:
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
    // to identify the loradisc packet
    if (loradisc_config.primitive != FLOODING)
        loradisc_config.packet_hash = DISC_HEADER;
    else
        loradisc_config.packet_hash = FLOODING_HEADER;
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

    loradisc_config.timeout_flag = 0;
    loradisc_config.initiator = 0;
}
/**
 * @description: To allocate payload among nodes according to the type of mixer (dissemination / collection)
 * @param mx_task: CB_DISSEMINATE / CB_COLLECT
 * @return: None
 */
void loradisc_payload_distribution()
{
    uint8_t i;
    payload_distribution = NULL;
    if (loradisc_config.primitive == DISSEMINATION)
    {
        payload_distribution = (uint8_t *)malloc(loradisc_config.mx_generation_size);
        /* Only the initiator has packets */
        for (i = 0; i < loradisc_config.mx_generation_size; i++)
            payload_distribution[i] = 0;
    }
    else if (loradisc_config.primitive == COLLECTION)
    {
        /* Each node has a packet */
        assert_reset((loradisc_config.mx_num_nodes == loradisc_config.mx_generation_size));
        payload_distribution = (uint8_t *)malloc(loradisc_config.mx_num_nodes);
        for (i = 0; i < loradisc_config.mx_num_nodes; i++)
            payload_distribution[i] = i;
    }
}

/**
 * @description: To allocate payload among nodes for LoRaWAN packets (dissemination / collection)
 * @param mx_task: CB_DISSEMINATE / CB_COLLECT
 * @return: None
 */
void loradisc_lorawan_payload_distribution()
{
    uint8_t i;
    payload_distribution = NULL;
    if (loradisc_config.primitive == DISSEMINATION)
    {
        payload_distribution = (uint8_t *)malloc(loradisc_config.mx_generation_size);
        /* Only the initiator has packets */
        for (i = 0; i < loradisc_config.mx_generation_size; i++)
            payload_distribution[i] = 0;
    }
    else if (loradisc_config.primitive == COLLECTION)
    {
        /* Each LoRaDisC node has a packet */
        assert_reset((loradisc_config.mx_num_nodes == loradisc_config.mx_generation_size + loradisc_discover_config.lorawan_num));
        payload_distribution = (uint8_t *)malloc(loradisc_config.mx_generation_size);
        for (i = 0; i < loradisc_config.mx_generation_size; i++)
            payload_distribution[i] = i + loradisc_discover_config.lorawan_num;
    }
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
