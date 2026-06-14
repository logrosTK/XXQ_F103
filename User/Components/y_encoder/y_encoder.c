/*
 * ================================================================================
 * @文件名称: y_encoder.c
 * @功能描述: 四路编码器驱动，使用TIM2/3/4/5的编码器模式读取电机转速
 *           每个定时器配置为TI12编码器模式，内部上拉，滤波器10
 * @所属模块: Components/y_encoder
 * @依赖: y_encoder.h, HAL库
 * @硬件连接:
 *   编码器A(左前): PA0(TIM5_CH1), PA1(TIM5_CH2)
 *   编码器B(右前): PB6(TIM4_CH1), PB7(TIM4_CH2)
 *   编码器C(右后): PA6(TIM3_CH1), PA7(TIM3_CH2)
 *   编码器D(左后): PA15(TIM2_CH1), PB3(TIM2_CH2)
 * @注意事项: TIM2需要AFIO重映射到PA15/PB3
 * ================================================================================
 */

#include "y_encoder/y_encoder.h"

/*
 * 函数名称: encoder_gpio_init
 * 功能描述: 初始化编码器GPIO引脚，配置为输入+内部上拉模式
 *           使能AFIO时钟，配置SWJ重映射（禁用JTAG保留SWD）和TIM2重映射
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 内部函数，由Encoder_Init()调用
 *           使用内部上拉是因为霍尔编码器通常为开漏输出
 */
static void encoder_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_AFIO_REMAP_SWJ_NOJTAG();
    __HAL_AFIO_REMAP_TIM2_ENABLE();

    /* 使用内部上拉：霍尔编码器通常为开漏输出，需要上拉才能产生边沿跳变 */
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_6 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/*
 * 函数名称: encoder_config
 * 功能描述: 配置单个定时器为编码器模式
 *           编码器模式: TI12（两路信号均计数），上升沿触发
 *           输入滤波: 10（滤除高频噪声）
 * 参数说明: htim - 定时器句柄指针（TIM2/3/4/5）
 * 返回值:   无
 * 使用说明: 内部函数，由Encoder_Init()调用
 *           计数器范围由ENCODER_TIM_PERIOD定义
 *           初始化后计数器清零并启动编码器模式
 */
static void encoder_config(TIM_HandleTypeDef *htim)
{
    TIM_Encoder_InitTypeDef sConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    htim->Init.Prescaler = 0;
    htim->Init.CounterMode = TIM_COUNTERMODE_UP;
    htim->Init.Period = ENCODER_TIM_PERIOD;
    htim->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
    sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
    sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
    sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
    sConfig.IC1Filter = 10;
    sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
    sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
    sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
    sConfig.IC2Filter = 10;

    if (HAL_TIM_Encoder_Init(htim, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(htim, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }

    __HAL_TIM_SET_COUNTER(htim, 0);
    if (HAL_TIM_Encoder_Start(htim, TIM_CHANNEL_ALL) != HAL_OK)
    {
        Error_Handler();
    }
}

/*
 * 函数名称: ENCODER_A_GetCounter
 * 功能描述: 获取编码器A（左前轮）的计数值并清零
 * 参数说明: 无
 * 返回值:   16位有符号整数，编码器A的脉冲计数值（读取后自动清零）
 * 使用说明: 每次调用返回自上次调用以来的脉冲增量
 *           正值表示正转，负值表示反转
 *           配合MEC_WHEEL_SCALE可换算为实际速度(m/s)
 */
int16_t ENCODER_A_GetCounter(void)
{
    int16_t Encoder_TIM = (int16_t)__HAL_TIM_GET_COUNTER(&htim5);
    __HAL_TIM_SET_COUNTER(&htim5, 0);
    return Encoder_TIM;
}

/*
 * 函数名称: ENCODER_B_GetCounter
 * 功能描述: 获取编码器B（右前轮）的计数值并清零
 * 参数说明: 无
 * 返回值:   16位有符号整数，编码器B的脉冲计数值（读取后自动清零）
 * 使用说明: 同ENCODER_A_GetCounter
 */
int16_t ENCODER_B_GetCounter(void)
{
    int16_t Encoder_TIM = (int16_t)__HAL_TIM_GET_COUNTER(&htim4);
    __HAL_TIM_SET_COUNTER(&htim4, 0);
    return Encoder_TIM;
}

/*
 * 函数名称: ENCODER_C_GetCounter
 * 功能描述: 获取编码器C（右后轮）的计数值并清零
 * 参数说明: 无
 * 返回值:   16位有符号整数，编码器C的脉冲计数值（读取后自动清零）
 * 使用说明: 同ENCODER_A_GetCounter
 */
int16_t ENCODER_C_GetCounter(void)
{
    int16_t Encoder_TIM = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);
    __HAL_TIM_SET_COUNTER(&htim3, 0);
    return Encoder_TIM;
}

/*
 * 函数名称: ENCODER_D_GetCounter
 * 功能描述: 获取编码器D（左后轮）的计数值并清零
 * 参数说明: 无
 * 返回值:   16位有符号整数，编码器D的脉冲计数值（读取后自动清零）
 * 使用说明: 同ENCODER_A_GetCounter
 */
int16_t ENCODER_D_GetCounter(void)
{
    int16_t Encoder_TIM = (int16_t)__HAL_TIM_GET_COUNTER(&htim2);
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    return Encoder_TIM;
}

/*
 * 函数名称: Encoder_Init
 * 功能描述: 初始化所有四路编码器（TIM2/3/4/5）
 *           使能AFIO时钟，配置TIM2重映射，初始化GPIO和编码器模式
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 在系统初始化时调用一次
 *           初始化顺序: AFIO时钟 -> TIM2重映射 -> GPIO -> 编码器配置
 *           调用后可通过ENCODER_A/B/C/D_GetCounter()读取各编码器脉冲数
 */
void Encoder_Init(void)
{
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_AFIO_REMAP_TIM2_ENABLE();
    encoder_gpio_init();

    htim2.Instance = TIM2;
    htim3.Instance = TIM3;
    htim4.Instance = TIM4;
    htim5.Instance = TIM5;

    encoder_config(&htim2);
    encoder_config(&htim4);
    encoder_config(&htim5);
    encoder_config(&htim3);
}
