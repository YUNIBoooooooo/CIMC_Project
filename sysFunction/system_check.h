#ifndef __SYSTEM_CHECK_H__
#define __SYSTEM_CHECK_H__

#include "mydefine.h" 

typedef enum 
{
    SYSTEM_CHECK_OK = 0,       
    SYSTEM_CHECK_ERROR = 1,    
    SYSTEM_CHECK_NOT_FOUND = 2 
} system_check_status_t;

typedef struct 
{
    uint32_t flash_id;            
    char model_name[16];         
    uint32_t capacity_mb;        
    system_check_status_t status; 
} flash_info_t;

typedef struct 
{
    uint32_t capacity_mb;         
    uint32_t sector_count;       
    uint16_t sector_size;         
    system_check_status_t status; 
} sd_card_info_t;

typedef struct 
{
    flash_info_t flash_info;          
    sd_card_info_t sd_info;           
    system_check_status_t rtc_status; 
} system_info_t;


void system_self_check(void);                                       
system_check_status_t check_flash_status(flash_info_t *flash_info); 
system_check_status_t check_tf_card_status(sd_card_info_t *sd_info); 
void print_rtc_time(void);                                          
void print_system_info(const system_info_t *info);                  
const char *get_flash_model_name(uint32_t flash_id);                 
void print_rtc_time_now(void);
#endif 
