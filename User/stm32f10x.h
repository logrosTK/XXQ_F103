#ifndef STM32F10X_HAL_COMPAT_H
#define STM32F10X_HAL_COMPAT_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef enum
{
    Bit_RESET = 0,
    Bit_SET = 1
} BitAction;

#define GPIO_Pin_0  GPIO_PIN_0
#define GPIO_Pin_1  GPIO_PIN_1
#define GPIO_Pin_2  GPIO_PIN_2
#define GPIO_Pin_3  GPIO_PIN_3
#define GPIO_Pin_4  GPIO_PIN_4
#define GPIO_Pin_5  GPIO_PIN_5
#define GPIO_Pin_6  GPIO_PIN_6
#define GPIO_Pin_7  GPIO_PIN_7
#define GPIO_Pin_8  GPIO_PIN_8
#define GPIO_Pin_9  GPIO_PIN_9
#define GPIO_Pin_10 GPIO_PIN_10
#define GPIO_Pin_11 GPIO_PIN_11
#define GPIO_Pin_12 GPIO_PIN_12
#define GPIO_Pin_13 GPIO_PIN_13
#define GPIO_Pin_14 GPIO_PIN_14
#define GPIO_Pin_15 GPIO_PIN_15

#define GPIO_Pin   Pin
#define GPIO_Mode  Mode
#define GPIO_Speed Speed

#define GPIO_Speed_50MHz GPIO_SPEED_FREQ_HIGH

#define GPIO_Mode_AIN         0x10000001U
#define GPIO_Mode_IN_FLOATING 0x10000002U
#define GPIO_Mode_IPU         0x10000003U
#define GPIO_Mode_IPD         0x10000004U
#define GPIO_Mode_Out_PP      0x10000005U
#define GPIO_Mode_Out_OD      0x10000006U
#define GPIO_Mode_AF_PP       0x10000007U
#define GPIO_Mode_AF_OD       0x10000008U

static inline void GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_InitStruct)
{
    GPIO_InitTypeDef init = {0};

    init.Pin = GPIO_InitStruct->Pin;
    init.Speed = GPIO_InitStruct->Speed;
    init.Pull = GPIO_NOPULL;

    switch (GPIO_InitStruct->Mode)
    {
    case GPIO_Mode_AIN:
        init.Mode = GPIO_MODE_ANALOG;
        break;
    case GPIO_Mode_IN_FLOATING:
        init.Mode = GPIO_MODE_INPUT;
        break;
    case GPIO_Mode_IPU:
        init.Mode = GPIO_MODE_INPUT;
        init.Pull = GPIO_PULLUP;
        break;
    case GPIO_Mode_IPD:
        init.Mode = GPIO_MODE_INPUT;
        init.Pull = GPIO_PULLDOWN;
        break;
    case GPIO_Mode_Out_PP:
        init.Mode = GPIO_MODE_OUTPUT_PP;
        break;
    case GPIO_Mode_Out_OD:
        init.Mode = GPIO_MODE_OUTPUT_OD;
        break;
    case GPIO_Mode_AF_PP:
        init.Mode = GPIO_MODE_AF_PP;
        break;
    case GPIO_Mode_AF_OD:
        init.Mode = GPIO_MODE_AF_OD;
        break;
    default:
        init.Mode = GPIO_InitStruct->Mode;
        break;
    }

    HAL_GPIO_Init(GPIOx, &init);
}

static inline void GPIO_SetBits(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_SET);
}

static inline void GPIO_ResetBits(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_RESET);
}

static inline uint8_t GPIO_ReadInputDataBit(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    return (HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) == GPIO_PIN_SET) ? 1U : 0U;
}

static inline uint8_t GPIO_ReadOutputDataBit(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    return ((GPIOx->ODR & GPIO_Pin) != 0U) ? 1U : 0U;
}

