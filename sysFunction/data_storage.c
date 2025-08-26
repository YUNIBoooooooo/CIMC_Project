#include "data_storage.h"
#include "fatfs.h"
#include "rtc_app.h"
#include "usart_app.h"
#include "string.h"
#include "stdio.h"

// 文件状态全局变量
static file_state_t g_file_states[STORAGE_TYPE_COUNT];
static uint32_t g_boot_count = 0;
static data_storage_status_t create_default_config_ini(void);
// 目录名和文件名前缀
static const char *g_directory_names[STORAGE_TYPE_COUNT] = {

    "sample",
    "overLimit",
    "log",
    "hideData"};

static const char *g_filename_prefixes[STORAGE_TYPE_COUNT] = {

    "sampleData",
    "overLimit",
    "log",
    "hideData"};

// 获取启动次数
static uint32_t get_boot_count_from_fatfs(void)
{
    FIL file;
    uint32_t boot_count = 0;
    UINT bytes_read;
    FRESULT res;

    res = f_open(&file, "boot_count.txt", FA_READ);
    if (res == FR_OK)
    {
        res = f_read(&file, &boot_count, sizeof(boot_count), &bytes_read);
        if (res != FR_OK || bytes_read != sizeof(boot_count))
        {
            boot_count = 0;
        }
        f_close(&file);
    }

    return boot_count;
}

// 保存启动次数
static data_storage_status_t save_boot_count_to_fatfs(uint32_t boot_count)
{
    FIL file;
    UINT bytes_written;
    FRESULT res;

    res = f_open(&file, "boot_count.txt", FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK)
    {
        return DATA_STORAGE_ERROR;
    }

    res = f_write(&file, &boot_count, sizeof(boot_count), &bytes_written);
    if (res != FR_OK || bytes_written != sizeof(boot_count))
    {
        f_close(&file);
        return DATA_STORAGE_ERROR;
    }

    f_close(&file);
    return DATA_STORAGE_OK;
}

// 创建存储目录
static data_storage_status_t create_storage_directories(void)
{
    FRESULT res;
    uint8_t success_count = 0;

    for (uint8_t i = 0; i < STORAGE_TYPE_COUNT; i++)
    {
        res = f_mkdir(g_directory_names[i]);
        if (res == FR_OK)
        {

            success_count++;
        }
        else if (res == FR_EXIST)
        {

            success_count++;
        }
        else
        {
        }
    }

    return (success_count == STORAGE_TYPE_COUNT) ? DATA_STORAGE_OK : DATA_STORAGE_ERROR;
}

// 数据存储初始化
data_storage_status_t data_storage_init(void)
{
    memset(g_file_states, 0, sizeof(g_file_states));
    FRESULT mount_result = f_mount(&SDFatFS, SDPath, 1);
    if (mount_result != FR_OK)
    {
        return DATA_STORAGE_NO_SD;
    }

    data_storage_status_t dir_result = create_storage_directories();
    if (dir_result != DATA_STORAGE_OK)
    {
        my_printf(&huart1, "Warning: Some directories creation failed, system may not work properly\r\n");
    }

    g_boot_count = get_boot_count_from_fatfs();
    g_boot_count++;

    data_storage_status_t boot_result = save_boot_count_to_fatfs(g_boot_count);
    if (boot_result != DATA_STORAGE_OK)
    {
        my_printf(&huart1, "Warning: Failed to save boot count\r\n");
    }

    create_default_config_ini();

    return DATA_STORAGE_OK;
}

// 检查并更新文件名
static data_storage_status_t check_and_update_filename(storage_type_t type)
{
    if (type >= STORAGE_TYPE_COUNT)
    {
        return DATA_STORAGE_INVALID;
    }

    file_state_t *state = &g_file_states[type];

    if (state->data_count >= 10 || !state->file_exists)
    {

        char filename[64];
        data_storage_status_t result = generate_filename(type, filename);
        if (result != DATA_STORAGE_OK)
        {
            return result;
        }

        strcpy(state->current_filename, filename);
        state->data_count = 0;
        state->file_exists = 1;
    }

    return DATA_STORAGE_OK;
}

