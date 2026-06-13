#include "stm32f10x.h"
#include "main.h"

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

    OLED_CLS(); // 清屏OLED，避免显示重叠

    // 第1行显示：电机A、B的速度 mm/s
    sprintf(text, "A:%d, B:%d mm/s", (int)(Wheel_A.RT * 1000), (int)(Wheel_B.RT * 1000));
    OLED_P6x8Str(0, 0, (uint8_t *)text);

    // 第2行显示：电机C、D的速度
    sprintf(text, "C:%d, D:%d mm/s", (int)(Wheel_C.RT * 1000), (int)(Wheel_D.RT * 1000));
    OLED_P6x8Str(0, 1, (uint8_t *)text);

    // 第3行显示：系统电压（保留2位小数）
    sprintf(text, "Voltage=%.2fv", voltage_v);
    OLED_P6x8Str(0, 3, (uint8_t *)text);
}

int main(void)
{
    SysTick_Init();       // 核心外设初始化：SysTick（系统滴答定时器，1ms计时，为millis()提供时间基准）
    SWJ_gpio_init();      // 调试端口重映射：释放部分SWJ引脚为普通IO
    led_init();           // LED指示灯初始化
    w25x_init();          // W25X闪存初始化
    ADC_init();           // ADC初始化
    BEEP_Init();          // 蜂鸣器初始化
    key_init();           // 按键初始化
    pwmServo_init();      // 舵机PWM初始化
    soft_i2c_gpio_init(); // 软件I2C初始化

    app_uart_init(); // 串口初始化
    // 系统参数初始化：从W25X读取参数，验证并配置默认值
    parameter_init();

    app_motor_init();  // 电机驱动初始化
    app_ps2_init();    // PS2手柄接收初始化
    app_sensor_init(); // 传感器初始化

    // 运动学参数配置：初始化运动学结构体
    setup_kinematics(100, 105, 88, 155, &kinematics); // 参数：轮距、轴距等，存入kinematics结构体

    OLED_Init(); // OLED显示屏初始化
    OLED_TEST(); // OLED测试

    // 开机自检：蜂鸣器响3次，每次100ms，提示系统启动完成
    beep_on_times(3, 20);

    // 系统信息打印
    printf("OpenSF4 VERSION=%d\n", VERSION);                                            // 打印固件版本号
    printf("w25q64 id=%x\n", w25x_readId());                                            // 打印W25Q64闪存ID（验证闪存通信正常）
    printf("Get_Voltage=%.2f\n", ((float)ADC_ConvertedValue * 11.0f * 3.3f) / 4095.0f); // 打印初始系统电压
		ultrasonic_rgb_r(0,255,0);
		ultrasonic_rgb_t(0,255,0);
		delay_ms(1000);
		ultrasonic_rgb_t(255,0,0);
		ultrasonic_rgb_r(255,0,0);
		delay_ms(1000);
		ultrasonic_rgb_t(0,0,255);
		ultrasonic_rgb_r(0,0,255);
    // motor_speed_set(0.1,0.1,0.1,0.1);
    while (1)
    {
        app_ps2_run();    // PS2手柄数据处理
        app_uart_run();   // 串口数据处理
        app_led_run();    // LED闪烁任务
        app_action_run(); // 动作执行任务

        app_sensor_run(); // 传感器数据处理

        // 执行显示任务
        if (oled_mode == 0)
        {
            OLED_Show_Motor_Speed(); // OLED显示电机速度和系统电压
        }

        // 按键检测与响应
        if (KEY_GET_LEVEL() == 1)
        {
            beep_on_times(1, 10); // 蜂鸣器短响1次，提示按键触发
            LED_TOGGLE();         // 翻转LED状态，直观反馈按键操作
        }
    }
}
