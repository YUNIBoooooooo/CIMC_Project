#include "system_check.h"
#include "gd25qxx.h"
#include "ff.h"
#include "fatfs.h"
#include "rtc_app.h"
#include "diskio.h"

// FLASH型号信息结构体
typedef struct
{
    uint32_t id;
    const char *model;
    uint32_t capacity_mb;
} flash_model_t;

// FLASH型号表
static const flash_model_t flash_models[] = {
    {0xC84017, "GD25Q64", 8},
    {0xC84016, "GD25Q32", 4},
    {0xC84015, "GD25Q16", 2},
    {0xC84014, "GD25Q80", 1},
    {0xC84013, "GD25Q40", 1},
    {0xEF4017, "W25Q64", 8},
    {0xEF4016, "W25Q32", 4},
    {0xEF4015, "W25Q16", 2},
    {0x000000, "Unknown", 0}};

// 获取FLASH型号名称
const char *get_flash_model_name(uint32_t flash_id)
{
    for (int i = 0; flash_models[i].id != 0x000000; i++)
    {
        if (flash_models[i].id == flash_id)
        {
            return flash_models[i].model;
        }
    }
    return "Unknown";
}

// 获取FLASH容量
static uint32_t get_flash_capacity(uint32_t flash_id)
{
    for (int i = 0; flash_models[i].id != 0x000000; i++)
    {
        if (flash_models[i].id == flash_id)
        {
            return flash_models[i].capacity_mb;
        }
    }
    return 0;
}

// 获取SD卡容量
static uint32_t sd_card_capacity_get(void)
{
    DWORD sector_count = 0;
    WORD sector_size = 0;

    if (disk_ioctl(0, GET_SECTOR_COUNT, &sector_count) == RES_OK)
    {

        if (disk_ioctl(0, GET_SECTOR_SIZE, &sector_size) == RES_OK)
        {
            return (uint32_t)((uint64_t)sector_count * sector_size / 1024);
        }
    }
    return 0;
}

// 检查FLASH状态
system_check_status_t check_flash_status(flash_info_t *flash_info)
{
    if (flash_info == NULL)
    {
        return SYSTEM_CHECK_ERROR;
    }

    flash_info->flash_id = spi_flash_read_id();

    if (flash_info->flash_id == 0x000000 || flash_info->flash_id == 0xFFFFFF)
    {
        flash_info->status = SYSTEM_CHECK_NOT_FOUND;
        strcpy(flash_info->model_name, "Not Found");
        flash_info->capacity_mb = 0;
        return SYSTEM_CHECK_NOT_FOUND;
    }
    const char *model = get_flash_model_name(flash_info->flash_id);
    strcpy(flash_info->model_name, model);
    flash_info->capacity_mb = get_flash_capacity(flash_info->flash_id);
    flash_info->status = SYSTEM_CHECK_OK;

    return SYSTEM_CHECK_OK;
}

// 检查TF卡状态
system_check_status_t check_tf_card_status(sd_card_info_t *sd_info)
{
    if (sd_info == NULL)
    {
        return SYSTEM_CHECK_ERROR;
    }

    DSTATUS sd_status = disk_initialize(0);
    if (sd_status == 0)
    {
        uint32_t capacity_kb = sd_card_capacity_get();

        sd_info->capacity_mb = capacity_kb / 1024;
        sd_info->sector_size = 512;
        sd_info->sector_count = capacity_kb * 2;
        sd_info->status = SYSTEM_CHECK_OK;

        return SYSTEM_CHECK_OK;
    }
    else
    {
        sd_info->status = SYSTEM_CHECK_NOT_FOUND;
        sd_info->capacity_mb = 0;
        sd_info->sector_count = 0;
        sd_info->sector_size = 0;

        return SYSTEM_CHECK_NOT_FOUND;
    }
}

// 打印RTC时间
void print_rtc_time(void)
{
    rtc_print_current_time();
}
void print_rtc_time_now(void)
{
    rtc_print_current_time_now();
}

// 打印系统信息
void print_system_info(const system_info_t *info)
{
    my_printf(&huart1, "======system selftest======\r\n");

    if (info->flash_info.status == SYSTEM_CHECK_OK)
    {
        my_printf(&huart1, "flash......ok\r\n");
    }
    else
    {
        my_printf(&huart1, "flash......error\r\n");
    }
    if (info->sd_info.status == SYSTEM_CHECK_OK)
    {
        my_printf(&huart1, "TF card......ok\r\n");
    }
    else
    {
        my_printf(&huart1, "TF card......error\r\n");
    }
    my_printf(&huart1, "flash ID:0x%06X\r\n", info->flash_info.flash_id);
    if (info->sd_info.status == SYSTEM_CHECK_OK)
    {
        uint32_t capacity_kb = info->sd_info.capacity_mb * 1024;
        my_printf(&huart1, "TF card memory: %d KB\r\n", capacity_kb);
    }
    else
    {
        my_printf(&huart1, "can not find TF card\r\n");
    }

    my_printf(&huart1, "RTC: ");
    print_rtc_time();

    my_printf(&huart1, "======system selftest======\r\n");
}

// 系统自检
void system_self_check(void)
{
    system_info_t system_info = {0};

    check_flash_status(&system_info.flash_info);
    check_tf_card_status(&system_info.sd_info);
    system_info.rtc_status = SYSTEM_CHECK_OK;

    print_system_info(&system_info);
}