// 写数据到文件
static data_storage_status_t write_data_to_file(storage_type_t type, const char *data)
{
    if (type >= STORAGE_TYPE_COUNT || data == NULL)
    {
        return DATA_STORAGE_INVALID;
    }

    data_storage_status_t result = check_and_update_filename(type);
    if (result != DATA_STORAGE_OK)
    {
        return result;
    }

    file_state_t *state = &g_file_states[type];

    char full_path[96];
    sprintf(full_path, "%s/%s", g_directory_names[type], state->current_filename);
    FIL file_handle;
    FRESULT res = f_open(&file_handle, full_path, FA_OPEN_ALWAYS | FA_WRITE);
    if (res != FR_OK)
    {
        return DATA_STORAGE_ERROR;
    }

    res = f_lseek(&file_handle, f_size(&file_handle));
    if (res != FR_OK)
    {
        f_close(&file_handle);
        return DATA_STORAGE_ERROR;
    }

    UINT bytes_written;
    res = f_write(&file_handle, data, strlen(data), &bytes_written);
    if (res != FR_OK || bytes_written != strlen(data))
    {
        return DATA_STORAGE_ERROR;
    }

    res = f_write(&file_handle, "\n", 1, &bytes_written);
    if (res != FR_OK || bytes_written != 1)
    {
        f_close(&file_handle);
        return DATA_STORAGE_ERROR;
    }

    f_sync(&file_handle);
    f_close(&file_handle);

    state->data_count++;

    return DATA_STORAGE_OK;
}

// 格式化采样数据
static data_storage_status_t format_sample_data(float voltage, char *formatted_data)
{
    if (formatted_data == NULL)
    {
        return DATA_STORAGE_INVALID;
    }

    RTC_TimeTypeDef current_rtc_time = {0};
    RTC_DateTypeDef current_rtc_date = {0};
    HAL_RTC_GetTime(&hrtc, &current_rtc_time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &current_rtc_date, RTC_FORMAT_BIN);

    sprintf(formatted_data, "%04d-%02d-%02d %02d:%02d:%02d %.1fV",
            current_rtc_date.Year + 2000,
            current_rtc_date.Month,
            current_rtc_date.Date,
            current_rtc_time.Hours,
            current_rtc_time.Minutes,
            current_rtc_time.Seconds,
            voltage);

    return DATA_STORAGE_OK;
}

// 写采样数据
data_storage_status_t data_storage_write_sample(float voltage)
{
    char formatted_data[128];

    data_storage_status_t result = format_sample_data(voltage, formatted_data);
    if (result != DATA_STORAGE_OK)
    {
        return result;
    }

    return write_data_to_file(STORAGE_SAMPLE, formatted_data);
}

// 格式化超限数据
static data_storage_status_t format_overlimit_data(float voltage, float limit, char *formatted_data)
{
    if (formatted_data == NULL)
    {
        return DATA_STORAGE_INVALID;
    }

    RTC_TimeTypeDef current_rtc_time = {0};
    RTC_DateTypeDef current_rtc_date = {0};
    HAL_RTC_GetTime(&hrtc, &current_rtc_time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &current_rtc_date, RTC_FORMAT_BIN);

    sprintf(formatted_data, "%04d-%02d-%02d %02d:%02d:%02d %.0fV limit %.0fV",
            current_rtc_date.Year + 2000,
            current_rtc_date.Month,
            current_rtc_date.Date,
            current_rtc_time.Hours,
            current_rtc_time.Minutes,
            current_rtc_time.Seconds,
            voltage,
            limit);

    return DATA_STORAGE_OK;
}

// 写超限数据
data_storage_status_t data_storage_write_overlimit(float voltage, float limit)
{
    char formatted_data[128];

    data_storage_status_t result = format_overlimit_data(voltage, limit, formatted_data);
    if (result != DATA_STORAGE_OK)
    {
        return result;
    }

    return write_data_to_file(STORAGE_OVERLIMIT, formatted_data);
}

// 格式化日志数据
static data_storage_status_t format_log_data(const char *operation, char *formatted_data)
{
    if (formatted_data == NULL || operation == NULL)
    {
        return DATA_STORAGE_INVALID;
    }

    RTC_TimeTypeDef current_rtc_time = {0};
    RTC_DateTypeDef current_rtc_date = {0};
    HAL_RTC_GetTime(&hrtc, &current_rtc_time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &current_rtc_date, RTC_FORMAT_BIN);

    sprintf(formatted_data, "%04d-%02d-%02d %02d:%02d:%02d %s",
            current_rtc_date.Year + 2000,
            current_rtc_date.Month,
            current_rtc_date.Date,
            current_rtc_time.Hours,
            current_rtc_time.Minutes,
            current_rtc_time.Seconds,
            operation);

    return DATA_STORAGE_OK;
}

