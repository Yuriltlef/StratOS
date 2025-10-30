/**
 ******************************************************************************
 * @file    st7786_spi.c
 * @author  Yurilt
 * @version V1.0.0
 * @date    31-October-2025
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
#include "stm32f10x_rcc.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_dma.h"
#include "st7786_spi.h"
#include "myDelay.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/**
 * @brief  用户通过更改此变量实现自定义配置
 */
St7789InitStruct St7789Init = {
    SPI1,
    0,
    GPIOA,
    RCC_APB2Periph_GPIOA,
    ST_SPI_DC,
    GPIO_Pin_12,
    ST_RES,
    ST_SPI_CS,
    60
};

/**
 * @brief  stm32通过spi发送单字节给st7789的函数
 * @note   注意在使用前初始化spi并连接好针脚
 * @param  bt  要传输的单个字节
 */
void st7789SpiSendByte(uint8_t bt){
    /* 切换MOSI为输出模式 */
    SPI_BiDirectionalLineConfig(St7789Init.SPIx, SPI_Direction_Tx);
    /* SPI通讯超时检测，默认尝试FFFFh次 */
    uint16_t SPITimeOut = 0xFFFF;
    /* 检测缓冲区状态，如果一直非空则终止发送 */
    while (SPI_I2S_GetFlagStatus(St7789Init.SPIx, SPI_I2S_FLAG_TXE) == RESET) {
        if (SPITimeOut-- == 0) {
#if DEBUG_FLAG
            /* only using in debug */
            dbgPrintf("spi txe timeout. Try again.\n");
#endif
            return;
        }
    }
    /* 片选 */
    GPIO_ResetBits(St7789Init.ST_CL_GPIOx, St7789Init.ST_CS_Pin);
    /* SPI发送数据 */
    SPI_I2S_SendData(St7789Init.SPIx, bt);
    /* 刷新计时器 */
    SPITimeOut = 0xFFFF;
    /* 检测SPI状态，如果一直忙终止发送 */
    while (SPI_I2S_GetFlagStatus(St7789Init.SPIx, SPI_I2S_FLAG_BSY) == SET) {
        if (SPITimeOut-- == 0) {
#if DEBUG_FLAG
            /* only using in debug */
            dbgPrintf("spi bsy timeout. Try again.\n");
#endif
            /* 终止SPI */
            GPIO_SetBits(St7789Init.ST_CL_GPIOx, St7789Init.ST_CS_Pin);
            return;
        }
    }
    /* 拉高片选，中止SPI */
    GPIO_SetBits(St7789Init.ST_CL_GPIOx, St7789Init.ST_CS_Pin);
#if DEBUG_FLAG
    /* only using in debug */
    //dbgPrint("数据发送成功\n");
#endif
}

/**
 * @brief  stm32通过spi(soft)发送字节块给st7789的函数
 * @note  可以考虑DMA发送减少cpu负担
 * @param  bts  数据数组
 * @param  size  数据长度（字节数）
 */
void st7789SpiSendBytes(uint8_t* bts, uint32_t size) {
    /* 切换MOSI为输出模式 */
    SPI_BiDirectionalLineConfig(St7789Init.SPIx, SPI_Direction_Tx);
    uint16_t SPITimeOut = 0xFFFF;
    /* 片选 */
    GPIO_ResetBits(St7789Init.ST_CL_GPIOx, St7789Init.ST_CS_Pin);
    /* SPI发送数据 */
    for (uint32_t i = 0; i < size; i++) {
    /* 检测缓冲区状态，如果一直非空则终止发送 */
        while (SPI_I2S_GetFlagStatus(St7789Init.SPIx, SPI_I2S_FLAG_TXE) == RESET) {  
            /* SPI通讯超时检测，默认尝试FFFFh次 */
            SPITimeOut = 0xFFFF;
            if (SPITimeOut-- == 0) {
#if DEBUG_FLAG
                /* only using in debug */
                dbgPrintf("spi txe timeout. Try again.\n");
#endif
                GPIO_SetBits(St7789Init.ST_CL_GPIOx, St7789Init.ST_CS_Pin);
                return;
            }
        }
        SPI_I2S_SendData(St7789Init.SPIx, bts[i]);
    }
    /* 刷新计时器，等待最后一个字节完成 */
    SPITimeOut = 0xFFFF;
    /* 检测SPI状态，如果一直忙终止发送 */
    while (SPI_I2S_GetFlagStatus(St7789Init.SPIx, SPI_I2S_FLAG_BSY) == SET) {
        if (SPITimeOut-- == 0) {
#if DEBUG_FLAG
            /* only using in debug */
            dbgPrintf("spi bsy timeout. Try again.\n");
#endif
            /* 终止SPI */
            GPIO_SetBits(St7789Init.ST_CL_GPIOx, St7789Init.ST_CS_Pin);
            return;
        }
    }
    /* 拉高片选，中止SPI */
    GPIO_SetBits(St7789Init.ST_CL_GPIOx, St7789Init.ST_CS_Pin);
#if DEBUG_FLAG
    /* only using in debug */
    //dbgPrint("数据发送成功\n");
#endif
}

