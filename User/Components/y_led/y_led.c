/*
 * ================================================================================
 * @文件名称: y_led.c
 * @功能描述: LED指示灯驱动，初始化LED引脚为推挽输出模式
 * @所属模块: Components/y_led
 * @依赖: y_led.h
 * ================================================================================
 */

#include "y_led/y_led.h"

/*
 * 函数名称: led_init
 * 功能描述: 初始化LED指示灯GPIO引脚，配置为推挽输出模式
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 在系统初始化时调用一次
 *           初始化后可通过LED_ON()、LED_OFF()、LED_TOGGLE()宏控制LED状态
 */
void led_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    RCC_APB2PeriphClockCmd(LED_GPIO_CLK, ENABLE); /* 使能端口时钟 */

    GPIO_InitStructure.GPIO_Pin = LED_PIN;            /* 配置 pin */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  /* 推挽输出 */
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; /* IO 翻转 50MHz */
    GPIO_Init(LED_GPIO_PORT, &GPIO_InitStructure);
}
