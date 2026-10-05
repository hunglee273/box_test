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
#include "stm32h7xx_hal.h"

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

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ENA3_Pin GPIO_PIN_2
#define ENA3_GPIO_Port GPIOE
#define OP_LM_SW1_Pin GPIO_PIN_13
#define OP_LM_SW1_GPIO_Port GPIOC
#define OP_LM_SW1_EXTI_IRQn EXTI15_10_IRQn
#define DIR3_Pin GPIO_PIN_1
#define DIR3_GPIO_Port GPIOF
#define DIR_L_XL_Pin GPIO_PIN_2
#define DIR_L_XL_GPIO_Port GPIOF
#define DIR_R_XL_Pin GPIO_PIN_3
#define DIR_R_XL_GPIO_Port GPIOF
#define ENA1_Pin GPIO_PIN_4
#define ENA1_GPIO_Port GPIOF
#define DIR1_Pin GPIO_PIN_5
#define DIR1_GPIO_Port GPIOF
#define ENA2_Pin GPIO_PIN_6
#define ENA2_GPIO_Port GPIOF
#define DIR2_Pin GPIO_PIN_7
#define DIR2_GPIO_Port GPIOF
#define LM_SW_END_H1_Pin GPIO_PIN_2
#define LM_SW_END_H1_GPIO_Port GPIOC
#define LM_SW_END_H1_EXTI_IRQn EXTI2_IRQn
#define CL_LM_SW1_Pin GPIO_PIN_3
#define CL_LM_SW1_GPIO_Port GPIOA
#define CL_LM_SW1_EXTI_IRQn EXTI3_IRQn
#define CL_LM_SW2_Pin GPIO_PIN_0
#define CL_LM_SW2_GPIO_Port GPIOB
#define CL_LM_SW2_EXTI_IRQn EXTI0_IRQn
#define OP_LM_SW2_Pin GPIO_PIN_12
#define OP_LM_SW2_GPIO_Port GPIOF
#define OP_LM_SW2_EXTI_IRQn EXTI15_10_IRQn
#define LM_SW1_H_Pin GPIO_PIN_14
#define LM_SW1_H_GPIO_Port GPIOF
#define LM_SW1_H_EXTI_IRQn EXTI15_10_IRQn
#define LM_SW2_V_Pin GPIO_PIN_15
#define LM_SW2_V_GPIO_Port GPIOF
#define LM_SW2_V_EXTI_IRQn EXTI15_10_IRQn
#define CT_LM_SW1_Pin GPIO_PIN_1
#define CT_LM_SW1_GPIO_Port GPIOG
#define CT_LM_SW1_EXTI_IRQn EXTI1_IRQn
#define LM_SW_END_H2_Pin GPIO_PIN_8
#define LM_SW_END_H2_GPIO_Port GPIOE
#define LM_SW_END_H2_EXTI_IRQn EXTI9_5_IRQn
#define STLINK_RX_Pin GPIO_PIN_8
#define STLINK_RX_GPIO_Port GPIOD
#define STLINK_TX_Pin GPIO_PIN_9
#define STLINK_TX_GPIO_Port GPIOD
#define PWM_SV_Pin GPIO_PIN_12
#define PWM_SV_GPIO_Port GPIOD
#define ALM1_Pin GPIO_PIN_5
#define ALM1_GPIO_Port GPIOG
#define ALM2_Pin GPIO_PIN_6
#define ALM2_GPIO_Port GPIOG
#define ALM3_Pin GPIO_PIN_7
#define ALM3_GPIO_Port GPIOG
#define LM_SW1_V_Pin GPIO_PIN_11
#define LM_SW1_V_GPIO_Port GPIOC
#define LM_SW1_V_EXTI_IRQn EXTI15_10_IRQn
#define LM_SW2_H_Pin GPIO_PIN_9
#define LM_SW2_H_GPIO_Port GPIOG
#define LM_SW2_H_EXTI_IRQn EXTI9_5_IRQn

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