/**
 * @brief  stm32通过spi发送1byte数据给st7789的函数
 * @param  dat  数据
 */
void st7789SpiSendData(uint8_t dat) {
    /* 拉高SDA */
    GPIO_SetBits(St7789Init.ST_CL_GPIOx, St7789Init.ST_DC_Pin);
    /* 发送 */
    st7789SpiSendByte(dat);
}

/* 标志位：DMA_busy */
volatile static uint8_t DMA_busy = 0;

/**
 * @brief  stm32通过spi dma发送字节块给st7789的函数
 * @param  bts  数据数组
 * @param  size  数据长度（字节数）
 * @note  默认的长数据块发送方法，DMA配置：8位，单次传输。
 */
void st7789SpiDMASendDatas(uint8_t* bts, uint32_t size) {
    /* 超时检测 */
    uint32_t OutTime = 0xFFFFFFFF;
    /* 等待中断复位标志位 */
    while (DMA_busy == 1) {
        if (OutTime-- == 0) {
#if DEBUG_FLAG
            /* only using in debug */
            dbgPrintf("dma bsy timeout. Try again.\n");
#endif      
            return;
        } 
    }
    //while (DMA_busy == 1)
    /* 设置标志位 */
    DMA_busy = 1;
#if DEBUG_FLAG
    /* only using in debug */
    dbgPrintf("DMA size: %X\nDMA ME from: %X\n", size, (uint32_t)bts);
#endif 
    /* 初始化ram buffer地址和size */
    ST_SPI_DMA->CNDTR = size;
    ST_SPI_DMA->CMAR = (uint32_t)bts;
    /* 片选 */
    GPIO_ResetBits(St7789Init.ST_CL_GPIOx, St7789Init.ST_CS_Pin);
    /* 拉高SDA */
    GPIO_SetBits(St7789Init.ST_CL_GPIOx, St7789Init.ST_DC_Pin);
    /* 使能SPI DMA发送 */
    SPI_I2S_DMACmd(St7789Init.SPIx, SPI_I2S_DMAReq_Tx, ENABLE);
    /* 使能DMA通道 */
    DMA_Cmd(ST_SPI_DMA, ENABLE);
    /* 阻塞一下 */
    while (DMA_busy == 1);
}

/**
 * @brief  DMA发送中断服务函数
 */
void DMA1_Channel3_IRQHandler(void) {
    /* 检查中断标志 */
    if (DMA_GetITStatus(DMA1_IT_TC3)) {
        /* 清除中断标志 */
        DMA_ClearITPendingBit(DMA1_IT_TC3);
        /* 失能DMA通道 */
        DMA_Cmd(ST_SPI_DMA, DISABLE);
        /* 失能SPI DMA发送 */
        SPI_I2S_DMACmd(St7789Init.SPIx, SPI_I2S_DMAReq_Tx, DISABLE);
        /* 释放片选，中止SPI */
        GPIO_SetBits(St7789Init.ST_CL_GPIOx, St7789Init.ST_CS_Pin);
        /* 复位DMA_busy */
        DMA_busy = 0;
#if DEBUG_FLAG
            /* only using in debug */
        dbgPrintf("DMA transfer done.\n");
#endif 
    }
}

/**
 * @brief  stm32通过spi发送字节块给st7789的函数
 * @note  大块可以考虑DMA发送减少cpu负担
 * @param  dats  数据数组
 * @param  size  数据长度（字节数）
 */
void st7789SpiSendDatas(uint8_t* dats, uint32_t size) {
    /* 拉高SDA */
    GPIO_SetBits(St7789Init.ST_CL_GPIOx, St7789Init.ST_DC_Pin);
    /* 发送 */
    st7789SpiSendBytes(dats, size);
}

/**
 * @brief  stm32通过spi 发送命令给st7789的函数
 * @param  cmd  命令
 */
void st7789SpiSendCmd(St7786SpiCmd cmd) {
    /* 拉低SDA */
    GPIO_ResetBits(St7789Init.ST_CL_GPIOx, St7789Init.ST_DC_Pin);
    /* 发送 */
    st7789SpiSendByte(cmd);
}

/**
 * @brief  stm32通过spi接受st7789数据的函数
 * @param  ptr  数据写入的地址
 */
void st7789SpiRecvByte(uint32_t* ptr) {
    /* 切换MOSI为输入模式 */
    SPI_BiDirectionalLineConfig(St7789Init.SPIx, SPI_Direction_Rx);
    /* SPI通讯超时检测，默认尝试0xFFFFh次 */
    uint16_t SPITimeOut = 0xFFFF;
    /* 检测接收缓冲区状态 */
    while (SPI_I2S_GetFlagStatus(St7789Init.SPIx, SPI_I2S_FLAG_RXNE) == RESET) {
        if (SPITimeOut-- == 0){
#if DEBUG_FLAG
            /* only using in debug */
            dbgPrintf("spi is busy. Try again.");
#endif
            /* 超时退出 */
            return;
                    }
    }
    /* 将数据写入目标地址 */
    *ptr = SPI_I2S_ReceiveData(St7789Init.SPIx);
#if DEBUG_FLAG
    /* only using in debug */
    dbgPrintf("写入成功\n");
#endif
}

