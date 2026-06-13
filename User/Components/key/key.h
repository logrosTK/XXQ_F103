#ifndef _Y_KEY_H_
#define _Y_KEY_H_
#include "user_main.h"

/* 定义KEY引脚，修改编号就可以修改KEY引脚 */
#define KEY_PIN GPIO_Pin_2
#define KEY_GPIO_PORT GPIOB               /* GPIO端口 */
#define KEY_GPIO_CLK RCC_APB2Periph_GPIOB /* GPIO端口时钟 */

// 读取KEY引脚电平
#define KEY_GET_LEVEL() GPIO_ReadInputDataBit(KEY_GPIO_PORT, KEY_PIN)

void key_init(void);
#endif
