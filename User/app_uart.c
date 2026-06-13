#include "app_uart.h"

/**
 * @函数描述: uart串口相关设备控制初始化
 * @return {*}
 */
void app_uart_init(void)
{
    uart1_init(115200); /* 连接总线设备串口 */
    uart2_init(115200); /* 连接总线设备串口 */
    uart3_init(115200); /* 连接总线设备串口 */
    uart5_init(115200); /* 连接总线设备串口 */
}

/**
 * @函数描述: 循环检测串口接收到的指令
 * @return {*}
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
