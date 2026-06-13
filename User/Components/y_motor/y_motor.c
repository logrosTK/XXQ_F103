#include "y_motor/y_motor.h"

#define MOTOR_PWM_PERIOD 2000U

int16_t motor_kp = 800;
int16_t motor_kd = 400;

ROBOT_Wheel Wheel_A, Wheel_B, Wheel_C, Wheel_D;

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

static int16_t speed_pid(float spd_target, float spd_current, int16_t *motor_pwm_out, float *bias_last)
{
    float bias = spd_target - spd_current;

    *motor_pwm_out += (int16_t)(motor_kp * bias + motor_kd * (bias - *bias_last));
    *bias_last = bias;

    if (*motor_pwm_out > 2000)
    {
        *motor_pwm_out = 2000;
    }
    if (*motor_pwm_out < -2000)
    {
        *motor_pwm_out = -2000;
    }

    return *motor_pwm_out;
}

int16_t SPEED_PidCtlA(float spd_target, float spd_current)
{
    static int16_t motor_pwm_out;
    static float bias_last;
    return speed_pid(spd_target, spd_current, &motor_pwm_out, &bias_last);
}

int16_t SPEED_PidCtlB(float spd_target, float spd_current)
{
    static int16_t motor_pwm_out;
    static float bias_last;
    return speed_pid(spd_target, spd_current, &motor_pwm_out, &bias_last);
}

int16_t SPEED_PidCtlC(float spd_target, float spd_current)
{
    static int16_t motor_pwm_out;
    static float bias_last;
    return speed_pid(spd_target, spd_current, &motor_pwm_out, &bias_last);
}

int16_t SPEED_PidCtlD(float spd_target, float spd_current)
{
    static int16_t motor_pwm_out;
    static float bias_last;
    return speed_pid(spd_target, spd_current, &motor_pwm_out, &bias_last);
}

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
