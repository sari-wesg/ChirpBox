#ifndef __GPI_ARM_SX1276_RADIO_H__
#define __GPI_ARM_SX1276_RADIO_H__


//**************************************************************************************************
//***** Includes ***********************************************************************************

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "stm32l4xx_hal.h"
#ifdef MX_CONFIG_FILE
#include STRINGIFY(MX_CONFIG_FILE)
#endif

/*sx1276mb1mas---------------------------------------------------------------------------*/
#define BOARD_WAKEUP_TIME  0                       // no TCXO

#define RADIO_ANT_SWITCH_SET_TX                    1
#define RADIO_ANT_SWITCH_SET_RX                    0

#define RF_MID_BAND_THRESH                         525000000

#define IRQ_HIGH_PRIORITY  1

/*!
 * Hardware IO IRQ callback function definition
 */
typedef void ( DioIrqHandler )();
/*sx1276mb1mas functions-------------------------------------------------------------------*/
void SX1276IoInit( void );
void SX1276IoIrqInit( DioIrqHandler **irqHandlers );
void SX1276IoDeInit( void );
void SX1276SetRfTxPower( int8_t power );
uint8_t SX1276GetPaSelect( uint32_t channel );
void SX1276SetAntSwLowPower( bool status );
void SX1276SetAntSw( uint8_t opMode );
/*radio config---------------------------------------------------------------------------*/
#define RADIO_RESET_PORT                          GPIOA
#define RADIO_RESET_PIN                           GPIO_PIN_0

#define RADIO_MOSI_PORT                           GPIOA
#define RADIO_MOSI_PIN                            GPIO_PIN_7

#define RADIO_MISO_PORT                           GPIOA
#define RADIO_MISO_PIN                            GPIO_PIN_6

#define RADIO_SCLK_PORT                           GPIOA
#define RADIO_SCLK_PIN                            GPIO_PIN_5

#define RADIO_NSS_PORT                            GPIOB
#define RADIO_NSS_PIN                             GPIO_PIN_6

#define RADIO_DIO_0_PORT                          GPIOA
#define RADIO_DIO_0_PIN                           GPIO_PIN_10

#define RADIO_DIO_1_PORT                          GPIOB
#define RADIO_DIO_1_PIN                           GPIO_PIN_3

#define RADIO_DIO_2_PORT                          GPIOB
#define RADIO_DIO_2_PIN                           GPIO_PIN_5

#define RADIO_DIO_3_PORT                          GPIOB
#define RADIO_DIO_3_PIN                           GPIO_PIN_4

#ifdef RADIO_DIO_4
#define RADIO_DIO_4_PORT                          GPIOA
#define RADIO_DIO_4_PIN                           GPIO_PIN_9
#endif

#ifdef RADIO_DIO_5
#define RADIO_DIO_5_PORT                          GPIOC
#define RADIO_DIO_5_PIN                           GPIO_PIN_7
#endif

#define RADIO_ANT_SWITCH_PORT                     GPIOC
#define RADIO_ANT_SWITCH_PIN                      GPIO_PIN_1
/*radio---------------------------------------------------------------------------*/
typedef enum
{
    MODEM_FSK = 0,
    MODEM_LORA,
}RadioModems_t;

typedef enum
{
    RF_IDLE = 0,   //!< The radio is idle
    RF_RX_RUNNING, //!< The radio is in reception state
    RF_TX_RUNNING, //!< The radio is in transmission state
    RF_CAD,        //!< The radio is doing channel activity detection
}RadioState_t;

typedef struct {
    void (*TxDone)(void);
    void (*TxTimeout)(void);
    void (*RxDone)(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr, uint8_t irqFlags);
    void (*RxTimeout)(void);
    void (*RxError)(void);
    void (*FhssChangeChannel)(uint8_t currentChannel);
    void (*CadDone) (int8_t channelActivityDetected);
} RadioEvents_t;
/*sx1276---------------------------------------------------------------------------*/

#define RSSI_OFFSET_LF                              -164
#define RSSI_OFFSET_HF                              -157
/*!
 * Radio wake-up time from sleep
 */