/**
 * @brief  stm32初始化SPI
 * @param  stInitStruct  初始化spi和针脚的结构
 * @note  只能选择SPI1,2 参数SPI_REMAP为针脚选择。默认为st文档默认
          SPI1_REMAP=0的配置(SPI2只能为0！)
          SPI1________________________________________________
           |0: 没有重映像(NSS/PA4, SCK/PA5, MISO/PA6, MOSI/PA7)| 
           |1: 重映像(NSS/PA15, SCK/PB3, MISO/PB3, MOSI/PB5)   |
 */
void st7789Init(St7789InitStruct* stInitStruct) {
#if DEBUG_FLAG
    /* only using in debug */
    dbgPrintf("Initializing st7789...\n");
#endif
    /* 初始化DMA结构体 */
    DMA_InitTypeDef DMA_InitStru;
    /* 初始化spi结构体 */
    SPI_InitTypeDef SPI_InitStr;
    /* 初始化GPIO结构体 */
    GPIO_InitTypeDef GPIO_InitStr;
    /* 初始化NVIC结构体 */
    NVIC_InitTypeDef NVIC_InitStr;
    /* 初始化GPIO变量 */
    GPIO_TypeDef* SPI_GPIOx;
    /* 初始化RCC */
    uint32_t SPI_RCC;
    /* 初始化SCK(SCL) MOSI(SDA) */
    uint16_t cousSCKPin, cousMOSIPin;
    /* 参数检测 */
    if (stInitStruct->SPIx != SPI1 && stInitStruct->SPI_REMAP != 0) {
#if DEBUG_FLAG
    /* only using in debug */
    dbgPrintf("valid SPIx and SPI_REMAP\n");
#endif
        return;
    }
    /* choose SPI1 */
    if (stInitStruct->SPIx == SPI1) {
        /* 重映射后 */
        if (stInitStruct->SPI_REMAP != 0) {
            /* 绑定到GPIOB */
            SPI_GPIOx = GPIOB;
            /* 绑定到PB3 */
            cousSCKPin = GPIO_Pin_3;
            /* 绑定到PB5 */
            cousMOSIPin = GPIO_Pin_5;
            /* 绑定到SPI1RCC */
            SPI_RCC = RCC_APB2Periph_SPI1;
#if DEBUG_FLAG
            /* only using in debug */
            dbgPrintf("choose SPI1 and remap...\n");
            dbgPrintf("choose GPIOB to SPI1, GPIOA to cs...\n");
#endif
        }
        else {
            /* 绑定到GPIOA */
            SPI_GPIOx = GPIOA;
            /* 绑定到PA5 */
            cousSCKPin = ST_SPI_SCL;
            /* 绑定到PA7 */
            cousMOSIPin = ST_SPI_SDA;
            /* 绑定到SPI1RCC */
            SPI_RCC = RCC_APB2Periph_SPI1;
#if DEBUG_FLAG
            /* only using in debug */
            dbgPrintf("choose SPI1 no remap...\n");
            dbgPrintf("choose GPIOA...\n");
#endif
        }
    }
    /* choose SPI2 */
    else if (stInitStruct->SPIx == SPI2) {
        /* 绑定到GPIOB */
        SPI_GPIOx = GPIOB;
        /* 绑定到PB13 */
        cousSCKPin = GPIO_Pin_13;
        /* 绑定到PB15 */
        cousMOSIPin = GPIO_Pin_15;            
        /* 绑定到SPI2RCC */
        SPI_RCC = RCC_APB1Periph_SPI2;
#if DEBUG_FLAG
        /* only using in debug */
        dbgPrintf("choose SPI2...\n");
        dbgPrintf("choose GPIOB...\n");
#endif
    }
    // /* choose SPI3 */
    // else if (stInitStruct->SPIx == SPI3) {
    //     /* 绑定到GPIOB */
    //     SPI_GPIOx = GPIOB;
    //     /* 绑定到PB13 */
    //     cousSCKPin = GPIO_Pin_13;
    //     /* 绑定到PB12 */
    //     cousNSS = GPIO_Pin_12;
    //     /* 绑定到PB15 */
    //     cousMOSIPin = GPIO_Pin_15;
    // }
    /* 使能SPI时钟 */
    RCC_APB2PeriphClockCmd(SPI_RCC, ENABLE);
    /* 使能GPIOx时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB
                            | stInitStruct->ST_GPIOx_RCC, ENABLE);
    /* 开启DMA时钟 */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    /* 配置SPI的 CS引脚，普通IO即可 */
    GPIO_InitStr.GPIO_Pin = stInitStruct->ST_CS_Pin;
    GPIO_InitStr.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStr.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(stInitStruct->ST_CL_GPIOx, &GPIO_InitStr);
    /* 停止SPI */
    GPIO_SetBits(stInitStruct->ST_CL_GPIOx, stInitStruct->ST_CS_Pin);
#if DEBUG_FLAG
    /* only using in debug */
    dbgPrintf("init GPIO_pinx... \n spi -> %X\n cl -> %X\n",SPI_GPIOx, stInitStruct->ST_CL_GPIOx);
    dbgPrintf("The DC pin is %X\n RES pin is %X\n BLK pin is %X\n CS pin is %X\n SDA pin is %X\n", 
        stInitStruct->ST_DC_Pin,
        stInitStruct->ST_RES_Pin,
        stInitStruct->ST_BLK_Pin,
        stInitStruct->ST_CS_Pin,
        cousMOSIPin
    );
#endif
    /* 配置LCD的 DC引脚*/
    GPIO_InitStr.GPIO_Pin = stInitStruct->ST_DC_Pin;
    GPIO_Init(stInitStruct->ST_CL_GPIOx, &GPIO_InitStr); 
    /* 配置LCD的 RES引脚*/
    GPIO_InitStr.GPIO_Pin = stInitStruct->ST_RES_Pin;
    GPIO_Init(stInitStruct->ST_CL_GPIOx, &GPIO_InitStr);
    /* 配置LCD的 BLK引脚*/
    GPIO_InitStr.GPIO_Pin = stInitStruct->ST_BLK_Pin;
    GPIO_Init(stInitStruct->ST_CL_GPIOx, &GPIO_InitStr);
    /* 配置SPI的 SCK引脚*/
    GPIO_InitStr.GPIO_Pin = cousSCKPin;
    GPIO_InitStr.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(SPI_GPIOx, &GPIO_InitStr);
    /* 配置SPI的 MOSI引脚*/
    GPIO_InitStr.GPIO_Pin = cousMOSIPin;
    GPIO_Init(SPI_GPIOx, &GPIO_InitStr);
    /* 源数据地址(临时) */
    DMA_InitStru.DMA_MemoryBaseAddr = (uint32_t)0;
    /* 外设地址:SPI1 DR */
    DMA_InitStru.DMA_PeripheralBaseAddr = (uint32_t)&(St7789Init.SPIx->DR);
    /* 大小 */
    DMA_InitStru.DMA_MemoryDataSize = 0;
    /* 传输方向sram -> 外设（SPI1） */
    DMA_InitStru.DMA_DIR = DMA_DIR_PeripheralDST;
    /* 外设地址增量模式:不递增 */
    DMA_InitStru.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    /* 存储器地址增量模式:递增 */
    DMA_InitStru.DMA_MemoryInc = DMA_MemoryInc_Enable;
    /* 外设数据宽度:1 byte */
    DMA_InitStru.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    /* SRAM数据宽度:1 byte */
    DMA_InitStru.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    /* DMA模式 */
    DMA_InitStru.DMA_Mode = DMA_Mode_Normal;
    /* DMA软件设置通道的中断优先级 */
    DMA_InitStru.DMA_Priority = DMA_Priority_Medium;
    /* DMA M2M禁用 */
    DMA_InitStru.DMA_M2M = DMA_M2M_Disable;
    /* 配置DMA通道 */
    DMA_Init(ST_SPI_DMA, &DMA_InitStru);
    /* 失能DMA通道 */
    DMA_Cmd(ST_SPI_DMA, DISABLE);
    /* 设置中断源 */
    NVIC_InitStr.NVIC_IRQChannel = DMA1_Channel3_IRQn;
    /* 设置抢占优先级 */
    NVIC_InitStr.NVIC_IRQChannelPreemptionPriority = 1;
    /* 设置子优先级 */
    NVIC_InitStr.NVIC_IRQChannelSubPriority = 1;
    /* 使能中断通道 */
    NVIC_InitStr.NVIC_IRQChannelCmd = ENABLE;
    /* 配置中断源DMA_Channl3_IQRn */
    NVIC_Init(&NVIC_InitStr);
    /* 配置DMA1启用传输完成中断 */
    DMA_ITConfig(ST_SPI_DMA, DMA_IT_TC, ENABLE);
    /* 方向为单线发送 */
    SPI_InitStr.SPI_Direction = SPI_Direction_1Line_Tx;
    /* 主机模式 */
    SPI_InitStr.SPI_Mode = SPI_Mode_Master;
    /* 数据帧长度为8位 */
    SPI_InitStr.SPI_DataSize = SPI_DataSize_8b;
    /* 设置空闲SCL低电平 */
    SPI_InitStr.SPI_CPOL = SPI_CPOL_Low;
    /* 设置奇数边采样 */
    SPI_InitStr.SPI_CPHA = SPI_CPHA_1Edge;
    /* 设置软件控制片选 */
    SPI_InitStr.SPI_NSS = SPI_NSS_Soft;
    /* 2分频 */
    SPI_InitStr.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
    /* 高位在前 */
    SPI_InitStr.SPI_FirstBit = SPI_FirstBit_MSB;
    /* CRC校验无效 */
    SPI_InitStr.SPI_CRCPolynomial = 7;
    /* 初始化SPIx */
    SPI_Init(stInitStruct->SPIx, &SPI_InitStr);
    /* 失能SPI DMA发送 */
    SPI_I2S_DMACmd(St7789Init.SPIx, SPI_I2S_DMAReq_Tx, DISABLE);
    /* 使能SPI */
    SPI_Cmd(stInitStruct->SPIx, ENABLE);
    /* 硬件重置SPI */
    st7789HardReset();
    /* 解除睡眠 */
    st7789SpiSendCmd(SLPOUT);
    myDelay(20);
    /* 设置颜色16位 */
    st7789SpiSendCmd(COLMOD);
    st7789SpiSendData(0x66);
    /* 页/列/rgb/行顺序 */
    st7789SpiSendCmd(MADCTL);
    st7789SpiSendData(0x00);
    /* 设置列地址 240 */
    st7789SpiSendCmd(CASET);
    st7789SpiSendData(0x00);
    st7789SpiSendData(0x00);
    st7789SpiSendData(0x00);
    st7789SpiSendData(0xEF);
    /* 设置行 280 */
    st7789SpiSendCmd(RASET);
    st7789SpiSendData(0x00);
    st7789SpiSendData(ST_R_OFFSET);
    st7789SpiSendData(0x01);
    st7789SpiSendData(0x2C);
    /* 正常模式下的帧率：75 */
    st7789SpiSendCmd(0xC6);
    st7789SpiSendData(0x09);
    /* 清屏 */
    st7789Clear();
    /* 开启背光 */
    st7789OnBg();
    /* 开启显示 */
    st7789SpiSendCmd(DISPON);
    myDelay(10);
}

