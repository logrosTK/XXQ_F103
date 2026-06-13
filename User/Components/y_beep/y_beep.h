#ifndef _Y_BEEP_H_
#define _Y_BEEP_H_
#include "user_main.h"

/* 定义BEEP引脚，修改编号就可以修改BEEP引脚 */
#define BEEP_PIN 		GPIO_Pin_5
#define BEEP_GPIO_PORT 	GPIOC               /* GPIO端口 */
#define BEEP_GPIO_CLK 	RCC_APB2Periph_GPIOC /* GPIO端口时钟 */

/* 控制BEEP的宏 */                   
#define BEEP_ON() 		GPIO_SetBits(BEEP_GPIO_PORT, BEEP_PIN)                                  // BEEP信号灯点亮
#define BEEP_OFF() 		GPIO_ResetBits(BEEP_GPIO_PORT, BEEP_PIN)                                   // BEEP信号灯熄灭
#define BEEP_TOGGLE() 	GPIO_WriteBit(BEEP_GPIO_PORT, BEEP_PIN, (BitAction)(1 - BEEP_GET_LEVEL())) // 翻转BEEP信号灯

/*******BEEP相关函数声明*******/
void BEEP_Init(void); // 初始化BEEP信号灯
void beep_on_times(int times, int delay);
#endif
