#include "ini_parser.h"
#include "string.h"
#include "stdlib.h"
#include "ctype.h"

// 解析状态枚举
typedef enum
{
    PARSE_IDLE = 0,
    PARSE_RATIO = 1,
    PARSE_LIMIT = 2
} parse_state_t;

// 去除字符串首尾空白
ini_status_t ini_trim_string(char *str)
{
    if (str == NULL)
        return INI_ERROR;

    char *start = str;
    while (*start && isspace(*start))
        start++;

    char *end = start + strlen(start) - 1;
    while (end > start && isspace(*end))
        end--;
    *(end + 1) = '\0';

    if (start != str)
    {
        memmove(str, start, strlen(start) + 1);
    }

    return INI_OK;
}

// 解析浮点数
ini_status_t ini_parse_float(const char *str, float *value)
{
    if (str == NULL || value == NULL)
        return INI_ERROR;

    char *endptr;
    *value = strtof(str, &endptr);

    if (endptr == str || *endptr != '\0')
    {
        return INI_VALUE_ERROR;
    }

    return INI_OK;
}

// 解析一行
ini_status_t ini_parse_line(const char *line, ini_config_t *config)
{
    if (line == NULL || config == NULL)
        return INI_ERROR;

    static parse_state_t current_state = PARSE_IDLE;
    char line_buffer[128];

    strncpy(line_buffer, line, sizeof(line_buffer) - 1);
    line_buffer[sizeof(line_buffer) - 1] = '\0';

    ini_trim_string(line_buffer);

    if (strlen(line_buffer) == 0 || line_buffer[0] == ';' || line_buffer[0] == '#')
    {
        return INI_OK;
    }

    if (line_buffer[0] == '[')
    {
        char *end_bracket = strchr(line_buffer, ']');
        if (end_bracket == NULL)
            return INI_FORMAT_ERROR;

        *end_bracket = '\0';
        char *section_name = line_buffer + 1;
        ini_trim_string(section_name);

        if (strcmp(section_name, "Ratio") == 0)
        {
            current_state = PARSE_RATIO;
        }
        else if (strcmp(section_name, "Limit") == 0)
        {
            current_state = PARSE_LIMIT;
        }
        else
        {
            current_state = PARSE_IDLE;
        }

        return INI_OK;
    }

    char *equal_sign = strchr(line_buffer, '=');
    if (equal_sign == NULL)
        return INI_FORMAT_ERROR;

    *equal_sign = '\0';
    char *key = line_buffer;
    char *value = equal_sign + 1;

    ini_trim_string(key);
    ini_trim_string(value);

    if (strcmp(key, "Ch0") == 0)
    {
        float parsed_value;
        if (ini_parse_float(value, &parsed_value) != INI_OK)
        {
            return INI_VALUE_ERROR;
        }

        if (current_state == PARSE_RATIO)
        {
            config->ratio = parsed_value;
            config->ratio_found = 1;
        }
        else if (current_state == PARSE_LIMIT)
        {
            config->limit = parsed_value;
            config->limit_found = 1;
        }
    }

    return INI_OK;
}

// 解析INI文件
ini_status_t ini_parse_file(const char *filename, ini_config_t *config)
{
    if (filename == NULL || config == NULL)
        return INI_ERROR;

    FIL file;
    FRESULT fr;
    char line_buffer[128];
    UINT bytes_read;

    config->ratio = 0.0f;
    config->limit = 0.0f;
    config->ratio_found = 0;
    config->limit_found = 0;

    fr = f_open(&file, filename, FA_READ);
    if (fr != FR_OK)
    {
        return INI_FILE_NOT_FOUND;
    }

    while (f_gets(line_buffer, sizeof(line_buffer), &file) != NULL)
    {
        ini_status_t status = ini_parse_line(line_buffer, config);
        if (status != INI_OK)
        {
            f_close(&file);
            return status;
        }
    }

    f_close(&file);

    return INI_OK;
}
