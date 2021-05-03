LORADISC_HEADER_LEN         = 8
LORADISC_NODE_ID_LEN        = 2

LORADISC_HEADER_C           = 'r '
LORADISC_PAYLOAD_C          = 'f '

CHIRPBOX_LINK_MIN_SF        = 7
CHIRPBOX_LINK_MAX_SF        = 12
CHIRPBOX_LINK_MIN_TP        = -1
CHIRPBOX_LINK_MAX_TP        = 14
CHIRPBOX_LINK_MIN_FREQ      = 100000
CHIRPBOX_LINK_MAX_FREQ      = 1000000
CHIRPBOX_LINK_MIN_PL        = 1
CHIRPBOX_LINK_MAX_PL        = 255

CHIRPBOX_LINK_PACKET_NUM    = 20

CHIRPBOX_LINK_MIN_VALUE_LEN = 8
CHIRPBOX_LINK_VALUE_LEN     = 12
CHIRPBOX_LINK_SF7           = 7
CHIRPBOX_LINK_RELIABILITY_LEN   = 2
CHIRPBOX_LINK_SNR_LEN       = 1
CHIRPBOX_LINK_RSSI_LEN      = 2
CHIRPBOX_LINK_AVG_LEN       = 2
CHIRPBOX_LINK_TEMP_LEN      = 1

CHIRPBOX_LINK_MAXHOP_ERROR  = -1

CHIRPBOX_VOLTAGE_START      = 3
CHIRPBOX_VOLTAGE_LEN        = 2

CHIRPBOX_ERROR_VALUE        = 0xFFFF

CHIRPBOX_CONFIG_FILE        = "chirpbox_cbmng_command_param.json"

CHIRPBOX_COLLECT_COMMAND    = "-coldata "
CHIRPBOX_LINK_COMMAND       = "-connect "

CHIRPBOX_TOPODATA_FLASH_START   = "0807F800 "
CHIRPBOX_TOPODATA_FLASH_END     = "0807FE30 "