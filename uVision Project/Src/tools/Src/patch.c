
#include "flash_if.h"
#include "chirpbox_func.h"
#include <string.h>
#define JANPATCH_STREAM Flash_FILE // use POSIX FILE
#include "janpatch.h"
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

#define BYTE unsigned char
#define WORD unsigned short
#define DWORD unsigned int

#define TRUE 1
#define FALSE 0

BYTE bThreshold;  //压缩阈值、长度大于等于2的匹配串才有必要压缩

BYTE bPreBufSizeBits;  //前向缓冲区占用的比特位
BYTE bWindowBufSizeBits;  //滑动窗口占用的比特位

WORD wPreBufSize;  //通过占用的比特位计算缓冲区大小
WORD wWindowBufSize;  //通过占用的比特位计算滑动窗口大小

// BYTE bPreBuf[1024];  //前向缓冲区
// BYTE bWindowBuf[4196];  //滑动窗口
// BYTE bMatchString[1024];  //匹配串
WORD wMatchIndex;  //滑动窗口匹配串起始下标

BYTE FindSameString(BYTE *pbStrA, WORD wLenA, BYTE *pbStrB, WORD wLenB, WORD *pwMatchIndex);  //查找匹配串
// DWORD LZSS_encode(Flash_FILE *pbReadFileName, Flash_FILE *pbWriteFileName);  //文件压缩
// DWORD LZSS_decode(Flash_FILE *pbReadFileName, Flash_FILE *pbWriteFileName);  //文件解压

// int main()
// {
// 	bThreshold = 2;
// 	bPreBufSizeBits = 6;
// 	bWindowBufSizeBits = 16 - bPreBufSizeBits;
// 	wPreBufSize = ((WORD)1 << bPreBufSizeBits) - 1 + bThreshold;
// 	wWindowBufSize = ((WORD)1 << bWindowBufSizeBits) - 1 + bThreshold;
//     printf("hello\n");

// 	LZSS_encode("115.bin", "encode.bin");
// 	LZSS_decode("encode", "decode.bin");
// 	return 0;
// }

BYTE FindSameString(BYTE *pbStrA, WORD wLenA, BYTE *pbStrB, WORD wLenB, WORD *pwMatchIndex)
{
	WORD i, j;

	for (i = 0; i < wLenA; i++)
	{
		if ((wLenA - i) < wLenB)
		{
			return FALSE;
		}

		if (pbStrA[i] == pbStrB[0])
		{
			for (j = 1; j < wLenB; j++)
			{
				if (pbStrA[i + j] != pbStrB[j])
				{
					break;
				}
			}

			if (j == wLenB)
			{
				*pwMatchIndex = i;
				return TRUE;
			}
		}
	}
	return FALSE;
}

