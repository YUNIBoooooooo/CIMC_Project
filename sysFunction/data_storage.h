#ifndef __DATA_STORAGE_H__
#define __DATA_STORAGE_H__

#include "mydefine.h" 
#include "ff.h"       

typedef enum 
{
    STORAGE_SAMPLE = 0,   
    STORAGE_OVERLIMIT = 1, 
    STORAGE_LOG = 2,      
    STORAGE_HIDEDATA = 3, 
    STORAGE_TYPE_COUNT = 4 
} storage_type_t;

typedef enum 
{
    DATA_STORAGE_OK = 0,      
    DATA_STORAGE_ERROR = 1,   
    DATA_STORAGE_INVALID = 2,
    DATA_STORAGE_FULL = 3,    
    DATA_STORAGE_NO_SD = 4    
} data_storage_status_t;

typedef struct 
{
    char current_filename[32]; 
    uint8_t data_count;        
    uint8_t file_exists;       
} file_state_t;


data_storage_status_t data_storage_init(void);                                         
data_storage_status_t data_storage_write_sample(float voltage);                        
data_storage_status_t data_storage_write_overlimit(float voltage, float limit);         
data_storage_status_t data_storage_write_log(const char *operation);                    
data_storage_status_t data_storage_write_hidedata(float voltage, uint8_t is_overlimit); 
data_storage_status_t data_storage_test(void);                                         


data_storage_status_t generate_datetime_string(char *datetime_str);           
data_storage_status_t generate_filename(storage_type_t type, char *filename); 

#endif
