//**************************************************************************************************
//**** Includes ************************************************************************************

#include "mixer_internal.h"

#include "gpi/olf.h"

#if MX_DOUBLE_BITMAP


//**************************************************************************************************
//***** Local Defines and Consts *******************************************************************



//**************************************************************************************************
//***** Local Typedefs and Class Declarations ******************************************************



//**************************************************************************************************
//***** Forward Declarations ***********************************************************************



//**************************************************************************************************
//***** Local (Static) Variables *******************************************************************
const uint16_t	BITMAP_SIZE = sizeof_member(Packet, coding_vector);
uint8_t temp_coding_vector[sizeof_member(Packet, coding_vector) * 2];

//**************************************************************************************************
//***** Global Variables ***************************************************************************


//**************************************************************************************************
//***** Local Functions ****************************************************************************


//**************************************************************************************************
//***** Global Functions ***************************************************************************

void set_start_up_flag()
{
	mx.start_up_flag = 1;
}

void clear_start_up_flag()
{
	mx.start_up_flag = 0;
}

uint8_t fill_local_map(uint8_t *p_packet_coding)
{
    uint8_t is_local_bitmap_full = 0;
    uint8_t temp_local[BITMAP_SIZE];

    uint8_t *p_temp_local = temp_local, *p_local = mx.local_double_map.coding_vector_8_1;
    int_fast16_t	i;

    for ( ; p_temp_local < &(temp_local[BITMAP_SIZE]); p_local++ )
        *(p_temp_local++) = ~ ( *p_local | *(p_local + BITMAP_SIZE) );
    i = (mx_get_leading_index(temp_local));

    if ((i < MX_GENERATION_SIZE) & (i != (-1)))
    {
        uint8_t *p_packet = p_packet_coding;
        p_temp_local = temp_local;
        uint8_t innovative_bitmap = 0;

        for ( ; p_packet < &(p_packet_coding[BITMAP_SIZE]); p_packet++, p_temp_local++)
        {
            if (*(p_packet++) & *(p_temp_local++))
            {
                innovative_bitmap = 1;
                break;
            }
        }

        if (innovative_bitmap)
        {
            p_packet = p_packet_coding;
            p_local = mx.local_double_map.coding_vector_8_1;
            for ( ; p_local < &(mx.local_double_map.coding_vector_8_1[BITMAP_SIZE]); )
                *(p_local++) |= *(p_packet++);
        }
    }
    else
        is_local_bitmap_full = 1;

    return is_local_bitmap_full;
}