/**
 * @brief  LCD显示点阵字体
 * @note  坐标为相对屏幕左上角，行间距和字间距由显示坐标控制
 * @param  ch  单个字符
 * @param  wx  横坐标
 * @param  hy  纵坐标
 * @param  fg  前景色
 * @param  bg  背景色
 * @param  wide  字体大小, 每个方形字符分辨率宽
 */
void st7789SpiShowChar(
    const unsigned char     ch, 
    uint16_t                wx,
    uint16_t                hy, 
    St7786Spi4Color18*      fg,
    St7786Spi4Color18*      bg,
    St7786SpiFontSize       siz
) {
    /* 重映射颜色 */
    uint8_t frgb[3];
    uint8_t brgb[3];
    st7789ColorMap(frgb, fg);
    st7789ColorMap(brgb, bg);
    /*    宽： (uint8_t)((siz >> 8) & 0xFF)
          高： (uint8_t)(siz & 0xFF) */
    /* buffer 大小 */
    uint32_t size =  (uint8_t)((siz >> 8) & 0xFF) * (uint8_t)(siz & 0xFF) * 3;
    /* 初始化缓冲区 */
    uint8_t buffer[size];
    /* 选择点阵字体 */
    switch (siz) {
//     case MINI:
// #if DEBUG_FLAG
//         /* only using in debug */
//         dbgPrintf("choose 16x16 %s\n", ch);
// #endif
//         /* 遍历点阵算出rgb */
//         /* (size / 3) / 8 为每个字符位图占用的字节 */
//         for (uint32_t i = 0; i < (size / 3) / 8 ; i++) {
//             /* ascii偏移量：32 */
//             /* 双循环遍历位 */
//             for (uint8_t j = 7; j >= 0; j--) {
//                 /* 按位检测,从高位开始 */
//                 if ((StAsciiFont16[ch - 32][i] >> j) & 1) {
//                     buffer[((i + 1) * 24 - 3 * j) - 3] = frgb[0];
//                     buffer[((i + 1) * 24 - 3 * j) - 2] = frgb[1];
//                     buffer[((i + 1) * 24 - 3 * j) - 1] = frgb[2];
//                 }
//                 else {
//                     buffer[((i + 1) * 24 - 3 * j) - 3] = brgb[0];
//                     buffer[((i + 1) * 24 - 3 * j) - 2] = brgb[1];
//                     buffer[((i + 1) * 24 - 3 * j) - 1] = brgb[2];
//                 }
//             }
//         }
//         break;
    case MID:
#if DEBUG_FLAG
        /* only using in debug */
        dbgPrintf("choose 24x32 %c\n", ch);
#endif
        /* 遍历点阵算出rgb */
        /* (size / 3) / 8 为每个字符位图占用的字节 */
        for (uint32_t i = 0; i < (size / 3) / 8; i++) {
            /* ascii偏移量：32 */
            /* 双循环遍历位 */
            for (int j = 7; j >= 0; j--) {
                /* 按位检测,从高位开始 */
                if ((((StAsciiFont32[ch - 32][i]) >> j) & 1) != 0) {
                    buffer[((i + 1) * 24 - 3 * j) - 3] = frgb[0];
                    buffer[((i + 1) * 24 - 3 * j) - 2] = frgb[1];
                    buffer[((i + 1) * 24 - 3 * j) - 1] = frgb[2];
                }
                else {
                    buffer[((i + 1) * 24 - 3 * j) - 3] = brgb[0];
                    buffer[((i + 1) * 24 - 3 * j) - 2] = brgb[1];
                    buffer[((i + 1) * 24 - 3 * j) - 1] = brgb[2];
                }
            }
        }
        break;
//     case BIG:
// #if DEBUG_FLAG
//         /* only using in debug */
//         dbgPrintf("choose 64x64 %s\n", ch);
// #endif
//         /* 遍历点阵算出rgb */
//         /* (size / 3) / 8 为每个字符位图占用的字节 */
//         for (uint32_t i = 0; i < (size / 3) / 8; i++) {
//             /* ascii偏移量：32 */
//             /* 双循环遍历位 */
//             for (uint8_t j = 7; j >= 0; j--) {
//                 /* 按位检测,从高位开始 */
//                 if ((StAsciiFont64[ch - 32][i] >> j) & 1) {
//                     buffer[((i + 1) * 24 - 3 * j) - 3] = frgb[0];
//                     buffer[((i + 1) * 24 - 3 * j) - 2] = frgb[1];
//                     buffer[((i + 1) * 24 - 3 * j) - 1] = frgb[2];
//                 }
//                 else {
//                     buffer[((i + 1) * 24 - 3 * j) - 3] = brgb[0];
//                     buffer[((i + 1) * 24 - 3 * j) - 2] = brgb[1];
//                     buffer[((i + 1) * 24 - 3 * j) - 1] = brgb[2];
//                 }
//             }
//         }
//         break;
    default:
        return;
    }
    /* 初始化窗口 */
    st7789SetWindow(wx, hy, wx + (uint8_t)(siz & 0xFF) - 1, hy + (uint8_t)((siz >> 8) & 0xFF) - 1);
    /* 发送缓冲区 */
    st7789SpiSendCmd(RAMWR);
    st7789SpiDMASendDatas(buffer, size);
}

