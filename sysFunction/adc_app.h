#ifndef __ADC_APP_H_
#define __ADC_APP_H_

#include "stdint.h"  
#include "mydefine.h" 

void adc_task(void);         
void dac_sin_init(void);     
void adc_dma_init(void);     
void adc_tim_dma_init(void);

#endif 
