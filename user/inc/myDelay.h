/**
  ******************************************************************************
  * @file    myDelay.h
  * @author  Yurilt
  * @version V1.0.0
  * @date    4-March-2025
  * @brief   这是自定义delay头文件。
  * @attention  使用前初始化
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
/** ********************************************************************************************************************************
  * @defgroup myDelay fuctions
  */
void myDelayInit(void);
void myDelay(uint32_t ms);
#ifdef __cplusplus
}
#endif
#endif