/**
 * @brief  LCD显示点阵字符串
 * @param  str  字符串
 * @param  columnSpace   列间距：字与字的间距（像素）
 * @param  wx  横坐标
 * @param  hy  纵坐标
 * @param  fg  前景色
 * @param  bg  背景色
 * @param  siz  字体大小
 */
void st7789SpiShowStr(
    const char*             str, 
    int16_t                 columnSpace,
    uint16_t                wx,
    uint16_t                hy, 
    St7786Spi4Color18*      fg,
    St7786Spi4Color18*      bg,
    St7786SpiFontSize       siz
) {
    /* 遍历字符串 */
    for (uint32_t i = 0; i < strlen(str); i++) {
        /* 显示字符 */
        st7789SpiShowChar(str[i], wx, hy, fg, bg, siz);
        /* 递增字间距 */
        wx += ((uint8_t)(siz & 0xFF) + columnSpace);
    }
}

/**
 * @brief  设置屏幕显示的windows
 * @note  等价于设置行列指针，记得每次绘图都要设置，x[0:240] y[0:280]
 * @param  x0  LCD的起始x坐标
 * @param  y0  LCD的起始y坐标
 * @param  x1  LCD的结束x坐标
 * @param  y1  LCD的结束y坐标
 */
void st7789SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    /* 显存映射偏移量 */
    y0 += ST_R_OFFSET;
    y1 += ST_R_OFFSET;
    /* 列坐标 */
    st7789SpiSendCmd(CASET);
    /* 先发高位字节 */
    st7789SpiSendData(x0 >> 8);
    st7789SpiSendData(x0 & 0xFF);
    st7789SpiSendData(x1 >> 8);
    st7789SpiSendData(x1 & 0xFF);
    /* 行坐标 */
    /* 先发高位字节 */
    st7789SpiSendCmd(RASET); 
    st7789SpiSendData(y0 >> 8);
    st7789SpiSendData(y0 & 0xFF);
    st7789SpiSendData(y1 >> 8);
    st7789SpiSendData(y1 & 0xFF);