#define RADIO_WAKEUP_TIME                           2 // [ms]

#define RF_MID_BAND_THRESH                          525000000
/*!
 * Sync word for Private LoRa networks
 */
#define LORA_MAC_PRIVATE_SYNCWORD                   0x12

/*!
 * Sync word for Public LoRa networks
 */
#define LORA_MAC_PUBLIC_SYNCWORD                    0x34

/*!
 * Radio FSK modem parameters
 */
typedef struct
{
    int8_t   Power;
    uint32_t Fdev;
    uint32_t Bandwidth;
    uint32_t BandwidthAfc;
    uint32_t Datarate;
    uint16_t PreambleLen;
    bool     FixLen;
    uint8_t  PayloadLen;
    bool     CrcOn;
    bool     IqInverted;
    bool     RxContinuous;
    uint32_t TxTimeout;
    uint32_t RxSingleTimeout;
}RadioFskSettings_t;

/*!
 * Radio FSK packet handler state
 */
typedef struct
{
    uint8_t  PreambleDetected;
    uint8_t  SyncWordDetected;
    int8_t   RssiValue;
    int32_t  AfcValue;
    uint8_t  RxGain;
    uint16_t Size;
    uint16_t NbBytes;
    uint8_t  FifoThresh;
    uint8_t  ChunkSize;
}RadioFskPacketHandler_t;
/*!
 * Radio LoRa modem parameters
 */
typedef struct
{
    int8_t   Power;
    uint32_t Bandwidth;
    uint32_t Datarate;
    bool     LowDatarateOptimize;
    uint8_t  Coderate;
    uint16_t PreambleLen;
    bool     FixLen;
    uint8_t  PayloadLen;
    bool     CrcOn;
    bool     FreqHopOn;
    uint8_t  HopPeriod;
    bool     IqInverted;
    bool     RxContinuous;
    uint32_t TxTimeout;
    bool     PublicNetwork;
}RadioLoRaSettings_t;

/*!
 * Radio LoRa packet handler state
 */
typedef struct
{
    int8_t SnrValue;
    int16_t RssiValue;
    uint8_t Size;
}RadioLoRaPacketHandler_t;
/*!
 * Radio Settings
 */
typedef struct
{
    RadioState_t             State;
    RadioModems_t            Modem;
    uint32_t                 Channel;
    RadioFskSettings_t       Fsk;
    RadioFskPacketHandler_t  FskPacketHandler;
    RadioLoRaSettings_t      LoRa;
    RadioLoRaPacketHandler_t LoRaPacketHandler;
}RadioSettings_t;
/*!
 * Radio hardware and global parameters
 */
typedef struct SX1276_s
{
    uint8_t       RxTx;
    RadioSettings_t Settings;
}SX1276_t;

extern SX1276_t SX1276;

/*!
 * SX1276 definitions
 */
#define XTAL_FREQ                                   32000000
#define FREQ_STEP                                   61.03515625
#define FREQ_STEP_8                                 15625 /* FREQ_STEP<<8 */

/*!
 * \brief Radio hardware registers initialization definition
 *
 * \remark Can be automatically generated by the SX1276 GUI (not yet implemented)
 */
#define RADIO_INIT_REGISTERS_VALUE                \
{                                                 \
    { MODEM_FSK , REG_LNA                , 0x23 },\
    { MODEM_FSK , REG_RXCONFIG           , 0x1E },\
    { MODEM_FSK , REG_RSSICONFIG         , 0xD2 },\
    { MODEM_FSK , REG_AFCFEI             , 0x01 },\
    { MODEM_FSK , REG_PREAMBLEDETECT     , 0xAA },\
    { MODEM_FSK , REG_OSC                , 0x07 },\
    { MODEM_FSK , REG_SYNCCONFIG         , 0x12 },\
    { MODEM_FSK , REG_SYNCVALUE1         , 0xC1 },\
    { MODEM_FSK , REG_SYNCVALUE2         , 0x94 },\
    { MODEM_FSK , REG_SYNCVALUE3         , 0xC1 },\
    { MODEM_FSK , REG_PACKETCONFIG1      , 0xD8 },\
    { MODEM_FSK , REG_FIFOTHRESH         , 0x8F },\
    { MODEM_FSK , REG_IMAGECAL           , 0x02 },\
    { MODEM_FSK , REG_DIOMAPPING1        , 0x00 },\
    { MODEM_FSK , REG_DIOMAPPING2        , 0x30 },\
    { MODEM_LORA, REG_LR_PAYLOADMAXLENGTH, 0xFF },\
}

