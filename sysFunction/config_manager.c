#include "config_manager.h"
#include "stddef.h"
#include "string.h"
#include "gd25qxx.h"

// 配置参数全局变量
static config_params_t g_config_params = {0};
static uint8_t g_config_initialized = 0;

// 默认配置
static const config_params_t default_config = {
    .magic = CONFIG_MAGIC,
    .version = CONFIG_VERSION,
    .ratio = 1.0f,
    .limit = 100.0f,
    .cycle = CYCLE_5S,
    .crc32 = 0};

// CRC32查找表
static const uint32_t crc32_table[16] = {
    0x00000000, 0x1DB71064, 0x3B6E20C8, 0x26D930AC,
    0x76DC4190, 0x6B6B51F4, 0x4DB26158, 0x5005713C,
    0xEDB88320, 0xF00F9344, 0xD6D6A3E8, 0xCB61B38C,
    0x9B64C2B0, 0x86D3D2D4, 0xA00AE278, 0xBDBDF21C};

// 计算CRC32
uint32_t config_calculate_crc32(const config_params_t *params)
{
    if (params == NULL)
        return 0;

    uint32_t crc = 0xFFFFFFFF;
    const uint8_t *data = (const uint8_t *)params;
    uint32_t len = sizeof(config_params_t) - sizeof(uint32_t);

    for (uint32_t i = 0; i < len; i++)
    {
        crc = crc32_table[(crc ^ data[i]) & 0x0F] ^ (crc >> 4);
        crc = crc32_table[(crc ^ (data[i] >> 4)) & 0x0F] ^ (crc >> 4);
    }

    return crc ^ 0xFFFFFFFF;
}

// 校验ratio参数
config_status_t config_validate_ratio(float ratio)
{
    return (ratio >= 0.0f && ratio <= 100.0f) ? CONFIG_OK : CONFIG_INVALID;
}

// 校验limit参数
config_status_t config_validate_limit(float limit)
{
    return (limit >= 0.0f && limit <= 200.0f) ? CONFIG_OK : CONFIG_INVALID;
}

// 校验采样周期
config_status_t config_validate_sampling_cycle(sampling_cycle_t cycle)
{
    return (cycle == CYCLE_5S || cycle == CYCLE_10S || cycle == CYCLE_15S) ? CONFIG_OK : CONFIG_INVALID;
}

// 配置初始化
config_status_t config_init(void)
{
    if (g_config_initialized)
    {
        return CONFIG_OK;
    }

    config_status_t status = config_load_from_flash();
    if (status != CONFIG_OK)
    {
        g_config_params = default_config;
        g_config_params.crc32 = config_calculate_crc32(&g_config_params);
    }

    g_config_initialized = 1;
    return CONFIG_OK;
}

// 获取配置参数
config_status_t config_get_params(config_params_t *params)
{
    if (params == NULL)
        return CONFIG_ERROR;
    if (!g_config_initialized)
        return CONFIG_ERROR;

    *params = g_config_params;
    return CONFIG_OK;
}

// 设置配置参数
config_status_t config_set_params(const config_params_t *params)
{
    if (params == NULL)
        return CONFIG_ERROR;
    if (!g_config_initialized)
        return CONFIG_ERROR;

    if (config_validate_ratio(params->ratio) != CONFIG_OK)
        return CONFIG_INVALID;
    if (config_validate_limit(params->limit) != CONFIG_OK)
        return CONFIG_INVALID;
    if (config_validate_sampling_cycle(params->cycle) != CONFIG_OK)
        return CONFIG_INVALID;

    g_config_params.ratio = params->ratio;
    g_config_params.limit = params->limit;
    g_config_params.cycle = params->cycle;
    g_config_params.crc32 = config_calculate_crc32(&g_config_params);

    return CONFIG_OK;
}

// 恢复默认配置
config_status_t config_reset_to_default(void)
{
    g_config_params = default_config;
    g_config_params.crc32 = config_calculate_crc32(&g_config_params);
    g_config_initialized = 1;
    return CONFIG_OK;
}

// 保存配置到FLASH
config_status_t config_save_to_flash(void)
{
    if (!g_config_initialized)
        return CONFIG_ERROR;

    g_config_params.crc32 = config_calculate_crc32(&g_config_params);

    spi_flash_sector_erase(CONFIG_FLASH_ADDR);

    spi_flash_buffer_write((uint8_t *)&g_config_params, CONFIG_FLASH_ADDR, sizeof(config_params_t));

    return CONFIG_OK;
}

// 从FLASH加载配置
config_status_t config_load_from_flash(void)
{
    config_params_t temp_config;

    spi_flash_buffer_read((uint8_t *)&temp_config, CONFIG_FLASH_ADDR, sizeof(config_params_t));

    if (temp_config.magic != CONFIG_MAGIC)
    {
        return CONFIG_ERROR;
    }

    if (temp_config.version != CONFIG_VERSION)
    {
        return CONFIG_ERROR;
    }

    uint32_t calculated_crc = config_calculate_crc32(&temp_config);
    if (calculated_crc != temp_config.crc32)
    {
        return CONFIG_CRC_ERROR;
    }

    if (config_validate_ratio(temp_config.ratio) != CONFIG_OK ||
        config_validate_limit(temp_config.limit) != CONFIG_OK ||
        config_validate_sampling_cycle(temp_config.cycle) != CONFIG_OK)
    {
        return CONFIG_INVALID;
    }

    g_config_params = temp_config;
    return CONFIG_OK;
}

// 设置采样周期
config_status_t config_set_sampling_cycle(sampling_cycle_t cycle)
{
    if (!g_config_initialized)
        return CONFIG_ERROR;

    if (config_validate_sampling_cycle(cycle) != CONFIG_OK)
    {
        return CONFIG_INVALID;
    }

    g_config_params.cycle = cycle;
    g_config_params.crc32 = config_calculate_crc32(&g_config_params);

    return CONFIG_OK;
}

// 获取采样周期
sampling_cycle_t config_get_sampling_cycle(void)
{
    if (!g_config_initialized)
        return CYCLE_5S;
    return g_config_params.cycle;
}