#if DEBUG_FLAG
    dbgPrintf("set (%d, %d) to (%d, %d)\n",
    x0, y0-ST_R_OFFSET, x1, y1-ST_R_OFFSET);
#endif
}

/**
 * @brief  设置屏幕显示的windows
 * @note  等价于设置行列指针，记得每次绘图都要设置，x[0:240] y[0:280]
 * @param  rct  指向自定义矩形的指针
 */
void st7789SetWinRec(St7786Rect* rct) {
    /* 设置window */
    st7789SetWindow(rct->xs,
        rct->ys,
        rct->xs + rct->wide - 1,
        rct->ys + rct->height - 1);
}

/**
 * @brief   关闭背光
 */
void st7789OffBg(void) {
	GPIO_ResetBits(St7789Init.ST_CL_GPIOx, St7789Init.ST_BLK_Pin);
}

/**
 * @brief   开启背光
 */
void st7789OnBg(void) {
	GPIO_SetBits(St7789Init.ST_CL_GPIOx, St7789Init.ST_BLK_Pin);
}

/**
 * @brief  用一种颜色填充某一块矩形区域
 * @param  rct  指向自定义矩形的指针
 * @param  fg  指向自定义颜色的指针
 * @note  颜色每一个通道的低2位无效, 无缓冲区，非DMA发送，内存友好，没有缓冲区限制
 */
