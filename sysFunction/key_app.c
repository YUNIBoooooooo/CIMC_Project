#include "key_app.h"

// 按键状态变量
uint8_t key_value = 0;
uint8_t key_value_old = 0;
uint8_t key_down = 0;
uint8_t key_up = 0;

// 读取按键值
uint8_t key_read(void)
{
    uint8_t temp = 0;

    if (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_7) == RESET)
        temp = 1;
    if (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_8) == RESET)
        temp = 2;
    if (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_9) == RESET)
        temp = 3;
    if (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_10) == RESET)
        temp = 4;
    if (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_11) == RESET)
        temp = 5;
    if (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_12) == RESET)
        temp = 6;
    return temp;
}

// 按键处理函数
void key_proc(void)
{
    key_value = key_read();
    key_down = key_value & (key_value ^ key_value_old);
    key_up = ~key_value & (key_value ^ key_value_old);
    key_value_old = key_value;

    switch (key_down)
    {
    case 1:

        sampling_init();

        if (sampling_get_state() == SAMPLING_IDLE)
        {

            if (sampling_start() == SAMPLING_OK)
            {
                sampling_cycle_t cycle = sampling_get_cycle();
                my_printf(&huart1, "Periodic Sampling\r\n");
                my_printf(&huart1, "sample cycle: %ds\r\n", (int)cycle);

                extern uint8_t g_sampling_output_enabled;
                extern uint32_t g_last_output_time;
                g_sampling_output_enabled = 1;
                g_last_output_time = HAL_GetTick();

                char log_msg[64];
                sprintf(log_msg, "sample start - cycle %ds (key1)", (int)cycle);
                data_storage_write_log(log_msg);
            }
            else
            {
                my_printf(&huart1, "sampling start failed.\r\n");
            }
        }
        else
        {

            if (sampling_stop() == SAMPLING_OK)
            {
                my_printf(&huart1, "Periodic Sampling STOP\r\n");

                extern uint8_t g_sampling_output_enabled;
                g_sampling_output_enabled = 0;

                data_storage_write_log("sample stop (key1)");
            }
            else
            {
                my_printf(&huart1, "sampling stop failed.\r\n");
            }
        }
        break;

    case 2:
        sampling_init();
        if (sampling_set_cycle(CYCLE_5S) == SAMPLING_OK)
        {
            my_printf(&huart1, "sample cycle adjust:5s\r\n");
            data_storage_write_log("cycle adjust to 5s (key2)");
        }
        else
        {
            my_printf(&huart1, "cycle adjust failed.\r\n");
        }
        break;

    case 3:
        sampling_init();
        if (sampling_set_cycle(CYCLE_10S) == SAMPLING_OK)
        {
            my_printf(&huart1, "sample cycle adjust:10s\r\n");
            data_storage_write_log("cycle adjust to 10s (key3)");
        }
        else
        {
            my_printf(&huart1, "cycle adjust failed.\r\n");
        }
        break;

    case 4:
        sampling_init();
        if (sampling_set_cycle(CYCLE_15S) == SAMPLING_OK)
        {
            my_printf(&huart1, "sample cycle adjust:15s\r\n");
            data_storage_write_log("cycle adjust to 15s (key4)");
        }
        else
        {
            my_printf(&huart1, "cycle adjust failed.\r\n");
        }
        break;

    default:
        break;
    }
}