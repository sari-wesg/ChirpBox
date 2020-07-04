#ifndef __EVALUATION_H__
#define __EVALUATION_H__


//**************************************************************************************************
//***** Includes ***********************************************************************************
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "stm32l4xx_hal.h"


//**************************************************************************************************

typedef struct Sta_result_tag
{
	uint32_t			mean;
	uint32_t			std;
	uint32_t			worst;
} Sta_result;

//**************************************************************************************************
void clear_real_packet();
void update_packet_table();
void read_packet_table();
void get_real_packet_group();
void calculate_action_time();
void evaluation_results();
void send_results();

void statistics_results();

//**************************************************************************************************

#endif /* __EVALUATION_H__ */