// check if there is updated bitmap, if is, then update local bitmap,
// and set the altered_coding_vector
uint8_t bitmap_update_check(uint8_t *p_packet_coding, uint8_t node_id)
{
    // uint8_t temp_coding_vector[BITMAP_SIZE * 2];

    uint8_t *p_temp_coding = temp_coding_vector, *p_local = mx.local_double_map.coding_vector_8_1;
    uint8_t *p_packet = p_packet_coding;
    uint8_t p_temp_coding_index = 0;

#if (!MX_PREAMBLE_UPDATE)
    mx.update_flag = 0;
    memset(temp_coding_vector, 0, BITMAP_SIZE * 2);

    for ( ; p_temp_coding < &(temp_coding_vector[BITMAP_SIZE * 2]); p_temp_coding++, p_temp_coding_index++ )
    {
        *(p_temp_coding) = (~ *(p_local++)) & (*(p_packet++));

        if(*p_temp_coding)
        {
            uint8_t temp_p_temp_coding = *p_temp_coding;
            mx.update_flag = 1;

            if (p_temp_coding_index < BITMAP_SIZE)
            {
                // printf("0:check: %d, %d\n", *p_temp_coding, mx.transition_time_table.time_bit.coding_vector_8_2[p_temp_coding_index]);
                (*p_temp_coding) &= (mx.transition_time_table.time_bit.coding_vector_8_2[p_temp_coding_index]);
                // printf("1:check: %d, %d\n", *p_temp_coding, mx.transition_time_table.time_bit.coding_vector_8_2[p_temp_coding_index]);
            }
            else
            {
                // printf("2:check: %d, %d\n", *p_temp_coding, mx.transition_time_table.time_bit.coding_vector_8_1[p_temp_coding_index - BITMAP_SIZE]);
                (*p_temp_coding) &= (mx.transition_time_table.time_bit.coding_vector_8_1[p_temp_coding_index - BITMAP_SIZE]);
                // printf("3:check: %d, %d\n", *p_temp_coding, mx.transition_time_table.time_bit.coding_vector_8_1[p_temp_coding_index - BITMAP_SIZE]);
            }

            if (temp_p_temp_coding != *p_temp_coding)
            {
                mx.update_flag = 0;
                break;
            }
        }
    }

    // the own bit has the highest update priority, so we omit this update
    if ((temp_coding_vector[node_id / 8] & (1 << (node_id % 8))) | (temp_coding_vector[node_id / 8 + BITMAP_SIZE] & (1 << (node_id % 8))))
    {
        // printf("own1\n");
        mx.update_flag = OWN_UPDATE;
    }
#endif

    if ((mx.update_flag) && (mx.update_flag != OWN_UPDATE))
    {
        for (p_temp_coding = temp_coding_vector ; p_temp_coding < &(temp_coding_vector[BITMAP_SIZE * 2]); p_temp_coding++, p_temp_coding_index++ )
        {
            if (*p_temp_coding)
            {
                // printf("-----other\n");
                mx.update_flag = OTHER_UPDATE;
                add_to_time_table((*p_temp_coding), p_temp_coding_index);
            }
        }
    }

    if (mx.update_flag == OTHER_UPDATE)
    {
        // update local bitmap
        p_temp_coding = temp_coding_vector;
        p_local = mx.local_double_map.coding_vector_8_1;
        for ( ; p_temp_coding < &(temp_coding_vector[BITMAP_SIZE]); p_temp_coding++, p_local++ )
        {
            *p_local ^= *(p_temp_coding) ^ *(p_temp_coding + BITMAP_SIZE);
            *(p_local + BITMAP_SIZE) ^= *(p_temp_coding) ^ *(p_temp_coding + BITMAP_SIZE);
        }

        // update altered_coding_vector
        p_temp_coding = temp_coding_vector;
        uint8_t *p_alter = mx.altered_coding_vector.coding_vector_8;
        for ( ; p_temp_coding < &(temp_coding_vector[BITMAP_SIZE]); p_temp_coding++ )
            *(p_alter++) = *(p_temp_coding) | *(p_temp_coding + BITMAP_SIZE);
        // printf("p_alter:%d, %d, %d\n", mx.altered_coding_vector.coding_vector_8[0], temp_coding_vector[0], temp_coding_vector[1]);

        // memcpy to the update_row
        wrap_coding_vector(p_packet_coding + BITMAP_SIZE, p_packet_coding);
        gpi_memcpy_dma(mx.update_row, p_packet_coding + BITMAP_SIZE, sizeof(mx.update_row));
        unwrap_chunk(mx.update_row);
    }

    return mx.update_flag;
}

//**************************************************************************************************

void add_to_time_table(uint8_t x, uint8_t index)
{
	uint8_t		x_msb_isolate = 0;
	uint8_t 	x_msb;
	uint8_t 	add_x = x;

    if (index < BITMAP_SIZE)
    {
        for ( ; add_x; )
        {
            x_msb = gpi_get_msb(add_x);
            x_msb_isolate = gpi_slu_8(1, x_msb);		// isolate MSB
            // printf("4:check: %d, %d, %d\n", mx.transition_time_table.time_bit.coding_vector_8_1[index], x_msb, x_msb_isolate);
            mx.transition_time_table.time_bit.coding_vector_8_1[index] &= ~x_msb_isolate;
            mx.transition_time_table.time_value[index * 8 + x_msb] = MX_TRANSITION_LENGTH;
            // printf("5:check: %d, %d, %d\n", mx.transition_time_table.time_bit.coding_vector_8_1[index],
            // mx.transition_time_table.time_value[index * 8 + x_msb], index * 8 + x_msb);
            // clear bit in add_x
            add_x &= ~x_msb_isolate;
        }
    }
    else
    {
        for ( ; add_x; )
        {
            x_msb = gpi_get_msb(add_x);
            x_msb_isolate = gpi_slu_8(1, x_msb);		// isolate MSB
            // printf("6:check: %d, %d, %d\n", mx.transition_time_table.time_bit.coding_vector_8_2[index - BITMAP_SIZE], x_msb, x_msb_isolate);
            mx.transition_time_table.time_bit.coding_vector_8_2[index - BITMAP_SIZE] &= ~x_msb_isolate;
            mx.transition_time_table.time_value[(index - BITMAP_SIZE) * 8 + x_msb + MX_GENERATION_SIZE] = MX_TRANSITION_LENGTH;
            // printf("7:check: %d, %d, %d\n", mx.transition_time_table.time_bit.coding_vector_8_2[index - BITMAP_SIZE],
            // mx.transition_time_table.time_value[(index - BITMAP_SIZE) * 8 + x_msb + MX_GENERATION_SIZE], (index - BITMAP_SIZE) * 8 + x_msb + MX_GENERATION_SIZE);

            // clear bit in add_x
            add_x &= ~x_msb_isolate;
        }
    }

}

