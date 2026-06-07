#include"stm32f10x_gpio.h"
#include"stm32f10x_usart.h"
#include"stm32f10x_rcc.h"

#ifndef __OLED_DUBG__
#define __OLED_DUBG__

#ifdef __cplusplus
extern "C" {
#endif

#define  DEBUG_USARTx                   USART1
#define  DEBUG_USART_CLK                RCC_APB2Periph_USART1
#define  DEBUG_USART_APBxClkCmd         RCC_APB2PeriphClockCmd
#define  DEBUG_USART_BAUDRATE           115200

// USART GPIO 引脚宏定义
#define  DEBUG_USART_GPIO_CLK           (RCC_APB2Periph_GPIOA)
#define  DEBUG_USART_GPIO_APBxClkCmd    RCC_APB2PeriphClockCmd

#define  DEBUG_USART_TX_GPIO_PORT       GPIOA
#define  DEBUG_USART_TX_GPIO_PIN        GPIO_Pin_9
#define  DEBUG_USART_RX_GPIO_PORT       GPIOA
#define  DEBUG_USART_RX_GPIO_PIN        GPIO_Pin_10

#define  DEBUG_USART_IRQ                USART1_IRQn
#define  DEBUG_USART_IRQHandler         USART1_IRQHandler

void USART_Config(void);

void Usart_SendByte( USART_TypeDef * pUSARTx, uint8_t ch);

void Usart_SendString( USART_TypeDef * pUSARTx,const char *str);

void dxprintf(const char *fmt, ...);

void dprint(const char *str);

uint8_t dscanf();

#ifdef __cplusplus
}
#endif

#endif