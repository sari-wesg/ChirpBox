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
