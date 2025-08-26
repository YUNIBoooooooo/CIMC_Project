#include "rtc_app.h"
#include "stm32f4xx_hal_rtc.h"
#include "stdio.h"
#include "string.h"

// RTC时间和日期全局变量
RTC_TimeTypeDef time;
RTC_DateTypeDef date;

// 星期和月份名称
static const char *weekday_names[] = {
    "", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};

static const char *month_names[] = {
    "", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

// RTC处理函数
void rtc_proc(void)
{
    HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
}

// 解析时间字符串
static HAL_StatusTypeDef parse_time_string(const char *time_str, RTC_TimeTypeDef *sTime, RTC_DateTypeDef *sDate)
{
    if (time_str == NULL || sTime == NULL || sDate == NULL)
    {
        return HAL_ERROR;
    }

    int year, month, day, hour, minute, second;
    int parsed = 0;

    parsed = sscanf(time_str, "%d年%d月%d日%d:%d:%d", &year, &month, &day, &hour, &minute, &second);

    if (parsed != 6)
    {
        parsed = sscanf(time_str, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);
    }

    if (parsed != 6)
    {
        parsed = sscanf(time_str, "%d-%d-%d %d-%d-%d", &year, &month, &day, &hour, &minute, &second);
    }

    if (parsed != 6)
    {
        return HAL_ERROR;
    }

    if (year < 2000 || year > 2099 || month < 1 || month > 12 ||
        day < 1 || day > 31 || hour > 23 || minute > 59 || second > 59)
    {
        return HAL_ERROR;
    }

    sTime->Hours = hour;
    sTime->Minutes = minute;
    sTime->Seconds = second;
    sTime->DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime->StoreOperation = RTC_STOREOPERATION_RESET;

    sDate->Year = year - 2000;
    sDate->Month = month;
    sDate->Date = day;
    sDate->WeekDay = RTC_WEEKDAY_MONDAY;

    return HAL_OK;
}

// 设置RTC时间（字符串输入）
HAL_StatusTypeDef rtc_set_time_from_string(const char *time_str)
{
    if (time_str == NULL)
    {
        return HAL_ERROR;
    }

    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    if (parse_time_string(time_str, &sTime, &sDate) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

// 打印当前时间（now格式）
void rtc_print_current_time_now(void)
{
    RTC_TimeTypeDef current_time = {0};
    RTC_DateTypeDef current_date = {0};

    HAL_RTC_GetTime(&hrtc, &current_time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &current_date, RTC_FORMAT_BIN);

    my_printf(&huart1, "Current Time:%04d-%02d-%02d %02d:%02d:%02d\r\n",
              current_date.Year + 2000,
              current_date.Month,
              current_date.Date,
              current_time.Hours,
              current_time.Minutes,
              current_time.Seconds);
}

// 打印当前时间
void rtc_print_current_time(void)
{
    RTC_TimeTypeDef current_time = {0};
    RTC_DateTypeDef current_date = {0};

    HAL_RTC_GetTime(&hrtc, &current_time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &current_date, RTC_FORMAT_BIN);

    my_printf(&huart1, "%04d-%02d-%02d %02d:%02d:%02d\r\n",
              current_date.Year + 2000,
              current_date.Month,
              current_date.Date,
              current_time.Hours,
              current_time.Minutes,
              current_time.Seconds);
}

// 获取当前时间信息
void rtc_get_time_info(RTC_TimeTypeDef *current_time, RTC_DateTypeDef *current_date)
{
    if (current_time != NULL && current_date != NULL)
    {
        HAL_RTC_GetTime(&hrtc, current_time, RTC_FORMAT_BIN);
        HAL_RTC_GetDate(&hrtc, current_date, RTC_FORMAT_BIN);
    }
}

// 格式化时间输出
void format_time_output(const RTC_TimeTypeDef *sTime, const RTC_DateTypeDef *sDate, char *buffer, size_t buffer_size)
{
    if (sTime != NULL && sDate != NULL && buffer != NULL && buffer_size > 0)
    {
        snprintf(buffer, buffer_size, "%04d-%02d-%02d %02d:%02d:%02d",
                 sDate->Year + 2000,
                 sDate->Month,
                 sDate->Date,
                 sTime->Hours,
                 sTime->Minutes,
                 sTime->Seconds);
    }
}
