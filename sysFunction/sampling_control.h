#ifndef __SAMPLING_CONTROL_H__
#define __SAMPLING_CONTROL_H__

#include "stdint.h" 


typedef enum
{
    SAMPLING_IDLE = 0,  
    SAMPLING_ACTIVE = 1 
} sampling_state_t;


typedef enum
{
    CYCLE_5S = 5,  
    CYCLE_10S = 10, 
    CYCLE_15S = 15  
} sampling_cycle_t;


typedef struct
{
    sampling_state_t state;    
    sampling_cycle_t cycle;    
    uint32_t last_sample_time; 
    uint32_t led_blink_time;   
    uint8_t led_blink_state;  
} sampling_control_t;


typedef enum
{
    SAMPLING_OK = 0,      
    SAMPLING_ERROR = 1,   
    SAMPLING_INVALID = 2,  
    SAMPLING_OVERLIMIT = 3 
} sampling_status_t;


sampling_status_t sampling_init(void);                       
sampling_status_t sampling_start(void);                       
sampling_status_t sampling_stop(void);                       
sampling_status_t sampling_set_cycle(sampling_cycle_t cycle); 
sampling_state_t sampling_get_state(void);                    
sampling_cycle_t sampling_get_cycle(void);                  
void sampling_task(void);                                    

float sampling_get_voltage(void);          
uint8_t sampling_check_overlimit(void);     
uint8_t sampling_should_sample(void);      
void sampling_update_led_blink(void);       
uint8_t sampling_get_led_blink_state(void); 
void sampling_handle_data_storage(float voltage, uint8_t is_overlimit);

#endif 
