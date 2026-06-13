#include "y_beep/y_beep.h"

/**
 * @简  述  初始化
 * @参  数  无
 * @返回值  无
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

/**
 * @函数描述: 蜂鸣器鸣叫时间，单位ms
 * @param {uint16_t} times
 * @return {*}
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
