/**
 ******************************************************************************
 * @file    debug.h
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

#ifndef __DUBG__
#define __DUBG__

#define DEBUG_FLAG 0

#if DEBUG_FLAG
/* Includes ------------------------------------------------------------------*/
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"


#ifdef __cplusplus
extern "C" {
#endif

#define DEBUG_USARTx USART1
#define DEBUG_USART_CLK RCC_APB2Periph_USART1
#define DEBUG_USART_APBxClkCmd RCC_APB2PeriphClockCmd
#define DEBUG_USART_BAUDRATE 115200
// USART GPIO 引脚宏定义
#define DEBUG_USART_GPIO_CLK (RCC_APB2Periph_GPIOA)
#define DEBUG_USART_GPIO_APBxClkCmd RCC_APB2PeriphClockCmd
#define DEBUG_USART_TX_GPIO_PORT GPIOA
#define DEBUG_USART_TX_GPIO_PIN GPIO_Pin_9
#define DEBUG_USART_RX_GPIO_PORT GPIOA
#define DEBUG_USART_RX_GPIO_PIN GPIO_Pin_10
#define DEBUG_USART_IRQ USART1_IRQn
#define DEBUG_USART_IRQHandler USART1_IRQHandler

void USART_Config(void);
void Usart_SendByte(USART_TypeDef* pUSARTx, uint8_t ch);
void Usart_SendString(USART_TypeDef* pUSARTx, const char* str);
void __dprint(const char* str);
void dbgPrintf(const char* format, ...);
uint8_t dscanf();
#ifdef __cplusplus
}
#endif
#endif
#endif