/* Freq = channel * FREQ_STEP */
#define SX_CHANNEL_TO_FREQ( channel, freq )                                                         \
    do                                                                                              \
    {                                                                                               \
        uint32_t initialChanInt, initialChanFrac;                                                   \
        initialChanInt = channel  >> 8;                                                             \
        initialChanFrac = channel - ( initialChanInt << 8 );                                        \
        freq = initialChanInt * FREQ_STEP_8 + ( ( initialChanFrac * FREQ_STEP_8 + ( 128 ) ) >> 8 ); \
    }while( 0 )

/* channel = Freq / FREQ_STEP */
#define SX_FREQ_TO_CHANNEL( channel, freq )                                                                       \
    do                                                                                                            \
    {                                                                                                             \
        uint32_t initialFreqInt, initialFreqFrac;                                                                 \
        initialFreqInt = freq / FREQ_STEP_8;                                                                      \
        initialFreqFrac = freq - ( initialFreqInt * FREQ_STEP_8 );                                                \
        channel = ( initialFreqInt << 8 ) + ( ( ( initialFreqFrac << 8 ) + ( FREQ_STEP_8 / 2 ) ) / FREQ_STEP_8 ); \
    }while( 0 )

#define RX_BUFFER_SIZE                                 256
/*sx1276 functions---------------------------------------------------------------------------*/
/*!
 * \brief Sets the SX1276
 */
void SX1276SetTx( uint32_t timeout );
void SX1276WriteFifo( uint8_t *buffer, uint8_t size );
void SX1276ReadFifo( uint8_t *buffer, uint8_t size );
void SX1276SetOpMode( uint8_t opMode );
// void SX1276OnDio0Irq();
void SX1276OnDio1Irq();
void SX1276OnDio2Irq();
// void SX1276OnDio3Irq();
void SX1276OnDio4Irq();
void SX1276OnDio5Irq();
// void SX1276OnTimeoutIrq();
/*----------------------------------------*/
void SX1276Reset( void );
uint32_t SX1276Init();
void SX1276SetOpMode( uint8_t opMode );
RadioState_t SX1276GetStatus( void );
void SX1276SetModem( RadioModems_t modem );
void SX1276SetChannel( uint32_t freq );
bool SX1276IsChannelFree( RadioModems_t modem, uint32_t freq, int16_t rssiThresh, uint32_t maxCarrierSenseTime );
// uint32_t SX1276Random( void );
void SX1276SetRxConfig( RadioModems_t modem, uint32_t bandwidth,
                         uint32_t datarate, uint8_t coderate,
                         uint32_t bandwidthAfc, uint16_t preambleLen,
                         uint16_t symbTimeout, bool fixLen,
                         uint8_t payloadLen,
                         bool crcOn, bool freqHopOn, uint8_t hopPeriod,
                         bool iqInverted, bool rxContinuous );
void SX1276SetTxConfig( RadioModems_t modem, int8_t power, uint32_t fdev,
                        uint32_t bandwidth, uint32_t datarate,
                        uint8_t coderate, uint16_t preambleLen,
                        bool fixLen, bool crcOn, bool freqHopOn,
                        uint8_t hopPeriod, bool iqInverted, uint32_t timeout );
