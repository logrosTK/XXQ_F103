/*
 * ================================================================================
 * @文件名称: y_motor.c
 * @功能描述: 麦轮电机驱动层，包含PWM初始化、PID速度控制、电机方向控制
 *           A=左前(LF) B=右前(RF) C=右后(RR) D=左后(LR)
 * @所属模块: Components/y_motor
 * @依赖: y_motor.h, HAL库
 * @硬件: 
 *   TIM8_CH1/CH2/CH3/CH4 -> 电机A/B驱动 (PC6/PC7/PC8/PC9)
 *   TIM1_CH1/CH2N/CH3N/CH4 -> 电机C/D驱动 (PA8/PB0/PB1/PA11)
 * @控制算法: PID速度控制，PWM周期2000
 * ================================================================================
 */

#include "y_motor/y_motor.h"

/** PWM周期（ARR值），2000对应20kHz的PWM频率 */
#define MOTOR_PWM_PERIOD 2000U
#define MOTOR_PID_OUTPUT_LIMIT 2000.0f
#define MOTOR_PID_INTEGRAL_LIMIT 40.0f

/** PID速度控制参数：比例、积分、微分系数 */
float motor_kp = 800.0f;
float motor_ki = 35.0f;
float motor_kd = 400.0f;

/*
 * 全局结构体: Wheel_A/B/C/D
 * 功能描述: 四个麦轮的实时状态
 *   每个轮子包含:
 *   .TG - 目标速度 (float, m/s)
 *   .RT - 实际速度 (float, m/s, 由编码器计算得出)
 *   .PWM - PID计算输出PWM值 (int, -2000~2000)
 */
ROBOT_Wheel Wheel_A, Wheel_B, Wheel_C, Wheel_D;

/*
 * 结构体: MotorPidState
 * 功能描述: 每个电机的PID状态
 */
typedef struct
{
    float integral;       /**< 积分累计项 */
    float error_last;     /**< 上一次速度偏差，用于微分计算 */
} MotorPidState;

/** 四个电机的PID状态 */
static MotorPidState pid_a;
static MotorPidState pid_b;
static MotorPidState pid_c;
static MotorPidState pid_d;

static float motor_clamp_float(float value, float min_value, float max_value)
{
    if (value > max_value)
    {
        return max_value;
    }
    if (value < min_value)
    {
        return min_value;
    }
    return value;
}

static int16_t motor_float_to_i16(float value)
{
    return (int16_t)((value >= 0.0f) ? (value + 0.5f) : (value - 0.5f));
}

static void speed_pid_state_reset(MotorPidState *state)
{
    state->integral = 0.0f;
    state->error_last = 0.0f;
}

/*
 * 函数名称: motor_gpio_init
 * 功能描述: 初始化电机驱动GPIO引脚，配置TIM1/TIM8的PWM输出引脚
 *           使能AFIO时钟，配置TIM1部分重映射
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 内部函数，由motor_init()调用
 *           PC6/7/8/9 -> TIM8_CH1/2/3/4
 *           PB0/1 -> TIM1_CH2N/CH3N, PA8/11 -> TIM1_CH1/CH4
 */
static void motor_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_AFIO_REMAP_TIM1_PARTIAL();

    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_11;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/*
 * 函数名称: motor_base_init
 * 功能描述: 初始化定时器基础PWM参数
 *           预分频=1，计数周期=MOTOR_PWM_PERIOD(2000)
 * 参数说明: htim - 定时器句柄指针（TIM1或TIM8）
 * 返回值:   无
 * 使用说明: 内部函数，由motor_init()调用
 */
static void motor_base_init(TIM_HandleTypeDef *htim)
{
    htim->Init.Prescaler = 1;
    htim->Init.CounterMode = TIM_COUNTERMODE_UP;
    htim->Init.Period = MOTOR_PWM_PERIOD;
    htim->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if ((htim->Instance == TIM1) || (htim->Instance == TIM8))
    {
        htim->Init.RepetitionCounter = 0;
    }

    if (HAL_TIM_PWM_Init(htim) != HAL_OK)
    {
        Error_Handler();
    }
}

