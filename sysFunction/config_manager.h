#ifndef __CONFIG_MANAGER_H__
#define __CONFIG_MANAGER_H__

#include "stdint.h"
#include "sampling_control.h" 

#define CONFIG_FLASH_ADDR 0x1F0000 
#define CONFIG_MAGIC 0x43464721     
#define CONFIG_VERSION 0x02        

typedef struct 
{
    uint32_t magic;              
    uint8_t version;            
    float ratio;                
    float limit;                 
    sampling_cycle_t cycle;      
    uint32_t crc32;             
} config_params_t;

typedef enum 
{
    CONFIG_OK = 0,          
    CONFIG_ERROR = 1,        
    CONFIG_INVALID = 2,      
    CONFIG_FLASH_ERROR = 3, 
    CONFIG_CRC_ERROR = 4     
} config_status_t;


config_status_t config_init(void);                                     
config_status_t config_get_params(config_params_t *params);             
config_status_t config_set_params(const config_params_t *params);       
config_status_t config_save_to_flash(void);                            
config_status_t config_load_from_flash(void);                           
config_status_t config_reset_to_default(void);                          
config_status_t config_validate_ratio(float ratio);                    
config_status_t config_validate_limit(float limit);                     
config_status_t config_validate_sampling_cycle(sampling_cycle_t cycle); 
config_status_t config_set_sampling_cycle(sampling_cycle_t cycle);      
sampling_cycle_t config_get_sampling_cycle(void);                      


uint32_t config_calculate_crc32(const config_params_t *params);        

#endif 
