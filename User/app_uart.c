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

#define APP_UART_DEFAULT_MOTOR_LOG_PERIOD_MS 3000U
#define APP_UART_DEFAULT_ULTRASONIC_LOG_PERIOD_MS 500U
#define APP_UART_MIN_LOG_PERIOD_MS 50U

static uint8_t g_motor_log_enabled = 1U;
static uint32_t g_motor_log_period_ms = APP_UART_DEFAULT_MOTOR_LOG_PERIOD_MS;
static uint8_t g_ultrasonic_log_enabled = 0U;
static uint32_t g_ultrasonic_log_period_ms = APP_UART_DEFAULT_ULTRASONIC_LOG_PERIOD_MS;

static uint32_t app_uart_sanitize_log_period(uint32_t period_ms, uint32_t default_period_ms)
{
    if (period_ms == 0U)
    {
        return default_period_ms;
    }

    if (period_ms < APP_UART_MIN_LOG_PERIOD_MS)
    {
        return APP_UART_MIN_LOG_PERIOD_MS;
    }

    return period_ms;
}

static int32_t app_uart_float_to_x10(float value)
{
    if (value >= 0.0f)
    {
        return (int32_t)(value * 10.0f + 0.5f);
    }

    return (int32_t)(value * 10.0f - 0.5f);
}

static int32_t app_uart_abs_i32(int32_t value)
{
    return (value < 0) ? -value : value;
}

static void app_uart_print_wheel_status(void)
{
    printf("MOTOR target_mm_s=[%d,%d,%d,%d] real_mm_s=[%d,%d,%d,%d] pwm=[%d,%d,%d,%d]\r\n",
           (int)(Wheel_A.TG * 1000.0f),
           (int)(Wheel_B.TG * 1000.0f),
           (int)(Wheel_C.TG * 1000.0f),
           (int)(Wheel_D.TG * 1000.0f),
           (int)(Wheel_A.RT * 1000.0f),
           (int)(Wheel_B.RT * 1000.0f),
           (int)(Wheel_C.RT * 1000.0f),
           (int)(Wheel_D.RT * 1000.0f),
           Wheel_A.PWM,
           Wheel_B.PWM,
           Wheel_C.PWM,
           Wheel_D.PWM);
}

void app_uart_set_motor_log(uint8_t enabled, uint32_t period_ms)
{
    g_motor_log_enabled = enabled ? 1U : 0U;
    g_motor_log_period_ms = app_uart_sanitize_log_period(period_ms, APP_UART_DEFAULT_MOTOR_LOG_PERIOD_MS);
}

uint8_t app_uart_is_motor_log_enabled(void)
{
    return g_motor_log_enabled;
}

uint32_t app_uart_get_motor_log_period_ms(void)
{
    return g_motor_log_period_ms;
}

void app_uart_set_ultrasonic_log(uint8_t enabled, uint32_t period_ms)
{
    g_ultrasonic_log_enabled = enabled ? 1U : 0U;
    g_ultrasonic_log_period_ms = app_uart_sanitize_log_period(period_ms, APP_UART_DEFAULT_ULTRASONIC_LOG_PERIOD_MS);
}

uint8_t app_uart_is_ultrasonic_log_enabled(void)
{
    return g_ultrasonic_log_enabled;
}

uint32_t app_uart_get_ultrasonic_log_period_ms(void)
{
    return g_ultrasonic_log_period_ms;
}

void app_uart_print_pid(void)
{
    int32_t kp10 = app_uart_float_to_x10(motor_kp);
    int32_t ki10 = app_uart_float_to_x10(motor_ki);
    int32_t kd10 = app_uart_float_to_x10(motor_kd);

    printf("PID kp=%ld.%ld ki=%ld.%ld kd=%ld.%ld\r\n",
           (long)(kp10 / 10),
           (long)app_uart_abs_i32(kp10 % 10),
           (long)(ki10 / 10),
           (long)app_uart_abs_i32(ki10 % 10),
           (long)(kd10 / 10),
           (long)app_uart_abs_i32(kd10 % 10));
}

