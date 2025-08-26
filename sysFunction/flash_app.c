#include "flash_app.h"
#include "fatfs.h"
#include "ff.h"
#include <string.h>
#include <stdlib.h>

lfs_t lfs;
struct lfs_config cfg;

// 递归列出目录
void list_dir_recursive(const char *path, int level)
{
    lfs_dir_t dir;
    struct lfs_info info;
    char full_path[128];

    if (lfs_dir_open(&lfs, &dir, path) != LFS_ERR_OK)
    {
        my_printf(&huart1, "Failed to open directory: %s\r\n", path);
        return;
    }

    while (true)
    {
        int res = lfs_dir_read(&lfs, &dir, &info);
        if (res <= 0)
        {
            break;
        }
        if (strcmp(info.name, ".") == 0 || strcmp(info.name, "..") == 0)
        {
            continue;
        }

        for (int i = 0; i < level; i++)
        {
            my_printf(&huart1, "    ");
        }

        if (info.type == LFS_TYPE_DIR)
        {
            my_printf(&huart1, "+-- [DIR] %s\r\n", info.name);

            if (strcmp(path, "/") == 0)
            {
                sprintf(full_path, "/%s", info.name);
            }
            else
            {
                sprintf(full_path, "%s/%s", path, info.name);
            }

            list_dir_recursive(full_path, level + 1);
        }
        else
        {
            my_printf(&huart1, "+-- [FILE] %s (%lu bytes)\r\n", info.name, (unsigned long)info.size);
        }
    }

    lfs_dir_close(&lfs, &dir);
}

// LittleFS基本测试
void lfs_basic_test(void)
{
    my_printf(&huart1, "\r\n--- LittleFS File System Test ---\r\n");
    int err = lfs_mount(&lfs, &cfg);
    if (err)
    {
        my_printf(&huart1, "LFS: Mount failed(%d), formatting...\n", err);
        if (lfs_format(&lfs, &cfg) || (err = lfs_mount(&lfs, &cfg)))
        {
            my_printf(&huart1, "LFS: Format/Mount failed(%d)!\n", err);
            return;
        }
        my_printf(&huart1, "LFS: Format & Mount OK.\n");
    }
    else
    {
        my_printf(&huart1, "LFS: Mount successful.\n");
    }
    err = lfs_mkdir(&lfs, "boot");
    if (err && err != LFS_ERR_EXIST)
    {
        my_printf(&huart1, "LFS: Failed to create 'boot' directory(%d)!\n", err);
        goto end_test;
    }
    if (err == LFS_ERR_OK)
    {
        my_printf(&huart1, "LFS: Directory 'boot' created successfully.\n");
    }

    uint32_t boot_count = 0;
    lfs_file_t file;
    const char *filename = "boot/boot_cnt.txt";
    err = lfs_file_open(&lfs, &file, filename, LFS_O_RDWR | LFS_O_CREAT);
    if (err)
    {
        my_printf(&huart1, "LFS: Failed to open file '%s'(%d)!\n", filename, err);
        goto end_test;
    }

    lfs_ssize_t r_sz = lfs_file_read(&lfs, &file, &boot_count, sizeof(boot_count));
    if (r_sz < 0)
    {
        my_printf(&huart1, "LFS: Failed to read file '%s'(%ld), initializing counter.\n", filename, (long)r_sz);
        boot_count = 0;
    }
    else if (r_sz != sizeof(boot_count))
    {
        my_printf(&huart1, "LFS: Read %ld bytes from '%s' (expected %d), initializing counter.\n", (long)r_sz, filename, (int)sizeof(boot_count));
        boot_count = 0;
    }

    boot_count++;
    my_printf(&huart1, "LFS: File '%s' current boot count: %lu\n", filename, boot_count);

    err = lfs_file_rewind(&lfs, &file);
    if (err)
    {
        my_printf(&huart1, "LFS: Failed to rewind file '%s'(%d)!\n", filename, err);
        lfs_file_close(&lfs, &file);
        goto end_test;
    }

    lfs_ssize_t w_sz = lfs_file_write(&lfs, &file, &boot_count, sizeof(boot_count));
    if (w_sz < 0)
    {
        my_printf(&huart1, "LFS: Failed to write file '%s'(%ld)!\n", filename, (long)w_sz);
    }
    else if (w_sz != sizeof(boot_count))
    {
        my_printf(&huart1, "LFS: Partial write to '%s' (%ld/%d bytes)!\n", filename, (long)w_sz, (int)sizeof(boot_count));
    }
    else
    {
        my_printf(&huart1, "LFS: File '%s' updated successfully.\n", filename);
    }

    if (lfs_file_close(&lfs, &file))
    {
        my_printf(&huart1, "LFS: Failed to close file '%s'!\n", filename);
    }
    my_printf(&huart1, "\r\n[File System Structure]\r\n");
    my_printf(&huart1, "/ (root directory)\r\n");
    list_dir_recursive("/", 0);

end_test:
    my_printf(&huart1, "--- LittleFS File System Test End ---\r\n");
}

