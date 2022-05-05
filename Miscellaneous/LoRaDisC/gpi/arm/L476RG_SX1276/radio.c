
//**************************************************************************************************
//**** Includes ************************************************************************************
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "gpi/platform.h"
#include GPI_PLATFORM_PATH(radio.h)
#include GPI_PLATFORM_PATH(sx1276Regs_Fsk.h)
#include GPI_PLATFORM_PATH(sx1276Regs_LoRa.h)
// TODO:
#include "mixer/mixer_internal.h"
//**************************************************************************************************
//***** Local Defines and Consts *******************************************************************

//**************************************************************************************************
//***** Local Typedefs and Class Declarations ******************************************************
/*sx1276mb1mas declaration---------------------------------------------------------------*/
static void SX1276AntSwInit( void );
static void SX1276AntSwDeInit( void );
//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************

/*sx1276mb1mas---------------------------------------------------------------------------*/
static bool RadioIsActive = false;
//**************************************************************************************************
//***** Global Functions ***************************************************************************
/*sx1276mb1mas---------------------------------------------------------------------------*/
void LoRaDS_SX1276IoInit( void )
{
    GPIO_InitTypeDef initStruct={0};

    initStruct.Mode = GPIO_MODE_IT_RISING;
    initStruct.Pull = GPIO_PULLDOWN;
    initStruct.Speed = GPIO_SPEED_HIGH;

    HW_GPIO_Init( RADIO_DIO_0_PORT, RADIO_DIO_0_PIN, &initStruct );
    HW_GPIO_Init( RADIO_DIO_1_PORT, RADIO_DIO_1_PIN, &initStruct );
    HW_GPIO_Init( RADIO_DIO_2_PORT, RADIO_DIO_2_PIN, &initStruct );
    HW_GPIO_Init( RADIO_DIO_3_PORT, RADIO_DIO_3_PIN, &initStruct );
}

void LoRaDS_SX1276IoIrqInit( DioIrqHandler **irqHandlers )
{
    HW_GPIO_SetIrq( RADIO_DIO_0_PORT, RADIO_DIO_0_PIN, IRQ_HIGH_PRIORITY, irqHandlers[0] );
    HW_GPIO_SetIrq( RADIO_DIO_1_PORT, RADIO_DIO_1_PIN, IRQ_HIGH_PRIORITY, irqHandlers[1] );
    HW_GPIO_SetIrq( RADIO_DIO_2_PORT, RADIO_DIO_2_PIN, IRQ_HIGH_PRIORITY, irqHandlers[2] );
    HW_GPIO_SetIrq( RADIO_DIO_3_PORT, RADIO_DIO_3_PIN, IRQ_HIGH_PRIORITY, irqHandlers[3] );
}

void LoRaDS_SX1276IoDeInit( void )
{
    GPIO_InitTypeDef initStruct={0};

    initStruct.Mode = GPIO_MODE_IT_RISING ;
    initStruct.Pull = GPIO_PULLDOWN;

    HW_GPIO_Init( RADIO_DIO_0_PORT, RADIO_DIO_0_PIN, &initStruct );
    HW_GPIO_Init( RADIO_DIO_1_PORT, RADIO_DIO_1_PIN, &initStruct );
    HW_GPIO_Init( RADIO_DIO_2_PORT, RADIO_DIO_2_PIN, &initStruct );
    HW_GPIO_Init( RADIO_DIO_3_PORT, RADIO_DIO_3_PIN, &initStruct );
}

void LoRaDS_SX1276SetRfTxPower( int8_t power )
{
    uint8_t paConfig = 0;
    uint8_t paDac = 0;

    paConfig = LoRaDS_SX1276Read( REG_PACONFIG );
    paDac = LoRaDS_SX1276Read( REG_PADAC );

    paConfig = ( paConfig & RF_PACONFIG_PASELECT_MASK ) | LoRaDS_SX1276GetPaSelect( SX1276.Settings.Channel );
    paConfig = ( paConfig & RF_PACONFIG_MAX_POWER_MASK ) | 0x70;

    if( ( paConfig & RF_PACONFIG_PASELECT_PABOOST ) == RF_PACONFIG_PASELECT_PABOOST )
    {
        if( power > 17 )
        {
            paDac = ( paDac & RF_PADAC_20DBM_MASK ) | RF_PADAC_20DBM_ON;
        }
        else
        {
            paDac = ( paDac & RF_PADAC_20DBM_MASK ) | RF_PADAC_20DBM_OFF;
        }
        if( ( paDac & RF_PADAC_20DBM_ON ) == RF_PADAC_20DBM_ON )
        {
            if( power < 5 )
            {
                power = 5;
            }
            if( power > 20 )
            {
                power = 20;
            }
            paConfig = ( paConfig & RF_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power - 5 ) & 0x0F );
        }
        else
        {
            if( power < 2 )
            {
                power = 2;
            }
            if( power > 17 )
            {
                power = 17;
            }
            paConfig = ( paConfig & RF_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power - 2 ) & 0x0F );
        }
        // LoRaDS_SX1276Write( REG_LR_OCP, 0x2F );
    }
    else
    {
        paDac = ( paDac & RF_PADAC_20DBM_MASK ) | RF_PADAC_20DBM_OFF;
        if( power < -1 )
        {
            power = -1;
        }
        if( power > 14 )
        {
            power = 14;
        }
        paConfig = ( paConfig & RF_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power + 1 ) & 0x0F );
    }
    LoRaDS_SX1276Write( REG_PACONFIG, paConfig );
    LoRaDS_SX1276Write( REG_PADAC, paDac );
}

uint8_t LoRaDS_SX1276GetPaSelect( uint32_t channel )
{
    return RF_PACONFIG_PASELECT_RFO;
    // return RF_PACONFIG_PASELECT_PABOOST;
}

void LoRaDS_SX1276SetAntSwLowPower( bool status )
{
    if( RadioIsActive != status )
    {
        RadioIsActive = status;

        if( status == false )
        {
            SX1276AntSwInit( );
        }
        else
        {
            SX1276AntSwDeInit( );
        }
    }
}

static void SX1276AntSwInit( void )
{
    GPIO_InitTypeDef initStruct={0};

    initStruct.Mode =GPIO_MODE_OUTPUT_PP;
    initStruct.Pull = GPIO_NOPULL;
    initStruct.Speed = GPIO_SPEED_HIGH;

    HW_GPIO_Init( RADIO_ANT_SWITCH_PORT, RADIO_ANT_SWITCH_PIN, &initStruct  );
    HW_GPIO_Write( RADIO_ANT_SWITCH_PORT, RADIO_ANT_SWITCH_PIN, RADIO_ANT_SWITCH_SET_RX);
}

static void SX1276AntSwDeInit( void )
{
    GPIO_InitTypeDef initStruct={0};

    initStruct.Mode = GPIO_MODE_OUTPUT_PP;
    initStruct.Pull = GPIO_NOPULL;
    initStruct.Speed = GPIO_SPEED_HIGH;

    HW_GPIO_Init(  RADIO_ANT_SWITCH_PORT, RADIO_ANT_SWITCH_PIN, &initStruct );
    HW_GPIO_Write( RADIO_ANT_SWITCH_PORT, RADIO_ANT_SWITCH_PIN, 0);
}

void LoRaDS_SX1276SetAntSw( uint8_t opMode )
{
    switch( opMode )
    {
    case RFLR_OPMODE_TRANSMITTER:
        HW_GPIO_Write( RADIO_ANT_SWITCH_PORT, RADIO_ANT_SWITCH_PIN, RADIO_ANT_SWITCH_SET_TX);
        break;
    case RFLR_OPMODE_RECEIVER:
    case RFLR_OPMODE_RECEIVER_SINGLE:
    case RFLR_OPMODE_CAD:
    default:
        HW_GPIO_Write( RADIO_ANT_SWITCH_PORT, RADIO_ANT_SWITCH_PIN, RADIO_ANT_SWITCH_SET_RX);
        break;
    }
}
/*sx1276---------------------------------------------------------------------------*/
typedef struct
{
    RadioModems_t Modem;
    uint8_t       Addr;
    uint8_t       Value;
}RadioRegisters_t;

typedef struct
{
    uint32_t bandwidth;
    uint8_t  RegValue;
}FskBandwidth_t;

const RadioRegisters_t LoRaDS_RadioRegsInit[] = RADIO_INIT_REGISTERS_VALUE;

const FskBandwidth_t LoRaDS_FskBandwidths[] =
{
    { 2600  , 0x17 },
    { 3100  , 0x0F },
    { 3900  , 0x07 },
    { 5200  , 0x16 },
    { 6300  , 0x0E },
    { 7800  , 0x06 },
    { 10400 , 0x15 },
    { 12500 , 0x0D },
    { 15600 , 0x05 },
    { 20800 , 0x14 },
    { 25000 , 0x0C },
    { 31300 , 0x04 },
    { 41700 , 0x13 },
    { 50000 , 0x0B },
    { 62500 , 0x03 },
    { 83333 , 0x12 },
    { 100000, 0x0A },
    { 125000, 0x02 },
    { 166700, 0x11 },
    { 200000, 0x09 },
    { 250000, 0x01 },
    { 300000, 0x00 }, // Invalid Bandwidth
};

static RadioEvents_t *RadioEvents;

uint8_t RxTxBuffer[RX_BUFFER_SIZE];

SX1276_t SX1276;

