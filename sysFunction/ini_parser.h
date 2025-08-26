#ifndef __INI_PARSER_H__
#define __INI_PARSER_H__

#include "stdint.h" 
#include "ff.h"    

typedef enum 
{
    INI_OK = 0,            
    INI_ERROR = 1,         
    INI_FILE_NOT_FOUND = 2, 
    INI_FORMAT_ERROR = 3,  
    INI_VALUE_ERROR = 4   
} ini_status_t;

typedef struct 
{
    float ratio;         
    float limit;        
    uint8_t ratio_found; 
    uint8_t limit_found; 
} ini_config_t;

ini_status_t ini_parse_file(const char *filename, ini_config_t *config); 
ini_status_t ini_parse_line(const char *line, ini_config_t *config);    

ini_status_t ini_trim_string(char *str);                     
ini_status_t ini_parse_float(const char *str, float *value); 

#endif 
