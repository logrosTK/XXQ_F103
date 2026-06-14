/*
 * ================================================================================
 * @文件名称: app_main.c
 * @功能描述: 应用层主程序文件，系统入口和主循环
 *           包含: 电机闭环状态日志、OLED显示
 *           电机映射: A=左前(LF) B=右前(RF) C=右后(RR) D=左后(LR)
 * @所属模块: User层
 * @依赖: app_main.h, user_main.h, app_motor.h, y_encoder.h
 * @调用流程:
 *   app_init() -> 初始化各模块 -> 电机保持停止
 *   app_loop() -> LED闪烁/闭环状态日志输出
 * ================================================================================
 */

#include "app_main.h"
#include "user_main.h"

#define MOTOR_STATUS_PERIOD_MS 3000U
#define OLED_STATUS_PERIOD_MS 500U
#define OLED_LINE_CHARS 21U

static int32_t oled_float_to_x10(float value)
{
    if (value >= 0.0f)
    {
        return (int32_t)(value * 10.0f + 0.5f);
    }

    return (int32_t)(value * 10.0f - 0.5f);
}

static int32_t oled_abs_i32(int32_t value)
{
    return (value < 0) ? -value : value;
}

static void oled_show_line(uint8_t y, const char *text)
{
    uint8_t index;
    char line[OLED_LINE_CHARS + 1U];

    for (index = 0; index < OLED_LINE_CHARS; index++)
    {
        line[index] = ' ';
    }

    for (index = 0; (index < OLED_LINE_CHARS) && (text[index] != '\0'); index++)
    {
        line[index] = text[index];
    }

    line[OLED_LINE_CHARS] = '\0';
    OLED_P6x8Str(0, y, (uint8_t *)line);
}

static void motor_control_stop(void)
{
    app_motor_set_closed_loop(0);
    motor_speed_set(0.0f, 0.0f, 0.0f, 0.0f);
    app_motor_run();
}