static inline void GPIO_WriteBit(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, BitAction BitVal)
{
    HAL_GPIO_WritePin(GPIOx, GPIO_Pin, (BitVal == Bit_RESET) ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

#define RCC_APB2Periph_AFIO   (1UL << 0)
#define RCC_APB2Periph_GPIOA  (1UL << 1)
#define RCC_APB2Periph_GPIOB  (1UL << 2)
#define RCC_APB2Periph_GPIOC  (1UL << 3)
#define RCC_APB2Periph_GPIOD  (1UL << 4)
#define RCC_APB2Periph_ADC1   (1UL << 5)
#define RCC_APB2Periph_USART1 (1UL << 6)
#define RCC_APB2Periph_TIM1   (1UL << 7)
#define RCC_APB2Periph_TIM8   (1UL << 8)

#define RCC_APB1Periph_SPI2   (1UL << 0)
#define RCC_APB1Periph_USART2 (1UL << 1)
#define RCC_APB1Periph_USART3 (1UL << 2)
#define RCC_APB1Periph_UART5  (1UL << 3)
#define RCC_APB1Periph_TIM2   (1UL << 4)
#define RCC_APB1Periph_TIM3   (1UL << 5)
#define RCC_APB1Periph_TIM4   (1UL << 6)
#define RCC_APB1Periph_TIM5   (1UL << 7)
#define RCC_APB1Periph_TIM7   (1UL << 8)

#define RCC_AHBPeriph_DMA1    (1UL << 0)

static inline void RCC_APB2PeriphClockCmd(uint32_t RCC_APB2Periph, FunctionalState NewState)
{
    if (NewState == DISABLE)
    {
        return;
    }

    if ((RCC_APB2Periph & RCC_APB2Periph_AFIO) != 0U)   { __HAL_RCC_AFIO_CLK_ENABLE(); }
    if ((RCC_APB2Periph & RCC_APB2Periph_GPIOA) != 0U)  { __HAL_RCC_GPIOA_CLK_ENABLE(); }
    if ((RCC_APB2Periph & RCC_APB2Periph_GPIOB) != 0U)  { __HAL_RCC_GPIOB_CLK_ENABLE(); }
    if ((RCC_APB2Periph & RCC_APB2Periph_GPIOC) != 0U)  { __HAL_RCC_GPIOC_CLK_ENABLE(); }
    if ((RCC_APB2Periph & RCC_APB2Periph_GPIOD) != 0U)  { __HAL_RCC_GPIOD_CLK_ENABLE(); }
    if ((RCC_APB2Periph & RCC_APB2Periph_ADC1) != 0U)   { __HAL_RCC_ADC1_CLK_ENABLE(); }
    if ((RCC_APB2Periph & RCC_APB2Periph_USART1) != 0U) { __HAL_RCC_USART1_CLK_ENABLE(); }
    if ((RCC_APB2Periph & RCC_APB2Periph_TIM1) != 0U)   { __HAL_RCC_TIM1_CLK_ENABLE(); }
    if ((RCC_APB2Periph & RCC_APB2Periph_TIM8) != 0U)   { __HAL_RCC_TIM8_CLK_ENABLE(); }
}

static inline void RCC_APB1PeriphClockCmd(uint32_t RCC_APB1Periph, FunctionalState NewState)
{
    if (NewState == DISABLE)
    {
        return;
    }

    if ((RCC_APB1Periph & RCC_APB1Periph_SPI2) != 0U)   { __HAL_RCC_SPI2_CLK_ENABLE(); }
    if ((RCC_APB1Periph & RCC_APB1Periph_USART2) != 0U) { __HAL_RCC_USART2_CLK_ENABLE(); }
    if ((RCC_APB1Periph & RCC_APB1Periph_USART3) != 0U) { __HAL_RCC_USART3_CLK_ENABLE(); }
    if ((RCC_APB1Periph & RCC_APB1Periph_UART5) != 0U)  { __HAL_RCC_UART5_CLK_ENABLE(); }
    if ((RCC_APB1Periph & RCC_APB1Periph_TIM2) != 0U)   { __HAL_RCC_TIM2_CLK_ENABLE(); }
    if ((RCC_APB1Periph & RCC_APB1Periph_TIM3) != 0U)   { __HAL_RCC_TIM3_CLK_ENABLE(); }
    if ((RCC_APB1Periph & RCC_APB1Periph_TIM4) != 0U)   { __HAL_RCC_TIM4_CLK_ENABLE(); }
    if ((RCC_APB1Periph & RCC_APB1Periph_TIM5) != 0U)   { __HAL_RCC_TIM5_CLK_ENABLE(); }
    if ((RCC_APB1Periph & RCC_APB1Periph_TIM7) != 0U)   { __HAL_RCC_TIM7_CLK_ENABLE(); }
}

static inline void RCC_AHBPeriphClockCmd(uint32_t RCC_AHBPeriph, FunctionalState NewState)
{
    if ((NewState != DISABLE) && ((RCC_AHBPeriph & RCC_AHBPeriph_DMA1) != 0U))
    {
        __HAL_RCC_DMA1_CLK_ENABLE();
    }
}

#define RCC_PCLK2_Div8 RCC_ADCPCLK2_DIV8

static inline void RCC_ADCCLKConfig(uint32_t RCC_PCLK2)
{
    __HAL_RCC_ADC_CONFIG(RCC_PCLK2);
}

#define GPIO_Remap_SWJ_Disable     1U
#define GPIO_Remap_SWJ_JTAGDisable 2U
#define GPIO_Remap_SWJ_NoJTRST     3U
#define GPIO_PartialRemap_TIM1     4U
#define GPIO_FullRemap_TIM2        5U

static inline void GPIO_PinRemapConfig(uint32_t GPIO_Remap, FunctionalState NewState)
{
    if (NewState == DISABLE)
    {
        return;
    }

    __HAL_RCC_AFIO_CLK_ENABLE();

    switch (GPIO_Remap)
    {
    case GPIO_Remap_SWJ_Disable:
        __HAL_AFIO_REMAP_SWJ_DISABLE();
        break;
    case GPIO_Remap_SWJ_JTAGDisable:
        __HAL_AFIO_REMAP_SWJ_NOJTAG();
        break;
    case GPIO_Remap_SWJ_NoJTRST:
        __HAL_AFIO_REMAP_SWJ_NONJTRST();
        break;
    case GPIO_PartialRemap_TIM1:
        __HAL_AFIO_REMAP_TIM1_PARTIAL();
        break;
    case GPIO_FullRemap_TIM2:
        __HAL_AFIO_REMAP_TIM2_ENABLE();
        break;
    default:
        break;
    }
}

typedef struct
{
    IRQn_Type NVIC_IRQChannel;
    uint32_t NVIC_IRQChannelPreemptionPriority;
    uint32_t NVIC_IRQChannelSubPriority;
    FunctionalState NVIC_IRQChannelCmd;
} NVIC_InitTypeDef;

static inline void NVIC_Init(NVIC_InitTypeDef *NVIC_InitStruct)
{
    HAL_NVIC_SetPriority(NVIC_InitStruct->NVIC_IRQChannel,
                         NVIC_InitStruct->NVIC_IRQChannelPreemptionPriority,
                         NVIC_InitStruct->NVIC_IRQChannelSubPriority);

    if (NVIC_InitStruct->NVIC_IRQChannelCmd == ENABLE)
    {
        HAL_NVIC_EnableIRQ(NVIC_InitStruct->NVIC_IRQChannel);
    }
    else
    {
        HAL_NVIC_DisableIRQ(NVIC_InitStruct->NVIC_IRQChannel);
    }
}

#define USART_IT_RXNE UART_IT_RXNE
#define USART_IT_TXE  UART_IT_TXE

typedef UART_InitTypeDef USART_InitTypeDef;

#define USART_BaudRate           BaudRate
#define USART_WordLength         WordLength
#define USART_StopBits           StopBits
#define USART_Parity             Parity
#define USART_Mode               Mode
#define USART_HardwareFlowControl HwFlowCtl

#define USART_WordLength_8b UART_WORDLENGTH_8B
#define USART_StopBits_1    UART_STOPBITS_1
#define USART_Parity_No     UART_PARITY_NONE
#define USART_Mode_Rx       UART_MODE_RX
#define USART_Mode_Tx       UART_MODE_TX
#define USART_HardwareFlowControl_None UART_HWCONTROL_NONE

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;
extern UART_HandleTypeDef huart5;

static inline UART_HandleTypeDef *compat_uart_handle(USART_TypeDef *USARTx)
{
    if (USARTx == USART1) { return &huart1; }
    if (USARTx == USART2) { return &huart2; }
    if (USARTx == USART3) { return &huart3; }
    if (USARTx == UART5)  { return &huart5; }
    return NULL;
}

static inline void USART_DeInit(USART_TypeDef *USARTx)
{
    UART_HandleTypeDef *huart = compat_uart_handle(USARTx);
    if (huart != NULL)
    {
        (void)HAL_UART_DeInit(huart);
    }
}

static inline void USART_Init(USART_TypeDef *USARTx, USART_InitTypeDef *USART_InitStruct)
{
    UART_HandleTypeDef *huart = compat_uart_handle(USARTx);
    if (huart == NULL)
    {
        return;
    }

    huart->Instance = USARTx;
    huart->Init = *USART_InitStruct;
    if (HAL_UART_Init(huart) != HAL_OK)
    {
        while (1)
        {
        }
    }
}

static inline void USART_HalfDuplexCmd(USART_TypeDef *USARTx, FunctionalState NewState)
{
    if (NewState == ENABLE)
    {
        SET_BIT(USARTx->CR3, USART_CR3_HDSEL);
    }
    else
    {
        CLEAR_BIT(USARTx->CR3, USART_CR3_HDSEL);
    }
}

static inline void USART_ITConfig(USART_TypeDef *USARTx, uint32_t USART_IT, FunctionalState NewState)
{
    if (USART_IT == USART_IT_RXNE)
    {
        if (NewState == ENABLE) { SET_BIT(USARTx->CR1, USART_CR1_RXNEIE); }
        else { CLEAR_BIT(USARTx->CR1, USART_CR1_RXNEIE); }
    }
    else if (USART_IT == USART_IT_TXE)
    {
        if (NewState == ENABLE) { SET_BIT(USARTx->CR1, USART_CR1_TXEIE); }
        else { CLEAR_BIT(USARTx->CR1, USART_CR1_TXEIE); }
    }
}

static inline void USART_Cmd(USART_TypeDef *USARTx, FunctionalState NewState)
{
    if (NewState == ENABLE)
    {
        SET_BIT(USARTx->CR1, USART_CR1_UE);
    }
    else
    {
        CLEAR_BIT(USARTx->CR1, USART_CR1_UE);
    }
}

static inline ITStatus USART_GetITStatus(USART_TypeDef *USARTx, uint32_t USART_IT)
{
    uint32_t flag = 0U;
    uint32_t source = 0U;

    if (USART_IT == USART_IT_RXNE)
    {
        flag = USART_SR_RXNE;
        source = USART_CR1_RXNEIE;
    }
    else if (USART_IT == USART_IT_TXE)
    {
        flag = USART_SR_TXE;
        source = USART_CR1_TXEIE;
    }

    return (((USARTx->SR & flag) != 0U) && ((USARTx->CR1 & source) != 0U)) ? SET : RESET;
}

static inline uint16_t USART_ReceiveData(USART_TypeDef *USARTx)
{
    return (uint16_t)(USARTx->DR & 0x01FFU);
}

static inline void USART_ClearITPendingBit(USART_TypeDef *USARTx, uint32_t USART_IT)
{
    (void)USARTx;
    (void)USART_IT;
}

#define SPI_BaudRatePrescaler_2   SPI_BAUDRATEPRESCALER_2
#define SPI_BaudRatePrescaler_4   SPI_BAUDRATEPRESCALER_4
#define SPI_BaudRatePrescaler_8   SPI_BAUDRATEPRESCALER_8
#define SPI_BaudRatePrescaler_16  SPI_BAUDRATEPRESCALER_16
#define SPI_BaudRatePrescaler_32  SPI_BAUDRATEPRESCALER_32
#define SPI_BaudRatePrescaler_64  SPI_BAUDRATEPRESCALER_64
#define SPI_BaudRatePrescaler_128 SPI_BAUDRATEPRESCALER_128
#define SPI_BaudRatePrescaler_256 SPI_BAUDRATEPRESCALER_256

#define SPI_Direction          Direction
#define SPI_Mode               Mode
#define SPI_DataSize           DataSize
#define SPI_CPOL               CLKPolarity
#define SPI_CPHA               CLKPhase
#define SPI_NSS                NSS
#define SPI_BaudRatePrescaler  BaudRatePrescaler
#define SPI_FirstBit           FirstBit
#define SPI_CRCPolynomial      CRCPolynomial

#define SPI_Direction_2Lines_FullDuplex SPI_DIRECTION_2LINES
#define SPI_Mode_Master                 SPI_MODE_MASTER
#define SPI_DataSize_8b                 SPI_DATASIZE_8BIT
#define SPI_CPOL_High                   SPI_POLARITY_HIGH
#define SPI_CPHA_2Edge                  SPI_PHASE_2EDGE
#define SPI_NSS_Soft                    SPI_NSS_SOFT
#define SPI_FirstBit_MSB                SPI_FIRSTBIT_MSB

#define SPI_I2S_FLAG_TXE SPI_FLAG_TXE
#define SPI_I2S_FLAG_RXNE SPI_FLAG_RXNE

extern SPI_HandleTypeDef hspi2;

static inline void SPI_Init(SPI_TypeDef *SPIx, SPI_InitTypeDef *SPI_InitStruct)
{
    if (SPIx != SPI2)
    {
        return;
    }

    hspi2.Instance = SPI2;
    hspi2.Init = *SPI_InitStruct;
    hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    if (HAL_SPI_Init(&hspi2) != HAL_OK)
    {
        while (1)
        {
        }
    }
}

static inline void SPI_Cmd(SPI_TypeDef *SPIx, FunctionalState NewState)
{
    if (SPIx != SPI2)
    {
        return;
    }

    if (NewState == ENABLE)
    {
        __HAL_SPI_ENABLE(&hspi2);
    }
    else
    {
        __HAL_SPI_DISABLE(&hspi2);
    }
}

static inline FlagStatus SPI_I2S_GetFlagStatus(SPI_TypeDef *SPIx, uint32_t SPI_I2S_FLAG)
{
    return ((SPIx->SR & SPI_I2S_FLAG) != 0U) ? SET : RESET;
}

static inline void SPI_I2S_SendData(SPI_TypeDef *SPIx, uint16_t Data)
{
    *(__IO uint8_t *)&SPIx->DR = (uint8_t)Data;
}

static inline uint16_t SPI_I2S_ReceiveData(SPI_TypeDef *SPIx)
{
    return (uint16_t)(SPIx->DR & 0x00FFU);
}

#endif
