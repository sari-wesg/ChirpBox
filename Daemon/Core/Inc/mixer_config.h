#ifndef __MIXER_CONFIG_H__
#define __MIXER_CONFIG_H__
// mixer configuration file
// Adapt the settings to the needs of your application.

#define DEBUG_CHIRPBOX 1
#if DEBUG_CHIRPBOX

#define PRINTF_CHIRP(...) printf(__VA_ARGS__)
#else
#define PRINTF_CHIRP(...)
#endif

/*********************************************************/
uint8_t *payload_distribution;

// turn verbose log messages on or off
#define MX_VERBOSE_CONFIG		0
#define MX_VERBOSE_STATISTICS	1
#define MX_VERBOSE_PACKETS		1

/*********************************************************/
#define MX_GENERATION_SIZE_MAX  0xFF /* 255 packets */
#define MX_NUM_NODES_MAX        0xFF /* 255 nodes */

#endif /* __MIXER_CONFIG_H__ */
