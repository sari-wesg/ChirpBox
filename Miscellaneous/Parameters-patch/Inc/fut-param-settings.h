#ifndef __FUT_PARAM_SETTING_H__
#define __FUT_PARAM_SETTING_H__

#define CUSTOM_LENGTH 0xFF

/*
Default LoRa Radio:
1. Frequency/bandwidth
2. Spreading factor (SF)
3. Tx power
4. Coding rate
*/
typedef struct
{
    uint32_t CUSTOM[CUSTOM_LENGTH];
} chirpbox_fut_config;


// Helper functions to print the input parameters injected by the testbed
void
static print_chirpbox_fut_config(chirpbox_fut_config* p)
{
	uint8_t i = 0;
    while(p->CUSTOM[i])
    {
        PRINTF("CUSTOM %d with value 0x%08x\n", i, p->CUSTOM[i]);
        i++;
    }
}

#endif // __FUT_PARAM_SETTING_H__
