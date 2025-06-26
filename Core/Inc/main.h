/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define TERMINAL_PHY_UART huart1
#define LON_INTERFACE_FREQ 250
#define LON_INTERFACE_TIMER htim3
#define configTOTAL_CCM_HEAP_SIZE 100000
#define LON_INTERFACE_TIMINIT MX_TIM3_Init
#define configRUNTIME_TIMER htim2
#define SD_CARD_HSD hsd1
#define SPI4_INT_Pin GPIO_PIN_3
#define SPI4_INT_GPIO_Port GPIOE
#define V_main_Pin GPIO_PIN_6
#define V_main_GPIO_Port GPIOF
#define V_acu_Pin GPIO_PIN_7
#define V_acu_GPIO_Port GPIOF
#define C_ext_B_Pin GPIO_PIN_8
#define C_ext_B_GPIO_Port GPIOF
#define C_ext_BF9_Pin GPIO_PIN_9
#define C_ext_BF9_GPIO_Port GPIOF
#define C_main_Pin GPIO_PIN_10
#define C_main_GPIO_Port GPIOF
#define C_core_Pin GPIO_PIN_0
#define C_core_GPIO_Port GPIOC
#define C_port_4_Pin GPIO_PIN_0
#define C_port_4_GPIO_Port GPIOA
#define C_port_5_Pin GPIO_PIN_3
#define C_port_5_GPIO_Port GPIOA
#define C_port_6_Pin GPIO_PIN_4
#define C_port_6_GPIO_Port GPIOA
#define C_port_7_Pin GPIO_PIN_5
#define C_port_7_GPIO_Port GPIOA
#define C_port_0_Pin GPIO_PIN_6
#define C_port_0_GPIO_Port GPIOA
#define C_port_1_Pin GPIO_PIN_0
#define C_port_1_GPIO_Port GPIOB
#define C_port_2_Pin GPIO_PIN_1
#define C_port_2_GPIO_Port GPIOB
#define ETH_RESET_Pin GPIO_PIN_2
#define ETH_RESET_GPIO_Port GPIOB
#define C_port_3_Pin GPIO_PIN_11
#define C_port_3_GPIO_Port GPIOF
#define DATA_0_Pin GPIO_PIN_8
#define DATA_0_GPIO_Port GPIOE
#define CLK_0_Pin GPIO_PIN_9
#define CLK_0_GPIO_Port GPIOE
#define DATA_1_Pin GPIO_PIN_10
#define DATA_1_GPIO_Port GPIOE
#define CLK_1_Pin GPIO_PIN_11
#define CLK_1_GPIO_Port GPIOE
#define DATA_2_Pin GPIO_PIN_12
#define DATA_2_GPIO_Port GPIOE
#define CLK_2_Pin GPIO_PIN_13
#define CLK_2_GPIO_Port GPIOE
#define DATA_3_Pin GPIO_PIN_14
#define DATA_3_GPIO_Port GPIOE
#define CLK_3_Pin GPIO_PIN_15
#define CLK_3_GPIO_Port GPIOE
#define SPI2_U1_Pin GPIO_PIN_8
#define SPI2_U1_GPIO_Port GPIOD
#define SPI2_U2_Pin GPIO_PIN_9
#define SPI2_U2_GPIO_Port GPIOD
#define SPI2_INT_Pin GPIO_PIN_10
#define SPI2_INT_GPIO_Port GPIOD
#define USB_sense_Pin GPIO_PIN_6
#define USB_sense_GPIO_Port GPIOG
#define Usart_U2_Pin GPIO_PIN_7
#define Usart_U2_GPIO_Port GPIOG
#define Usart_U1_Pin GPIO_PIN_8
#define Usart_U1_GPIO_Port GPIOG
#define SD_DET_A_Pin GPIO_PIN_6
#define SD_DET_A_GPIO_Port GPIOC
#define SD_DET_A_EXTI_IRQn EXTI9_5_IRQn
#define CAN_U1_Pin GPIO_PIN_3
#define CAN_U1_GPIO_Port GPIOD
#define DATA_4_Pin GPIO_PIN_4
#define DATA_4_GPIO_Port GPIOD
#define CLK_4_Pin GPIO_PIN_5
#define CLK_4_GPIO_Port GPIOD
#define DATA_5_Pin GPIO_PIN_6
#define DATA_5_GPIO_Port GPIOD
#define CLK_5_Pin GPIO_PIN_7
#define CLK_5_GPIO_Port GPIOD
#define DATA_6_Pin GPIO_PIN_9
#define DATA_6_GPIO_Port GPIOG
#define CLK_6_Pin GPIO_PIN_10
#define CLK_6_GPIO_Port GPIOG
#define DATA_7_Pin GPIO_PIN_11
#define DATA_7_GPIO_Port GPIOG
#define CLK_7_Pin GPIO_PIN_12
#define CLK_7_GPIO_Port GPIOG
#define CAN_U2_Pin GPIO_PIN_13
#define CAN_U2_GPIO_Port GPIOG
#define SPI4_U2_Pin GPIO_PIN_14
#define SPI4_U2_GPIO_Port GPIOG
#define SPI4_U1_Pin GPIO_PIN_5
#define SPI4_U1_GPIO_Port GPIOB
#define Switch1_Pin GPIO_PIN_6
#define Switch1_GPIO_Port GPIOB
#define Switch2_Pin GPIO_PIN_7
#define Switch2_GPIO_Port GPIOB
#define Led1_Pin GPIO_PIN_8
#define Led1_GPIO_Port GPIOB
#define Led2_Pin GPIO_PIN_9
#define Led2_GPIO_Port GPIOB
#define PWR_ACU_LOAD_Pin GPIO_PIN_0
#define PWR_ACU_LOAD_GPIO_Port GPIOE
#define PWR_ACU_ENA_Pin GPIO_PIN_1
#define PWR_ACU_ENA_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