DioIrqHandler *LoRaDS_DioIrq[] = { SX1276OnDio0Irq, LoRaDS_SX1276OnDio1Irq, LoRaDS_SX1276OnDio2Irq, SX1276OnDio3Irq, LoRaDS_SX1276OnDio4Irq};
/*---------------------------------------------------------------------------*/
uint32_t LoRaDS_SX1276Init()
{
    uint8_t i;
    LoRaDS_SX1276IoIrqInit( LoRaDS_DioIrq );
    HAL_NVIC_DisableIRQ( EXTI4_IRQn );

    LoRaDS_SX1276Reset( );
    // Clear dio3 IRQ (PB4), as LoRaDS_SX1276Reset will generate dio3 IRQ
    __HAL_GPIO_EXTI_CLEAR_FLAG(RADIO_DIO_3_PIN);
    HAL_NVIC_EnableIRQ( EXTI4_IRQn );

    LoRaDS_SX1276SetOpMode( RF_OPMODE_SLEEP );

    // LoRaBoardCallbacks->SX1276BoardIoIrqInit( LoRaDS_DioIrq );
    // LoRaDS_SX1276IoIrqInit( LoRaDS_DioIrq );

    for( i = 0; i < sizeof( LoRaDS_RadioRegsInit ) / sizeof( RadioRegisters_t ); i++ )
    {
        LoRaDS_SX1276SetModem( LoRaDS_RadioRegsInit[i].Modem );
        LoRaDS_SX1276Write( LoRaDS_RadioRegsInit[i].Addr, LoRaDS_RadioRegsInit[i].Value );
    }
    LoRaDS_SX1276SetModem( MODEM_LORA );

    // Launch Rx chain calibration
    LoRaDS_SX1276Write( REG_IMAGECAL, ( LoRaDS_SX1276Read( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_MASK ) | RF_IMAGECAL_IMAGECAL_START );
    while( ( LoRaDS_SX1276Read( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_RUNNING ) == RF_IMAGECAL_IMAGECAL_RUNNING )
    {
    }

    SX1276.Settings.State = RF_IDLE;

    return ( uint32_t )(BOARD_WAKEUP_TIME + RADIO_WAKEUP_TIME);// BOARD_WAKEUP_TIME;
}

RadioState_t LoRaDS_SX1276GetStatus( void )
{
    return SX1276.Settings.State;
}

void LoRaDS_SX1276SetChannel( uint32_t freq )
{
    uint32_t channel;

    SX1276.Settings.Channel = freq;

    SX_FREQ_TO_CHANNEL( channel, freq );

    LoRaDS_SX1276Write( REG_FRFMSB, ( uint8_t )( ( channel >> 16 ) & 0xFF ) );
    LoRaDS_SX1276Write( REG_FRFMID, ( uint8_t )( ( channel >> 8 ) & 0xFF ) );
    LoRaDS_SX1276Write( REG_FRFLSB, ( uint8_t )( channel & 0xFF ) );
}

bool LoRaDS_SX1276IsChannelFree( RadioModems_t modem, uint32_t freq, int16_t rssiThresh, uint32_t maxCarrierSenseTime )
{
    bool status = true;
    int16_t rssi = 0;
    uint32_t carrierSenseTime = 0;

    LoRaDS_SX1276SetModem( modem );

    LoRaDS_SX1276SetChannel( freq );

    LoRaDS_SX1276SetOpMode( RF_OPMODE_RECEIVER );


    // carrierSenseTime = TimerGetCurrentTime( );
    carrierSenseTime = Get_Current_LPTIM_Tick();

    // Perform carrier sense for maxCarrierSenseTime
    // while( TimerGetElapsedTime( carrierSenseTime ) < maxCarrierSenseTime )
    while( ElapsedTick( carrierSenseTime ) < LPTIM_ms2Tick( maxCarrierSenseTime ) )
    {
        rssi = LoRaDS_SX1276ReadRssi( modem );

        if( rssi > rssiThresh )
        {
            status = false;
            break;
        }
    }
    LoRaDS_SX1276SetSleep( );
    return status;
}

void LoRaDS_RxChainCalibration( void )
{
    uint8_t regPaConfigInitVal;
    uint32_t initialFreq;
    uint32_t channel;

    // Save context
    regPaConfigInitVal = LoRaDS_SX1276Read( REG_PACONFIG );

    channel = ( ( ( uint32_t )LoRaDS_SX1276Read( REG_FRFMSB ) << 16 ) |
                ( ( uint32_t )LoRaDS_SX1276Read( REG_FRFMID ) << 8 ) |
                ( ( uint32_t )LoRaDS_SX1276Read( REG_FRFLSB ) ) );

    SX_CHANNEL_TO_FREQ(channel, initialFreq);

    // Cut the PA just in case, RFO output, power = -1 dBm
    LoRaDS_SX1276Write( REG_PACONFIG, 0x00 );

    // Launch Rx chain calibration for LF band
    LoRaDS_SX1276Write( REG_IMAGECAL, ( LoRaDS_SX1276Read( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_MASK ) | RF_IMAGECAL_IMAGECAL_START );
    while( ( LoRaDS_SX1276Read( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_RUNNING ) == RF_IMAGECAL_IMAGECAL_RUNNING )
    {
    }

    // Sets a Frequency in HF band
    LoRaDS_SX1276SetChannel( 868000000 );

    // Launch Rx chain calibration for HF band
    LoRaDS_SX1276Write( REG_IMAGECAL, ( LoRaDS_SX1276Read( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_MASK ) | RF_IMAGECAL_IMAGECAL_START );
    while( ( LoRaDS_SX1276Read( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_RUNNING ) == RF_IMAGECAL_IMAGECAL_RUNNING )
    {
    }

    // Restore context
    LoRaDS_SX1276Write( REG_PACONFIG, regPaConfigInitVal );
    LoRaDS_SX1276SetChannel( initialFreq );
}

static uint8_t GetFskBandwidthRegValue( uint32_t bandwidth )
{
    uint8_t i;

    for( i = 0; i < ( sizeof( LoRaDS_FskBandwidths ) / sizeof( FskBandwidth_t ) ) - 1; i++ )
    {
        if( ( bandwidth >= LoRaDS_FskBandwidths[i].bandwidth ) && ( bandwidth < LoRaDS_FskBandwidths[i + 1].bandwidth ) )
        {
            return LoRaDS_FskBandwidths[i].RegValue;
        }
    }
    // ERROR: Value not found
    while( 1 );
}

void LoRaDS_SX1276SetRxConfig( RadioModems_t modem, uint32_t bandwidth,
                        uint32_t datarate, uint8_t coderate,
                        uint32_t bandwidthAfc, uint16_t preambleLen,
                        uint16_t symbTimeout, bool fixLen,
                        uint8_t payloadLen,
                        bool crcOn, bool freqHopOn, uint8_t hopPeriod,
                        bool iqInverted, bool rxContinuous )
{
    LoRaDS_SX1276SetModem( modem );

    switch( modem )
    {
    case MODEM_FSK:
        {
            SX1276.Settings.Fsk.Bandwidth = bandwidth;
            SX1276.Settings.Fsk.Datarate = datarate;
            SX1276.Settings.Fsk.BandwidthAfc = bandwidthAfc;
            SX1276.Settings.Fsk.FixLen = fixLen;
            SX1276.Settings.Fsk.PayloadLen = payloadLen;
            SX1276.Settings.Fsk.CrcOn = crcOn;
            SX1276.Settings.Fsk.IqInverted = iqInverted;
            SX1276.Settings.Fsk.RxContinuous = rxContinuous;
            SX1276.Settings.Fsk.PreambleLen = preambleLen;
            SX1276.Settings.Fsk.RxSingleTimeout = ( uint32_t )( symbTimeout * ( ( 1.0 / ( double )datarate ) * 8.0 ) * 1000 );

            datarate = ( uint16_t )( ( double )XTAL_FREQ / ( double )datarate );
            LoRaDS_SX1276Write( REG_BITRATEMSB, ( uint8_t )( datarate >> 8 ) );
            LoRaDS_SX1276Write( REG_BITRATELSB, ( uint8_t )( datarate & 0xFF ) );

            LoRaDS_SX1276Write( REG_RXBW, GetFskBandwidthRegValue( bandwidth ) );
            LoRaDS_SX1276Write( REG_AFCBW, GetFskBandwidthRegValue( bandwidthAfc ) );

            LoRaDS_SX1276Write( REG_PREAMBLEMSB, ( uint8_t )( ( preambleLen >> 8 ) & 0xFF ) );
            LoRaDS_SX1276Write( REG_PREAMBLELSB, ( uint8_t )( preambleLen & 0xFF ) );

            if( fixLen == 1 )
            {
                LoRaDS_SX1276Write( REG_PAYLOADLENGTH, payloadLen );
            }
            else
            {
                LoRaDS_SX1276Write( REG_PAYLOADLENGTH, 0xFF ); // Set payload length to the maximum
            }

            LoRaDS_SX1276Write( REG_PACKETCONFIG1,
                        ( LoRaDS_SX1276Read( REG_PACKETCONFIG1 ) &
                        RF_PACKETCONFIG1_CRC_MASK &
                        RF_PACKETCONFIG1_PACKETFORMAT_MASK ) |
                        ( ( fixLen == 1 ) ? RF_PACKETCONFIG1_PACKETFORMAT_FIXED : RF_PACKETCONFIG1_PACKETFORMAT_VARIABLE ) |
                        ( crcOn << 4 ) );
            LoRaDS_SX1276Write( REG_PACKETCONFIG2, ( LoRaDS_SX1276Read( REG_PACKETCONFIG2 ) | RF_PACKETCONFIG2_DATAMODE_PACKET ) );
        }
        break;
    case MODEM_LORA:
        {
            SX1276.Settings.LoRa.Bandwidth = bandwidth;
            SX1276.Settings.LoRa.Datarate = datarate;
            SX1276.Settings.LoRa.Coderate = coderate;
            SX1276.Settings.LoRa.PreambleLen = preambleLen;
            SX1276.Settings.LoRa.FixLen = fixLen;
            SX1276.Settings.LoRa.PayloadLen = payloadLen;
            SX1276.Settings.LoRa.CrcOn = crcOn;
            SX1276.Settings.LoRa.FreqHopOn = freqHopOn;
            SX1276.Settings.LoRa.HopPeriod = hopPeriod;
            SX1276.Settings.LoRa.IqInverted = iqInverted;
            SX1276.Settings.LoRa.RxContinuous = rxContinuous;

            if( datarate > 12 )
            {
                datarate = 12;
            }
            else if( datarate < 6 )
            {
                datarate = 6;
            }

            if( ( ( bandwidth == 7 ) && ( ( datarate == 11 ) || ( datarate == 12 ) ) ) ||
                ( ( bandwidth == 8 ) && ( datarate == 12 ) ) )
            {
                SX1276.Settings.LoRa.LowDatarateOptimize = 0x01;
            }
            else
            {
                SX1276.Settings.LoRa.LowDatarateOptimize = 0x00;
            }
            LoRaDS_SX1276Write( REG_LR_MODEMCONFIG1,
                        ( LoRaDS_SX1276Read( REG_LR_MODEMCONFIG1 ) &
                        RFLR_MODEMCONFIG1_BW_MASK &
                        RFLR_MODEMCONFIG1_CODINGRATE_MASK &
                        RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK ) |
                        ( bandwidth << 4 ) | ( coderate << 1 ) |
                        fixLen );

            LoRaDS_SX1276Write( REG_LR_MODEMCONFIG2,
                        ( LoRaDS_SX1276Read( REG_LR_MODEMCONFIG2 ) &
                        RFLR_MODEMCONFIG2_SF_MASK &
                        RFLR_MODEMCONFIG2_RXPAYLOADCRC_MASK &
                        RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) |
                        ( datarate << 4 ) | ( crcOn << 2 ) |
                        ( ( symbTimeout >> 8 ) & ~RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) );

            LoRaDS_SX1276Write( REG_LR_MODEMCONFIG3,
                        ( LoRaDS_SX1276Read( REG_LR_MODEMCONFIG3 ) &
                        RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_MASK ) |
                        ( SX1276.Settings.LoRa.LowDatarateOptimize << 3 ) );

            LoRaDS_SX1276Write( REG_LR_SYMBTIMEOUTLSB, ( uint8_t )( symbTimeout & 0xFF ) );

            LoRaDS_SX1276Write( REG_LR_PREAMBLEMSB, ( uint8_t )( ( preambleLen >> 8 ) & 0xFF ) );
            LoRaDS_SX1276Write( REG_LR_PREAMBLELSB, ( uint8_t )( preambleLen & 0xFF ) );

            if( fixLen == 1 )
            {
                LoRaDS_SX1276Write( REG_LR_PAYLOADLENGTH, payloadLen );
            }

            if( SX1276.Settings.LoRa.FreqHopOn == true )
            {
                LoRaDS_SX1276Write( REG_LR_PLLHOP, ( LoRaDS_SX1276Read( REG_LR_PLLHOP ) & RFLR_PLLHOP_FASTHOP_MASK ) | RFLR_PLLHOP_FASTHOP_ON );
                LoRaDS_SX1276Write( REG_LR_HOPPERIOD, SX1276.Settings.LoRa.HopPeriod );
            }

            if( ( bandwidth == 9 ) && ( SX1276.Settings.Channel > RF_MID_BAND_THRESH ) )
            {
                // ERRATA 2.1 - Sensitivity Optimization with a 500 kHz Bandwidth
                LoRaDS_SX1276Write( REG_LR_HIGHBWOPTIMIZE1, 0x02 );
                LoRaDS_SX1276Write( REG_LR_HIGHBWOPTIMIZE2, 0x64 );
            }
            else if( bandwidth == 9 )
            {
                // ERRATA 2.1 - Sensitivity Optimization with a 500 kHz Bandwidth
                LoRaDS_SX1276Write( REG_LR_HIGHBWOPTIMIZE1, 0x02 );
                LoRaDS_SX1276Write( REG_LR_HIGHBWOPTIMIZE2, 0x7F );
            }
            else
            {
                // ERRATA 2.1 - Sensitivity Optimization with a 500 kHz Bandwidth
                LoRaDS_SX1276Write( REG_LR_HIGHBWOPTIMIZE1, 0x03 );
            }

            if( datarate == 6 )
            {
                LoRaDS_SX1276Write( REG_LR_DETECTOPTIMIZE,
                            ( LoRaDS_SX1276Read( REG_LR_DETECTOPTIMIZE ) &
                            RFLR_DETECTIONOPTIMIZE_MASK ) |
                            RFLR_DETECTIONOPTIMIZE_SF6 );
                LoRaDS_SX1276Write( REG_LR_DETECTIONTHRESHOLD,
                            RFLR_DETECTIONTHRESH_SF6 );
            }
            else
            {
                LoRaDS_SX1276Write( REG_LR_DETECTOPTIMIZE,
                            ( LoRaDS_SX1276Read( REG_LR_DETECTOPTIMIZE ) &
                            RFLR_DETECTIONOPTIMIZE_MASK ) |
                            RFLR_DETECTIONOPTIMIZE_SF7_TO_SF12 );
                LoRaDS_SX1276Write( REG_LR_DETECTIONTHRESHOLD,
                            RFLR_DETECTIONTHRESH_SF7_TO_SF12 );
            }
        }
        break;
    }
}

void LoRaDS_SX1276SetTxConfig( RadioModems_t modem, int8_t power, uint32_t fdev,
                        uint32_t bandwidth, uint32_t datarate,
                        uint8_t coderate, uint16_t preambleLen,
                        bool fixLen, bool crcOn, bool freqHopOn,
                        uint8_t hopPeriod, bool iqInverted, uint32_t timeout )
{
    LoRaDS_SX1276SetModem( modem );

    LoRaDS_SX1276SetRfTxPower( power );

    switch( modem )
    {
    case MODEM_FSK:
        {
            SX1276.Settings.Fsk.Power = power;
            SX1276.Settings.Fsk.Fdev = fdev;
            SX1276.Settings.Fsk.Bandwidth = bandwidth;
            SX1276.Settings.Fsk.Datarate = datarate;
            SX1276.Settings.Fsk.PreambleLen = preambleLen;
            SX1276.Settings.Fsk.FixLen = fixLen;
            SX1276.Settings.Fsk.CrcOn = crcOn;
            SX1276.Settings.Fsk.IqInverted = iqInverted;
            SX1276.Settings.Fsk.TxTimeout = timeout;

            fdev = ( uint16_t )( ( double )fdev / ( double )FREQ_STEP );
            LoRaDS_SX1276Write( REG_FDEVMSB, ( uint8_t )( fdev >> 8 ) );
            LoRaDS_SX1276Write( REG_FDEVLSB, ( uint8_t )( fdev & 0xFF ) );

            datarate = ( uint16_t )( ( double )XTAL_FREQ / ( double )datarate );
            LoRaDS_SX1276Write( REG_BITRATEMSB, ( uint8_t )( datarate >> 8 ) );
            LoRaDS_SX1276Write( REG_BITRATELSB, ( uint8_t )( datarate & 0xFF ) );

            LoRaDS_SX1276Write( REG_PREAMBLEMSB, ( preambleLen >> 8 ) & 0x00FF );
            LoRaDS_SX1276Write( REG_PREAMBLELSB, preambleLen & 0xFF );

            LoRaDS_SX1276Write( REG_PACKETCONFIG1,
                        ( LoRaDS_SX1276Read( REG_PACKETCONFIG1 ) &
                            RF_PACKETCONFIG1_CRC_MASK &
                            RF_PACKETCONFIG1_PACKETFORMAT_MASK ) |
                            ( ( fixLen == 1 ) ? RF_PACKETCONFIG1_PACKETFORMAT_FIXED : RF_PACKETCONFIG1_PACKETFORMAT_VARIABLE ) |
                            ( crcOn << 4 ) );
            LoRaDS_SX1276Write( REG_PACKETCONFIG2, ( LoRaDS_SX1276Read( REG_PACKETCONFIG2 ) | RF_PACKETCONFIG2_DATAMODE_PACKET ) );
        }
        break;
    case MODEM_LORA:
        {
            SX1276.Settings.LoRa.Bandwidth = bandwidth;
            SX1276.Settings.LoRa.Datarate = datarate;
            SX1276.Settings.LoRa.Coderate = coderate;
            SX1276.Settings.LoRa.PreambleLen = preambleLen;
            SX1276.Settings.LoRa.FixLen = fixLen;
            SX1276.Settings.LoRa.FreqHopOn = freqHopOn;
            SX1276.Settings.LoRa.HopPeriod = hopPeriod;
            SX1276.Settings.LoRa.CrcOn = crcOn;
            SX1276.Settings.LoRa.IqInverted = iqInverted;
            SX1276.Settings.LoRa.TxTimeout = timeout;

            if( datarate > 12 )
            {
                datarate = 12;
            }
            else if( datarate < 6 )
            {
                datarate = 6;
            }
            if( ( ( bandwidth == 7 ) && ( ( datarate == 11 ) || ( datarate == 12 ) ) ) ||
                ( ( bandwidth == 8 ) && ( datarate == 12 ) ) )
            {
                SX1276.Settings.LoRa.LowDatarateOptimize = 0x01;
            }
            else
            {
                SX1276.Settings.LoRa.LowDatarateOptimize = 0x00;
            }
            // SX1276.Settings.LoRa.LowDatarateOptimize = 0x00;

            if( SX1276.Settings.LoRa.FreqHopOn == true )
            {
                LoRaDS_SX1276Write( REG_LR_PLLHOP, ( LoRaDS_SX1276Read( REG_LR_PLLHOP ) & RFLR_PLLHOP_FASTHOP_MASK ) | RFLR_PLLHOP_FASTHOP_ON );
                LoRaDS_SX1276Write( REG_LR_HOPPERIOD, SX1276.Settings.LoRa.HopPeriod );
            }

            LoRaDS_SX1276Write( REG_LR_MODEMCONFIG1,
                        ( LoRaDS_SX1276Read( REG_LR_MODEMCONFIG1 ) &
                            RFLR_MODEMCONFIG1_BW_MASK &
                            RFLR_MODEMCONFIG1_CODINGRATE_MASK &
                            RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK ) |
                            ( bandwidth << 4 ) | ( coderate << 1 ) |
                            fixLen );

            LoRaDS_SX1276Write( REG_LR_MODEMCONFIG2,
                        ( LoRaDS_SX1276Read( REG_LR_MODEMCONFIG2 ) &
                            RFLR_MODEMCONFIG2_SF_MASK &
                            RFLR_MODEMCONFIG2_RXPAYLOADCRC_MASK ) |
                            ( datarate << 4 ) | ( crcOn << 2 ) );

            LoRaDS_SX1276Write( REG_LR_MODEMCONFIG3,
                        ( LoRaDS_SX1276Read( REG_LR_MODEMCONFIG3 ) &
                            RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_MASK ) |
                            ( SX1276.Settings.LoRa.LowDatarateOptimize << 3 ) );

            LoRaDS_SX1276Write( REG_LR_PREAMBLEMSB, ( preambleLen >> 8 ) & 0x00FF );
            LoRaDS_SX1276Write( REG_LR_PREAMBLELSB, preambleLen & 0xFF );

            if( datarate == 6 )
            {
                LoRaDS_SX1276Write( REG_LR_DETECTOPTIMIZE,
                            ( LoRaDS_SX1276Read( REG_LR_DETECTOPTIMIZE ) &
                                RFLR_DETECTIONOPTIMIZE_MASK ) |
                                RFLR_DETECTIONOPTIMIZE_SF6 );
                LoRaDS_SX1276Write( REG_LR_DETECTIONTHRESHOLD,
                            RFLR_DETECTIONTHRESH_SF6 );
            }
            else
            {
                LoRaDS_SX1276Write( REG_LR_DETECTOPTIMIZE,
                            ( LoRaDS_SX1276Read( REG_LR_DETECTOPTIMIZE ) &
                            RFLR_DETECTIONOPTIMIZE_MASK ) |
                            RFLR_DETECTIONOPTIMIZE_SF7_TO_SF12 );
                LoRaDS_SX1276Write( REG_LR_DETECTIONTHRESHOLD,
                            RFLR_DETECTIONTHRESH_SF7_TO_SF12 );
            }
        }
        break;
    }
}

double LoRaDS_SX1276GetTimeOnAir( RadioModems_t modem, uint8_t pktLen )
{
    double airTime = 0;

    switch( modem )
    {
    case MODEM_FSK:
        {
            airTime = (uint32_t) round( ( 8 * ( SX1276.Settings.Fsk.PreambleLen +
                        ( ( LoRaDS_SX1276Read( REG_SYNCCONFIG ) & ~RF_SYNCCONFIG_SYNCSIZE_MASK ) + 1 ) +
                        ( ( SX1276.Settings.Fsk.FixLen == 0x01 ) ? 0.0 : 1.0 ) +
                        ( ( ( LoRaDS_SX1276Read( REG_PACKETCONFIG1 ) & ~RF_PACKETCONFIG1_ADDRSFILTERING_MASK ) != 0x00 ) ? 1.0 : 0 ) +
                        pktLen +
                        ( ( SX1276.Settings.Fsk.CrcOn == 0x01 ) ? 2.0 : 0 ) ) /
                        SX1276.Settings.Fsk.Datarate ) * 1000 );
        }
        break;
    case MODEM_LORA:
        {
            double bw = 0.0;
            // REMARK: When using LoRa modem only bandwidths 125, 250 and 500 kHz are supported
            switch( SX1276.Settings.LoRa.Bandwidth )
            {
            case 7: // 125 kHz
                bw = 125000;
                break;
            case 8: // 250 kHz
                bw = 250000;
                break;
            case 9: // 500 kHz
                bw = 500000;
                break;
            }

            // Symbol rate : time for one symbol (secs)
            double rs = bw / ( 1 << SX1276.Settings.LoRa.Datarate );
            double ts = 1 / rs;
            // time of preamble
            double tPreamble = ( SX1276.Settings.LoRa.PreambleLen + 4.25 ) * ts;
            // Symbol length of payload and time
            double tmp = ceil( ( 8 * pktLen - 4 * SX1276.Settings.LoRa.Datarate +
                                 28 + 16 * SX1276.Settings.LoRa.CrcOn -
                                ( SX1276.Settings.LoRa.FixLen ? 20 : 0 ) ) /
                                ( double )( 4 * ( SX1276.Settings.LoRa.Datarate -
                                ( ( SX1276.Settings.LoRa.LowDatarateOptimize > 0 ) ? 2 : 0 ) ) ) ) *
                                ( SX1276.Settings.LoRa.Coderate + 4 );
            double nPayload = 8 + ( ( tmp > 0 ) ? tmp : 0 );

            double tPayload = nPayload * ts;
            // Time on air
            double tOnAir = tPreamble + tPayload;
            // return ms secs
            // airTime = (uint32_t) floor( tOnAir * 1000 + 0.999 );
            airTime = (double) tOnAir * 1000;
        }
        break;
    }
    return airTime;
}

void LoRaDS_SX1276Send( uint8_t *buffer, uint8_t size )
{
    uint32_t txTimeout = 0;
    switch( SX1276.Settings.Modem )
    {
    case MODEM_FSK:
        {
            SX1276.Settings.FskPacketHandler.NbBytes = 0;
            SX1276.Settings.FskPacketHandler.Size = size;

            if( SX1276.Settings.Fsk.FixLen == false )
            {
                LoRaDS_SX1276WriteFifo( ( uint8_t* )&size, 1 );
            }
            else
            {
                LoRaDS_SX1276Write( REG_PAYLOADLENGTH, size );
            }

            if( ( size > 0 ) && ( size <= 64 ) )
            {
                SX1276.Settings.FskPacketHandler.ChunkSize = size;
            }
            else
            {
                memcpy( RxTxBuffer, buffer, size );
                SX1276.Settings.FskPacketHandler.ChunkSize = 32;
            }

            // Write payload buffer
            LoRaDS_SX1276WriteFifo( buffer, SX1276.Settings.FskPacketHandler.ChunkSize );
            SX1276.Settings.FskPacketHandler.NbBytes += SX1276.Settings.FskPacketHandler.ChunkSize;
            txTimeout = SX1276.Settings.Fsk.TxTimeout;
        }
        break;
    case MODEM_LORA:
        {
            if( SX1276.Settings.LoRa.IqInverted == true )
            {
                LoRaDS_SX1276Write( REG_LR_INVERTIQ, ( ( LoRaDS_SX1276Read( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_ON ) );
                LoRaDS_SX1276Write( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_ON );
            }
            else
            {
                LoRaDS_SX1276Write( REG_LR_INVERTIQ, ( ( LoRaDS_SX1276Read( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_OFF ) );
                LoRaDS_SX1276Write( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_OFF );
            }

            SX1276.Settings.LoRaPacketHandler.Size = size;

            // Initializes the payload size
            LoRaDS_SX1276Write( REG_LR_PAYLOADLENGTH, size );

            // Full buffer used for Tx
            LoRaDS_SX1276Write( REG_LR_FIFOTXBASEADDR, 0 );
            LoRaDS_SX1276Write( REG_LR_FIFOADDRPTR, 0 );

            // FIFO operations can not take place in Sleep mode
            if( ( LoRaDS_SX1276Read( REG_OPMODE ) & ~RF_OPMODE_MASK ) == RF_OPMODE_SLEEP )
            {
                LoRaDS_SX1276SetStby( );
            }
            // Write payload buffer
            LoRaDS_SX1276WriteFifo( buffer, size );
            txTimeout = SX1276.Settings.LoRa.TxTimeout;
        }
        break;
    }
    LoRaDS_SX1276SetTx( txTimeout );
}

void LoRaDS_SX1276SetSleep( void )
{
    LoRaDS_SX1276SetOpMode( RF_OPMODE_SLEEP );
    SX1276.Settings.State = RF_IDLE;
}

void LoRaDS_SX1276SetStby( void )
{
	LoRaDS_SX1276IoDeInit();
    LoRaDS_SX1276SetOpMode( RF_OPMODE_STANDBY );
    SX1276.Settings.State = RF_IDLE;
}

void LoRaDS_SX1276SetRx( uint32_t timeout )
{
    bool rxContinuous = false;

    switch( SX1276.Settings.Modem )
    {
    case MODEM_FSK:
        {
            rxContinuous = SX1276.Settings.Fsk.RxContinuous;

            // DIO0=PayloadReady
            // DIO1=FifoLevel
            // DIO2=SyncAddr
            // DIO3=FifoEmpty
            // DIO4=Preamble
            // DIO5=ModeReady
            LoRaDS_SX1276Write( REG_DIOMAPPING1, ( LoRaDS_SX1276Read( REG_DIOMAPPING1 ) & RF_DIOMAPPING1_DIO0_MASK &
                                                                            RF_DIOMAPPING1_DIO1_MASK &
                                                                            RF_DIOMAPPING1_DIO2_MASK ) |
                                                                            RF_DIOMAPPING1_DIO0_00 |
                                                                            RF_DIOMAPPING1_DIO1_00 |
                                                                            RF_DIOMAPPING1_DIO2_11 );

            LoRaDS_SX1276Write( REG_DIOMAPPING2, ( LoRaDS_SX1276Read( REG_DIOMAPPING2 ) & RF_DIOMAPPING2_DIO4_MASK &
                                                                            RF_DIOMAPPING2_MAP_MASK ) |
                                                                            RF_DIOMAPPING2_DIO4_11 |
                                                                            RF_DIOMAPPING2_MAP_PREAMBLEDETECT );

            SX1276.Settings.FskPacketHandler.FifoThresh = LoRaDS_SX1276Read( REG_FIFOTHRESH ) & 0x3F;

            LoRaDS_SX1276Write( REG_RXCONFIG, RF_RXCONFIG_AFCAUTO_ON | RF_RXCONFIG_AGCAUTO_ON | RF_RXCONFIG_RXTRIGER_PREAMBLEDETECT );

            SX1276.Settings.FskPacketHandler.PreambleDetected = false;
            SX1276.Settings.FskPacketHandler.SyncWordDetected = false;
            SX1276.Settings.FskPacketHandler.NbBytes = 0;
            SX1276.Settings.FskPacketHandler.Size = 0;
        }
        break;
    case MODEM_LORA:
        {
            if( SX1276.Settings.LoRa.IqInverted == true )
            {
                LoRaDS_SX1276Write( REG_LR_INVERTIQ, ( ( LoRaDS_SX1276Read( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_ON | RFLR_INVERTIQ_TX_OFF ) );
                LoRaDS_SX1276Write( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_ON );
            }
            else
            {
                LoRaDS_SX1276Write( REG_LR_INVERTIQ, ( ( LoRaDS_SX1276Read( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_OFF ) );
                LoRaDS_SX1276Write( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_OFF );
            }

            // ERRATA 2.3 - Receiver Spurious Reception of a LoRa Signal
            if( SX1276.Settings.LoRa.Bandwidth < 9 )
            {
                LoRaDS_SX1276Write( REG_LR_DETECTOPTIMIZE, LoRaDS_SX1276Read( REG_LR_DETECTOPTIMIZE ) & 0x7F );
                LoRaDS_SX1276Write( REG_LR_IFFREQ2, 0x00 );
                switch( SX1276.Settings.LoRa.Bandwidth )
                {
                case 0: // 7.8 kHz
                    LoRaDS_SX1276Write( REG_LR_IFFREQ1, 0x48 );
                    LoRaDS_SX1276SetChannel(SX1276.Settings.Channel + 7810 );
                    break;
                case 1: // 10.4 kHz
                    LoRaDS_SX1276Write( REG_LR_IFFREQ1, 0x44 );
                    LoRaDS_SX1276SetChannel(SX1276.Settings.Channel + 10420 );
                    break;
                case 2: // 15.6 kHz
                    LoRaDS_SX1276Write( REG_LR_IFFREQ1, 0x44 );
                    LoRaDS_SX1276SetChannel(SX1276.Settings.Channel + 15620 );
                    break;
                case 3: // 20.8 kHz
                    LoRaDS_SX1276Write( REG_LR_IFFREQ1, 0x44 );
                    LoRaDS_SX1276SetChannel(SX1276.Settings.Channel + 20830 );
                    break;
                case 4: // 31.2 kHz
                    LoRaDS_SX1276Write( REG_LR_IFFREQ1, 0x44 );
                    LoRaDS_SX1276SetChannel(SX1276.Settings.Channel + 31250 );
                    break;
                case 5: // 41.4 kHz
                    LoRaDS_SX1276Write( REG_LR_IFFREQ1, 0x44 );
                    LoRaDS_SX1276SetChannel(SX1276.Settings.Channel + 41670 );
                    break;
                case 6: // 62.5 kHz
                    LoRaDS_SX1276Write( REG_LR_IFFREQ1, 0x40 );
                    break;
                case 7: // 125 kHz
                    LoRaDS_SX1276Write( REG_LR_IFFREQ1, 0x40 );
                    break;
                case 8: // 250 kHz
                    LoRaDS_SX1276Write( REG_LR_IFFREQ1, 0x40 );
                    break;
                }
            }
            else
            {
                LoRaDS_SX1276Write( REG_LR_DETECTOPTIMIZE, LoRaDS_SX1276Read( REG_LR_DETECTOPTIMIZE ) | 0x80 );
            }

            rxContinuous = SX1276.Settings.LoRa.RxContinuous;

            if( SX1276.Settings.LoRa.FreqHopOn == true )
            {
                LoRaDS_SX1276Write( REG_LR_IRQFLAGSMASK, //RFLR_IRQFLAGS_RXTIMEOUT |
                                                // RFLR_IRQFLAGS_RXDONE |
                                                //RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                                //RFLR_IRQFLAGS_VALIDHEADER |
                                                RFLR_IRQFLAGS_TXDONE |
                                                RFLR_IRQFLAGS_CADDONE |
                                                //RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                                RFLR_IRQFLAGS_CADDETECTED );

                // DIO0=RxDone, DIO2=FhssChangeChannel
                LoRaDS_SX1276Write( REG_DIOMAPPING1, ( LoRaDS_SX1276Read( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK & RFLR_DIOMAPPING1_DIO2_MASK  ) | RFLR_DIOMAPPING1_DIO0_00 | RFLR_DIOMAPPING1_DIO2_00 );
            }
            else
            {
                LoRaDS_SX1276Write( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
                                                  //RFLR_IRQFLAGS_RXDONE |
                                                  //RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                                RFLR_IRQFLAGS_VALIDHEADER |
                                                RFLR_IRQFLAGS_TXDONE |
                                                RFLR_IRQFLAGS_CADDONE |
                                                RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                                RFLR_IRQFLAGS_CADDETECTED );

                // DIO0=RxDone
                // LoRaDS_SX1276Write( REG_DIOMAPPING1, ( LoRaDS_SX1276Read( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK ) | RFLR_DIOMAPPING1_DIO0_00 );
                LoRaDS_SX1276Write( REG_DIOMAPPING1, ( LoRaDS_SX1276Read( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK & RFLR_DIOMAPPING1_DIO1_MASK) | RFLR_DIOMAPPING1_DIO0_00 | RFLR_DIOMAPPING1_DIO1_00);
            }
            LoRaDS_SX1276Write( REG_LR_FIFORXBASEADDR, 0 );
            LoRaDS_SX1276Write( REG_LR_FIFOADDRPTR, 0 );
        }
        break;
    }

    memset( RxTxBuffer, 0, ( size_t )RX_BUFFER_SIZE );

    SX1276.Settings.State = RF_RX_RUNNING;
    if( timeout != 0 )
    {
//        TimerSetValue( &RxTimeoutTimer, timeout );
//        TimerStart( &RxTimeoutTimer );
    }

    if( SX1276.Settings.Modem == MODEM_FSK )
    {
        LoRaDS_SX1276SetOpMode( RF_OPMODE_RECEIVER );

        if( rxContinuous == false )
        {
//            TimerSetValue( &RxTimeoutSyncWord, SX1276.Settings.Fsk.RxSingleTimeout );
//            TimerStart( &RxTimeoutSyncWord );
        }
    }
    else
    {
        if( rxContinuous == true )
        {
            LoRaDS_SX1276SetOpMode( RFLR_OPMODE_RECEIVER );
        }
        else
        {
            LoRaDS_SX1276SetOpMode( RFLR_OPMODE_RECEIVER_SINGLE );
        }
    }
}

void LoRaDS_SX1276SetTx( uint32_t timeout )
{
//    TimerSetValue( &TxTimeoutTimer, timeout );

    switch( SX1276.Settings.Modem )
    {
    case MODEM_FSK:
        {
            // DIO0=PacketSent
            // DIO1=FifoEmpty
            // DIO2=FifoFull
            // DIO3=FifoEmpty
            // DIO4=LowBat
            // DIO5=ModeReady
            LoRaDS_SX1276Write( REG_DIOMAPPING1, ( LoRaDS_SX1276Read( REG_DIOMAPPING1 ) & RF_DIOMAPPING1_DIO0_MASK &
                                                                            RF_DIOMAPPING1_DIO1_MASK &
                                                                            RF_DIOMAPPING1_DIO2_MASK ) |
                                                                            RF_DIOMAPPING1_DIO1_01 );

            LoRaDS_SX1276Write( REG_DIOMAPPING2, ( LoRaDS_SX1276Read( REG_DIOMAPPING2 ) & RF_DIOMAPPING2_DIO4_MASK &
                                                                            RF_DIOMAPPING2_MAP_MASK ) );
            SX1276.Settings.FskPacketHandler.FifoThresh = LoRaDS_SX1276Read( REG_FIFOTHRESH ) & 0x3F;
        }
        break;
    case MODEM_LORA:
        {
            if( SX1276.Settings.LoRa.FreqHopOn == true )
            {
                LoRaDS_SX1276Write( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
                                                    RFLR_IRQFLAGS_RXDONE |
                                                    RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                                    RFLR_IRQFLAGS_VALIDHEADER |
                                                    //RFLR_IRQFLAGS_TXDONE |
                                                    RFLR_IRQFLAGS_CADDONE |
                                                    //RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                                    RFLR_IRQFLAGS_CADDETECTED );

                // DIO0=TxDone, DIO2=FhssChangeChannel
                LoRaDS_SX1276Write( REG_DIOMAPPING1, ( LoRaDS_SX1276Read( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK & RFLR_DIOMAPPING1_DIO2_MASK ) | RFLR_DIOMAPPING1_DIO0_01 | RFLR_DIOMAPPING1_DIO2_00 );
            }
            else
            {
                LoRaDS_SX1276Write( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
                                                    RFLR_IRQFLAGS_RXDONE |
                                                    RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                                    RFLR_IRQFLAGS_VALIDHEADER |
                                                    //RFLR_IRQFLAGS_TXDONE |
                                                    RFLR_IRQFLAGS_CADDONE |
                                                    RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                                    RFLR_IRQFLAGS_CADDETECTED );

                // DIO0=TxDone
                LoRaDS_SX1276Write( REG_DIOMAPPING1, ( LoRaDS_SX1276Read( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK ) | RFLR_DIOMAPPING1_DIO0_01 );
            }
        }
        break;
    }
    SX1276.Settings.State = RF_TX_RUNNING;
    //    TimerStart( &TxTimeoutTimer );
    LoRaDS_SX1276SetOpMode( RF_OPMODE_TRANSMITTER );
}

void LoRaDS_SX1276StartCad( void )
{
    switch( SX1276.Settings.Modem )
    {
    case MODEM_FSK:
        {

        }
        break;
    case MODEM_LORA:
        {
            LoRaDS_SX1276Write( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
                                        RFLR_IRQFLAGS_RXDONE |
                                        RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                        RFLR_IRQFLAGS_VALIDHEADER |
                                        RFLR_IRQFLAGS_TXDONE |
                                        //RFLR_IRQFLAGS_CADDONE |
                                        RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL // |
                                        //RFLR_IRQFLAGS_CADDETECTED
                                        );

            // DIO3=CADDone
            LoRaDS_SX1276Write( REG_DIOMAPPING1, ( LoRaDS_SX1276Read( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO3_MASK ) | RFLR_DIOMAPPING1_DIO3_00 );

            SX1276.Settings.State = RF_CAD;
            LoRaDS_SX1276SetOpMode( RFLR_OPMODE_CAD );
        }
        break;
    default:
        break;
    }
}

void LoRaDS_SX1276SetTxContinuousWave( uint32_t freq, int8_t power, uint16_t time )
{
    uint32_t timeout = ( uint32_t )( time * 1000 );

    LoRaDS_SX1276SetChannel( freq );

    LoRaDS_SX1276SetTxConfig( MODEM_FSK, power, 0, 0, 4800, 0, 5, false, false, 0, 0, 0, timeout );

    LoRaDS_SX1276Write( REG_PACKETCONFIG2, ( LoRaDS_SX1276Read( REG_PACKETCONFIG2 ) & RF_PACKETCONFIG2_DATAMODE_MASK ) );
    // Disable radio interrupts
    LoRaDS_SX1276Write( REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_11 | RF_DIOMAPPING1_DIO1_11 );
    LoRaDS_SX1276Write( REG_DIOMAPPING2, RF_DIOMAPPING2_DIO4_10 | RF_DIOMAPPING2_DIO5_10 );

    // TimerSetValue( &TxTimeoutTimer, timeout );

    SX1276.Settings.State = RF_TX_RUNNING;
    // TimerStart( &TxTimeoutTimer );
    LoRaDS_SX1276SetOpMode( RF_OPMODE_TRANSMITTER );
}

int16_t LoRaDS_SX1276ReadRssi( RadioModems_t modem )
{
    int16_t rssi = 0;

    switch( modem )
    {
    case MODEM_FSK:
        rssi = -( LoRaDS_SX1276Read( REG_RSSIVALUE ) >> 1 );
        break;
    case MODEM_LORA:
        if( SX1276.Settings.Channel > RF_MID_BAND_THRESH )
        {
            rssi = RSSI_OFFSET_HF + LoRaDS_SX1276Read( REG_LR_RSSIVALUE );
        }
        else
        {
            rssi = RSSI_OFFSET_LF + LoRaDS_SX1276Read( REG_LR_RSSIVALUE );
        }
        break;
    default:
        rssi = -1;
        break;
    }
    return rssi;
}

void LoRaDS_SX1276Reset( void )
{
    GPIO_InitTypeDef initStruct = { 0 };

    initStruct.Mode =GPIO_MODE_OUTPUT_PP;
    initStruct.Pull = GPIO_NOPULL;
    initStruct.Speed = GPIO_SPEED_HIGH;

    // Set RESET pin to 0
    HW_GPIO_Init( RADIO_RESET_PORT, RADIO_RESET_PIN, &initStruct );
    HW_GPIO_Write( RADIO_RESET_PORT, RADIO_RESET_PIN, 0 );


    // Configure RESET as input
    initStruct.Mode = GPIO_NOPULL;
    HW_GPIO_Init( RADIO_RESET_PORT, RADIO_RESET_PIN, &initStruct );

}

void LoRaDS_SX1276SetOpMode( uint8_t opMode )
{
    if( opMode == RF_OPMODE_SLEEP )
    {
        LoRaDS_SX1276Write( REG_OPMODE, ( LoRaDS_SX1276Read( REG_OPMODE ) & RF_OPMODE_MASK ) | opMode );

        LoRaDS_SX1276SetAntSwLowPower( true );
    }
    else
    {
        LoRaDS_SX1276SetAntSwLowPower( false );

        LoRaDS_SX1276SetAntSw( opMode );

        LoRaDS_SX1276Write( REG_OPMODE, ( LoRaDS_SX1276Read( REG_OPMODE ) & RF_OPMODE_MASK ) | opMode );
    }
}

void LoRaDS_SX1276SetModem( RadioModems_t modem )
{
    SX1276.Settings.Modem = modem;
    switch( SX1276.Settings.Modem )
    {
    default:
    case MODEM_FSK:
        LoRaDS_SX1276SetSleep( );
        LoRaDS_SX1276Write( REG_OPMODE, ( LoRaDS_SX1276Read( REG_OPMODE ) & RFLR_OPMODE_LONGRANGEMODE_MASK ) | RFLR_OPMODE_LONGRANGEMODE_OFF );

        LoRaDS_SX1276Write( REG_DIOMAPPING1, 0x00 );
        LoRaDS_SX1276Write( REG_DIOMAPPING2, 0x30 ); // DIO5=ModeReady
        break;
    case MODEM_LORA:
        LoRaDS_SX1276SetSleep( );
        LoRaDS_SX1276Write( REG_OPMODE, ( LoRaDS_SX1276Read( REG_OPMODE ) & RFLR_OPMODE_LONGRANGEMODE_MASK ) | RFLR_OPMODE_LONGRANGEMODE_ON );

        LoRaDS_SX1276Write( REG_DIOMAPPING1, 0x00 );
        LoRaDS_SX1276Write( REG_DIOMAPPING2, 0x00 );
        break;
    }
}

void LoRaDS_SX1276Write( uint16_t addr, uint8_t data )
{
    LoRaDS_SX1276WriteBuffer( addr, &data, 1 );
}

uint8_t LoRaDS_SX1276Read( uint16_t addr )
{
    uint8_t data;
    LoRaDS_SX1276ReadBuffer( addr, &data, 1 );
    return data;
}

void LoRaDS_SX1276WriteBuffer( uint16_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;

    //NSS = 0;
    HW_GPIO_Write( RADIO_NSS_PORT, RADIO_NSS_PIN, 0 );

    HW_SPI_InOut( addr | 0x80 );
    for( i = 0; i < size; i++ )
    {
        HW_SPI_InOut( buffer[i] );
    }

    //NSS = 1;
    HW_GPIO_Write( RADIO_NSS_PORT, RADIO_NSS_PIN, 1 );
}

void LoRaDS_SX1276ReadBuffer( uint16_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;

    //NSS = 0;
    HW_GPIO_Write( RADIO_NSS_PORT, RADIO_NSS_PIN, 0 );

    HW_SPI_InOut( addr & 0x7F );

    for( i = 0; i < size; i++ )
    {
        buffer[i] = HW_SPI_InOut( 0 );
    }

    //NSS = 1;
    HW_GPIO_Write( RADIO_NSS_PORT, RADIO_NSS_PIN, 1 );
}

void LoRaDS_SX1276WriteFifo( uint8_t *buffer, uint8_t size )
{
    LoRaDS_SX1276WriteBuffer( 0, buffer, size );
}

void LoRaDS_SX1276ReadFifo( uint8_t *buffer, uint8_t size )
{
    LoRaDS_SX1276ReadBuffer( 0, buffer, size );
}

void LoRaDS_SX1276SetMaxPayloadLength( RadioModems_t modem, uint8_t max )
{
    LoRaDS_SX1276SetModem( modem );

    switch( modem )
    {
    case MODEM_FSK:
        if( SX1276.Settings.Fsk.FixLen == false )
        {
            LoRaDS_SX1276Write( REG_PAYLOADLENGTH, max );
        }
        break;
    case MODEM_LORA:
        LoRaDS_SX1276Write( REG_LR_PAYLOADMAXLENGTH, max );
        break;
    }
}

void LoRaDS_SX1276SetPublicNetwork( bool enable )
{
    LoRaDS_SX1276SetModem( MODEM_LORA );
    SX1276.Settings.LoRa.PublicNetwork = enable;
    if( enable == true )
    {
        // Change LoRa modem SyncWord
        LoRaDS_SX1276Write( REG_LR_SYNCWORD, LORA_MAC_PUBLIC_SYNCWORD );
    }
    else
    {
        // Change LoRa modem SyncWord
        LoRaDS_SX1276Write( REG_LR_SYNCWORD, LORA_MAC_PRIVATE_SYNCWORD );
    }
}

void LoRaDS_SX1276OnDio1Irq()
{
    switch( SX1276.Settings.State )
    {
        case RF_RX_RUNNING:
            switch( SX1276.Settings.Modem )
            {
            case MODEM_FSK:
                // FifoLevel interrupt
                // Read received packet size
                if( ( SX1276.Settings.FskPacketHandler.Size == 0 ) && ( SX1276.Settings.FskPacketHandler.NbBytes == 0 ) )
                {
                    if( SX1276.Settings.Fsk.FixLen == false )
                    {
                        LoRaDS_SX1276ReadFifo( ( uint8_t* )&SX1276.Settings.FskPacketHandler.Size, 1 );
                    }
                    else
                    {
                        SX1276.Settings.FskPacketHandler.Size = LoRaDS_SX1276Read( REG_PAYLOADLENGTH );
                    }
                }

                // ERRATA 3.1 - PayloadReady Set for 31.25ns if FIFO is Empty
                //
                //              When FifoLevel interrupt is used to offload the
                //              FIFO, the microcontroller should  monitor  both
                //              PayloadReady  and FifoLevel interrupts, and
                //              read only (FifoThreshold-1) bytes off the FIFO
                //              when FifoLevel fires
                if( ( SX1276.Settings.FskPacketHandler.Size - SX1276.Settings.FskPacketHandler.NbBytes ) >= SX1276.Settings.FskPacketHandler.FifoThresh )
                {
                    LoRaDS_SX1276ReadFifo( ( RxTxBuffer + SX1276.Settings.FskPacketHandler.NbBytes ), SX1276.Settings.FskPacketHandler.FifoThresh - 1 );
                    SX1276.Settings.FskPacketHandler.NbBytes += SX1276.Settings.FskPacketHandler.FifoThresh - 1;
                }
                else
                {
                    LoRaDS_SX1276ReadFifo( ( RxTxBuffer + SX1276.Settings.FskPacketHandler.NbBytes ), SX1276.Settings.FskPacketHandler.Size - SX1276.Settings.FskPacketHandler.NbBytes );
                    SX1276.Settings.FskPacketHandler.NbBytes += ( SX1276.Settings.FskPacketHandler.Size - SX1276.Settings.FskPacketHandler.NbBytes );
                }
                break;
            case MODEM_LORA:
                // Sync time out
                // TimerStop( &RxTimeoutTimer );
                // Clear Irq
                LoRaDS_SX1276Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXTIMEOUT );

                SX1276.Settings.State = RF_IDLE;
                if( ( RadioEvents != NULL ) && ( RadioEvents->RxTimeout != NULL ) )
                {
                    RadioEvents->RxTimeout( );
                }
                break;
            default:
                break;
            }
            break;
        case RF_TX_RUNNING:
            switch( SX1276.Settings.Modem )
            {
            case MODEM_FSK:
                // FifoEmpty interrupt
                if( ( SX1276.Settings.FskPacketHandler.Size - SX1276.Settings.FskPacketHandler.NbBytes ) > SX1276.Settings.FskPacketHandler.ChunkSize )
                {
                    LoRaDS_SX1276WriteFifo( ( RxTxBuffer + SX1276.Settings.FskPacketHandler.NbBytes ), SX1276.Settings.FskPacketHandler.ChunkSize );
                    SX1276.Settings.FskPacketHandler.NbBytes += SX1276.Settings.FskPacketHandler.ChunkSize;
                }
                else
                {
                    // Write the last chunk of data
                    LoRaDS_SX1276WriteFifo( RxTxBuffer + SX1276.Settings.FskPacketHandler.NbBytes, SX1276.Settings.FskPacketHandler.Size - SX1276.Settings.FskPacketHandler.NbBytes );
                    SX1276.Settings.FskPacketHandler.NbBytes += SX1276.Settings.FskPacketHandler.Size - SX1276.Settings.FskPacketHandler.NbBytes;
                }
                break;
            case MODEM_LORA:
                break;
            default:
                break;
            }
            break;
        default:
            break;
    }
}

void LoRaDS_SX1276OnDio2Irq()
{
    uint32_t afcChannel = 0;

    switch( SX1276.Settings.State )
    {
        case RF_RX_RUNNING:
            switch( SX1276.Settings.Modem )
            {
            case MODEM_FSK:
                // Checks if DIO4 is connected. If it is not PreambleDetected is set to true.
#ifndef RADIO_DIO_4
                SX1276.Settings.FskPacketHandler.PreambleDetected = true;
#endif

                if( ( SX1276.Settings.FskPacketHandler.PreambleDetected == true ) && ( SX1276.Settings.FskPacketHandler.SyncWordDetected == false ) )
                {
                //    TimerStop( &RxTimeoutSyncWord );

                    SX1276.Settings.FskPacketHandler.SyncWordDetected = true;

                    SX1276.Settings.FskPacketHandler.RssiValue = -( LoRaDS_SX1276Read( REG_RSSIVALUE ) >> 1 );

                    afcChannel = ( ( ( uint16_t )LoRaDS_SX1276Read( REG_AFCMSB ) << 8 ) |
                                ( uint16_t )LoRaDS_SX1276Read( REG_AFCLSB ) );

                    SX_CHANNEL_TO_FREQ( afcChannel, SX1276.Settings.FskPacketHandler.AfcValue );

                    SX1276.Settings.FskPacketHandler.RxGain = ( LoRaDS_SX1276Read( REG_LNA ) >> 5 ) & 0x07;
                }
                break;
            case MODEM_LORA:
                if( SX1276.Settings.LoRa.FreqHopOn == true )
                {
                    // Clear Irq
                    LoRaDS_SX1276Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL );

                    if( ( RadioEvents != NULL ) && ( RadioEvents->FhssChangeChannel != NULL ) )
                    {
                        RadioEvents->FhssChangeChannel( ( LoRaDS_SX1276Read( REG_LR_HOPCHANNEL ) & RFLR_HOPCHANNEL_CHANNEL_MASK ) );
                    }
                }
                break;
            default:
                break;
            }
            break;
        case RF_TX_RUNNING:
            switch( SX1276.Settings.Modem )
            {
            case MODEM_FSK:
                break;
            case MODEM_LORA:
                if( SX1276.Settings.LoRa.FreqHopOn == true )
                {
                    // Clear Irq
                    LoRaDS_SX1276Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL );

                    if( ( RadioEvents != NULL ) && ( RadioEvents->FhssChangeChannel != NULL ) )
                    {
                        RadioEvents->FhssChangeChannel( ( LoRaDS_SX1276Read( REG_LR_HOPCHANNEL ) & RFLR_HOPCHANNEL_CHANNEL_MASK ) );
                    }
                }
                break;
            default:
                break;
            }
            break;
        default:
            break;
    }
}

// void SX1276OnDio3Irq()
// {
//     switch( SX1276.Settings.Modem )
//     {
//     case MODEM_FSK:
//         break;
//     case MODEM_LORA:
//         break;
//     default:
//         break;
//     }
// }

void LoRaDS_SX1276OnDio4Irq()
{
    switch( SX1276.Settings.Modem )
    {
    case MODEM_FSK:
        {
            if( SX1276.Settings.FskPacketHandler.PreambleDetected == false )
            {
                SX1276.Settings.FskPacketHandler.PreambleDetected = true;
            }
        }
        break;
    case MODEM_LORA:
        break;
    default:
        break;
    }
}

void LoRaDS_SX1276OnDio5Irq()
{
    switch( SX1276.Settings.Modem )
    {
    case MODEM_FSK:
        break;
    case MODEM_LORA:
        break;
    default:
        break;
    }
}


/*sx1276 functions---------------------------------------------------------------------------*/
void LoRaDS_SX1276WriteFIFO( uint8_t *buffer, uint8_t size )
{
    switch( SX1276.Settings.Modem )
    {
    case MODEM_FSK:
        {
            SX1276.Settings.FskPacketHandler.NbBytes = 0;
            SX1276.Settings.FskPacketHandler.Size = size;

            if( SX1276.Settings.Fsk.FixLen == false )
            {
                LoRaDS_SX1276WriteFifo( ( uint8_t* )&size, 1 );
            }
            else
            {
                LoRaDS_SX1276Write( REG_PAYLOADLENGTH, size );
            }

            if( ( size > 0 ) && ( size <= 64 ) )
            {
                SX1276.Settings.FskPacketHandler.ChunkSize = size;
            }
            else
            {
                memcpy( RxTxBuffer, buffer, size );
                SX1276.Settings.FskPacketHandler.ChunkSize = 32;
            }

            // Write payload buffer
            LoRaDS_SX1276WriteFifo( buffer, SX1276.Settings.FskPacketHandler.ChunkSize );
            SX1276.Settings.FskPacketHandler.NbBytes += SX1276.Settings.FskPacketHandler.ChunkSize;
        }
        break;
    case MODEM_LORA:
        {
            if( SX1276.Settings.LoRa.IqInverted == true )
            {
                LoRaDS_SX1276Write( REG_LR_INVERTIQ, ( ( LoRaDS_SX1276Read( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_ON ) );
                LoRaDS_SX1276Write( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_ON );
            }
            else
            {
                LoRaDS_SX1276Write( REG_LR_INVERTIQ, ( ( LoRaDS_SX1276Read( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_OFF ) );
                LoRaDS_SX1276Write( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_OFF );
            }

            SX1276.Settings.LoRaPacketHandler.Size = size;

            // Initializes the payload size
            LoRaDS_SX1276Write( REG_LR_PAYLOADLENGTH, size );

            // Full buffer used for Tx
            LoRaDS_SX1276Write( REG_LR_FIFOTXBASEADDR, 0 );
            LoRaDS_SX1276Write( REG_LR_FIFOADDRPTR, 0 );

            // FIFO operations can not take place in Sleep mode
            if( ( LoRaDS_SX1276Read( REG_OPMODE ) & ~RF_OPMODE_MASK ) == RF_OPMODE_SLEEP )
            {
                LoRaDS_SX1276SetStby( );
            }
            // Write payload buffer
            LoRaDS_SX1276WriteFifo( buffer, size );
        }
        break;
    }
}

uint16_t SX1276GetSymbolNum( uint8_t pktLen )
{
    uint16_t symbol_num = 0;
    // Symbol length of preamble
    double preamble_num = ( SX1276.Settings.LoRa.PreambleLen + 4.25 );
    // Symbol length of payload
    double tmp = ceil( ( 8 * pktLen - 4 * SX1276.Settings.LoRa.Datarate +
                28 + 16 * SX1276.Settings.LoRa.CrcOn -
                ( SX1276.Settings.LoRa.FixLen ? 20 : 0 ) ) /
                ( double )( 4 * ( SX1276.Settings.LoRa.Datarate -
                ( ( SX1276.Settings.LoRa.LowDatarateOptimize > 0 ) ? 2 : 0 ) ) ) ) *
                ( SX1276.Settings.LoRa.Coderate + 4 );
    double payload_num = 8 + ( ( tmp > 0 ) ? tmp : 0 );
    symbol_num = (uint16_t)(ceil( preamble_num + payload_num ));

    return symbol_num;
}

void SX1276SetSymbolTimeout( uint16_t symbol_num )
{
    LoRaDS_SX1276Write( REG_LR_MODEMCONFIG2, ( LoRaDS_SX1276Read( REG_LR_MODEMCONFIG2 ) &
                RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) |
                ( ( symbol_num >> 8 ) & ~RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) );
    LoRaDS_SX1276Write( REG_LR_SYMBTIMEOUTLSB, ( uint8_t )( symbol_num & 0xFF ) );
}

void SX1276SetRxLoraContinuous( bool rxContinuous )
{
    SX1276.Settings.LoRa.RxContinuous = rxContinuous;
}

double SX1276GetPreambleDetect()
{
    double bw = 0.0;
    // REMARK: When using LoRa modem only bandwidths 125, 250 and 500 kHz are supported
    switch( SX1276.Settings.LoRa.Bandwidth )
    {
    case 7: // 125 kHz
        bw = 125000;
        break;
    case 8: // 250 kHz
        bw = 250000;
        break;
    case 9: // 500 kHz
        bw = 500000;
        break;
    }

    // double rs = bw / ( 1 << SX1276.Settings.LoRa.Datarate );
    double rs = bw / ( 1 << 7 );
    double ts = 1 / rs;
    // time of detect the preamble at certain SF (4 symbols)
    double detect_time = 8 * ts;
    return detect_time;
}
void SX1276SetRxSF( uint8_t datarate )
{
    SX1276.Settings.LoRa.Datarate = datarate;

    if( datarate > 12 )
    {
        datarate = 12;
    }
    else if( datarate < 6 )
    {
        datarate = 6;
    }

    if( ( ( SX1276.Settings.LoRa.Bandwidth == 7 ) && ( ( datarate == 11 ) || ( datarate == 12 ) ) ) ||
    ( ( SX1276.Settings.LoRa.Bandwidth == 8 ) && ( datarate == 12 ) ) )
    {
        SX1276.Settings.LoRa.LowDatarateOptimize = 0x01;
    }
    else
    {
        SX1276.Settings.LoRa.LowDatarateOptimize = 0x00;
    }
    LoRaDS_SX1276Write( REG_LR_MODEMCONFIG2, ( LoRaDS_SX1276Read( REG_LR_MODEMCONFIG2 ) & RFLR_MODEMCONFIG2_SF_MASK) |
                ( datarate << 4 ) );
}

uint32_t SX1276GetPacketTime(uint8_t sf, uint8_t bandwidth, uint8_t cr, uint8_t header, uint8_t preamble_len, uint8_t pktLen)
{
    uint32_t bw = 0;
    uint8_t Crc_on, FixLen, LowDatarateOptimize;
    uint32_t tPreamble = 0;
    if (header) /* if is for calculate the explict header */
    {
        Crc_on = 0;
        FixLen = 1;
        cr = 4;
        pktLen = 2;
    }
    else
    {
        Crc_on = 1;
        FixLen = 0;
    }
    // REMARK: When using LoRa modem only bandwidths 125, 250 and 500 kHz are supported
    switch( bandwidth )
    {
    case 7: // 125 kHz
        bw = 125000;
        break;
    case 8: // 250 kHz
        bw = 250000;
        break;
    case 9: // 500 kHz
        bw = 500000;
        break;
    }
    if( ( ( bandwidth == 7 ) && ( ( sf == 11 ) || ( sf == 12 ) ) ) ||
    ( ( bandwidth == 8 ) && ( sf == 12 ) ) )
    {
        LowDatarateOptimize = 0x01;
    }
    else
    {
        LowDatarateOptimize = 0x00;
    }

    uint32_t rs = (1e3 * bw) / ( 1 << sf );
    uint32_t ts = (uint32_t)1e9 / rs; /* Symbol time : time for one symbol (us) */
    uint32_t tmp = (uint32_t)(ceil( (int32_t)( 8 * pktLen - 4 * sf + 28 + 16 * Crc_on - ( FixLen ? 20 : 0 ) ) / (double)( 4 * (sf - ( ( LowDatarateOptimize > 0 ) ? 2 : 0 ))) ) * ( cr + 4 ));
    if (preamble_len)
        tPreamble = (preamble_len + 4) * ts + ts / 4;
    uint32_t nPayload = 8 + ( ( tmp > 0 ) ? tmp : 0 );
    uint32_t tPayload = nPayload * ts;
    uint32_t tOnAir = tPreamble + tPayload;

    return tOnAir;
}

uint32_t SX1276GetSymbolTime(uint8_t sf, uint8_t bandwidth)
{
    uint32_t bw = 0;
    // REMARK: When using LoRa modem only bandwidths 125, 250 and 500 kHz are supported
    switch( bandwidth )
    {
    case 7: // 125 kHz
        bw = 125000;
        break;
    case 8: // 250 kHz
        bw = 250000;
        break;
    case 9: // 500 kHz
        bw = 500000;
        break;
    }
    uint32_t rs = bw / ( 1 << sf );
    uint32_t ts = (uint32_t)1e6 / rs; /* Symbol time : time for one symbol (us) */
    return ts;
}

uint8_t SX1276GetRawTemp()
{
    int8_t temp = 0;

    uint8_t previousOpMode = LoRaDS_SX1276Read( REG_OPMODE );

    if ((previousOpMode & RFLR_OPMODE_LONGRANGEMODE_ON) == RFLR_OPMODE_LONGRANGEMODE_ON)
    {
        LoRaDS_SX1276Write( REG_OPMODE, RFLR_OPMODE_SLEEP );
    }

    LoRaDS_SX1276Write( REG_OPMODE, RFLR_OPMODE_STANDBY );

    LoRaDS_SX1276Write( REG_OPMODE, RF_OPMODE_SYNTHESIZER_RX );
    uint8_t RegImageCal = LoRaDS_SX1276Read( REG_IMAGECAL);
    RegImageCal = (RegImageCal & RF_IMAGECAL_TEMPMONITOR_MASK) | RF_IMAGECAL_TEMPMONITOR_ON;
    LoRaDS_SX1276Write( REG_IMAGECAL, RegImageCal );

    // Delay 150 us
    Gpi_Fast_Tick_Native deadline = gpi_tick_fast_native() + GPI_TICK_US_TO_FAST(150);
    while (gpi_tick_compare_fast_native(gpi_tick_fast_native(), deadline) < 0);

    RegImageCal = LoRaDS_SX1276Read( REG_IMAGECAL);
    RegImageCal = (RegImageCal & RF_IMAGECAL_TEMPMONITOR_MASK) | RF_IMAGECAL_TEMPMONITOR_OFF;
    LoRaDS_SX1276Write( REG_IMAGECAL, RegImageCal );

    LoRaDS_SX1276Write( REG_OPMODE, RFLR_OPMODE_STANDBY );

    uint8_t RegTemp = LoRaDS_SX1276Read( REG_TEMP);

    if ((RegTemp & 0x80) == 0x80)
        temp = 255 - RegTemp;
    else
    {
        temp = RegTemp;
        temp *= (-1);
    }

    if ((previousOpMode & RFLR_OPMODE_LONGRANGEMODE_ON) == RFLR_OPMODE_LONGRANGEMODE_ON)
    {
        LoRaDS_SX1276Write( REG_OPMODE, RFLR_OPMODE_SLEEP );
    }

    LoRaDS_SX1276Write( REG_OPMODE, previousOpMode );
    // return temp;
    return RegTemp;
}
//sx1276-arch***************************************************************************************

//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************

/*sx1276-arch---------------------------------------------------------------------------*/
int8_t RssiValue = 0;
int8_t SnrValue = 0;


//**************************************************************************************************
//***** Global Functions ***************************************************************************

/*sx1276-arch functions---------------------------------------------------------------------------*/
void gpi_radio_init()
{
    LoRaDS_SX1276IoInit();
    spi_init();
    LoRaDS_SX1276Init();

    LoRaDS_SX1276SetChannel(loradisc_config.lora_freq);

#if defined( USE_MODEM_LORA )

    LoRaDS_SX1276SetTxConfig( MODEM_LORA, loradisc_config.lora_tx_pwr, 0, loradisc_config.lora_bw,
                    loradisc_config.lora_sf, loradisc_config.lora_cr,
                    loradisc_config.lora_plen, LORA_FIX_LENGTH_PAYLOAD_ON,
                    true, 0, 0, LORA_IQ_INVERSION_ON, TX_TIMEOUT_VALUE );

    LoRaDS_SX1276SetRxConfig( MODEM_LORA, loradisc_config.lora_bw, loradisc_config.lora_sf,
                    loradisc_config.lora_cr, 0, loradisc_config.lora_plen,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true );

#elif defined( USE_MODEM_FSK )
    #error "Please define FSK parameters."
#else
    #error "Please define a frequency band in the compiler options."
#endif
}

void gpi_radio_set_spreading_factor(uint32_t datarate)
{
#if defined( USE_MODEM_LORA )
    assert_reset((datarate >= 6));  /* SF6 */
    assert_reset((datarate <= 12)); /* SF12 */
    SX1276.Settings.LoRa.Datarate = datarate;
    uint32_t bandwidth = SX1276.Settings.LoRa.Bandwidth;
    if( ( ( bandwidth == 7 ) && ( ( datarate == 11 ) || ( datarate == 12 ) ) ) ||
    ( ( bandwidth == 8 ) && ( datarate == 12 ) ) )
    {
        SX1276.Settings.LoRa.LowDatarateOptimize = 0x01;
    }
    else
    {
        SX1276.Settings.LoRa.LowDatarateOptimize = 0x00;
    }
    // HAL_NVIC_DisableIRQ( EXTI15_10_IRQn );

    LoRaDS_SX1276Write( REG_LR_MODEMCONFIG2, ( LoRaDS_SX1276Read( REG_LR_MODEMCONFIG2 ) &
    RFLR_MODEMCONFIG2_SF_MASK) | ( datarate << 4 ));

    LoRaDS_SX1276Write( REG_LR_MODEMCONFIG3, ( LoRaDS_SX1276Read( REG_LR_MODEMCONFIG3 ) &
    RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_MASK ) | ( SX1276.Settings.LoRa.LowDatarateOptimize << 3 ));

    if( datarate == 6 )
    {
        LoRaDS_SX1276Write( REG_LR_DETECTOPTIMIZE, ( LoRaDS_SX1276Read( REG_LR_DETECTOPTIMIZE ) &
        RFLR_DETECTIONOPTIMIZE_MASK ) | RFLR_DETECTIONOPTIMIZE_SF6 );
        LoRaDS_SX1276Write( REG_LR_DETECTIONTHRESHOLD, RFLR_DETECTIONTHRESH_SF6 );
    }
    else
    {
        LoRaDS_SX1276Write( REG_LR_DETECTOPTIMIZE,( LoRaDS_SX1276Read( REG_LR_DETECTOPTIMIZE ) &
        RFLR_DETECTIONOPTIMIZE_MASK ) | RFLR_DETECTIONOPTIMIZE_SF7_TO_SF12 );
        LoRaDS_SX1276Write( REG_LR_DETECTIONTHRESHOLD, RFLR_DETECTIONTHRESH_SF7_TO_SF12 );
    }

    // __HAL_GPIO_EXTI_CLEAR_FLAG(RADIO_DIO_0_PIN);
    // HAL_NVIC_EnableIRQ( EXTI15_10_IRQn );

#elif defined( USE_MODEM_FSK )
    #error "Please define FSK parameters."
#endif
}

void gpi_radio_set_bandwidth (uint32_t bandwidth)
{
#if defined( USE_MODEM_LORA )
    assert_reset((bandwidth >= 0)); /* 7.8 kHz */
    assert_reset((bandwidth <= 9)); /* 500 kHz */
    SX1276.Settings.LoRa.Bandwidth = bandwidth;
    uint32_t datarate = SX1276.Settings.LoRa.Datarate;

    if( ( ( bandwidth == 7 ) && ( ( datarate == 11 ) || ( datarate == 12 ) ) ) ||
    ( ( bandwidth == 8 ) && ( datarate == 12 ) ) )
    {
        SX1276.Settings.LoRa.LowDatarateOptimize = 0x01;
    }
    else
    {
        SX1276.Settings.LoRa.LowDatarateOptimize = 0x00;
    }
    LoRaDS_SX1276Write( REG_LR_MODEMCONFIG1, ( LoRaDS_SX1276Read( REG_LR_MODEMCONFIG1 ) &
    RFLR_MODEMCONFIG1_BW_MASK) | ( bandwidth << 4 ) );

    LoRaDS_SX1276Write( REG_LR_MODEMCONFIG3, ( LoRaDS_SX1276Read( REG_LR_MODEMCONFIG3 ) &
    RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_MASK ) |
    ( SX1276.Settings.LoRa.LowDatarateOptimize << 3 ));
#elif defined( USE_MODEM_FSK )
    #error "Please define FSK parameters."
#endif
}

void gpi_radio_set_channel(uint8_t channel)
{
    assert_reset((channel >= CHANNEL_MIN));
    assert_reset((channel <= CHANNEL_MAX));
    LoRaDS_SX1276SetSleep();
    uint32_t freq = RF_FREQUENCY + CHANNEL_STEP * channel;
    LoRaDS_SX1276SetChannel( freq );
}

void gpi_radio_set_coding_rate(uint8_t coderate)
{
#if defined( USE_MODEM_LORA )
    assert_reset((coderate >= 1)); /* 4/5 */
    assert_reset((coderate <= 4)); /* 4/8 */
    SX1276.Settings.LoRa.Coderate = coderate;
    LoRaDS_SX1276Write(REG_LR_MODEMCONFIG1, ( LoRaDS_SX1276Read( REG_LR_MODEMCONFIG1) & RFLR_MODEMCONFIG1_CODINGRATE_MASK) | (coderate << 1));
#elif defined( USE_MODEM_FSK )
    #error "Please define FSK parameters."
#endif
}

void gpi_radio_set_preamble_len (uint16_t preambleLen)
{
#if defined( USE_MODEM_LORA )
    SX1276.Settings.LoRa.PreambleLen = preambleLen;
    LoRaDS_SX1276Write( REG_LR_PREAMBLEMSB, ( preambleLen >> 8 ) & 0x00FF );
    LoRaDS_SX1276Write( REG_LR_PREAMBLELSB, preambleLen & 0xFF );
#elif defined( USE_MODEM_FSK )
    #error "Please define FSK parameters."
#endif
}

void gpi_radio_set_tx_power(int8_t power)
{
    LoRaDS_SX1276SetRfTxPower( power );
#if defined( USE_MODEM_LORA )
#elif defined( USE_MODEM_FSK )
    SX1276.Settings.Fsk.Power = power;
#endif
}

RadioLoRaPacketHandler_t gpi_read_rssi(uint8_t rxdone)
{
    RadioLoRaPacketHandler_t read_value;

    // Returns SNR value [dB] rounded to the nearest integer value
    read_value.SnrValue = ( ( ( int8_t )LoRaDS_SX1276Read( REG_LR_PKTSNRVALUE ) ) + 2 ) >> 2;
    int16_t rssi;

    if (rxdone)
    {
        rssi = LoRaDS_SX1276Read( REG_LR_PKTRSSIVALUE );

        if( read_value.SnrValue < 0 )
        {
            if( SX1276.Settings.Channel > RF_MID_BAND_THRESH )
            {
                read_value.RssiValue = RSSI_OFFSET_HF + rssi + ( rssi >> 4 ) +
                                                                read_value.SnrValue;
            }
            else
            {
                read_value.RssiValue = RSSI_OFFSET_LF + rssi + ( rssi >> 4 ) +
                                                                read_value.SnrValue;
            }
        }
        else
        {
            if( SX1276.Settings.Channel > RF_MID_BAND_THRESH )
            {
                read_value.RssiValue = RSSI_OFFSET_HF + rssi + ( rssi >> 4 );
            }
            else
            {
                read_value.RssiValue = RSSI_OFFSET_LF + rssi + ( rssi >> 4 );
            }
        }
    }
    else
    {
        rssi = LoRaDS_SX1276Read( REG_LR_RSSIVALUE );

        if( SX1276.Settings.Channel > RF_MID_BAND_THRESH )
        {
            read_value.RssiValue = RSSI_OFFSET_HF + rssi;
        }
        else
        {
            read_value.RssiValue = RSSI_OFFSET_LF + rssi;
        }
    }

    return read_value;
}



//**************************************************************************************************
//***** Global Functions ***************************************************************************
/*radio functions---------------------------------------------------------------------------*/
