
#include "flash_if.h"
#include "chirpbox_func.h"
#include <string.h>
#include "menu.h"
#include "md5.h"

#include "mixer_config.h"

#if DEBUG_CHIRPBOX
#include <stdio.h>
#include <stdlib.h>

#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


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
    // uint32_t BankActive = READ_BIT(SYSCFG->MEMRMP, SYSCFG_MEMRMP_FB_MODE);
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

static void progress(uint8_t percentage)
{
    printf("Patch progress: %d%%\n", percentage);
}

Flash_FILE Filepatch(uint8_t originalBank, uint32_t originalPage, uint32_t originalSize, uint8_t patchBank, uint32_t patchPage, uint32_t patchSize, uint8_t newBank, uint32_t newPage)
{
    janpatch_ctx ctx = {
        // fread/fwrite buffers for every file, minimum size is 1 byte
        // when you run on an embedded system with block size flash, set it to the size of a block for best performance
        {(unsigned char *)malloc(FLASH_PAGE), FLASH_PAGE},
        {(unsigned char *)malloc(FLASH_PAGE), FLASH_PAGE},
        {(unsigned char *)malloc(FLASH_PAGE), FLASH_PAGE},

        &the_fread,
        &the_fwrite,
        &the_fseek,
        &progress};

    Flash_FILE originalFile = {originalBank, originalPage, 0, originalSize};
    Flash_FILE patchFile = {patchBank, patchPage, 0, patchSize};
    Flash_FILE newFile = {newBank, newPage};

    printf("originalSize:%lu, %lu\n", originalSize, patchSize);

    int jpr = janpatch(ctx, &originalFile, &patchFile, &newFile);

    free(ctx.source_buffer.buffer);
    free(ctx.patch_buffer.buffer);
    free(ctx.target_buffer.buffer);

    printf("size:%lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu\n", newFile.file_size, originalBank, originalPage, originalSize, patchBank, patchSize, newBank, newPage);

    /* Patch failed, set file size as 0 */
    if (jpr)
        newFile.file_size = 0;
    return newFile;
}

bool FirmwareUpgrade(uint8_t patch_update, uint8_t originalBank, uint32_t originalPage, uint32_t originalSize, uint8_t patchBank, uint32_t patchPage, uint32_t patchSize, uint8_t *md5_code)
{
    /* Note:
    When patching daemon firmware, source file (page 0) and patch file (decided by the size of source file) are located at BANK1, new file (page 0) at BANK2;
    When patching FUT, source file (page 0) and patch file (decided by the size of source file) are located at BANK2, new file (decided by the size of source file and patch file) at BANK2;
    */

    /* 0. The whole process must in bank 1 */
    assert(!READ_BIT(SYSCFG->MEMRMP, SYSCFG_MEMRMP_FB_MODE));
    Flash_FILE newFile;
    uint8_t i, newPage;
    if (patch_update)
    {
        /* 1. config patch */
        /*The generated new file must located in bank 2, otherwise it may harm the daemon file */
        uint8_t newBank = 1;
        /* Original file and patch file are in the same bank, original files (daemon or FUT) are in the first page by default, patch file must after the original file */
        assert_reset((originalBank == patchBank) && (originalPage == 0) && (patchPage >= (originalSize + FLASH_PAGE - 1) / FLASH_PAGE));
        /* If original file is in bank 1, we patch the daemon, then we generating new file in bank 2 page 0, otherwise in bank 2 page after patch */
        if (!originalBank)
            newPage = 0;
        else
            newPage = patchPage + (patchSize + FLASH_PAGE - 1) / FLASH_PAGE;
        newFile = Filepatch(originalBank, originalPage, originalSize, patchBank, patchPage, patchSize, newBank, newPage);

        /* 2. check patch result */
        if (newFile.file_size)
            PRINTF("Patch success!:%lu, %lu, %lu\n", newFile.file_size, newFile.origin_page, newFile.now_page);
        else
        {
            PRINTF("Patch failed!\n");
            /* If new file is daemon, erase the whole bank 2, else, erase patch file and new file in bank 2 */
            if (!originalBank)
                FLASH_If_Erase(0);
            else
            {
                for (i = patchPage; i < newPage + (newFile.file_size + FLASH_PAGE - 1) / FLASH_PAGE; i++)
                {
                    FLASH_If_Erase_Pages(0, i);
                }
            }
            return false;
        }
    }
    else
    {
        newFile.bank = 1;
        newFile.file_size = patchSize;
        newFile.now_page = 0;
        newFile.origin_page = 0;
    }

    /* 3. check file integrity */
    PRINTF("Md5 check: %lu\n", newFile.file_size);
    for (i = 0; i < 16; i++)
    {
        PRINTF("%02X", md5_code[i]);
    }
    PRINTF("\n");

    if (!MD5_File(newFile, md5_code))
    {
        PRINTF("md5 error\n");
        /*
        patching:
        If new file is daemon, erase the whole bank 2, else, erase patch file and new file in bank 2
        no patching:
        erase the whole bank 2
        */
        if ((!originalBank)||(!patch_update))
            FLASH_If_Erase(0);
        else
        {
            for (i = patchPage; i < newPage + (newFile.file_size + FLASH_PAGE - 1) / FLASH_PAGE; i++)
            {
                FLASH_If_Erase_Pages(0, i);
            }
        }
        return false;
    }
    else
    {
        /* if patching, move the new file */
        if ((originalBank)&&(patch_update))
        {
            /* Move the FUT new firmware to page 0 */
            for (i = 0; i < (newFile.file_size + FLASH_PAGE - 1) / FLASH_PAGE; i++)
            {
                FLASH_If_Erase_Pages(0, i);
                Flash_Bank_Copy_Bank(FLASH_START_BANK2 + (newPage + i) * FLASH_PAGE, FLASH_START_BANK2 + i * FLASH_PAGE, FLASH_PAGE, 0);
            }
            /* Erase page to the end of new FUT */
            for (i = (newFile.file_size + FLASH_PAGE - 1) / FLASH_PAGE; i < newPage + (newFile.file_size + FLASH_PAGE - 1) / FLASH_PAGE; i++)
                FLASH_If_Erase_Pages(0, i);
        }
    }

    /* 4. Write the size of firmware to bank 2 */
    uint32_t firmware_size_buffer[1];
    firmware_size_buffer[0] = newFile.file_size;
    FLASH_If_Erase_Pages(0, FIRMWARE_PAGE);
    FLASH_If_Write(FIRMWARE_FLASH_ADDRESS_2, (uint32_t *)firmware_size_buffer, 2);
    return true;
}
