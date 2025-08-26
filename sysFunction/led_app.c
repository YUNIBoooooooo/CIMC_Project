#include "led_app.h"
#include "gpio.h"
#include "math.h"

// LED状态数组
uint8_t ucLed[6] = {0, 0, 0, 0, 0, 0};

// LED显示函数
void led_disp(uint8_t *ucLed)
{

    uint8_t temp = 0x00;

    static uint8_t temp_old = 0xff;

    for (int i = 0; i < 6; i++)
    {

        if (ucLed[i])
            temp |= (1 << i);
    }

    if (temp_old != temp)
    {

        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, (temp & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11, (temp & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, (temp & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, (temp & 0x08) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, (temp & 0x10) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, (temp & 0x20) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        temp_old = temp;
    }
}

// LED任务
void led_task(void)
{
    static uint32_t led1_blink_time = 0;
    static uint8_t led1_blink_state = 0;

    if (sampling_get_state() == SAMPLING_ACTIVE)
    {

        uint32_t current_time = HAL_GetTick();
        if (current_time - led1_blink_time >= 500)
        {
            led1_blink_state ^= 1;
            led1_blink_time = current_time;
        }
        ucLed[0] = led1_blink_state;
    }
    else
    {
        ucLed[0] = 0;
        led1_blink_state = 0;
    }

    if (sampling_check_overlimit() && sampling_get_state() == SAMPLING_ACTIVE)
    {
        ucLed[1] = 1;
    }
    else
    {

        ucLed[1] = 0;
    }
    led_disp(ucLed);
}
