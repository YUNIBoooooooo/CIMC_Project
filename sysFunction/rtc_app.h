#ifndef __RTC_APP_H__
#define __RTC_APP_H__

#include "mydefine.h"

void rtc_proc(void); 
HAL_StatusTypeDef rtc_set_time_from_string(const char *time_str); 
void rtc_print_current_time(void);                                
void rtc_print_current_time_now(void);
void rtc_get_time_info(RTC_TimeTypeDef *current_time, RTC_DateTypeDef *current_date);                                  
void format_time_output(const RTC_TimeTypeDef *sTime, const RTC_DateTypeDef *sDate, char *buffer, size_t buffer_size); 

#endif
