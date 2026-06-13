/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, PC13_SERVO0_Pin|PC14_SERVO1_Pin|PC15_SERVO2_Pin|PC0_SERVO3_Pin
                          |PC1_SERVO4_Pin|PC2_SERVO5_Pin|PC4_LED_Pin|PC5_BEEP_Pin
                          |PC11_PS1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, PA4_SDA_Pin|PA5_SCL_Pin|PA12_PS3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(PD2_PS2_GPIO_Port, PD2_PS2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : PC13_SERVO0_Pin PC14_SERVO1_Pin PC15_SERVO2_Pin PC0_SERVO3_Pin
                           PC1_SERVO4_Pin PC2_SERVO5_Pin PC4_LED_Pin PC5_BEEP_Pin
                           PC11_PS1_Pin */
  GPIO_InitStruct.Pin = PC13_SERVO0_Pin|PC14_SERVO1_Pin|PC15_SERVO2_Pin|PC0_SERVO3_Pin
                          |PC1_SERVO4_Pin|PC2_SERVO5_Pin|PC4_LED_Pin|PC5_BEEP_Pin
                          |PC11_PS1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA4_SDA_Pin PA5_SCL_Pin PA12_PS3_Pin */
  GPIO_InitStruct.Pin = PA4_SDA_Pin|PA5_SCL_Pin|PA12_PS3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB2_KEY_Pin PB4_X4_Pin PB5_X3_Pin PB8_X1_Pin
                           PB9_X2_Pin */
  GPIO_InitStruct.Pin = PB2_KEY_Pin|PB4_X4_Pin|PB5_X3_Pin|PB8_X1_Pin
                          |PB9_X2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PC10_PS0_Pin */
  GPIO_InitStruct.Pin = PC10_PS0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(PC10_PS0_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PD2_PS2_Pin */
  GPIO_InitStruct.Pin = PD2_PS2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(PD2_PS2_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
