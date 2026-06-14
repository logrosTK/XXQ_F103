/*
 * ================================================================================
 * @文件名称: y_timer.c
 * @功能描述: 系统定时器相关函数，提供毫秒级时间基准和电机闭环定时调用
 * @所属模块: Components/y_timer
 * @依赖: y_timer.h, HAL库
 * ================================================================================
 */

#include "y_timer/y_timer.h"

/** 系统滴答毫秒计数器，由SysTick_Init初始化为0 */
static u32 systick_ms = 0;

/*
 * 函数名称: SysTick_Init
 * 功能描述: 初始化系统滴答计数器，将毫秒计数清零
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 在系统启动时调用一次，初始化时间基准
 *           必须在其他使用millis()的模块之前调用
 */
void SysTick_Init(void)
{
    systick_ms = 0;
}

/*
 * 函数名称: y_timer_tick
 * 功能描述: 系统定时器中断回调，每1ms调用一次
 *           负责更新毫秒计数，并按20ms周期调用电机闭环控制
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 此函数需要在SysTick中断服务函数（或HAL_SYSTICK_Callback）中每1ms调用一次
 *           内部每20ms自动调用app_motor_run()执行电机PID控制
 */
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

/*
 * 函数名称: millis
 * 功能描述: 获取系统自启动以来的毫秒数
 * 参数说明: 无
 * 返回值:   32位无符号整数，系统启动至今的毫秒计数
 * 使用说明: 用于非阻塞延时判断和时间戳记录
 *           注意：返回值约49.7天后会溢出回0
 *           示例: u32 now = millis();
 *                if(millis() - last_time > 100) { ... }
 */
u32 millis(void)
{
    return HAL_GetTick();
}
