#include "scheduler.h"
uint8_t task_num; 
typedef struct 
{
    void (*task_func)(void); 
    uint32_t rate_ms;        
    uint32_t last_run;      
} task_t;

static task_t scheduler_task[] =
    {
        {led_task, 1, 0},   
        {adc_task, 100, 0}, 
        {key_proc, 5, 0},
        {uart_task, 5, 0},     
        {oled_task, 1, 0},     
        {sampling_task, 10, 0} 
};

void scheduler_init(void) 
{
    task_num = sizeof(scheduler_task) / sizeof(task_t); 
}

void scheduler_run(void) 
{
    for (uint8_t i = 0; i < task_num; i++) 
    {
        uint32_t now_time = HAL_GetTick(); 
        
        if (now_time >= scheduler_task[i].rate_ms + scheduler_task[i].last_run) 
        {
            scheduler_task[i].last_run = now_time; 
            scheduler_task[i].task_func();         
        }
    }
}
