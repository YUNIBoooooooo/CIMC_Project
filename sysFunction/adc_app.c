#include "adc_app.h"
#include "tim.h"

#define ADC_MODE (3)

// ADC模式1：单次采样
#if ADC_MODE == 1

__IO uint32_t adc_val;
__IO float voltage;

void adc_task(void)
{
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 1000) == HAL_OK)
    {
        adc_val = HAL_ADC_GetValue(&hadc1);

        voltage = (float)adc_val * 3.3f / 4096.0f;

        my_printf(&huart1, "ADC Value: %lu, Voltage: %.2fV\n", adc_val, voltage);
    }
    else
    {
    }
}

// ADC模式2：DMA采样
#elif ADC_MODE == 2

#define ADC_DMA_BUFFER_SIZE 32
uint32_t adc_dma_buffer[ADC_DMA_BUFFER_SIZE];
__IO uint32_t adc_val;
__IO float voltage;

void adc_dma_init(void)
{
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_dma_buffer, ADC_DMA_BUFFER_SIZE);
}

void adc_task(void)
{
    uint32_t adc_sum = 0;

    for (uint16_t i = 0; i < ADC_DMA_BUFFER_SIZE; i++)
    {
        adc_sum += adc_dma_buffer[i];
    }

    adc_val = adc_sum / ADC_DMA_BUFFER_SIZE;

    voltage = ((float)adc_val * 3.3f) / 4096.0f;

    my_printf(&huart1, "Average ADC: %lu, Voltage: %.2fV\n", adc_val, voltage);
}

// ADC模式3：双通道DMA采样
#elif ADC_MODE == 3

#define BUFFER_SIZE 2048

extern DMA_HandleTypeDef hdma_adc1;

uint32_t dac_val_buffer[BUFFER_SIZE / 2];
uint32_t res_val_buffer[BUFFER_SIZE / 2];
__IO uint32_t adc_val_buffer[BUFFER_SIZE];
__IO float voltage;
__IO uint8_t AdcConvEnd = 0;
uint8_t wave_analysis_flag = 0;
uint8_t wave_query_type = 0;

// ADC+定时器+DMA初始化
void adc_tim_dma_init(void)
{
    HAL_TIM_Base_Start(&htim3);
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_val_buffer, BUFFER_SIZE);
    __HAL_DMA_DISABLE_IT(&hdma_adc1, DMA_IT_HT);
}

// ADC转换完成回调
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    UNUSED(hadc);
    if (hadc == &hadc1)
    {
        HAL_ADC_Stop_DMA(hadc);
        AdcConvEnd = 1;
    }
}

// ADC任务
void adc_task(void)
{
    if (AdcConvEnd)
    {
        for (uint16_t i = 0; i < BUFFER_SIZE / 2; i++)
        {
            dac_val_buffer[i] = adc_val_buffer[i * 2 + 1];
            res_val_buffer[i] = adc_val_buffer[i * 2];
        }
        uint32_t res_sum = 0;
        for (uint16_t i = 0; i < BUFFER_SIZE / 2; i++)
        {
            res_sum += res_val_buffer[i];
        }

        uint32_t res_avg = res_sum / (BUFFER_SIZE / 2);
        voltage = (float)res_avg * 3.3f / 4096.0f;
        AdcConvEnd = 0;

        HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_val_buffer, BUFFER_SIZE);
        __HAL_DMA_DISABLE_IT(&hdma_adc1, DMA_IT_HT);
    }
}

#endif