/*
 * 函数名称: motor_channel_init
 * 功能描述: 配置单个PWM通道参数
 *           PWM模式1，高电平有效，初始占空比0
 * 参数说明: htim - 定时器句柄指针
 *          channel - 通道号 (TIM_CHANNEL_1/2/3/4)
 * 返回值:   无
 * 使用说明: 内部函数，由motor_init()调用
 */
static void motor_channel_init(TIM_HandleTypeDef *htim, uint32_t channel)
{
    TIM_OC_InitTypeDef sConfigOC = {0};

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;

    if (HAL_TIM_PWM_ConfigChannel(htim, &sConfigOC, channel) != HAL_OK)
    {
        Error_Handler();
    }
}

/*
 * 函数名称: motor_break_dead_time_init
 * 功能描述: 配置高级定时器的刹车和死区时间
 *           死区时间设为0，刹车功能禁用
 * 参数说明: htim - 高级定时器句柄指针（TIM1或TIM8）
 * 返回值:   无
 * 使用说明: 内部函数，仅TIM1/TIM8（高级定时器）需要此配置
 */
static void motor_break_dead_time_init(TIM_HandleTypeDef *htim)
{
    TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

    sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
    sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
    sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
    sBreakDeadTimeConfig.DeadTime = 0;
    sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
    sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
    sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_ENABLE;

    if (HAL_TIMEx_ConfigBreakDeadTime(htim, &sBreakDeadTimeConfig) != HAL_OK)
    {
        Error_Handler();
    }
}

/*
 * 函数名称: motor_init
 * 功能描述: 初始化所有电机驱动硬件
 *           配置TIM1（电机C/D）和TIM8（电机A/B）的PWM输出
 *           电机A: TIM8_CH1(前进)/CH2(后退)
 *           电机B: TIM8_CH3(前进)/CH4(后退)
 *           电机C: TIM1_CH2N(前进)/CH3N(后退)
 *           电机D: TIM1_CH1(前进)/CH4(后退)
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 在系统初始化时调用一次
 *           调用后可通过MOTOR_A/B/C/D_SetSpeed()设置各电机PWM
 *           初始化失败会调用Error_Handler()
 */
void motor_init(void)
{
    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_TIM8_CLK_ENABLE();
    motor_gpio_init();

    htim8.Instance = TIM8;
    motor_base_init(&htim8);
    motor_channel_init(&htim8, TIM_CHANNEL_1);
    motor_channel_init(&htim8, TIM_CHANNEL_2);
    motor_channel_init(&htim8, TIM_CHANNEL_3);
    motor_channel_init(&htim8, TIM_CHANNEL_4);
    motor_break_dead_time_init(&htim8);
    HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_4);

    htim1.Instance = TIM1;
    motor_base_init(&htim1);
    motor_channel_init(&htim1, TIM_CHANNEL_1);
    motor_channel_init(&htim1, TIM_CHANNEL_2);
    motor_channel_init(&htim1, TIM_CHANNEL_3);
    motor_channel_init(&htim1, TIM_CHANNEL_4);
    motor_break_dead_time_init(&htim1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
}

/*
 * 函数名称: SPEED_PidReset
 * 功能描述: 重置所有四个电机的PID状态（积分累加和上一次偏差清零）
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 在切换闭环控制模式或停止电机时调用
 *           防止PID积分饱和导致电机重新启动时的突变
 */
void SPEED_PidReset(void)
{
    SPEED_PidResetA();
    SPEED_PidResetB();
    SPEED_PidResetC();
    SPEED_PidResetD();
}

void SPEED_PidResetA(void)
{
    speed_pid_state_reset(&pid_a);
}

