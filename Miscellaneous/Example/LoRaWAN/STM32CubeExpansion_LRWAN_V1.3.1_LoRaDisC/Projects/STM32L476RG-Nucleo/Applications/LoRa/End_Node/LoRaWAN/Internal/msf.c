//**************************************************************************************************
//**** Includes ************************************************************************************

//**************************************************************************************************
// #include "hw.h"
// #include "lora.h"
#include "msf.h"
#include "loradisc.h"
#include "lorawan_internal.h"
#include "gpi/platform.h"
#include GPI_PLATFORM_PATH(radio.h)
#include GPI_PLATFORM_PATH(sx1276Regs_Fsk.h)
#include GPI_PLATFORM_PATH(sx1276Regs_LoRa.h)
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

//**************************************************************************************************
//***** Local Functions ****************************************************************************

//**************************************************************************************************

//**************************************************************************************************


//**************************************************************************************************
//***** Global Functions ***************************************************************************

void msf_start(uint8_t node_id)
{
    int16_t rssi_tmp[5000];
    uint32_t i = 0;
    uint8_t time_flag = 0;
    Gpi_Fast_Tick_Native packet_start, packet_end;
    if (node_id)
    {
        lorawan_listen_init(node_id);
    }
    else
    {
        LoRaDS_SX1276SetOpMode( RFLR_OPMODE_SLEEP );
        // loradisc_radio_config(12, 1, 14, 486300);
        loradisc_radio_config(12, 1, 14, 486200);
        chirp_isr.state = ISR_LORAWAN;
        LoRaDS_SX1276SetPublicNetwork(true);

        // LoRaDS_SX1276SetModem( modem );

        // LoRaDS_SX1276SetChannel( freq );

        LoRaDS_SX1276SetOpMode( RF_OPMODE_RECEIVER );
        int16_t rssi;
        while (1)
        {
            if (i >= 5000)
            {
                packet_end = gpi_tick_fast_native();
                break;
            }
            rssi = LoRaDS_SX1276ReadRssi( MODEM_LORA );
            if (rssi > -170)
            {
                if (!time_flag)
                {
                    packet_start = gpi_tick_fast_native();
                    time_flag = 1;
                }
                rssi_tmp[i++] = rssi;
            }
        }
    }
    for ( i = 0; i < 5000; i++)
    {
        printf("%d\n", rssi_tmp[i]);
    }
    printf("time:%lu, %lu, %lu\n", gpi_tick_fast_to_us(packet_end - packet_start), packet_start, packet_end);
}