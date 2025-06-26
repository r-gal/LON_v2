#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

#include "GeneralConfig.h"
#include "SignalList.hpp"
#include "LonInterfacePhy.hpp"

#include "tim.h"

const uint16_t phyClkPin[] = {CLK_0_Pin
#if NO_OF_BASIC_CHANNELS > 1
 ,CLK_1_Pin
 #endif 
#if NO_OF_BASIC_CHANNELS > 2
 ,CLK_2_Pin
 #endif 
#if NO_OF_BASIC_CHANNELS > 3
 ,CLK_3_Pin
 #endif 
#if NO_OF_BASIC_CHANNELS > 4
 ,CLK_4_Pin
 #endif 
#if NO_OF_BASIC_CHANNELS > 5
 ,CLK_5_Pin 
#endif 
#if NO_OF_BASIC_CHANNELS > 6
 ,CLK_6_Pin
 #endif 
#if NO_OF_BASIC_CHANNELS > 7
 ,CLK_7_Pin
 #endif 
};

const uint16_t phyDataPin[]  = {DATA_0_Pin
#if NO_OF_BASIC_CHANNELS > 1
 ,DATA_1_Pin
 #endif 
#if NO_OF_BASIC_CHANNELS > 2 
,DATA_2_Pin
 #endif 
#if NO_OF_BASIC_CHANNELS > 3 
,DATA_3_Pin
 #endif 
#if NO_OF_BASIC_CHANNELS > 4
 ,DATA_4_Pin
 #endif 
#if NO_OF_BASIC_CHANNELS > 5
 ,DATA_5_Pin 
#endif 
#if NO_OF_BASIC_CHANNELS > 6 
,DATA_6_Pin 
#endif 
#if NO_OF_BASIC_CHANNELS > 7
 ,DATA_7_Pin
 #endif 
};
const GPIO_TypeDef* phyClkGPIO[] = {CLK_0_GPIO_Port
#if NO_OF_BASIC_CHANNELS > 1
 ,CLK_1_GPIO_Port
 #endif 
#if NO_OF_BASIC_CHANNELS > 2
 ,CLK_2_GPIO_Port
 #endif 
#if NO_OF_BASIC_CHANNELS > 3
 ,CLK_3_GPIO_Port 
#endif 
#if NO_OF_BASIC_CHANNELS > 4
 ,CLK_4_GPIO_Port 
#endif 
#if NO_OF_BASIC_CHANNELS > 5
 ,CLK_5_GPIO_Port
 #endif 
#if NO_OF_BASIC_CHANNELS > 6 
,CLK_6_GPIO_Port
 #endif 
#if NO_OF_BASIC_CHANNELS > 7 
,CLK_7_GPIO_Port
 #endif 
};
const GPIO_TypeDef* phyDataGPIO[]  = {DATA_0_GPIO_Port
#if NO_OF_BASIC_CHANNELS > 1 
,DATA_1_GPIO_Port
 #endif 
#if NO_OF_BASIC_CHANNELS > 2
 ,DATA_2_GPIO_Port
 #endif 
#if NO_OF_BASIC_CHANNELS > 3
 ,DATA_3_GPIO_Port 
#endif 
#if NO_OF_BASIC_CHANNELS > 4 
,DATA_4_GPIO_Port 
#endif 
#if NO_OF_BASIC_CHANNELS > 5 
,DATA_5_GPIO_Port
 #endif 
#if NO_OF_BASIC_CHANNELS > 6
 ,DATA_6_GPIO_Port
 #endif 
#if NO_OF_BASIC_CHANNELS > 7
 ,DATA_7_GPIO_Port
 #endif 
};

LonInterfaceTick_c lonInterfaceTick_sig; /* static signal */

LonInterfacePhy_c::LonInterfacePhy_c(uint8_t portNo_) : portNo(portNo_)
{
/*
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Pin = phyClkPin[portNo];
  HAL_GPIO_Init((GPIO_TypeDef*)phyClkGPIO[portNo], &GPIO_InitStruct);
  GPIO_InitStruct.Pin = phyDataPin[portNo];
  HAL_GPIO_Init((GPIO_TypeDef*)phyDataGPIO[portNo], &GPIO_InitStruct);
*/
}

void TimerIcCallback( struct __TIM_HandleTypeDef *htim )
{
  lonInterfaceTick_sig.SendISR();
}

void LonInterfacePhy_c::InitInterfaceTick(void)
{

 //LON_INTERFACE_TIMINIT();

 LON_INTERFACE_TIMER.PeriodElapsedCallback = TimerIcCallback;

 HAL_TIM_Base_Start_IT(&LON_INTERFACE_TIMER);
  

}

bool LonInterfacePhy_c::getClk(void)
{
  /* read IO input port */
  return HAL_GPIO_ReadPin((GPIO_TypeDef*)phyClkGPIO[portNo],phyClkPin[portNo]); 
}

 bool LonInterfacePhy_c::getDat(void)
{
  /* read IO input port */
  return HAL_GPIO_ReadPin( (GPIO_TypeDef*)phyDataGPIO[portNo],phyDataPin[portNo]); 
}


void LonInterfacePhy_c::setClkPhy(bool data)
{
  /* write IO input port */
  HAL_GPIO_WritePin((GPIO_TypeDef*)phyClkGPIO[portNo],phyClkPin[portNo],data?GPIO_PIN_SET:GPIO_PIN_RESET);
}

void LonInterfacePhy_c::setDatPhy(bool data)
{
  /* write IO input port */
  HAL_GPIO_WritePin( (GPIO_TypeDef*)phyDataGPIO[portNo],phyDataPin[portNo],data?GPIO_PIN_SET:GPIO_PIN_RESET);
}

