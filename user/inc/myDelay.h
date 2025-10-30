/**
 ******************************************************************************
 * @file    myDelay.h
 * @author  Yurilt
 * @version V1.0.0
 * @date    31-October-2025
 * @brief   STM32标准库头文件
 * @note    此文件包含STM32标准库的函数声明和宏定义
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

#ifndef __MY_DELAY__
#define __MY_DELAY__
/* Includes ------------------------------------------------------------------*/
#include "stm32f10x_tim.h"

#ifdef __cplusplus
extern "C" {
#endif

void myDelayInit(void);
void myDelay(uint32_t ms);

#ifdef __cplusplus
}
#endif
#endif