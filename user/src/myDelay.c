/**
  ******************************************************************************
  * @file    myDelay.c
  * @author  Yurilt
  * @version V1.0.0
  * @date    30-October-2025
  * @brief   STM32标准库驱动源文件
  * @note    此文件包含STM32标准库的外设驱动实现
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 Yurilt.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "myDelay.h"
#include "stm32f10x_rcc.h"
/**
 * @brief  初始化延时函数
 */
void myDelayInit(void) {
    TIM_TimeBaseInitTypeDef TIM_Conf;
    TIM_Conf.TIM_Prescaler = 71;
    TIM_Conf.TIM_Period =  1000;
    TIM_Conf.TIM_ClockDivision = 0;
    TIM_Conf.TIM_CounterMode = TIM_CounterMode_Up;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    TIM_TimeBaseInit(TIM2,&TIM_Conf);
    TIM_Cmd(TIM2, ENABLE);
}
/**
 * @brief  延时函数
 * @param  ms  延时 毫秒
 */
void myDelay(uint32_t ms){
    for (uint32_t i = 0; i < ms; i++) {
        while (!TIM_GetFlagStatus(TIM2, TIM_FLAG_Update));
        TIM_ClearFlag(TIM2, TIM_FLAG_Update); 
    }
}