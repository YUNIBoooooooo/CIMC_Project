#ifndef __BSP_SYSTEM_H
#define __BSP_SYSTEM_H

#include "bsp_system.h"

#include "main.h"
#include "gd25qxx.h"

#include "stdio.h"
#include "string.h"
#include "stdarg.h"
#include "stdint.h"
#include "stdlib.h"


#include "scheduler.h"
#include "ringbuffer.h"
#include "i2c.h"
#include "usart.h"
#include "adc.h"
#include "tim.h"
#include "dac.h"
#include "oled.h"
#include "lfs.h"
#include "lfs_port.h"
#include "ff.h"   
#include "fatfs.h"
#include "math.h"
#include "arm_math.h"
#include "WouoUI.h"
#include "WouoUI_user.h"
#include "u8g2.h"

#include "key_app.h" 
#include "led_app.h"
#include "oled_app.h"
#include "adc_app.h"
#include "usart_app.h"
#include "rtc_app.h"
#include "system_check.h"     
#include "config_manager.h"  
#include "flash_app.h"
#include "ini_parser.h"  
#include "sampling_control.h"

extern uint8_t uart_send_flag;
extern uint8_t wave_analysis_flag;

extern uint16_t uart_rx_index;
extern uint32_t uart_rx_ticks;
extern uint8_t uart_rx_buffer[128];
extern uint8_t uart_rx_dma_buffer[128];

extern struct rt_ringbuffer uart_ringbuffer;
extern uint8_t ringbuffer_pool[128];

extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef hdma_usart1_rx;

extern u8g2_t u8g2; 
extern struct lfs_config cfg;
extern lfs_t lfs;
extern RTC_HandleTypeDef hrtc;

extern uint8_t retSD; 
extern char SDPath[4]; 
extern FATFS SDFatFS; 
extern FIL SDFile;    

#endif
