/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
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
     PA8   ------> RCC_MCO_1
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, ETH_RESET_Pin|SPI4_U1_Pin|Led1_Pin|Led2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, DATA_0_Pin|CLK_0_Pin|DATA_1_Pin|CLK_1_Pin
                          |DATA_2_Pin|CLK_2_Pin|DATA_3_Pin|CLK_3_Pin
                          |PWR_ACU_LOAD_Pin|PWR_ACU_ENA_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, SPI2_U2_Pin|SPI2_INT_Pin|CAN_U1_Pin|DATA_4_Pin
                          |CLK_4_Pin|DATA_5_Pin|CLK_5_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOG, Usart_U2_Pin|Usart_U1_Pin|DATA_6_Pin|CLK_6_Pin
                          |DATA_7_Pin|CLK_7_Pin|CAN_U2_Pin|SPI4_U2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : SPI4_INT_Pin */
  GPIO_InitStruct.Pin = SPI4_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(SPI4_INT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ETH_RESET_Pin SPI4_U1_Pin Led1_Pin Led2_Pin */
  GPIO_InitStruct.Pin = ETH_RESET_Pin|SPI4_U1_Pin|Led1_Pin|Led2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : DATA_0_Pin CLK_0_Pin DATA_1_Pin CLK_1_Pin
                           DATA_2_Pin CLK_2_Pin DATA_3_Pin CLK_3_Pin */
  GPIO_InitStruct.Pin = DATA_0_Pin|CLK_0_Pin|DATA_1_Pin|CLK_1_Pin
                          |DATA_2_Pin|CLK_2_Pin|DATA_3_Pin|CLK_3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI2_U1_Pin */
  GPIO_InitStruct.Pin = SPI2_U1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(SPI2_U1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : SPI2_U2_Pin SPI2_INT_Pin CAN_U1_Pin */
  GPIO_InitStruct.Pin = SPI2_U2_Pin|SPI2_INT_Pin|CAN_U1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_sense_Pin */
  GPIO_InitStruct.Pin = USB_sense_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USB_sense_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : Usart_U2_Pin Usart_U1_Pin CAN_U2_Pin SPI4_U2_Pin */
  GPIO_InitStruct.Pin = Usart_U2_Pin|Usart_U1_Pin|CAN_U2_Pin|SPI4_U2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pin : SD_DET_A_Pin */
  GPIO_InitStruct.Pin = SD_DET_A_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(SD_DET_A_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PA8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF0_MCO;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : DATA_4_Pin CLK_4_Pin DATA_5_Pin CLK_5_Pin */
  GPIO_InitStruct.Pin = DATA_4_Pin|CLK_4_Pin|DATA_5_Pin|CLK_5_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : DATA_6_Pin CLK_6_Pin DATA_7_Pin CLK_7_Pin */
  GPIO_InitStruct.Pin = DATA_6_Pin|CLK_6_Pin|DATA_7_Pin|CLK_7_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pins : Switch1_Pin Switch2_Pin */
  GPIO_InitStruct.Pin = Switch1_Pin|Switch2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PWR_ACU_LOAD_Pin PWR_ACU_ENA_Pin */
  GPIO_InitStruct.Pin = PWR_ACU_LOAD_Pin|PWR_ACU_ENA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 12, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