void st7789FillRect(St7786Rect* rct, St7786Spi4Color18* fg) {
#if DEBUG_FLAG
    /* only using in debug */
    dbgPrintf("fill rgb6bit[%d,%d,%d] from %d,%d to %d,%d ...\n",
        fg->red,
        fg->green,
        fg->blue,
        rct->xs,
        rct->ys,
        rct->xs + rct->wide - 1,
        rct->ys + rct->height - 1
    );
#endif   
    /* 设置window */
    st7789SetWinRec(rct);
    /* 开始写入 */
    uint8_t rgb[3];
    st7789ColorMap(rgb, fg);
    st7789SpiSendCmd(RAMWR);
    for (uint32_t i = 0; i < rct->height * rct->wide; i++) {
        st7789SpiSendData(rgb[0]);
        st7789SpiSendData(rgb[1]);
        st7789SpiSendData(rgb[2]);
    }
}

/**
 * @brief  用一种颜色填充某一块矩形区域
 * @param  rct  指向自定义矩形的指针
 * @param  fg  指向自定义颜色的指针
 * @note  颜色每一个通道的低2位无效, 双缓冲区，DMA发送，内存占用<=12KB
 */
void st7789DMAFillRect(St7786Rect* rct, St7786Spi4Color18* fg) {
#if DEBUG_FLAG
    /* only using in debug */
    dbgPrintf("DMA fill rgb6bit[%d,%d,%d] from %d,%d to %d,%d ...\n",
        fg->red,
        fg->green,
        fg->blue,
        rct->xs,
        rct->ys,
        rct->xs + rct->wide - 1,
        rct->ys + rct->height - 1
    );
#endif
    /* 转换格式 */
    uint8_t rgb[3];
    st7789ColorMap(rgb, fg);
    /* 首先判断矩形大小是否小于12000byte */
    if (rct->height * rct->wide <= ST_MAX_RECT_SIZE) { //用面积判断
        /* 单缓冲直接发送 */
        uint8_t buffer[rct->height * rct->wide * 3];
        for (uint32_t i = 0; i < rct->height * rct->wide * 3; i += 3) {
            buffer[i] = rgb[0];
            buffer[i + 1] = rgb[1];
            buffer[i + 2] = rgb[2];
        }
        st7789SetWinRec(rct);
        /* DMA发送 */
        st7789SpiSendCmd(RAMWR);
        st7789SpiDMASendDatas(buffer, rct->height * rct->wide * 3); 
    }
    /* 大于12000 byte */
    else {
    /* 根据矩形大小创建缓冲区 */
    /* 设置最大块高度 */
    uint32_t maxHeight = ST_MAX_RECT_SIZE / rct->wide;
    /* 设置缓冲块数量 */
    uint8_t subRectNum = rct->height / maxHeight;
    /* 剩余块高度 */
    uint8_t lastRectHeight = rct->height % maxHeight;
    /* 重新设置窗口高度 */
    rct->height = maxHeight;
#if DEBUG_FLAG
    /* only using in debug */
    dbgPrintf("maxHeight:%d; subRectNum:%d; lastRectHeight:%d; wide:%d; total size:%d\n",
        maxHeight,
        subRectNum,
        lastRectHeight,
        rct->wide,
        rct->height * rct->wide
    );
#endif
    uint32_t size = rct->wide * maxHeight * 3;
    /* 分配缓冲区 */
    uint8_t* buffer = (uint8_t*)malloc(size);
    /* 先不判断是否整除，直接填充规则部分 */
    /* 循环填充缓冲区 */
    for (uint32_t j = 0; j < size; j += 3) {
        buffer[j] = rgb[0];
        buffer[j + 1] = rgb[1];
        buffer[j + 2] = rgb[2];
    }
    for (uint8_t i = 0; i < subRectNum; i++) {
        st7789SetWinRec(rct);
        /* 窗口y坐标递增 */
        rct->ys += maxHeight;
        /* DMA发送 */
        st7789SpiSendCmd(RAMWR);
        /* 发给DMA后无需管理，CPU填充缓冲区 */
        st7789SpiDMASendDatas(buffer, size);  
    }
    /* 释放内存 */
    if (lastRectHeight != 0) {
        /* 发送剩余块 */
        uint8_t buffer[lastRectHeight * rct->wide * 3];
        for (uint32_t i = 0; i < lastRectHeight * rct->wide * 3; i += 3) {
            buffer[i] = rgb[0];
            buffer[i + 1] = rgb[1];
            buffer[i + 2] = rgb[2];
        }
        rct->height = lastRectHeight;
        st7789SetWinRec(rct);
        /* DMA发送 */
        st7789SpiSendCmd(RAMWR);
        st7789SpiDMASendDatas(buffer, lastRectHeight * rct->wide * 3);
    }
    free(buffer);
    }
}
/**
 * @brief  将用户自定义颜色转化成可发送的像素数据
 * @note  输入颜色色深为6bit[0,63]
 * @param  buffer  输出像素颜色缓冲区，默认[0] ::= R [1] ::= G [2] ::= B
 * @param  color  输入的6bit颜色
 */
