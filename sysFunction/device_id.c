#include "device_id.h"
#include "gd25qxx.h"
#include "usart_app.h"
#include "string.h"
#include "stdio.h"

// 设备ID全局变量
static char g_device_id[DEVICE_ID_MAX_LENGTH] = {0};

// 设备ID初始化
device_id_status_t device_id_init(void)
{
    char device_id[DEVICE_ID_MAX_LENGTH];

    device_id_status_t status = device_id_read(device_id);

    if (status != DEVICE_ID_OK)
    {

        status = device_id_set_default();
        if (status != DEVICE_ID_OK)
        {
            return status;
        }

        status = device_id_read(device_id);
        if (status != DEVICE_ID_OK)
        {
            return status;
        }
    }

    strcpy(g_device_id, device_id);

    return DEVICE_ID_OK;
}

// 读取设备ID
device_id_status_t device_id_read(char *device_id)
{
    if (device_id == NULL)
    {
        return DEVICE_ID_INVALID;
    }

    uint8_t buffer[DEVICE_ID_MAX_LENGTH];

    spi_flash_buffer_read(buffer, DEVICE_ID_FLASH_ADDR, DEVICE_ID_MAX_LENGTH);

    if (strncmp((char *)buffer, "Device_ID:", 10) != 0)
    {
        return DEVICE_ID_NOT_FOUND;
    }

    strncpy(device_id, (char *)buffer, DEVICE_ID_MAX_LENGTH - 1);
    device_id[DEVICE_ID_MAX_LENGTH - 1] = '\0';

    return DEVICE_ID_OK;
}

// 写入设备ID
device_id_status_t device_id_write(const char *device_id)
{
    if (device_id == NULL || strlen(device_id) >= DEVICE_ID_MAX_LENGTH)
    {
        return DEVICE_ID_INVALID;
    }

    uint8_t buffer[DEVICE_ID_MAX_LENGTH];
    memset(buffer, 0, sizeof(buffer));

    strncpy((char *)buffer, device_id, DEVICE_ID_MAX_LENGTH - 1);

    spi_flash_sector_erase(DEVICE_ID_FLASH_ADDR);

    spi_flash_buffer_write(buffer, DEVICE_ID_FLASH_ADDR, DEVICE_ID_MAX_LENGTH);

    strcpy(g_device_id, device_id);

    return DEVICE_ID_OK;
}

// 设置默认设备ID
device_id_status_t device_id_set_default(void)
{
    const char *default_id = "Device_ID:2025-CIMC-2025514171";
    return device_id_write(default_id);
}

// 打印启动信息
void device_id_print_startup_info(void)
{
    my_printf(&huart1, "======system init======\r\n");

    if (strlen(g_device_id) > 0)
    {
        my_printf(&huart1, "%s\r\n", g_device_id);
    }
    else
    {
        my_printf(&huart1, "Device_ID:2025-CIMC-2025514171\r\n");
    }

    my_printf(&huart1, "======system ready======\r\n");
}