void SPEED_PidResetB(void)
{
    speed_pid_state_reset(&pid_b);
}

void SPEED_PidResetC(void)
{
    speed_pid_state_reset(&pid_c);
}

void SPEED_PidResetD(void)
{
    speed_pid_state_reset(&pid_d);
}

void SPEED_PidSetParam(float kp, float ki, float kd)
{
    motor_kp = kp;
    motor_ki = ki;
    motor_kd = kd;
    SPEED_PidReset();
}

/*
 * 函数名称: speed_pid
 * 功能描述: 位置式PID速度控制算法
 *           output = Kp * error + Ki * integral + Kd * derivative
 *           error = spd_target - spd_current（速度偏差）
 * 参数说明: spd_target - 目标速度 (float, m/s)
 *          spd_current - 当前实际速度 (float, m/s)
 *          state - PID状态结构体指针（保存积分和上一次误差）
 * 返回值:   16位有符号整数，PWM输出值（范围: -2000 ~ 2000）
 * 使用说明: 内部函数，由SPEED_PidCtlA/B/C/D()调用
 *           PWM输出被限制在[-2000, 2000]范围内
 */
static int16_t speed_pid(float spd_target, float spd_current, MotorPidState *state)
{
    float error = spd_target - spd_current;
    float derivative = error - state->error_last;
    float output;

    state->integral += error;
    state->integral = motor_clamp_float(state->integral,
                                        -MOTOR_PID_INTEGRAL_LIMIT,
                                        MOTOR_PID_INTEGRAL_LIMIT);
    state->error_last = error;

    output = (motor_kp * error) +
             (motor_ki * state->integral) +
             (motor_kd * derivative);
    output = motor_clamp_float(output,
                               -MOTOR_PID_OUTPUT_LIMIT,
                               MOTOR_PID_OUTPUT_LIMIT);

    return motor_float_to_i16(output);
}

/*
 * 函数名称: SPEED_PidCtlA
 * 功能描述: 电机A（左前轮）的PID速度控制
 * 参数说明: spd_target - 目标速度 (m/s, 正值向前)
 *          spd_current - 当前实际速度 (m/s, 由编码器计算)
 * 返回值:   16位有符号整数，PWM输出值
 * 使用说明: 由app_motor_run()周期调用
 */
int16_t SPEED_PidCtlA(float spd_target, float spd_current)
{
    return speed_pid(spd_target, spd_current, &pid_a);
}

/*
 * 函数名称: SPEED_PidCtlB
 * 功能描述: 电机B（右前轮）的PID速度控制
 * 参数说明: spd_target - 目标速度 (m/s)
 *          spd_current - 当前实际速度 (m/s)
 * 返回值:   16位有符号整数，PWM输出值
 * 使用说明: 由app_motor_run()周期调用
 */
int16_t SPEED_PidCtlB(float spd_target, float spd_current)
{
    return speed_pid(spd_target, spd_current, &pid_b);
}

/*
 * 函数名称: SPEED_PidCtlC
 * 功能描述: 电机C（右后轮）的PID速度控制
 * 参数说明: spd_target - 目标速度 (m/s)
 *          spd_current - 当前实际速度 (m/s)
 * 返回值:   16位有符号整数，PWM输出值
 * 使用说明: 由app_motor_run()周期调用
 */
int16_t SPEED_PidCtlC(float spd_target, float spd_current)
{
    return speed_pid(spd_target, spd_current, &pid_c);
}

/*
 * 函数名称: SPEED_PidCtlD
 * 功能描述: 电机D（左后轮）的PID速度控制
 * 参数说明: spd_target - 目标速度 (m/s)
 *          spd_current - 当前实际速度 (m/s)
 * 返回值:   16位有符号整数，PWM输出值
 * 使用说明: 由app_motor_run()周期调用
 */
int16_t SPEED_PidCtlD(float spd_target, float spd_current)
{
    return speed_pid(spd_target, spd_current, &pid_d);
}

