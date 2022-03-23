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


//**************************************************************************************************
//***** Global Variables ***************************************************************************
extern LoRaDisC_Config loradisc_config;

//**************************************************************************************************
//***** Local Functions ****************************************************************************

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
    uint8_t MX_NUM_NODES_CONF;
    uint8_t network_num_nodes = MX_NUM_NODES_CONF;
    printf("dev_id:%x\n", dev_id);

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
