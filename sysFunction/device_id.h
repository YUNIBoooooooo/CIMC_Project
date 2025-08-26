#ifndef __DEVICE_ID_H
#define __DEVICE_ID_H

#include "mydefine.h"


#define DEVICE_ID_MAX_LENGTH 32  
#define DEVICE_ID_FLASH_ADDR 0x0000  


typedef enum {
    DEVICE_ID_OK = 0,
    DEVICE_ID_ERROR,
    DEVICE_ID_NOT_FOUND,
    DEVICE_ID_INVALID
} device_id_status_t;

device_id_status_t device_id_init(void);  
device_id_status_t device_id_read(char *device_id);  
device_id_status_t device_id_write(const char *device_id);  
device_id_status_t device_id_set_default(void);  
void device_id_print_startup_info(void);  

#endif 