double SX1276GetTimeOnAir( RadioModems_t modem, uint8_t pktLen );
void SX1276Send( uint8_t *buffer, uint8_t size );
void SX1276SetSleep( void );
void SX1276SetStby( void );
void SX1276SetRx( uint32_t timeout );
void SX1276StartCad( void );
void SX1276SetTxContinuousWave( uint32_t freq, int8_t power, uint16_t time );
int16_t SX1276ReadRssi( RadioModems_t modem );
void RxChainCalibration( void );
void SX1276SetSyncWord( uint8_t data );
void SX1276Write( uint16_t addr, uint8_t data );
uint8_t SX1276Read( uint16_t addr );
void SX1276WriteBuffer( uint16_t addr, uint8_t *buffer, uint8_t size );
void SX1276ReadBuffer( uint16_t addr, uint8_t *buffer, uint8_t size );
void SX1276SetMaxPayloadLength( RadioModems_t modem, uint8_t max );
void SX1276SetPublicNetwork( bool enable );
// uint32_t SX1276GetWakeupTime( void );
/*sx1276 functions---------------------------------------------------------------------------*/
void SX1276WriteFIFO( uint8_t *buffer, uint8_t size );
uint16_t SX1276GetSymbolNum( uint8_t pktLen );
void SX1276SetSymbolTimeout( uint16_t symbol_num );
void SX1276SetRxLoraContinuous( bool rxContinuous );
double SX1276GetPreambleDetect();
void SX1276SetRxSF( uint8_t datarate );
uint32_t SX1276GetPacketTime(uint8_t sf, uint8_t bandwidth, uint8_t cr, uint8_t header, uint8_t preamble_len, uint8_t pktLen);
uint32_t SX1276GetSymbolTime(uint8_t sf, uint8_t bandwidth);
uint8_t SX1276GetRawTemp();

/*sx1276-arch---------------------------------------------------------------------------*/

//**************************************************************************************************
//***** Global (Public) Defines and Consts *********************************************************

#define tx_BUFFER_SIZE                                 256
#define rx_BUFFER_SIZE                                 256

//**************************************************************************************************
//***** Local (Private) Defines and Consts *********************************************************



//**************************************************************************************************
//***** Forward Class and Struct Declarations ******************************************************



//**************************************************************************************************
//***** Global Typedefs and Class Declarations *****************************************************

/*FREQUENCY---------------------------------------------------------------------------*/
#if defined( REGION_AS923 )

#define RF_FREQUENCY                                923000000 // Hz

#elif defined( REGION_AU915 )

#define RF_FREQUENCY                                915000000 // Hz

#elif defined( REGION_CN470 )

#define RF_FREQUENCY                                470000000 // Hz

#elif defined( REGION_CN779 )

#define RF_FREQUENCY                                779000000 // Hz

#elif defined( REGION_EU433 )

#define RF_FREQUENCY                                433000000 // Hz

#elif defined( REGION_EU868 )

#define RF_FREQUENCY                                868000000 // Hz

#elif defined( REGION_KR920 )

#define RF_FREQUENCY                                920000000 // Hz

#elif defined( REGION_IN865 )

#define RF_FREQUENCY                                865000000 // Hz

#elif defined( REGION_US915 )

#define RF_FREQUENCY                                915000000 // Hz

#elif defined( REGION_RU864 )

#define RF_FREQUENCY                                864000000 // Hz

#else
    #error "Please define a frequency band in the compiler options."
#endif

//**************************************************************************************************
//***** Global Variables ***************************************************************************



//**************************************************************************************************
//***** Prototypes of Global Functions *************************************************************

#ifdef __cplusplus
	extern "C" {
#endif

// standard init functions
void 					gpi_radio_init();
void 					gpi_radio_set_spreading_factor(uint32_t datarate);
void 					gpi_radio_set_bandwidth (uint32_t bandwidth);
void 					gpi_radio_set_channel(uint8_t channel);
void                    gpi_radio_set_coding_rate(uint8_t coderate);
void 					gpi_radio_set_preamble_len (uint16_t preambleLen);
void 					gpi_radio_set_tx_power(int8_t power);
RadioLoRaPacketHandler_t gpi_read_rssi(uint8_t rxdone);
#ifdef __cplusplus
	}
#endif

//**************************************************************************************************
//***** Implementations of Inline Functions ********************************************************



//**************************************************************************************************
//**************************************************************************************************

#endif /* __GPI_ARM_SX1276_RADIO_H__ */
