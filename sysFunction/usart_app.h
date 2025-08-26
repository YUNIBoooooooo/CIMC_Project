#ifndef __USART_APP_H__
#define __USART_APP_H__

#include "mydefine.h"     
#include "data_storage.h" 

int my_printf(UART_HandleTypeDef *huart, const char *format, ...);        
void uart_task(void);                                                     
void parse_uart_command(uint8_t *buffer, uint16_t length);                

typedef enum 
{
    CMD_STATE_IDLE = 0,       
    CMD_STATE_WAIT_RATIO = 1, 
    CMD_STATE_WAIT_LIMIT = 2, 
    CMD_STATE_WAIT_RTC = 3    
} cmd_state_t;

typedef enum 
{
    OUTPUT_FORMAT_NORMAL = 0, 
    OUTPUT_FORMAT_HIDDEN = 1  
} output_format_t;

void handle_conf_command(void);             
void handle_ratio_command(void);            
void handle_limit_command(void);           
void handle_configsave_command(void);       
void handle_configread_command(void);       
void handle_start_command(void);            
void handle_stop_command(void);            
void handle_hide_command(void);             
void handle_unhide_command(void);           
void handle_rtc_config_command(void);       
void handle_sampling_output(void);         
void handle_interactive_input(char *input); 

uint32_t convert_rtc_to_unix_timestamp(RTC_TimeTypeDef *time, RTC_DateTypeDef *date);          
void format_hex_output(uint32_t timestamp, float voltage, uint8_t is_overlimit, char *output); 

extern uint8_t g_sampling_output_enabled; 
extern uint32_t g_last_output_time;        
extern output_format_t g_output_format;    

#endif
