#include "oled_app.h"

// PID参数范围定义
#define PID_PARAM_P_MIN 0
#define PID_PARAM_P_MAX 1000
#define PID_PARAM_P_STEP 10
#define PID_PARAM_I_MIN 0
#define PID_PARAM_I_MAX 1000
#define PID_PARAM_I_STEP 10
#define PID_PARAM_D_MIN 0
#define PID_PARAM_D_MAX 100
#define PID_PARAM_D_STEP 1

// OLED格式化输出
int oled_printf(uint8_t x, uint8_t y, const char *format, ...)
{
  char buffer[512];
  va_list arg;
  int len;

  va_start(arg, format);

  len = vsnprintf(buffer, sizeof(buffer), format, arg);
  va_end(arg);

  OLED_ShowStr(x, y, buffer, 8);
  return len;
}

// OLED任务
void oled_task(void)
{
  static uint8_t last_display_state = 0xFF;
  uint8_t current_state = 0;

  if (sampling_get_state() == SAMPLING_ACTIVE)
  {
    current_state = 1;

    RTC_TimeTypeDef current_rtc_time = {0};
    RTC_DateTypeDef current_rtc_date = {0};
    HAL_RTC_GetTime(&hrtc, &current_rtc_time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &current_rtc_date, RTC_FORMAT_BIN);

    float voltage = sampling_get_voltage();

    oled_printf(0, 0, "%02d:%02d:%02d      ",
                current_rtc_time.Hours,
                current_rtc_time.Minutes,
                current_rtc_time.Seconds);

    oled_printf(0, 1, "%.2f V  ", voltage);

    if (last_display_state != current_state)
    {
      oled_printf(0, 2, "        ");
      oled_printf(0, 3, "        ");
    }
  }
  else
  {
    current_state = 0;
    oled_printf(0, 0, "system idle");

    if (last_display_state != current_state)
    {
      oled_printf(0, 1, "        ");
      oled_printf(0, 2, "        ");
      oled_printf(0, 3, "        ");
    }
  }

  last_display_state = current_state;
}
