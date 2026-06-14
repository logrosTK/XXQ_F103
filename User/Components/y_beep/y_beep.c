/*
 * ================================================================================
 * @文件名称: y_beep.c
 * @功能描述: 蜂鸣器驱动，初始化GPIO并提供鸣叫控制函数
 * @所属模块: Components/y_beep
 * @依赖: y_beep.h
 * ================================================================================
 */

#include "y_beep/y_beep.h"

/*
 * 函数名称: BEEP_Init
 * 功能描述: 初始化蜂鸣器GPIO引脚，配置为推挽输出模式
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 在系统初始化时调用一次
 *           初始化后可通过BEEP_ON()/BEEP_OFF()宏控制蜂鸣器
 */
void BEEP_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  RCC_APB2PeriphClockCmd(BEEP_GPIO_CLK, ENABLE); /* 使能端口时钟 */

  GPIO_InitStructure.GPIO_Pin = BEEP_PIN;           /* 配置 pin */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  /* 推挽输出 */
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; /* IO 翻转 50MHz */
  GPIO_Init(BEEP_GPIO_PORT, &GPIO_InitStructure);
}

/*
 * 函数名称: beep_on_times
 * 功能描述: 控制蜂鸣器鸣叫指定次数，每次鸣叫和间隔时间相同
 * 参数说明: times - 鸣叫次数（蜂鸣器通断循环次数）
 *          delay - 每次鸣叫和每次间隔的时间，单位ms
 * 返回值:   无
 * 使用说明: 这是一个阻塞函数，会占用CPU直到鸣叫完成
 *           示例: beep_on_times(3, 100);  // 鸣叫3次，每次100ms响+100ms停
 *           注意: 不要在中断服务函数中调用，会导致长时间阻塞
 */
void beep_on_times(int times, int delay)
{
  int i;
  for (i = 0; i < times; i++)
  {
    BEEP_ON();
    delay_ms(delay);
    BEEP_OFF();
    delay_ms(delay);
  }
}
