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
uint32_t dev_id_list[NODE_LENGTH] = {0x004a0022, 0x00350017}; // TODO:
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

    uint8_t hop_count = network_num_nodes > 10? 6 : 4;

    // radio config
    loradisc_radio_config(12, 1, 14, CN470_FREQUENCY);
    // packet config
    loradisc_packet_config(MX_NUM_NODES_CONF, 0, 0, FLOODING);


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
                            PADDING_SIZE((loradisc_config.mx_generation_size + 7) / 8)
                            + PADDING_SIZE(loradisc_config.mx_payload_size)
            #if (MX_REQUEST || MX_SMART_SHUTDOWN_MAP)
                            - ((loradisc_config.mx_generation_size + 7) / 8)
            #endif
                            );
    loradisc_config.rand.pos = loradisc_config._padding_2.pos + loradisc_config._padding_2.len;
    loradisc_config.rand.len = 1;
    loradisc_config._padding_3.pos = loradisc_config.rand.pos + loradisc_config.rand.len;
    loradisc_config._padding_3.len = PADDING_SIZE(
                                ((loradisc_config.mx_generation_size + 7) / 8) +	// coding_vector
                                loradisc_config.mx_payload_size +					// payload
#if (MX_REQUEST || MX_SMART_SHUTDOWN_MAP)
                                ((loradisc_config.mx_generation_size + 7) / 8) +	// info_vector
    #if !GPI_ARCH_IS_BOARD(TMOTE)
                                PADDING_MAX(0,						// _padding_2
                                    PADDING_SIZE((loradisc_config.mx_generation_size + 7) / 8)
                                    + PADDING_SIZE(loradisc_config.mx_payload_size)
                                    - ((loradisc_config.mx_generation_size + 7) / 8)
                                    ) +
    #endif
#else
    #if !GPI_ARCH_IS_BOARD(TMOTE)
                                PADDING_MAX(0,						// _padding_2
                                    PADDING_SIZE((loradisc_config.mx_generation_size + 7) / 8)
                                    + PADDING_SIZE(loradisc_config.mx_payload_size)) +
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