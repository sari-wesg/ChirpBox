//**************************************************************************************************
//**** Includes ************************************************************************************
#include "API_ChirpBox.h"

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
//***** Global Functions ***************************************************************************
int process_fread(janpatch_ctx *ctx, janpatch_buffer *source, size_t count, uint8_t *buffer) {
    // it can be that ESC character is actually in the data, but then it's prefixed with another ESC
    // so... we're looking for a lone ESC character
    size_t cnt = 0;
    while (1) {
        int m = jp_getc(ctx, source);
			// printf("m:%lu, %d, %d, %d\n", m, (unsigned char)m, cnt, count);
        if (m == -1) {
            // End of file stream... rewind 1 character and return, this will yield back to janpatch main function, which will exit
            // jp_fseek(source, -1, SEEK_CUR);
            break;
        }
        else
        {
            buffer[cnt] = (unsigned char)m;
        }
        cnt++;
        if (cnt >= count)
            break;
    }
    return cnt;
}

int process_fwrite(janpatch_ctx *ctx, janpatch_buffer *target, size_t count, uint8_t *buffer) {
    // it can be that ESC character is actually in the data, but then it's prefixed with another ESC
    // so... we're looking for a lone ESC character
    size_t cnt = 0;
    while (1) {
        uint8_t m = buffer[cnt];
        jp_putc(m, ctx, target);
        cnt++;
        if (cnt >= count)
            break;
    }
    return cnt;
}

int the_fseek(Flash_FILE *file, long int offset, int origin)
{
    if (origin == SEEK_SET)
    {
        file->now_page = file->origin_page + offset / FLASH_PAGE;
    }
    else if (origin == SEEK_CUR)
    {
        file->now_page += offset / FLASH_PAGE;
    }
    else
    {
        return -1;
    }
    return 0;
}

size_t the_fwrite(const void *ptr, size_t size, size_t count, Flash_FILE *file)
{
    uint8_t bank_active = 0;
    /* self update or another bank */
    if (!(file->bank))
        bank_active = 1;
    else
        bank_active = 0;

    FLASH_If_Erase_Pages(bank_active, file->now_page);
    uint32_t flash_bank_address = (bank_active == 1) ? FLASH_START_BANK1 : FLASH_START_BANK2;
    /* The flash write below is in the double-word form, so we need to ensure the count is multiple of sizeof(uint64_t) */
    /* restore current count */
    size_t temp_count = count;
    if (count % 8)
        count += 8 - (count % 8);
    FLASH_If_Write(flash_bank_address + file->now_page * FLASH_PAGE, (uint32_t *)ptr, count / sizeof(uint32_t));
    count = temp_count;
    return count;
}

size_t the_fread(void *ptr, size_t size, size_t count, Flash_FILE *file)
{
    uint8_t bank_active = 0;
    /* self update or another bank */
    if (!(file->bank))
        bank_active = 1;
    else
        bank_active = 0;

    uint32_t flash_bank_address = (bank_active == 1) ? FLASH_START_BANK1 : FLASH_START_BANK2;

    memcpy((uint32_t *)ptr, (uint32_t *)(flash_bank_address + file->now_page * FLASH_PAGE), count);
    uint32_t left_count = file->file_size - (file->now_page - file->origin_page) * FLASH_PAGE;
    // Ensure the real useful bytes are no larger than left bytes
    if ((count > left_count) && (file->file_size))
        count = left_count;
    return count;
}
