#ifndef __CHIRPBOX_PARAM_SETTING_H__
#define __CHIRPBOX_PARAM_SETTING_H__

#define NODE_LENGTH 0xFF

/*
Device:
1. UID_list: see how to get unique device ID (UID) in https://chirpbox.github.io/diy_chirpbox/2-2-Firmware-configuration
2. UID_version: version in 16 bit of the UID_list

Radio:
3. Frequency: radio reference freqency in kHz
*/
typedef struct
{
// Device:
    uint32_t UID_list[NODE_LENGTH];
    uint16_t DAEMON_version;
// Radio:
    uint32_t Frequency;
} chirpbox_daemon_config;


// Helper functions to print the input parameters injected by the testbed
static void
print_chirpbox_daemon_config(chirpbox_daemon_config* p)
{
	uint8_t i = 0;
    while(p->UID_list[i])
    {
        PRINTF("Node %d with UID 0x%08x\n", i, p->UID_list[i]);
        i++;
    }
	PRINTF("DAEMON_version: 0x%04x\nFrequency:%lu kHz\n", p->DAEMON_version, p->Frequency);
}

#endif // __CHIRPBOX_PARAM_SETTING_H__