void app_uart_print_ultrasonic_status(void)
{
    uint32_t distance_x100 = app_sensor_get_ultrasonic_distance_x100();

    if (app_sensor_is_ultrasonic_distance_valid())
    {
        printf("ULTRA valid=1 distance_cm=%lu.%02lu\r\n",
               (unsigned long)(distance_x100 / 100U),
               (unsigned long)(distance_x100 % 100U));
        return;
    }

    printf("ULTRA valid=0 distance_cm=NA\r\n");
}

void app_uart_print_tracking_status(void)
{
    uint8_t tracking_data[TRACKING_CHANNEL_COUNT];

    printf("TRACK online=%u mode=%u mask=0x%02X cross=%u hold=%u",
           (unsigned int)app_sensor_is_tracking_online(),
           (unsigned int)app_sensor_get_tracking_mode(),
           (unsigned int)app_sensor_get_tracking_mask(),
           (unsigned int)app_sensor_get_tracking_cross_count(),
           (unsigned int)app_sensor_get_tracking_cross_hold());

    if (app_sensor_get_tracking_state(tracking_data, TRACKING_CHANNEL_COUNT))
    {
        printf(" data=%u,%u,%u,%u,%u,%u,%u,%u\r\n",
               tracking_data[0],
               tracking_data[1],
               tracking_data[2],
               tracking_data[3],
               tracking_data[4],
               tracking_data[5],
               tracking_data[6],
               tracking_data[7]);
        return;
    }

    printf(" data=NA\r\n");
}

void app_uart_print_log_config(void)
{
    printf("LOG motor=%u,%lu ultrasonic=%u,%lu\r\n",
           (unsigned int)g_motor_log_enabled,
           (unsigned long)g_motor_log_period_ms,
           (unsigned int)g_ultrasonic_log_enabled,
           (unsigned long)g_ultrasonic_log_period_ms);
}

void app_uart_print_status(void)
{
    uint32_t now = millis();

    printf("STATUS tick=%lu closed_loop=%u ai_mode=%u version=%u\r\n",
           (unsigned long)now,
           (unsigned int)app_motor_get_closed_loop(),
           (unsigned int)AI_mode,
           (unsigned int)VERSION);
    app_uart_print_log_config();
    app_uart_print_pid();
    app_uart_print_wheel_status();
    app_uart_print_ultrasonic_status();
    app_uart_print_tracking_status();
}

void app_uart_print_help(void)
{
    printf("UART debug commands:\r\n");
    printf("  $HELP!                      Show command list\r\n");
    printf("  $STATUS!                    Show full system status\r\n");
    printf("  $PID:GET!                   Show PID parameters\r\n");
    printf("  $ULTRA:GET!                 Show ultrasonic status\r\n");
    printf("  $TRACK:GET!                 Show tracking status\r\n");
    printf("  $LOG:GET!                   Show debug log config\r\n");
    printf("  $MOTOR:CL:x!                Set closed-loop 0/1\r\n");
    printf("  $MOTOR:SET:a,b,c,d!         Set target wheel speed only\r\n");
    printf("  $MOTOR:RUN:a,b,c,d!         Enable closed-loop and set target wheel speed\r\n");
    printf("  $MOTOR:STOP!                Stop motor and close closed-loop\r\n");
    printf("  $PID:kp,ki,kd!              Update PID parameters\r\n");
    printf("  $PID:RST!                   Reset PID state\r\n");
    printf("  $AI:STOP|TRACK|TRACKPRO|AVOID|FOLLOW!\r\n");
    printf("  $LOG:MOTOR:en,period!       Motor log enable + period(ms)\r\n");
    printf("  $LOG:ULTRA:en,period!       Ultrasonic log enable + period(ms)\r\n");
    printf("Legacy protocol remains available: $Car/$DST/$ZNXJ/$ZYBZ/$DJGS/...\r\n");
}

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
    app_uart_set_motor_log(1U, APP_UART_DEFAULT_MOTOR_LOG_PERIOD_MS);
    app_uart_set_ultrasonic_log(0U, APP_UART_DEFAULT_ULTRASONIC_LOG_PERIOD_MS);
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