// SPI FLASH测试
void test_spi_flash(void)
{
    uint32_t flash_id;
    uint8_t write_buffer[SPI_FLASH_PAGE_SIZE];
    uint8_t read_buffer[SPI_FLASH_PAGE_SIZE];
    uint32_t test_addr = 0x000000;

    my_printf(&huart1, "SPI FLASH Test Start\r\n");

    spi_flash_init();
    my_printf(&huart1, "SPI Flash Initialized.\r\n");

    flash_id = spi_flash_read_id();
    my_printf(&huart1, "Flash ID: 0x%lX\r\n", flash_id);

    my_printf(&huart1, "Erasing sector at address 0x%lX...\r\n", test_addr);
    spi_flash_sector_erase(test_addr);
    my_printf(&huart1, "Sector erased.\r\n");

    spi_flash_buffer_read(read_buffer, test_addr, SPI_FLASH_PAGE_SIZE);
    int erased_check_ok = 1;
    for (int i = 0; i < SPI_FLASH_PAGE_SIZE; i++)
    {
        if (read_buffer[i] != 0xFF)
        {
            erased_check_ok = 0;
            break;
        }
    }
    if (erased_check_ok)
    {
        my_printf(&huart1, "Erase check PASSED. Sector is all 0xFF.\r\n");
    }
    else
    {
        my_printf(&huart1, "Erase check FAILED.\r\n");
    }

    const char *message = "Hello from STM32 to SPI FLASH! Microunion Studio Test - 12345.";
    uint16_t data_len = strlen(message);
    if (data_len >= SPI_FLASH_PAGE_SIZE)
    {
        data_len = SPI_FLASH_PAGE_SIZE - 1;
    }
    memset(write_buffer, 0, SPI_FLASH_PAGE_SIZE);
    memcpy(write_buffer, message, data_len);
    write_buffer[data_len] = '\0';

    my_printf(&huart1, "Writing data to address 0x%lX: \"%s\"\r\n", test_addr, write_buffer);
    spi_flash_buffer_write(write_buffer, test_addr, SPI_FLASH_PAGE_SIZE);

    my_printf(&huart1, "Data written.\r\n");

    my_printf(&huart1, "Reading data from address 0x%lX...\r\n", test_addr);
    memset(read_buffer, 0, SPI_FLASH_PAGE_SIZE);
    spi_flash_buffer_read(read_buffer, test_addr, SPI_FLASH_PAGE_SIZE);
    my_printf(&huart1, "Data read: \"%s\"\r\n", read_buffer);

    if (memcmp(write_buffer, read_buffer, SPI_FLASH_PAGE_SIZE) == 0)
    {
        my_printf(&huart1, "Data VERIFIED! Write and Read successful.\r\n");
    }
    else
    {
        my_printf(&huart1, "Data VERIFICATION FAILED!\r\n");
    }

    my_printf(&huart1, "SPI FLASH Test End\r\n");
}

// SD卡FATFS测试
void test_sd_fatfs(void)
{
    FRESULT res;
    DIR dir;
    FILINFO fno;
    uint32_t byteswritten;
    uint32_t bytesread;
    char ReadBuffer[256];
    char WriteBuffer[] = "123";
    UINT bw, br;
    const char *TestFileName = "SD_TEST.TXT";
    if (f_mount(&SDFatFS, SDPath, 1) != FR_OK)
    {
        return;
    }
    res = f_mkdir("??????");
    res = f_open(&SDFile, TestFileName, FA_CREATE_ALWAYS | FA_WRITE);
    if (res == FR_OK)
    {
        res = f_write(&SDFile, WriteBuffer, strlen(WriteBuffer), &bw);
        f_close(&SDFile);
    }

    memset(ReadBuffer, 0, sizeof(ReadBuffer));
    res = f_open(&SDFile, TestFileName, FA_READ);
    if (res == FR_OK)
    {
        res = f_read(&SDFile, ReadBuffer, sizeof(ReadBuffer) - 1, &br);
        if (res == FR_OK)
        {
            ReadBuffer[br] = '\0';
        }
        f_close(&SDFile);
    }

    res = f_opendir(&dir, "/");
    if (res == FR_OK)
    {
        for (;;)
        {
            res = f_readdir(&dir, &fno);
            if (res != FR_OK || fno.fname[0] == 0)
                break;
        }
        f_closedir(&dir);
    }
    f_mount(NULL, SDPath, 0);
}