static int process_fread(janpatch_ctx *ctx, janpatch_buffer *source, size_t count, uint8_t *buffer) {
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

static int process_fwrite(janpatch_ctx *ctx, janpatch_buffer *target, size_t count, uint8_t *buffer) {
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

uint32_t LZSS_encode(Flash_FILE *pbReadFileName, Flash_FILE *pbWriteFileName)
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
		NULL};

    ctx.source_buffer.current_page = 0xffffffff;
    ctx.patch_buffer.current_page = 0xffffffff;
    ctx.target_buffer.current_page = 0xffffffff;

    ctx.source_buffer.position = 0;
    ctx.patch_buffer.position = 0;
    ctx.target_buffer.position = 0;

    ctx.source_buffer.stream = pbReadFileName;
    // ctx.patch_buffer.stream = patch;
    ctx.target_buffer.stream = pbWriteFileName;

	bThreshold = 2;
	bPreBufSizeBits = 6;
	bWindowBufSizeBits = 16 - bPreBufSizeBits;
	wPreBufSize = ((WORD)1 << bPreBufSizeBits) - 1 + bThreshold;
	wWindowBufSize = ((WORD)1 << bWindowBufSizeBits) - 1 + bThreshold;

	WORD i, j;
	WORD wPreBufCnt = 0;
	WORD wWindowBufCnt = 0;
	WORD wMatchStringCnt = 0;
	BYTE bRestoreBuf[17] = { 0 };
	BYTE bRestoreBufCnt = 1;
	BYTE bItemNum = 0;
	// FILE *pfRead = fopen(pbReadFileName, "rb");
	// FILE *pfWrite = fopen(pbWriteFileName, "wb");
	Flash_FILE *pfRead = pbReadFileName;
	Flash_FILE *pfWrite = pbWriteFileName;

    BYTE *bPreBuf = (BYTE *)malloc(1024);
    BYTE *bWindowBuf = (BYTE *)malloc(4196);
    BYTE *bMatchString = (BYTE *)malloc(1024);
	//前向缓冲区没数据可操作了即为压缩结束
	while (wPreBufCnt += process_fread(&ctx, &ctx.source_buffer, wPreBufSize - wPreBufCnt, &bPreBuf[wPreBufCnt]))
	{
		wMatchStringCnt = 0;  //刚开始没有匹配到数据
		wMatchIndex = 0xFFFF;  //初始化一个最大值，表示没匹配到

		for (i = bThreshold; i <= wPreBufCnt; i++)  //在滑动窗口中寻找最长的匹配串
		{
			if (TRUE == FindSameString(bWindowBuf, wWindowBufCnt, bPreBuf, i, &wMatchIndex))
			{
				memcpy(bMatchString, &bWindowBuf[wMatchIndex], i);
				wMatchStringCnt = i;
			}
			else
			{
				break;
			}
		}

		//如果没找到匹配串或者匹配长度为1，直接输出原始数据
		if ((0xFFFF == wMatchIndex))
		{
			wMatchStringCnt = 1;
			bMatchString[0] = bPreBuf[0];
			bRestoreBuf[bRestoreBufCnt++] = bPreBuf[0];
		}
		else
		{
			j = (wMatchIndex << bPreBufSizeBits) + wMatchStringCnt - bThreshold;
			bRestoreBuf[bRestoreBufCnt++] = (BYTE)j;
			bRestoreBuf[bRestoreBufCnt++] = (BYTE)(j >> 8);
			bRestoreBuf[0] |= (BYTE)1 << (7 - bItemNum);
		}

		bItemNum += 1;  //操作完一个项目+1

		if (bItemNum >= 8)  //项目数达到8了，说明做完了一组压缩，将这一组数据写入文件，同时清空缓存
		{
            process_fwrite(&ctx, &ctx.target_buffer, bRestoreBufCnt, bRestoreBuf);
			bItemNum = 0;
			memset(bRestoreBuf, 0, sizeof(bRestoreBuf));
			bRestoreBufCnt = 1;
		}

		//将刚刚匹配过的数据移出前向缓冲区
		for (i = 0; i < (wPreBufCnt - wMatchStringCnt); i++)
		{
			bPreBuf[i] = bPreBuf[i + wMatchStringCnt];
		}
		wPreBufCnt -= wMatchStringCnt;

		//如果滑动窗口将要溢出，先提前把前面的部分数据移出窗口
		if ((wWindowBufCnt + wMatchStringCnt) >  wWindowBufSize)
		{
			j = ((wWindowBufCnt + wMatchStringCnt) - wWindowBufSize);
			for (i = 0; i < (wWindowBufSize - j); i++)
			{
				bWindowBuf[i] = bWindowBuf[i + j];
			}
			wWindowBufCnt = wWindowBufSize - wMatchStringCnt;
		}

		//将刚刚匹配过的数据加入滑动窗口
		memcpy((BYTE *)&bWindowBuf[wWindowBufCnt], bMatchString, wMatchStringCnt);
		wWindowBufCnt += wMatchStringCnt;
	}

	//文件最后可能不满一组数据量，直接写到文件里
	if (0 != bRestoreBufCnt)
	{
        process_fwrite(&ctx, &ctx.target_buffer, bRestoreBufCnt, bRestoreBuf);
	}

    ctx.target_buffer.stream->file_size = jp_final_flush(&ctx, &ctx.target_buffer) + (ctx.target_buffer.stream->now_page - ctx.target_buffer.stream->origin_page) * ctx.target_buffer.size;
    printf("target:%lu\n", ctx.target_buffer.stream->file_size);

    free(bPreBuf);
    free(bWindowBuf);
    free(bMatchString);

    free(ctx.source_buffer.buffer);
    free(ctx.patch_buffer.buffer);
    free(ctx.target_buffer.buffer);

	return (ctx.target_buffer.stream->file_size);
}

