#include "key/key.h"

/**
 * @函数描述:  初始化引脚，配置为输入
 * @return {*}
 */
void key_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(KEY_GPIO_CLK, ENABLE);

    GPIO_InitStructure.GPIO_Pin = KEY_PIN;            /* 配置 pin */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;     /* 输入 */
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; /* IO 翻转 50MHz */
    GPIO_Init(KEY_GPIO_PORT, &GPIO_InitStructure);
}
