#include "sampling_control.h"
#include "stddef.h"
#include "config_manager.h"
#include "data_storage.h"
#include "usart_app.h"
#include "adc_app.h"

// 采样控制全局变量
static sampling_control_t g_sampling_control = {0};
static uint8_t g_sampling_initialized = 0;

#define LED_BLINK_PERIOD_MS 1000

// 采样初始化
sampling_status_t sampling_init(void)
{
    if (g_sampling_initialized)
    {
        return SAMPLING_OK;
    }

    config_init();

    g_sampling_control.state = SAMPLING_IDLE;
    g_sampling_control.cycle = config_get_sampling_cycle();
    g_sampling_control.last_sample_time = 0;
    g_sampling_control.led_blink_time = 0;
    g_sampling_control.led_blink_state = 0;

    g_sampling_initialized = 1;
    return SAMPLING_OK;
}

// 启动采样
sampling_status_t sampling_start(void)
{
    if (!g_sampling_initialized)
        return SAMPLING_ERROR;

    g_sampling_control.state = SAMPLING_ACTIVE;
    g_sampling_control.last_sample_time = HAL_GetTick();
    g_sampling_control.led_blink_time = HAL_GetTick();
    g_sampling_control.led_blink_state = 0;

    return SAMPLING_OK;
}

// 停止采样
sampling_status_t sampling_stop(void)
{
    if (!g_sampling_initialized)
        return SAMPLING_ERROR;

    g_sampling_control.state = SAMPLING_IDLE;
    g_sampling_control.led_blink_state = 0;

    return SAMPLING_OK;
}

// 设置采样周期
sampling_status_t sampling_set_cycle(sampling_cycle_t cycle)
{
    if (!g_sampling_initialized)
        return SAMPLING_ERROR;

    if (cycle != CYCLE_5S && cycle != CYCLE_10S && cycle != CYCLE_15S)
    {
        return SAMPLING_INVALID;
    }

    g_sampling_control.cycle = cycle;

    if (config_set_sampling_cycle(cycle) == CONFIG_OK)
    {
        config_save_to_flash();
    }

    return SAMPLING_OK;
}

// 获取采样状态
sampling_state_t sampling_get_state(void)
{
    if (!g_sampling_initialized)
        return SAMPLING_IDLE;
    return g_sampling_control.state;
}

// 获取采样周期
sampling_cycle_t sampling_get_cycle(void)
{
    if (!g_sampling_initialized)
        return CYCLE_5S;
    return g_sampling_control.cycle;
}

// 判断是否到采样时间
uint8_t sampling_should_sample(void)
{
    if (!g_sampling_initialized || g_sampling_control.state != SAMPLING_ACTIVE)
    {
        return 0;
    }

    uint32_t current_time = HAL_GetTick();
    uint32_t elapsed_time = current_time - g_sampling_control.last_sample_time;
    uint32_t cycle_ms = g_sampling_control.cycle * 1000;

    return (elapsed_time >= cycle_ms) ? 1 : 0;
}

// 更新LED闪烁状态
void sampling_update_led_blink(void)
{
    if (!g_sampling_initialized || g_sampling_control.state != SAMPLING_ACTIVE)
    {
        g_sampling_control.led_blink_state = 0;
        return;
    }

    uint32_t current_time = HAL_GetTick();
    uint32_t elapsed_time = current_time - g_sampling_control.led_blink_time;

    if (elapsed_time >= LED_BLINK_PERIOD_MS)
    {
        g_sampling_control.led_blink_state ^= 1;
        g_sampling_control.led_blink_time = current_time;
    }
}

// 获取采样电压
float sampling_get_voltage(void)
{
    extern __IO float voltage;
    config_params_t config_params;

    if (config_get_params(&config_params) != CONFIG_OK)
    {
        return voltage;
    }

    return voltage * config_params.ratio;
}

// 检查是否超限
uint8_t sampling_check_overlimit(void)
{
    config_params_t config_params;

    if (config_get_params(&config_params) != CONFIG_OK)
    {
        return 0;
    }

    float current_voltage = sampling_get_voltage();

    return (current_voltage > config_params.limit) ? 1 : 0;
}

// 采样任务
void sampling_task(void)
{
    if (!g_sampling_initialized)
        return;

    sampling_update_led_blink();
    if (g_sampling_control.state == SAMPLING_ACTIVE)
    {
        if (sampling_should_sample())
        {
            g_sampling_control.last_sample_time = HAL_GetTick();

            float current_voltage = sampling_get_voltage();
            uint8_t is_overlimit = sampling_check_overlimit();

            sampling_handle_data_storage(current_voltage, is_overlimit);
        }
    }
}

// 获取LED闪烁状态
uint8_t sampling_get_led_blink_state(void)
{
    if (!g_sampling_initialized)
        return 0;
    return g_sampling_control.led_blink_state;
}

// 采样数据存储处理
void sampling_handle_data_storage(float voltage, uint8_t is_overlimit)
{

    extern output_format_t g_output_format;

    if (g_output_format == OUTPUT_FORMAT_HIDDEN)
    {

        data_storage_status_t result = data_storage_write_hidedata(voltage, is_overlimit);
    }
    else
    {

        data_storage_status_t result = data_storage_write_sample(voltage);
    }

    if (is_overlimit)
    {
        config_params_t config_params;
        float limit_value = 0.0f;

        {
            limit_value = config_params.limit;
        }
        data_storage_status_t result = data_storage_write_overlimit(voltage, limit_value);
    }
}
