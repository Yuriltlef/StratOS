#include "user/libraries/test_log/inc/debug.hpp"
#include <stdarg.h>
#include <stdio.h>


void USART_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    // 打开串口GPIO的时钟
    DEBUG_USART_GPIO_APBxClkCmd(DEBUG_USART_GPIO_CLK, ENABLE);

    // 打开串口外设的时钟
    DEBUG_USART_APBxClkCmd(DEBUG_USART_CLK, ENABLE);

    // 将USART Tx的GPIO配置为推挽复用模式
    GPIO_InitStructure.GPIO_Pin = DEBUG_USART_TX_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DEBUG_USART_TX_GPIO_PORT, &GPIO_InitStructure);

    // 将USART Rx的GPIO配置为浮空输入模式
    GPIO_InitStructure.GPIO_Pin = DEBUG_USART_RX_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(DEBUG_USART_RX_GPIO_PORT, &GPIO_InitStructure);

    // 配置串口的工作参数
    // 配置波特率
    USART_InitStructure.USART_BaudRate = DEBUG_USART_BAUDRATE;
    // 配置 针数据字长
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    // 配置停止位
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    // 配置校验位
    USART_InitStructure.USART_Parity = USART_Parity_No ;
    // 配置硬件流控制
    USART_InitStructure.USART_HardwareFlowControl =
        USART_HardwareFlowControl_None;
    // 配置工作模式，收发一起
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    // 完成串口的初始化配置
    USART_Init(DEBUG_USARTx, &USART_InitStructure);

    // 使能串口
    USART_Cmd(DEBUG_USARTx, ENABLE);
}

void Usart_SendByte( USART_TypeDef * pUSARTx, uint8_t ch)
{
    /* 发送一个字节数据到USART */
    USART_SendData(pUSARTx,ch);

    /* 等待发送数据寄存器为空 */
    while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TXE) == RESET);
}

void Usart_SendString( USART_TypeDef * pUSARTx,const char *str)
{
    unsigned int k=0;
    do {
        Usart_SendByte( pUSARTx, *(str + k) );
        k++;
    } while (*(str + k)!='\0');

    /* 等待发送完成 */
    while (USART_GetFlagStatus(pUSARTx,USART_FLAG_TC)==RESET) {
    }
}

void dprint(const char *str){
    USART_Config();
    Usart_SendString(DEBUG_USARTx,str);
}

uint8_t dscanf(){
    USART_Config();
   return USART_ReceiveData( DEBUG_USARTx );
}

static void print_char(char c) {
    Usart_SendByte(DEBUG_USARTx, (uint8_t)c);
}

static void print_str(const char *s) {
    while (*s) {
        print_char(*s++);
    }
}

static void print_hex(uint32_t val, int upper_case) {
    char buf[8];
    int i;
    for (i = 7; i >= 0; i--) {
        uint8_t nibble = (val >> (i * 4)) & 0xF;
        buf[7 - i] = (nibble < 10) ? ('0' + nibble) : ((upper_case ? 'A' : 'a') + (nibble - 10));
    }
    for (i = 0; i < 8; i++) {
        print_char(buf[i]);
    }
}

static void print_int(int32_t val) {
    if (val < 0) {
        print_char('-');
        val = -val;
    }
    // 递归或循环转换
    char buf[12];
    int i = 0;
    do {
        buf[i++] = '0' + (val % 10);
        val /= 10;
    } while (val > 0);
    while (i > 0) {
        print_char(buf[--i]);
    }
}

static void print_uint(uint32_t val) {
    char buf[12];
    int i = 0;
    do {
        buf[i++] = '0' + (val % 10);
        val /= 10;
    } while (val > 0);
    while (i > 0) {
        print_char(buf[--i]);
    }
}

void dxprintf(const char *fmt, ...) {
    static int initialized = 0;
    if (!initialized) {
        USART_Config();
        initialized = 1;
    }

    va_list args;
    va_start(args, fmt);
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 'd': {
                    int32_t val = va_arg(args, int32_t);
                    print_int(val);
                    break;
                }
                case 'u': {
                    uint32_t val = va_arg(args, uint32_t);
                    print_uint(val);
                    break;
                }
                case 'x':
                case 'X': {
                    uint32_t val = va_arg(args, uint32_t);
                    print_hex(val, (*fmt == 'X'));
                    break;
                }
                case 's': {
                    const char *s = va_arg(args, const char *);
                    if (s == NULL) s = "(null)";
                    print_str(s);
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    print_char(c);
                    break;
                }
                case '%':
                    print_char('%');
                    break;
                default:
                    print_char('%');
                    print_char(*fmt);
                    break;
            }
            fmt++;
        } else {
            print_char(*fmt++);
        }
    }
    va_end(args);
}