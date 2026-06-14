/*
 * ================================================================================
 * @文件名称: y_ADC.c
 * @功能描述: ADC模数转换驱动，使用DMA循环采集PC3引脚的模拟电压
 *           用于电池电压检测等模拟量采集
 * @所属模块: Components/y_ADC
 * @依赖: y_ADC.h, HAL库
 * @硬件: PC3 -> ADC1_CH13, DMA1_Channel1
 * @采样参数: 连续转换模式，软件触发，239.5周期采样时间
 * ================================================================================
 */

#include "y_ADC/y_ADC.h"

/*
 * 全局变量: ADC_ConvertedValue
 * 功能描述: ADC转换结果存储变量，由DMA自动更新
 *           12位ADC精度，取值范围 0~4095
 *           对应模拟电压: ADC_ConvertedValue * 3.3V / 4095
 */
__IO uint16_t ADC_ConvertedValue;

/** DMA句柄，用于ADC1的DMA数据传输 */
static DMA_HandleTypeDef hdma_adc1_user;

/*
 * 函数名称: ADC_init
 * 功能描述: 初始化ADC1，配置DMA循环采集模式
 *           使用PC3引脚（ADC_CHANNEL_13），连续转换
 *           DMA自动将转换结果存入ADC_ConvertedValue
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 在系统初始化时调用一次
 *           调用后ADC_ConvertedValue会由DMA自动持续更新
 *           读取ADC_ConvertedValue即可获得当前ADC采样值
 *           电压计算: V = ADC_ConvertedValue * 3.3 / 4095
 *           注意: 初始化失败会调用Error_Handler()
 */
void ADC_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    ADC_ChannelConfTypeDef sConfig = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_ADC_CONFIG(RCC_ADCPCLK2_DIV8);

    GPIO_InitStructure.Pin = GPIO_PIN_3;
    GPIO_InitStructure.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);

    hdma_adc1_user.Instance = DMA1_Channel1;
    hdma_adc1_user.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_adc1_user.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_adc1_user.Init.MemInc = DMA_MINC_DISABLE;
    hdma_adc1_user.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc1_user.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_adc1_user.Init.Mode = DMA_CIRCULAR;
    hdma_adc1_user.Init.Priority = DMA_PRIORITY_HIGH;
    if (HAL_DMA_Init(&hdma_adc1_user) != HAL_OK)
    {
        Error_Handler();
    }

    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode = ENABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    if (HAL_ADC_Init(&hadc1) != HAL_OK)
    {
        Error_Handler();
    }

    __HAL_LINKDMA(&hadc1, DMA_Handle, hdma_adc1_user);

    sConfig.Channel = ADC_CHANNEL_13;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)&ADC_ConvertedValue, 1) != HAL_OK)
    {
        Error_Handler();
    }
}
