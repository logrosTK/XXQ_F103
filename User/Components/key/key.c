/*
 * ================================================================================
 * @文件名称: key.c
 * @功能描述: 按键驱动，初始化按键GPIO为上拉输入模式
 * @所属模块: Components/key
 * @依赖: key.h
 * ================================================================================
 */

#include "key/key.h"

/*
 * 函数名称: key_init
 * 功能描述: 初始化按键GPIO引脚，配置为上拉输入模式
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 在系统初始化时调用一次
 *           按键未按下时引脚为高电平，按下时为低电平
 *           使用KEY_GET_LEVEL()宏读取按键状态
 */
void key_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(KEY_GPIO_CLK, ENABLE);

    GPIO_InitStructure.GPIO_Pin = KEY_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(KEY_GPIO_PORT, &GPIO_InitStructure);
}
