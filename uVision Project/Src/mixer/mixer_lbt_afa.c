//**************************************************************************************************
//**** Includes ************************************************************************************

#include "mixer_internal.h"

#if MX_LBT_AFA

#ifdef MX_CONFIG_FILE
	#include STRINGIFY(MX_CONFIG_FILE)
#endif

#include "gpi/olf.h"
#include GPI_PLATFORM_PATH(radio.h)

#include "L476RG_SX1276/mixer_transport.h"
//**************************************************************************************************
//***** Local Defines and Consts *******************************************************************

//**************************************************************************************************
//***** Local Typedefs and Class Declarations ******************************************************

//**************************************************************************************************
//***** Forward Declarations ***********************************************************************

//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************
// channels are using, eg: 1, 2, ..., 20
// uint8_t current_channel[AFA_CHANNEL_NUM];
// channles are not available in the current channels, eg: 0b 011 00000
// uint8_t current_channel_occupy;
// occupied channel has been added to the channel_bitmap, eg: 0b 100 00000
// uint8_t occupied_channel_flag;
const uint16_t	CODING_VECTOR_SIZE = sizeof_member(Packet, coding_vector);
uint8_t temp_coding_vector[sizeof_member(Packet, coding_vector)];
uint8_t channel_tx_used[AFA_CHANNEL_NUM];
uint8_t max_ch = AFA_CHANNEL_NUM - 1;
//**************************************************************************************************
//***** Global Variables ***************************************************************************

//**************************************************************************************************
//***** Local Functions ****************************************************************************

//**************************************************************************************************
//***** Global Functions ***************************************************************************
uint8_t assign_tx_channel(uint8_t tx_channel)
{
    ASSERT_CT(AFA_CHANNEL_NUM <= 8, available_channel_num_is_below_8);
	assert_reset(tx_channel < AFA_CHANNEL_NUM);
    // tx_channel = 2
    int8_t tx_channel_tmp;
    // 0b0001 1111->0b0001 1000
    // 0b0000 0111->0b0001 1000
    uint8_t current_channel_occupy_last = mx.current_channel_occupy & ~((1 << (tx_channel + 1)) - 1);
    tx_channel_tmp = gpi_get_lsb(current_channel_occupy_last);
    if (tx_channel_tmp >= 0)
        return tx_channel_tmp;
    else
    {
        // 0b0001 1111->0b0000 0011
        // 0b0001 1111  0b0000 1111
        uint8_t current_channel_occupy_first = mx.current_channel_occupy & ((1 << (tx_channel)) - 1);
        tx_channel_tmp = gpi_get_lsb(current_channel_occupy_first);
        if (tx_channel_tmp >= 0)
            return tx_channel_tmp;
    }
    // should not happen: in the noisy environment, all the channels are unavailable
    return (tx_channel);
}

uint8_t coding_vector_check(uint8_t *p_packet_coding)
{
    memset(temp_coding_vector, 0, CODING_VECTOR_SIZE);

    uint8_t *p_temp_coding = temp_coding_vector, *p_packet = p_packet_coding, *p_coding_vector_map = mx.coding_vector_map;
    uint8_t innovative_packet = 0;

    for ( ; p_temp_coding < &(temp_coding_vector[CODING_VECTOR_SIZE]); p_temp_coding++)
    {
        *(p_temp_coding) = (~ *(p_coding_vector_map++)) & (*(p_packet++));

        if(*p_temp_coding)
        {
            innovative_packet = 1;
            break;
        }
    }
    return innovative_packet;
}

uint8_t update_tx_channel(uint8_t tx_channel)
{
	assert_reset(tx_channel & mx.current_channel_occupy);
    channel_tx_used[tx_channel]++;
    uint8_t channel_is_full = 0;
    if (channel_tx_used[tx_channel] >= MAX_TX_COUNT)
    {
        channel_is_full = 1;
        mx.current_channel_occupy &= ~(1 << tx_channel);
        mx.current_channel_used_num[tx_channel]++;
    }
    return channel_is_full;
}

void update_rx_channel(uint8_t *current_channel_used_num)
{
    uint8_t i;
    for (i = 0; i < AFA_CHANNEL_NUM; i++)
    {
        if (channel_tx_used[i] == MAX_TX_COUNT)
        {
            if (mx.current_channel_used_num[i] < current_channel_used_num[i])
                mx.current_channel_used_num[i] = current_channel_used_num[i];
            if ((mx.current_channel_used_num[i] == MX_NUM_NODES))
            {
                mx.current_channel[i] = max_ch + 1;
                max_ch ++;
                mx.occupied_channel_flag = 0;
                mx.current_channel_occupy |= 1 << i;
                channel_tx_used[i] = 0;
            }
            break;
        }
    }
}

//**************************************************************************************************
//**************************************************************************************************

#endif	// MX_LBT_AFA
