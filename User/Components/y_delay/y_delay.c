/*
 * ================================================================================
 * @文件名称: y_delay.c
 * @功能描述: 软件延时函数，提供纳秒级、微秒级、毫秒级阻塞延时
 * @所属模块: Components/y_delay
 * @依赖: y_delay.h
 * @注意事项: 这些是阻塞延时，会占用CPU。实时性要求高的场景建议使用定时器中断
 * ================================================================================
 */

#include "./y_delay/y_delay.h"

/*
 * 函数名称: delay_ns
 * 功能描述: 纳秒级粗略延时（通过空循环实现）
 * 参数说明: t - 延时循环次数（非精确纳秒值，需根据主频校准）
 * 返回值:   无
 * 使用说明: 用于极短延时，精度取决于CPU主频和编译器优化
 *           示例: delay_ns(100);
 */
void delay_ns(u16 t)
{
   while (t--)
      ;
   return;
}


/*
 * 函数名称: delay_us
 * 功能描述: 微秒级软件延时（通过空循环实现）
 * 参数说明: delay_us - 延时的微秒数（近似值，需根据主频校准）
 * 返回值:   无
 * 使用说明: 用于需要微秒级精度的延时场景，如I2C时序控制
 *           示例: delay_us(10);  // 延时约10微秒
 */
void delay_us(uint16_t delay_us)
{    
  volatile unsigned int num;
  volatile unsigned int t;
 
  
  for (num = 0; num < delay_us; num++)
  {
    t = 11;
    while (t != 0)
    {
      t--;
    }
  }
}

/*
 * 函数名称: delay_ms
 * 功能描述: 毫秒级软件延时（通过调用delay_us实现）
 * 参数说明: delay_ms - 延时的毫秒数
 * 返回值:   无
 * 使用说明: 用于需要毫秒级延时的场景
 *           注意：这是阻塞延时，会占用CPU。长时间延时建议使用定时器
 *           示例: delay_ms(500);  // 延时约500毫秒
 */
void delay_ms(uint16_t delay_ms)
{    
  volatile unsigned int num;
  for (num = 0; num < delay_ms; num++)
  {
    delay_us(1000);
  }
}