void clear_time_table()
{
    uint8_t *p_time_value = mx.transition_time_table.time_value;
    uint8_t p_index;
    for ( ; p_time_value < &(mx.transition_time_table.time_value[MX_GENERATION_SIZE * 2]); p_time_value++)
    {
        // printf("1p_time: %d\n", *p_time_value);

        if (*p_time_value)
        {

            (*p_time_value)--;
            p_index = ARRAY_INDEX(p_time_value, mx.transition_time_table.time_value);
        // printf("2p_time: %d, %d, %d\n", *p_time_value, mx.transition_time_table.time_bit.coding_vector_8_1[p_index / 8],
        // mx.transition_time_table.time_bit.coding_vector_8_2[p_index / 8]);

            if (!(*p_time_value))
            {
                if (p_index < MX_GENERATION_SIZE)
                    mx.transition_time_table.time_bit.coding_vector_8_1[p_index / 8] |= gpi_slu_8(1, p_index % 8);
                else
                    mx.transition_time_table.time_bit.coding_vector_8_2[(p_index - MX_GENERATION_SIZE) / 8] |= gpi_slu_8(1, (p_index - MX_GENERATION_SIZE) % 8);
            }
        }
    }
}

//**************************************************************************************************
#if MX_PREAMBLE_UPDATE

uint8_t bitmap_update_check_header(uint8_t *p_packet_coding, uint8_t node_id)
{
    // uint8_t temp_coding_vector[BITMAP_SIZE * 2];
    memset(temp_coding_vector, 0, BITMAP_SIZE * 2);

    uint8_t *p_temp_coding = temp_coding_vector, *p_local = mx.local_double_map.coding_vector_8_1;
    uint8_t *p_packet = p_packet_coding;
    uint8_t p_temp_coding_index = 0;

    mx.update_flag = 0;

    for ( ; p_temp_coding < &(temp_coding_vector[BITMAP_SIZE * 2]); p_temp_coding++, p_temp_coding_index++ )
    {
        *(p_temp_coding) = (~ *(p_local++)) & (*(p_packet++));

        if(*p_temp_coding)
        {
            uint8_t temp_p_temp_coding = *p_temp_coding;
            mx.update_flag = 1;

            if (p_temp_coding_index < BITMAP_SIZE)
                (*p_temp_coding) &= (mx.transition_time_table.time_bit.coding_vector_8_2[p_temp_coding_index]);
            else
                (*p_temp_coding) &= (mx.transition_time_table.time_bit.coding_vector_8_1[p_temp_coding_index - BITMAP_SIZE]);

            if (temp_p_temp_coding != *p_temp_coding)
            {
                mx.update_flag = 0;
                break;
            }
        }
    }

    // the own bit has the highest update priority, so we omit this update
    if ((temp_coding_vector[node_id / 8] & (1 << (node_id % 8))) | (temp_coding_vector[node_id / 8 + BITMAP_SIZE] & (1 << (node_id % 8))))
        mx.update_flag = OWN_UPDATE;

    return mx.update_flag;
}
#endif	// MX_PREAMBLE_UPDATE

//**************************************************************************************************
//**************************************************************************************************

#endif	// MX_DOUBLE_BITMAP