/*
 * 函数名称: motor_limit_speed
 * 功能描述: 限制PWM速度值在有效范围内 [-2000, 2000]
 * 参数说明: speed - 输入的PWM速度值
 * 返回值:   限制后的PWM速度值
 * 使用说明: 内部函数
 */
static int16_t motor_limit_speed(int16_t speed)
{
    if (speed > 2000)
    {
        return 2000;
    }
    if (speed < -2000)
    {
        return -2000;
    }
    return speed;
}

/*
 * 函数名称: MOTOR_A_SetSpeed
 * 功能描述: 设置电机A（左前轮）的PWM速度
 *           正值向前: CH2=2000, CH1=2000-speed
 *           负值向后: CH2=2000+speed, CH1=2000
 * 参数说明: speed - PWM速度值，范围[-2000, 2000]，正值向前
 * 返回值:   无
 * 使用说明: 由app_motor_run()周期调用
 *           TIM8_CH1=前进通道, TIM8_CH2=后退通道
 */
void MOTOR_A_SetSpeed(int16_t speed)
{
    int16_t temp = motor_limit_speed(speed);

    if (temp > 0)
    {
        __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_2, MOTOR_PWM_PERIOD);
        __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_1, MOTOR_PWM_PERIOD - temp);
    }
    else
    {
        __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_2, MOTOR_PWM_PERIOD + temp);
        __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_1, MOTOR_PWM_PERIOD);
    }
}

/*
 * 函数名称: MOTOR_B_SetSpeed
 * 功能描述: 设置电机B（右前轮）的PWM速度
 *           正值向前: CH4=2000, CH3=2000-speed
 * 参数说明: speed - PWM速度值，范围[-2000, 2000]，正值向前
 * 返回值:   无
 * 使用说明: TIM8_CH3=前进通道, TIM8_CH4=后退通道
 */
void MOTOR_B_SetSpeed(int16_t speed)
{
    int16_t temp = motor_limit_speed(speed);

    if (temp > 0)
    {
        __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_4, MOTOR_PWM_PERIOD);
        __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_3, MOTOR_PWM_PERIOD - temp);
    }
    else
    {
        __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_4, MOTOR_PWM_PERIOD + temp);
        __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_3, MOTOR_PWM_PERIOD);
    }
}

/*
 * 函数名称: MOTOR_C_SetSpeed
 * 功能描述: 设置电机C（右后轮）的PWM速度
 *           正值向前: CH3=2000, CH2=2000-speed
 * 参数说明: speed - PWM速度值，范围[-2000, 2000]，正值向前
 * 返回值:   无
 * 使用说明: TIM1_CH2N=前进通道, TIM1_CH3N=后退通道
 */
void MOTOR_C_SetSpeed(int16_t speed)
{
    int16_t temp = motor_limit_speed(speed);

    if (temp > 0)
    {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, MOTOR_PWM_PERIOD);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, MOTOR_PWM_PERIOD - temp);
    }
    else
    {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, MOTOR_PWM_PERIOD + temp);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, MOTOR_PWM_PERIOD);
    }
}

/*
 * 函数名称: MOTOR_D_SetSpeed
 * 功能描述: 设置电机D（左后轮）的PWM速度
 *           正值向前: CH4=2000, CH1=2000-speed
 * 参数说明: speed - PWM速度值，范围[-2000, 2000]，正值向前
 * 返回值:   无
 * 使用说明: TIM1_CH1=前进通道, TIM1_CH4=后退通道
 */
void MOTOR_D_SetSpeed(int16_t speed)
{
    int16_t temp = motor_limit_speed(speed);

    if (temp > 0)
    {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, MOTOR_PWM_PERIOD);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, MOTOR_PWM_PERIOD - temp);
    }
    else
    {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, MOTOR_PWM_PERIOD + temp);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, MOTOR_PWM_PERIOD);
    }
}
