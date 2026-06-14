/*
 * ================================================================================
 * @文件名称: app_uart.c
 * @功能描述: 应用层串口通信模块，初始化4路串口并处理接收数据
 *           UART1/UART2/UART3/UART5均配置为115200波特率
 *           支持命令模式、舵机调试模式、存储模式
 * @所属模块: User层
 * @依赖: app_uart.h, y_usart.h, y_global.h
 * @串口模式:
 *   mode=1: 命令模式, 调用parse_cmd()解析如$DST!等命令
 *   mode=2: 单个舵机调试, 调用parse_action()解析#XXXPxxxxTxxxx!
 *   mode=3: 多路舵机调试, 同mode=2
 *   mode=4: 存储模式, 调用save_action()保存动作组到Flash
 * ================================================================================
 */

#include "app_uart.h"

/*
 * 函数名称: app_uart_init
 * 功能描述: 初始化所有4路串口（UART1/2/3/5），波特率均为115200
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 在系统初始化时调用一次
 *           UART1=PA9/PA10, UART2=PA2/PA3, UART3=PB10/PB11, UART5=PC12(半双工)
 *           UART5使用半双工模式，用于总线通信
 */
void app_uart_init(void)
{
    uart1_init(115200); /* 连接总线设备串口 */
    uart2_init(115200); /* 连接总线设备串口 */
    uart3_init(115200); /* 连接总线设备串口 */
    uart5_init(115200); /* 连接总线设备串口 */
}

/*
 * 函数名称: app_uart_run
 * 功能描述: 循环检测各串口接收到的数据，根据mode执行相应处理
 *           每个串口收到数据后(通过uartX_get_ok标志判断)，
 *           根据uartX_mode选择: 命令解析/舵机调试/动作存储
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 在主循环中周期调用（建议每几ms调用一次）
 *           处理流程: 
 *             检查uartX_get_ok -> 根据uartX_mode选择处理函数 -> 清除标志
 *           UART1和UART3接收的数据同时转发到UART2(调试输出)
 */
void app_uart_run(void)
{
    if (uart1_get_ok)
    {
        uart2_send_str(uart1_receive_buf);
        if (uart1_mode == 1)
        {
            // 命令模式
            parse_cmd(uart1_receive_buf);
        }
        else if (uart1_mode == 2)
        {
            // 单个舵机调试
            parse_action(uart1_receive_buf);
        }
        else if (uart1_mode == 3)
        {
            // 多路舵机调试
            parse_action(uart1_receive_buf);
        }
        else if (uart1_mode == 4)
        {
            // 存储模式
            save_action(uart1_receive_buf);
        }

        uart1_get_ok = 0;
        uart1_mode = 0;
    }

    if (uart2_get_ok)
    {
        if (uart2_mode == 1)
        {
            // 命令模式
            parse_cmd(uart2_receive_buf);
        }
        else if (uart2_mode == 2)
        {
            // 单个舵机调试
            parse_action(uart2_receive_buf);
        }
        else if (uart2_mode == 3)
        {
            // 多路舵机调试
            parse_action(uart2_receive_buf);
        }
        else if (uart2_mode == 4)
        {
            // 存储模式
            save_action(uart2_receive_buf);
        }

        uart2_get_ok = 0;
        uart2_mode = 0;
    }

    if (uart3_get_ok)
    {
        uart2_send_str(uart3_receive_buf);
        uart3_send_str(uart3_receive_buf);
        if (uart3_mode == 1)
        {
            // 命令模式
            parse_cmd(uart3_receive_buf);
        }
        else if (uart3_mode == 2)
        {
            // 单个舵机调试
            parse_action(uart3_receive_buf);
        }
        else if (uart3_mode == 3)
        {
            // 多路舵机调试
            parse_action(uart3_receive_buf);
        }
        else if (uart3_mode == 4)
        {
            // 存储模式
            save_action(uart3_receive_buf);
        }

        uart3_get_ok = 0;
        uart3_mode = 0;
    }

    if (uart5_get_ok)
    {
        uart2_send_str(uart5_receive_buf);
        if (uart5_mode == 1)
        {
            // 命令模式
            parse_cmd(uart5_receive_buf);
        }
        else if (uart5_mode == 2)
        {
            // 单个舵机调试
            parse_action(uart5_receive_buf);
        }
        else if (uart5_mode == 3)
        {
            // 多路舵机调试
            parse_action(uart5_receive_buf);
        }
        else if (uart5_mode == 4)
        {
            // 存储模式
            save_action(uart5_receive_buf);
        }

        uart5_get_ok = 0;
        uart5_mode = 0;
    }
}
