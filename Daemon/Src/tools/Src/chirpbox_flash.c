
#include "chirpbox_func.h"
#include <string.h>
#include "menu.h"
#include "md5.h"
#include "stm32l4xx_hal_def.h"

#include "mixer_config.h"

#if DEBUG_CHIRPBOX
#include <stdio.h>
#include <stdlib.h>

#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


/**
  * @brief  Set the FLASH_WRP1xR status of daemon flash area.
  * @param  none
  * @retval HAL_StatusTypeDef HAL_OK if change is applied.
  */

uint32_t Bank1_WRP(uint32_t strtA_offset, uint32_t endA_offset)
{
	FLASH_OBProgramInitTypeDef OptionsBytesStruct1;
	HAL_StatusTypeDef retr;

	/* Check the parameters */
	assert_param(IS_FLASH_PAGE(strtA_offset));
	assert_param(IS_FLASH_PAGE(endA_offset));
	/* Unlock the Flash to enable the flash control register access *************/
	retr = HAL_FLASH_Unlock();

	/* Unlock the Options Bytes *************************************************/
	retr |= HAL_FLASH_OB_Unlock();

	OptionsBytesStruct1.RDPLevel = OB_RDP_LEVEL_0;
	OptionsBytesStruct1.OptionType = OPTIONBYTE_WRP;
	OptionsBytesStruct1.WRPArea = OB_WRPAREA_BANK1_AREAA;
	OptionsBytesStruct1.WRPEndOffset = endA_offset;
	OptionsBytesStruct1.WRPStartOffset = strtA_offset;
	retr |= HAL_FLASHEx_OBProgram(&OptionsBytesStruct1);

	return (retr == HAL_OK ? FLASHIF_OK : FLASHIF_PROTECTION_ERRROR);
}

/**
  * @brief  Reset the FLASH_WRP1xR status of daemon flash area.
  * @param  none
  * @retval HAL_StatusTypeDef HAL_OK if change is applied.
  */
uint32_t Bank1_nWRP( void )
{
	FLASH_OBProgramInitTypeDef OptionsBytesStruct1;
	HAL_StatusTypeDef retr;

	/* Unlock the Flash to enable the flash control register access *************/
	retr = HAL_FLASH_Unlock();

	/* Unlock the Options Bytes *************************************************/
	retr |= HAL_FLASH_OB_Unlock();

	OptionsBytesStruct1.RDPLevel = OB_RDP_LEVEL_0;
	OptionsBytesStruct1.OptionType = OPTIONBYTE_WRP;
	OptionsBytesStruct1.WRPArea = OB_WRPAREA_BANK1_AREAA;
	OptionsBytesStruct1.WRPEndOffset = 0x00;
	OptionsBytesStruct1.WRPStartOffset = 0xFF;
	retr |= HAL_FLASHEx_OBProgram(&OptionsBytesStruct1);

	OptionsBytesStruct1.WRPArea = OB_WRPAREA_BANK1_AREAB;
	retr |= HAL_FLASHEx_OBProgram(&OptionsBytesStruct1);

	return (retr == HAL_OK ? FLASHIF_OK : FLASHIF_PROTECTION_ERRROR);
}

/**
 * @description: Read the flag in flash to check if the bank is under write protected, if under WRT, clear the corresponding option bytes and reset the bank to make it effective.
 * @param None
 * @return: None
 */
void Bank_WRT_Check( void )
{
	FLASH_OBProgramInitTypeDef OptionsBytesStruct1;

	/* Unlock the Flash to enable the flash control register access *************/
	HAL_FLASH_Unlock();

	/* Unlock the Options Bytes *************************************************/
	HAL_FLASH_OB_Unlock();

	OptionsBytesStruct1.WRPArea = OB_WRPAREA_BANK1_AREAA;
	HAL_FLASHEx_OBGetConfig(&OptionsBytesStruct1);
	if((OptionsBytesStruct1.WRPStartOffset == 0) && (OptionsBytesStruct1.WRPEndOffset == 0xff))
	{
        /* Boot come back from FUT in Bank 2, need to unlock the write protection of flash */
        Bank1_nWRP();
        HAL_FLASH_OB_Launch();
	}
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

    printf("size:%lu, %u, %lu, %lu, %u, %lu, %u, %lu\n", newFile.file_size, originalBank, originalPage, originalSize, patchBank, patchSize, newBank, newPage);

    /* Patch failed, set file size as 0 */
    if (jpr)
        newFile.file_size = 0;
    return newFile;
}

bool FirmwareUpgrade(uint8_t patch_update, uint8_t originalBank, uint32_t originalPage, uint32_t originalSize, uint8_t patchBank, uint32_t patchPage, uint32_t patchSize, uint8_t *md5_code, uint8_t file_compression)
{
    printf("FirmwareUpgrade:%u, %u, %lu, %lu, %u, %lu, %lu, %u\n", patch_update, originalBank, originalPage, originalSize, patchBank, patchPage, patchSize, file_compression);
    uint8_t i;
    if(file_compression)
    {
        /* encode for patch:
        patch daemon: decode file behind patch and move it to the patch location;
        patch FUT: decode file behind patch and move it to the patch location;
        encode for whole firmware:
        decode file behind encode file and move it to the encode location; */
        Flash_FILE encode_file, decode_file;
        encode_file.bank = patch_update?originalBank:1;
        encode_file.origin_page = patch_update?patchPage:0;
        encode_file.file_size = patchSize;
        decode_file.bank = encode_file.bank;
        decode_file.origin_page = encode_file.origin_page + (patchSize + FLASH_PAGE - 1) / FLASH_PAGE;
        decode_file.file_size = LZSS_decode(&encode_file, &decode_file);
        PRINTF("LZSS_decode:%lu\n", decode_file.file_size);
        if(!decode_file.file_size)
            return false;
        else
        {
            uint8_t flash_erase_bank = decode_file.bank?0:1;
            uint32_t flash_copy_addr = decode_file.bank?FLASH_START_BANK2:FLASH_START_BANK1;
            /* Move the decode file to encode file */
            for (i = 0; i < (decode_file.file_size + FLASH_PAGE - 1) / FLASH_PAGE; i++)
            {
                FLASH_If_Erase_Pages(flash_erase_bank, encode_file.origin_page + i);
                Flash_Bank_Copy_Bank(flash_copy_addr + (decode_file.origin_page + i) * FLASH_PAGE, flash_copy_addr + (encode_file.origin_page + i) * FLASH_PAGE, FLASH_PAGE, 0);
            }
            /* Erase page to the end of decode file */
            for (i = encode_file.origin_page + (decode_file.file_size + FLASH_PAGE - 1) / FLASH_PAGE; i < decode_file.origin_page + (decode_file.file_size + FLASH_PAGE - 1) / FLASH_PAGE; i++)
                FLASH_If_Erase_Pages(flash_erase_bank, i);

            patchSize = decode_file.file_size;
        }
    }

    /* Note:
    When patching daemon firmware, source file (page 0) and patch file (decided by the size of source file) are located at BANK1, new file (page 0) at BANK2;
    When patching FUT, source file (page 0) and patch file (decided by the size of source file) are located at BANK2, new file (decided by the size of source file and patch file) at BANK2;
    */

    /* 0. The whole process must in bank 1 */
    // assert(!READ_BIT(SYSCFG->MEMRMP, SYSCFG_MEMRMP_FB_MODE));
    Flash_FILE newFile;
    uint8_t newPage;
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
