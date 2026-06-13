#include "y_timer/y_timer.h"

static u32 systick_ms = 0;

void SysTick_Init(void)
{
    systick_ms = 0;
}

void y_timer_tick(void)
{
    static uint8_t motor_run_cnt = 0;

    systick_ms++;
    motor_run_cnt++;
    if (motor_run_cnt >= 20)
    {
        motor_run_cnt = 0;
        if (app_motor_get_closed_loop())
        {
            app_motor_run();
        }
    }
}

u32 millis(void)
{
    return HAL_GetTick();
}
