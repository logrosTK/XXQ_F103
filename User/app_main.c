#include "app_main.h"
#include "user_main.h"

#define MOTOR_TEST_PWM 250
#define KEY_ACTIVE_LEVEL 1U
#define KEY_DEBOUNCE_MS 30U
#define ENCODER_DEBUG_PERIOD_MS 500U

// A=left-front, B=right-front, C=right-rear, D=left-rear.
// Flip the sign of one macro if that wheel runs backward.
#define MOTOR_A_FORWARD_PWM  (MOTOR_TEST_PWM)
#define MOTOR_B_FORWARD_PWM  (-MOTOR_TEST_PWM)
#define MOTOR_C_FORWARD_PWM  (-MOTOR_TEST_PWM)
#define MOTOR_D_FORWARD_PWM  (MOTOR_TEST_PWM)

static uint8_t motor_test_enabled = 0;

static void motor_test_stop(void)
{
    MOTOR_A_SetSpeed(0);
    MOTOR_B_SetSpeed(0);
    MOTOR_C_SetSpeed(0);
    MOTOR_D_SetSpeed(0);
}

static void motor_test_forward(void)
{
    MOTOR_A_SetSpeed(MOTOR_A_FORWARD_PWM);
    MOTOR_B_SetSpeed(MOTOR_B_FORWARD_PWM);
    MOTOR_C_SetSpeed(MOTOR_C_FORWARD_PWM);
    MOTOR_D_SetSpeed(MOTOR_D_FORWARD_PWM);
}

static void motor_test_set_enabled(uint8_t enabled)
{
    motor_test_enabled = enabled ? 1U : 0U;

    if (motor_test_enabled)
    {
        motor_test_forward();
        printf("KEY: motor ON, PWM A=%d B=%d C=%d D=%d\r\n",
               MOTOR_A_FORWARD_PWM,
               MOTOR_B_FORWARD_PWM,
               MOTOR_C_FORWARD_PWM,
               MOTOR_D_FORWARD_PWM);
    }
    else
    {
        motor_test_stop();
        printf("KEY: motor OFF\r\n");
    }
}

static void key_toggle_run(void)
{
    static uint8_t last_sample = 0;
    static uint8_t stable_level = 0;
    static u32 last_change_ms = 0;

    uint8_t sample = KEY_GET_LEVEL() ? 1U : 0U;
    u32 now = millis();

    if (sample != last_sample)
    {
        last_sample = sample;
        last_change_ms = now;
    }

    if ((now - last_change_ms) < KEY_DEBOUNCE_MS)
    {
        return;
    }

    if (sample == stable_level)
    {
        return;
    }

    stable_level = sample;
    if (stable_level == KEY_ACTIVE_LEVEL)
    {
        motor_test_set_enabled((uint8_t)!motor_test_enabled);
    }
}

static void encoder_debug_run(void)
{
    static u32 last_print_ms = 0;
    u32 now = millis();

    if ((now - last_print_ms) < ENCODER_DEBUG_PERIOD_MS)
    {
        return;
    }
    last_print_ms = now;

    int16_t enc_a = ENCODER_A_GetCounter();
    int16_t enc_b = ENCODER_B_GetCounter();
    int16_t enc_c = ENCODER_C_GetCounter();
    int16_t enc_d = ENCODER_D_GetCounter();

    printf("DBG t=%lu run=%u key=%u enc[A,B,C,D]=[%d,%d,%d,%d] "
           "pins A=%u%u B=%u%u C=%u%u D=%u%u\r\n",
           (unsigned long)now,
           motor_test_enabled,
           KEY_GET_LEVEL() ? 1U : 0U,
           enc_a,
           enc_b,
           enc_c,
           enc_d,
           HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET,
           HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_SET,
           HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6) == GPIO_PIN_SET,
           HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7) == GPIO_PIN_SET,
           HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6) == GPIO_PIN_SET,
           HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7) == GPIO_PIN_SET,
           HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == GPIO_PIN_SET,
           HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_3) == GPIO_PIN_SET);
}

#define MOTOR_TEST_SPEED_MPS 0.01f   // 电机测试速度，单位：米/秒

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

void app_init(void)
{
    SysTick_Init();
    SWJ_gpio_init();
    led_init();
    key_init();
    app_motor_init();
    app_motor_set_closed_loop(0);
    motor_test_stop();

    (void)ENCODER_A_GetCounter();
    (void)ENCODER_B_GetCounter();
    (void)ENCODER_C_GetCounter();
    (void)ENCODER_D_GetCounter();

    printf("\r\nMotor open-loop test ready.\r\n");
    printf("USART1: 115200 8N1, TX=PA9, RX=PA10.\r\n");
    printf("KEY toggles motor. Mapping: A=LF B=RF C=RR D=LR.\r\n");
    printf("Forward PWM: A=%d B=%d C=%d D=%d\r\n",
           MOTOR_A_FORWARD_PWM,
           MOTOR_B_FORWARD_PWM,
           MOTOR_C_FORWARD_PWM,
           MOTOR_D_FORWARD_PWM);
}

void app_loop(void)
{
    app_led_run();
    key_toggle_run();
    encoder_debug_run();
}