// 写日志
data_storage_status_t data_storage_write_log(const char *operation)
{
    char formatted_data[256];

    data_storage_status_t result = format_log_data(operation, formatted_data);
    if (result != DATA_STORAGE_OK)
    {
        return result;
    }

    return write_data_to_file(STORAGE_LOG, formatted_data);
}

// 格式化隐藏数据
static data_storage_status_t format_hidedata(float voltage, uint8_t is_overlimit, char *formatted_data)
{
    if (formatted_data == NULL)
    {
        return DATA_STORAGE_INVALID;
    }

    RTC_TimeTypeDef current_rtc_time = {0};
    RTC_DateTypeDef current_rtc_date = {0};
    HAL_RTC_GetTime(&hrtc, &current_rtc_time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &current_rtc_date, RTC_FORMAT_BIN);

    char original_line[128];
    sprintf(original_line, "%04d-%02d-%02d %02d:%02d:%02d %.1fV",
            current_rtc_date.Year + 2000,
            current_rtc_date.Month,
            current_rtc_date.Date,
            current_rtc_time.Hours,
            current_rtc_time.Minutes,
            current_rtc_time.Seconds,
            voltage);

    uint32_t timestamp = convert_rtc_to_unix_timestamp(&current_rtc_time, &current_rtc_date);
    char hex_output[32];
    format_hex_output(timestamp, voltage, is_overlimit, hex_output);

    sprintf(formatted_data, "%s\nhide: %s", original_line, hex_output);

    return DATA_STORAGE_OK;
}

// 写隐藏数据
data_storage_status_t data_storage_write_hidedata(float voltage, uint8_t is_overlimit)
{
    char formatted_data[256];

    data_storage_status_t result = format_hidedata(voltage, is_overlimit, formatted_data);
    if (result != DATA_STORAGE_OK)
    {
        return result;
    }

    return write_data_to_file(STORAGE_HIDEDATA, formatted_data);
}

// 生成日期时间字符串
data_storage_status_t generate_datetime_string(char *datetime_str)
{
    if (datetime_str == NULL)
    {
        return DATA_STORAGE_INVALID;
    }

    RTC_TimeTypeDef current_rtc_time = {0};
    RTC_DateTypeDef current_rtc_date = {0};
    HAL_RTC_GetTime(&hrtc, &current_rtc_time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &current_rtc_date, RTC_FORMAT_BIN);

    sprintf(datetime_str, "%04d%02d%02d%02d%02d%02d",
            current_rtc_date.Year + 2000,
            current_rtc_date.Month,
            current_rtc_date.Date,
            current_rtc_time.Hours,
            current_rtc_time.Minutes,
            current_rtc_time.Seconds);

    return DATA_STORAGE_OK;
}

// 生成文件名
data_storage_status_t generate_filename(storage_type_t type, char *filename)
{
    if (filename == NULL || type >= STORAGE_TYPE_COUNT)
    {
        return DATA_STORAGE_INVALID;
    }

    if (type == STORAGE_LOG)
    {
        sprintf(filename, "%s%lu.txt", g_filename_prefixes[type], g_boot_count - 1);
    }
    else
    {
        char datetime_str[16];
        data_storage_status_t result = generate_datetime_string(datetime_str);
        if (result != DATA_STORAGE_OK)
        {
            return result;
        }
        sprintf(filename, "%s%s.txt", g_filename_prefixes[type], datetime_str);
    }

    return DATA_STORAGE_OK;
}

// 创建默认config.ini
static data_storage_status_t create_default_config_ini(void)
{
    FIL ini_file;
    FRESULT res = f_open(&ini_file, "config.ini", FA_OPEN_EXISTING | FA_READ);
    if (res == FR_OK)
    {

        f_close(&ini_file);
        return DATA_STORAGE_OK;
    }
    res = f_open(&ini_file, "config.ini", FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK)
    {
        return DATA_STORAGE_ERROR;
    }
    const char *default_content = "[Ratio]\r\nCh0 = 1.99\r\n\r\n[Limit]\r\nCh0 = 10.11\r\n";
    UINT bw;
    f_write(&ini_file, default_content, strlen(default_content), &bw);
    f_close(&ini_file);
    return (bw == strlen(default_content)) ? DATA_STORAGE_OK : DATA_STORAGE_ERROR;
}

// 数据存储测试
data_storage_status_t data_storage_test(void)
{
    my_printf(&huart1, "Data storage system test - placeholder\r\n");
    return DATA_STORAGE_OK;
}