uint32_t LZSS_decode(Flash_FILE *pbReadFileName, Flash_FILE *pbWriteFileName)
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
		NULL};

    ctx.source_buffer.current_page = 0xffffffff;
    ctx.patch_buffer.current_page = 0xffffffff;
    ctx.target_buffer.current_page = 0xffffffff;

    ctx.source_buffer.position = 0;
    ctx.patch_buffer.position = 0;
    ctx.target_buffer.position = 0;

    ctx.source_buffer.stream = pbReadFileName;
    // ctx.patch_buffer.stream = patch;
    ctx.target_buffer.stream = pbWriteFileName;

	bThreshold = 2;
	bPreBufSizeBits = 6;
	bWindowBufSizeBits = 16 - bPreBufSizeBits;
	wPreBufSize = ((WORD)1 << bPreBufSizeBits) - 1 + bThreshold;
	wWindowBufSize = ((WORD)1 << bWindowBufSizeBits) - 1 + bThreshold;

	WORD i, j;
	BYTE bItemNum;
	BYTE bFlag;
	WORD wStart;
	WORD wMatchStringCnt = 0;
	WORD wWindowBufCnt = 0;
	Flash_FILE *pfRead = pbReadFileName;
	Flash_FILE *pfWrite = pbWriteFileName;

    BYTE *bPreBuf = (BYTE *)malloc(1024);
    BYTE *bWindowBuf = (BYTE *)malloc(4196);
    BYTE *bMatchString = (BYTE *)malloc(1024);

	while (0 != process_fread(&ctx, &ctx.source_buffer, 1, &bFlag))  //先读一个标记字节以确定接下来怎么解压数据
	{
		for (bItemNum = 0; bItemNum < 8; bItemNum++)  //8个项目为一组进行解压
		{
			//从标记字节的最高位开始解析，0代表原始数据，1代表(下标，匹配数)解析
			if (0 == (bFlag & ((BYTE)1 << (7 - bItemNum))))
			{
				if (process_fread(&ctx, &ctx.source_buffer, 1, bPreBuf) < 1)
				{
					goto LZSS_decode_out_;
				}
                process_fwrite(&ctx, &ctx.target_buffer, 1, bPreBuf);

				bMatchString[0] = bPreBuf[0];
				wMatchStringCnt = 1;
			}
			else
			{
				if (process_fread(&ctx, &ctx.source_buffer, 2, bPreBuf) < 2)
				{
					goto LZSS_decode_out_;
				}
				//取出高位的滑动窗口匹配串下标
				wStart = ((WORD)bPreBuf[0] | ((WORD)bPreBuf[1] << 8)) / ((WORD)1 << bPreBufSizeBits);
				//取出低位的匹配长度
				wMatchStringCnt = ((WORD)bPreBuf[0] | ((WORD)bPreBuf[1] << 8)) % ((WORD)1 << bPreBufSizeBits) + bThreshold;
				//将解压出的数据写入文件
                process_fwrite(&ctx, &ctx.target_buffer, wMatchStringCnt, &bWindowBuf[wStart]);
				memcpy(bMatchString, &bWindowBuf[wStart], wMatchStringCnt);
			}
			//如果滑动窗口将要溢出，先提前把前面的部分数据移出窗口
			if ((wWindowBufCnt + wMatchStringCnt) > wWindowBufSize)
			{
				j = (wWindowBufCnt + wMatchStringCnt) - wWindowBufSize;
				for (i = 0; i < wWindowBufCnt - j; i++)
				{
					bWindowBuf[i] = bWindowBuf[i + j];
				}
				wWindowBufCnt -= j;
			}

			//将解压处的数据同步写入到滑动窗口
			memcpy(&bWindowBuf[wWindowBufCnt], bMatchString, wMatchStringCnt);
			wWindowBufCnt += wMatchStringCnt;
			// printf("3wMatchStringCnt:%lu, %lu, %lu\n", wMatchStringCnt, wWindowBufCnt, wWindowBufSize);
		}
	}

LZSS_decode_out_:
    ctx.target_buffer.stream->file_size = jp_final_flush(&ctx, &ctx.target_buffer) + (ctx.target_buffer.stream->now_page - ctx.target_buffer.stream->origin_page) * ctx.target_buffer.size;
    printf("target:%lu\n", ctx.target_buffer.stream->file_size);

    free(bPreBuf);
    free(bWindowBuf);
    free(bMatchString);

    free(ctx.source_buffer.buffer);
    free(ctx.patch_buffer.buffer);
    free(ctx.target_buffer.buffer);

	return (ctx.target_buffer.stream->file_size);
}
