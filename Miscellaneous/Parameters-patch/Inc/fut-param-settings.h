#ifndef __FUT_PARAM_SETTING_H__
#define __FUT_PARAM_SETTING_H__

#define NODE_LENGTH 0xFF

/*
Default LoRa Radio:
1. Frequency/bandwidth
2. Spreading factor (SF)
3. Tx power
4. Coding rate
*/
typedef struct
{
    uint32_t Frequency; // according to the region
    uint8_t SF; // 7 - 12
    int8_t TP;  // -1 - 14 dBm
    uint8_t CR; // 1 - 4 ~ 4/5 - 4/8s
} chirpbox_fut_config;


// Helper functions to print the input parameters injected by the testbed
void
print_chirpbox_fut_config(chirpbox_fut_config* p)
{
	PRINTF("Frequency: %lu kHz\nSF: %d\nTP: %d\nCR: %d\n", p->Frequency, p->SF, p->TP, p->CR);
}

#endif // __FUT_PARAM_SETTING_H__
