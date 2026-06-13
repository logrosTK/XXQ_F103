/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define PC13_SERVO0_Pin GPIO_PIN_13
#define PC13_SERVO0_GPIO_Port GPIOC
#define PC14_SERVO1_Pin GPIO_PIN_14
#define PC14_SERVO1_GPIO_Port GPIOC
#define PC15_SERVO2_Pin GPIO_PIN_15
#define PC15_SERVO2_GPIO_Port GPIOC
#define PC0_SERVO3_Pin GPIO_PIN_0
#define PC0_SERVO3_GPIO_Port GPIOC
#define PC1_SERVO4_Pin GPIO_PIN_1
#define PC1_SERVO4_GPIO_Port GPIOC
#define PC2_SERVO5_Pin GPIO_PIN_2
#define PC2_SERVO5_GPIO_Port GPIOC
#define PA4_SDA_Pin GPIO_PIN_4
#define PA4_SDA_GPIO_Port GPIOA
#define PA5_SCL_Pin GPIO_PIN_5
#define PA5_SCL_GPIO_Port GPIOA
#define PC4_LED_Pin GPIO_PIN_4
#define PC4_LED_GPIO_Port GPIOC
#define PC5_BEEP_Pin GPIO_PIN_5
#define PC5_BEEP_GPIO_Port GPIOC
#define PB2_KEY_Pin GPIO_PIN_2
#define PB2_KEY_GPIO_Port GPIOB
#define PA12_PS3_Pin GPIO_PIN_12
#define PA12_PS3_GPIO_Port GPIOA
#define PC10_PS0_Pin GPIO_PIN_10
#define PC10_PS0_GPIO_Port GPIOC
#define PC11_PS1_Pin GPIO_PIN_11
#define PC11_PS1_GPIO_Port GPIOC
#define PD2_PS2_Pin GPIO_PIN_2
#define PD2_PS2_GPIO_Port GPIOD
#define PB4_X4_Pin GPIO_PIN_4
#define PB4_X4_GPIO_Port GPIOB
#define PB5_X3_Pin GPIO_PIN_5
#define PB5_X3_GPIO_Port GPIOB
#define PB8_X1_Pin GPIO_PIN_8
#define PB8_X1_GPIO_Port GPIOB
#define PB9_X2_Pin GPIO_PIN_9
#define PB9_X2_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