void st7789ColorMap(uint8_t* buffer, St7786Spi4Color18* color) {
    buffer[0] = (63 - color->red) << 2;
    buffer[1] = (63 - color->green) << 2;
    buffer[2] = (63 - color->blue) << 2;
}

/**
 * @brief  重新设置矩形
 * @param  rct  指向矩形指针
 * @param  xs  起始x
 * @param  ys  起始y
 * @param  wide  x方向长度
 * @param  height y方向长度
 */
void st7789RectSet(St7786Rect* rct, uint32_t xs, uint32_t ys, uint32_t wide, uint32_t height) {
    rct->xs = xs;
    rct->ys = ys;
    rct->wide = wide;
    rct->height = height;
}

/**
 * @brief  重新设置颜色
 * @param  color  指向目标颜色
 * @param  R  RDE
 * @param  G  GREEN
 * @param  B  BLUE
 */
void st7789ColorSet(St7786Spi4Color18* color, uint8_t R, uint8_t G, uint8_t B) {
    color->red = R;
    color->green = G;
    color->blue = B;
}

/**
 * @brief  设置窗口内的前景
 * @note  不影响背景色（黑色）
 * @param  rct  窗口矩形指针
 * @param  fg  前景色指针
 * @param  bg  背景色指针
 */
void st7789SetRectFg(
    St7786Rect*         rct, 
    St7786Spi4Color18*  fg, 
    St7786Spi4Color18*  bg
) {
}

/**
 * @brief  设置窗口内的背景
 * @note  不影响前景（已显示的内容）
 * @param  rct  窗口矩形指针
 * @param  fg  前景色指针
 * @param  bg  背景色指针
 */
void st7789SetRectBg(
    St7786Rect* rct, 
    St7786Spi4Color18* fg, 
    St7786Spi4Color18* bg
) {
}

/**
 * @brief  st7789硬件重置
 */
void st7789HardReset(void) {
    /* 拉低RES */
    GPIO_ResetBits(St7789Init.ST_CL_GPIOx, St7789Init.ST_RES_Pin);
    myDelay(20);
    /* 拉高RES */
    GPIO_SetBits(St7789Init.ST_CL_GPIOx, St7789Init.ST_RES_Pin);
    myDelay(20);
#if DEBUG_FLAG
        /* only using in debug */
        dbgPrintf("HardReset...\n");
#endif
}

/**
 * @brief  st7789软件重置
 */
void st7789SoftReset(void) {
#if DEBUG_FLAG
    /* only using in debug */
    dbgPrintf("SoftReset...\n");
#endif
    st7789SpiSendCmd(SWREST);
    myDelay(20);
}

/**
 * @brief  LCD清屏
 */
void st7789Clear(void) {
#if DEBUG_FLAG
    /* only using in debug */
    dbgPrintf("clean LCD...\n");
#endif
    St7786Spi4Color18 black = {0, 0, 0};
    St7786Rect screen = {
        0, 0, 240, 280
      };
    st7789DMAFillRect(&screen, &black);
}

/**
 * @brief  设置LCD的亮度
 * @param  lv  0:最暗 255:最亮
 * @note  在此lcd上无效
 */
void st7789SetLightLv(uint8_t lv) {
    st7789SpiSendCmd(WRDISBV);
    st7789SpiSendData(lv);
}

/**
 * @brief  LCD画线函数
 * @note  暂未启用，开销太大
 * @param  line  直线
 * @param  thickness  粗细
 * @param  fg  前景色
 */
void st7789DrawLine(St7786Line* line, uint8_t thickness, St7786Spi4Color18* fg) {
}

/**
 * @brief  LCD画框框函数
 * @note  注意检查绘图缓冲区域
 * @param  rct  矩形
 * @param  thickness  线条宽度
 * @param  fg  前景色
 */
void st7789DrawRect(St7786Rect* rct, uint8_t thickness, St7786Spi4Color18* fg) {
}

/**
 * @brief  LCD画点函数
 * @note  暂未启用，开销太大
 * @param  point  点
 * @param  thickness  大小
 * @param  fg  前景色
 */
void st7789DrawPoint(St7786Point* point, uint8_t thickness, St7786Spi4Color18* fg) {
}

/**
 * @brief  LCD画椭圆函数
 * @note  暂未启用，开销太大
 * @param  oval  椭圆
 * @param  thickness  线条粗细
 * @param  fg  前景色
 */
void st7789DrawOval(St7786Oval* oval, uint8_t thickness, St7786Spi4Color18* fg) {
}
