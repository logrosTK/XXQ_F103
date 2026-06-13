#include "y_ADC/y_ADC.h"

__IO uint16_t ADC_ConvertedValue;

static DMA_HandleTypeDef hdma_adc1_user;

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