static void encoder_debug_run(void)
{
    static u32 last_print_ms = 0;
    u32 now = millis();

    if (!app_motor_get_closed_loop())
    {
        return;
    }

    if ((now - last_print_ms) < MOTOR_STATUS_PERIOD_MS)
    {
        return;
    }
    last_print_ms = now;

    printf("CL t=%lu target[A,B,C,D]=[%d,%d,%d,%d] "
           "speed[A,B,C,D]=[%d,%d,%d,%d] pwm[A,B,C,D]=[%d,%d,%d,%d]\r\n",
           (unsigned long)now,
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

/*
 * 函数名：SWJ_gpio_init
 * 功能：配置STM32的SWJ调试端口（JTAG/SWD）重映射，释放部分引脚作为普通IO使用
 * 说明：STM32默认SWJ端口（PA13/PA14/PA15/PB3/PB4）用于调试，需重映射才能作为普通IO
 */
void SWJ_gpio_init(void)
{
    /********************** 重映射配置说明 **********************
    1. 端口重映射依赖AFIO外设时钟，必须先使能RCC_APB2Periph_AFIO；
    2. 重映射模式选择（三选一，根据调试需求保留对应端口）：
       - GPIO_Remap_SWJ_Disable：完全禁用SWJ（JTAG+SWD），PA13/PA14/PA15/PB3/PB4均释放为普通IO；
       - GPIO_Remap_SWJ_JTAGDisable：禁用JTAG，保留SWD（仅PA13/PA14用于调试），PA15/PB3/PB4释放；
       - GPIO_Remap_SWJ_NoJTRST：保留完整SWJ，仅禁用JTRST（PB4），仅PB4释放为普通IO；
    *************************************************************/
    // 使能PA、PB端口时钟（调试端口所在引脚）和AFIO时钟（重映射必须）
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    // 选择"禁用JTAG，保留SWD"模式（兼顾调试和IO复用，常用模式）
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
}

/**
 * @函数描述: 工作指示灯循环任务，实现LED每1秒翻转一次（闪烁效果）
 * @return: 无
 * @说明：使用静态变量time_count记录上一次翻转时间，避免使用阻塞延时
 */
void app_led_run(void)
{
    static u32 time_count = 0; // 静态变量，函数调用后仍保留值，用于记录上一次LED翻转的毫秒时间

    // 若当前时间与上一次翻转时间差小于1000ms，直接返回，不执行翻转
    if (millis() - time_count < 1000)
        return;

    time_count = millis(); // 更新时间为当前时间
    LED_TOGGLE();          // 翻转LED状态
}

/*
 * 函数名：parameter_init
 * 功能：初始化系统参数（从W25X闪存读取配置，验证并初始化默认值）
 * 涉及：W25X闪存（存储参数）、舵机偏置PWM、预存命令解析
 */
void parameter_init(void)
{
    uint8_t i = 0;

    // delay_ms(10); // 短延时，等待W25X闪存初始化稳定

    // 从W25Q64闪存的指定地址（W25Q64_INFO_ADDR_SAVE_STR）读取参数，存入全局变量eeprom_info
    w25x_read((u8 *)(&eeprom_info), W25Q64_INFO_ADDR_SAVE_STR, sizeof(eeprom_info));

    // 版本验证
    if (eeprom_info.version != VERSION)
    {
        eeprom_info.version = VERSION; // 将当前固件版本号写入参数结构体
    }

    // 舵机偏置PWM验证：若验证标志不正确，初始化默认偏置
    if (eeprom_info.dj_bias_pwm[SERVO_NUM] != FLAG_VERIFY)
    {
        // 遍历所有舵机，将偏置PWM初始化为0
        for (i = 0; i < SERVO_NUM; i++)
        {
            eeprom_info.dj_bias_pwm[i] = 0;
        }
        eeprom_info.dj_bias_pwm[SERVO_NUM] = FLAG_VERIFY; // 写入验证标志，标记参数合法
    }

    // 应用舵机偏置
    for (i = 0; i < SERVO_NUM; i++)
    {
        pwmServo_bias_set(i, eeprom_info.dj_bias_pwm[i]);
    }

    // 预存命令执行：若预存命令验证通过且格式合法，解析并执行命令
    if (eeprom_info.pre_cmd[PRE_CMD_SIZE] == FLAG_VERIFY) // 预存命令的验证标志正确
    {
        if (eeprom_info.pre_cmd[0] == '$') // 命令格式校验
        {
            parse_cmd(eeprom_info.pre_cmd); // 解析预存命令
        }
    }
}

/*
 * 函数名：OLED_Show_Motor_Speed
 * 功能：在OLED屏上显示4个电机（A/B/C/D）的速度和系统电压，1.5秒刷新一次
 */
void OLED_Show_Motor_Speed(void)
{
    char text[20];             // 用于格式化显示字符串的缓冲区
    float voltage_v = 0;       // 存储计算后的系统电压
    static u32 time_count = 0; // 静态变量，记录上一次OLED刷新时间

    // 若当前时间与上一次刷新时间差小于1500ms（1.5秒），直接返回，不刷新
    if (millis() - time_count < 1500)
        return;

    time_count = millis(); // 更新刷新时间戳为当前时间
    // 系统电压计算
    voltage_v = ((float)ADC_ConvertedValue * 11.0f * 3.3f) / 4095.0f;

    // 电压报警
    if (voltage_v > 5 && voltage_v < 10)
    {
        beep_on_times(3, 10);
    }

    // 第1行显示：电机A、B的速度 mm/s
    sprintf(text, "A:%d, B:%d mm/s", (int)(Wheel_A.RT * 1000), (int)(Wheel_B.RT * 1000));
    oled_show_line(0, text);

    // 第2行显示：电机C、D的速度
    sprintf(text, "C:%d, D:%d mm/s", (int)(Wheel_C.RT * 1000), (int)(Wheel_D.RT * 1000));
    oled_show_line(1, text);

    oled_show_line(2, "");

    // 第3行显示：系统电压（保留2位小数）
    sprintf(text, "Voltage=%.2fv", voltage_v);
    oled_show_line(3, text);

    oled_show_line(4, "");
    oled_show_line(5, "");
    oled_show_line(6, "");
    oled_show_line(7, "");
}

static void oled_show_motor_closed_loop_status(void)
{
    char text[64];
    int32_t kp10;
    int32_t ki10;
    int32_t kd10;
    uint32_t ultrasonic_x100;
    static u32 time_count = 0;

    if (millis() - time_count < OLED_STATUS_PERIOD_MS)
        return;

    time_count = millis();
    kp10 = oled_float_to_x10(motor_kp);
    ki10 = oled_float_to_x10(motor_ki);
    kd10 = oled_float_to_x10(motor_kd);
    ultrasonic_x100 = app_sensor_get_ultrasonic_distance_x100();

    snprintf(text, sizeof(text), "P:%ld.%ld I:%ld.%ld D:%ld.%ld",
             (long)(kp10 / 10), (long)oled_abs_i32(kp10 % 10),
             (long)(ki10 / 10), (long)oled_abs_i32(ki10 % 10),
             (long)(kd10 / 10), (long)oled_abs_i32(kd10 % 10));
    oled_show_line(0, text);

    oled_show_line(1, "---------------------");

    snprintf(text, sizeof(text), "     A   B   C   D");
    oled_show_line(2, text);

    snprintf(text, sizeof(text), "TG%4d%4d%4d%4d",
             (int)(Wheel_A.TG * 1000.0f),
             (int)(Wheel_B.TG * 1000.0f),
             (int)(Wheel_C.TG * 1000.0f),
             (int)(Wheel_D.TG * 1000.0f));
    oled_show_line(3, text);

    snprintf(text, sizeof(text), "RT%4d%4d%4d%4d",
             (int)(Wheel_A.RT * 1000.0f),
             (int)(Wheel_B.RT * 1000.0f),
             (int)(Wheel_C.RT * 1000.0f),
             (int)(Wheel_D.RT * 1000.0f));
    oled_show_line(4, text);

    snprintf(text, sizeof(text), "PW%4d%4d%4d%4d",
             Wheel_A.PWM,
             Wheel_B.PWM,
             Wheel_C.PWM,
             Wheel_D.PWM);
    oled_show_line(5, text);

    oled_show_line(6, "---------------------");

    if (app_sensor_is_ultrasonic_distance_valid())
    {
        snprintf(text, sizeof(text), "US:%lu.%02lu cm",
                 (unsigned long)(ultrasonic_x100 / 100U),
                 (unsigned long)(ultrasonic_x100 % 100U));
    }
    else
    {
        snprintf(text, sizeof(text), "US: read failed");
    }
    oled_show_line(7, text);
}

void app_init(void)
{
    SysTick_Init();
    SWJ_gpio_init();
    led_init();
    LED_ON();
    app_uart_init();
    OLED_Init();

    printf("\r\nBOOT: app_init enter, tick=%lu\r\n",
           (unsigned long)millis());

    printf("BOOT: motor init start\r\n");
    app_motor_init();
    app_motor_set_closed_loop(0);
    motor_control_stop();
    printf("BOOT: motor init ok\r\n");

    printf("BOOT: sensor init start\r\n");
    app_sensor_init();
    printf("BOOT: sensor init ok\r\n");

    (void)ENCODER_A_GetCounter();
    (void)ENCODER_B_GetCounter();
    (void)ENCODER_C_GetCounter();
    (void)ENCODER_D_GetCounter();

    printf("\r\nMotor closed-loop ready.\r\n");
    printf("USART1: 115200 8N1, TX=PA9, RX=PA10.\r\n");
    printf("Use motor_speed_set(A,B,C,D) and app_motor_set_closed_loop(1) to run.\r\n");
    printf("Mapping: A=LF B=RF C=RR D=LR. PID period=%d ms, log period=%lu ms\r\n",
           1000 / PID_RATE,
           (unsigned long)MOTOR_STATUS_PERIOD_MS);
}

void app_loop(void)
{
    app_uart_run();
    app_sensor_run();
    app_led_run();
    encoder_debug_run();
    oled_show_motor_closed_loop_status();
}
