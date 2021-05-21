//**************************************************************************************************
//**** Includes ************************************************************************************
#include "API_ChirpBox.h"
#include "flash_if.h"

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
