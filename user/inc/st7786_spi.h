/**
 ******************************************************************************
 * @file    st7786_spi.h
 * @author  Yurilt
 * @version V1.0.0
 * @date    31-October-2025
 * @brief   ST7789显示屏驱动头文件
 * @note    声明ST7789显示屏驱动的函数接口和参数
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

#ifndef __ST7786_SPI__
#define __ST7786_SPI__
/* Includes ------------------------------------------------------------------*/
#include "stm32f10x_gpio.h"

/* 链接器配置 */
#if defined(__GNUC__)
#define LD_CONST_INTO_FLASH __attribute__((section(".rodata")))
#elif defined(__ICCARM__)
#define LD_CONST_INTO_FLASH _Pragma("location=\\" FLASH_DATA "")
#else
#define LD_CONST_INTO_FLASH
#endif
/* 默认的配置：SPI1 通过宏定义，用户可以通过修改St7789Init变量来自定义配置 */
/* 主要控制驱动芯片的针脚口 */
#define ST_MAIN_GPIOx GPIOA
/* spi的时钟信号 */
#define ST_SPI_SCL GPIO_Pin_5
/* spi的数据信号 */
#define ST_SPI_SDA GPIO_Pin_7
/* 重置信号，低电平使能 */
#define ST_RES GPIO_Pin_2
/* cmd/data选择，高电平data/arguments 低电平cmd */
#define ST_SPI_DC GPIO_Pin_1
/* spi片选信号 */
#define ST_SPI_CS GPIO_Pin_4
/* DMA 选择 */
#define ST_SPI_DMA DMA1_Channel3
/* 控制背光的针脚口 */
#define ST_BLK_GPIOx GPIOA
/* 显示屏背光开关，高电平使能 */
#define ST_BLK GPIO_Pin_13

/* 屏幕宽度 */
#define ST_WIDE (uint8_t)240
/* 屏幕高度 */
#define ST_HIGHT (uint8_t)280
/* 行偏移量 */
#define ST_R_OFFSET (uint8_t)20
/* LCD颜色格式 */
#define COLOR_BITS "18bits"
/* 最大缓冲区大小,默认的大小为12K.注意不要让内存溢出 */
#define ST_MAX_BUFFER_SIZE (uint32_t)0x3000
/* 由缓冲区域计算出的最大矩形面积 */
#define ST_MAX_RECT_SIZE (uint32_t)(ST_MAX_BUFFER_SIZE / 3)

/* 常用颜色 */
#define ST_RED(name) St7786Spi4Color18 name = {63, 0, 0}
#define ST_GREEN(name) St7786Spi4Color18 name = {0, 63, 0}
#define ST_BLUE(name) St7786Spi4Color18 name = {0, 0, 63}
#define ST_BLACK(name) St7786Spi4Color18 name = {0, 0, 0}
#define ST_WHITE(name) St7786Spi4Color18 name = {63, 63, 63}
#define ST_YELLO(name) St7786Spi4Color18 name = {63, 63, 0}
#define ST_CYAN(name) St7786Spi4Color18 name = {0, 63, 63}
#define ST_MAGENTA(name) St7786Spi4Color18 name = {63, 0, 63}

#ifdef __cplusplus
extern "C" {
#endif
/* st7786的所有命令 */
typedef enum {
    /* 无 */
    NOP = 0x00,
    /* 软件复位，显示模块执行软件复位，寄存器被写入其软件复位默认值。 */
    SWREST = 0x01,
    /* 读取显示ID */
    RDDID = 0x04,
    /* 读取显示状态 */
    RDDST = 0x09,
    /* 读取显示电源 */
    RDDPM = 0x0A,
    /* 读取显示 */
    RDDMADCTL = 0x0B,
    /* 读取显示像素 */
    RDDCOLMOD = 0x0C,
    /* 读取显示图像 */
    RDDIM = 0x0D,
    /* 读取显示信号 */
    RDDSM = 0x0E,
    /* 读取显示自诊断结果 */
    RDDSDR = 0x0F,
    /* 进入睡眠 */
    SLPIN = 0x10,
    /* 退出睡眠 */
    SLPOUT = 0x11,
    /* 部分显示模式开启 */
    PTLON = 0x12,
    /* 正常显示模式开启 */
    NORON = 0x13,
    /* 显示反转关闭 */
    INVOFF = 0x20,
    /* 显示反转开启 */
    INVON = 0x21,
    /* 选择伽马表 */
    GAMSET = 0x26,
    /* 关闭显示 */
    DISPOFF = 0x28,
    /* 开启显示 */
    DISPON = 0x29,
    /* 列地址设置 */
    CASET = 0x2A,
    /* 行地址设置 */
    RASET = 0x2B,
    /* 内存写入 */
    RAMWR = 0x2C,
    /* 内存读取 */
    RAMRD = 0x2E,
    /* 部分区域显示 */
    PTLAR = 0x30,
    /* 垂直滚动定义 */
    VSCRDEF = 0x33,
    /* 撕裂效果线关闭 */
    TEOFF = 0x34,
    /* 撕裂效果线开启 */
    TEON = 0x35,
    /* 内存数据访问控制 */
    MADCTL = 0x36,
    VSCSAD = 0x37,
    IDMOFF = 0x38,
    IDMON  = 0x39,
    COLMOD = 0x3A,
    WRMEMC = 0x3C,
    RDMEMC = 0x3E,
    STE    = 0x44,
    GSCAN  = 0x45,
    /* 亮度设置 */
    WRDISBV = 0x51,
    RDDISBV = 0x52,
    WRCTRLD = 0x53,
    RDCTRLD = 0x54,
    /* 写入内容自适应亮度控制和色彩增强 */
    WRCACE   = 0x55,
    RDCABC   = 0x56,
    WRCABCMB = 0x5E,
    RDCABCMB = 0x5F,
    RDABCSDR = 0x68,
    RDID1    = 0xDA,
    RDID2    = 0xDB,
    RDID3    = 0xDC,
    RAMCTRL  = 0xB0,
    RGBCTRL  = 0xB1,
    PORCTRL  = 0xB2,
    FRCTRL1  = 0xB3,
    PARCTRL  = 0xB5,
    GCTRL    = 0xB7,
    GTADJ    = 0xB8,
    DGMEN    = 0xBA,
    VCMOS    = 0xBB,
    LCMCTRL  = 0xC0,
    IDSET    = 0xC1,
    VDVVRHEN = 0xC2,
    VRHS     = 0xC3,
    VDVS     = 0xC4,
    VCMOFSET = 0xC5,
    /* 正常模式下的帧率控制 */
    FRCTRL2   = 0xC6,
    CABCCTRL  = 0xC7,
    REGSEL1   = 0xC8,
    REGSEL2   = 0xCA,
    PWMFRSEL  = 0xCC,
    PWCTRL    = 0xD0,
    VAPVANEN  = 0xD2,
    CMD2EN    = 0xDF,
    PVGAMCTRL = 0xE0,
    NVGAMCTRL = 0xE1,
    DGMLUTR   = 0xE2,
    DGMLUTB   = 0xE3,
    /* 门控控制 */
    GATECTRL = 0xE4,
    SPI2EN   = 0xE7,
    PWCTRL2  = 0xE8,
    EQCTRL   = 0xE9,
    PROMCTRL = 0xEC,
    PROMEN   = 0xFA,
    NVMSET   = 0xFC,
    PROMACT  = 0xFE
} St7786SpiCmd;

/* st7789 4wide spi 18bits color */
typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} St7786Spi4Color18;

/* st7789 lcd初始化 */
typedef struct {
    SPI_TypeDef* SPIx;         /* SPI选择 */
    uint8_t SPI_REMAP;         /* SPI针脚是否复用 0:未复用 */
    GPIO_TypeDef* ST_CL_GPIOx; /* LCD控制端口 */
    uint32_t ST_GPIOx_RCC;     /* LCD控制端口RCC */
    uint16_t ST_DC_Pin;        /* DC针脚 */
    uint16_t ST_BLK_Pin;       /* BLK针脚 */
    uint16_t ST_RES_Pin;       /* RES针脚 */
    uint16_t ST_CS_Pin;        /* 软件片选 */
    uint8_t frq;               /* 刷新率 */
} St7789InitStruct;

#ifdef __cplusplus
}
#endif

/* 用户必须提供一个实例去初始化屏幕 */
extern St7789InitStruct St7789Init;

#ifdef __cplusplus
extern "C" {
#endif
/* st7789 font size byte/char，高8位表示高度，低8位表示宽度 */
typedef enum {
    MINI = (uint16_t)(16 << 8 | 10), /* 10x16 */
    MID  = (uint16_t)(32 << 8 | 24), /* 24x32 */
    BIG  = (uint16_t)(64 << 8 | 48)  /* 48x64 */
} St7786SpiFontSize;

/* st7789 draw forms */
typedef enum {
    Rect,
    Line,
    Point,
    Oval
} St7786SpiForms;

/* st7789 draw forms */
typedef struct {
    uint16_t xs; /* 左上角 */
    uint16_t ys; /* 左上角 */
    uint16_t wide;
    uint16_t height;
} St7786Rect;

typedef struct {
    uint16_t xs;
    uint16_t ys;
    uint16_t xe;
    uint16_t ye;
} St7786Line;

typedef struct {
    uint16_t x;
    uint16_t y;
} St7786Point;

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t a;
    uint16_t b;
} St7786Oval;

// /* 48x64的点阵,过大 */
// extern const uint8_t StAsciiFont64[95][512] LD_CONST_INTO_FLASH;
/* 24x32的点阵 */
extern const uint8_t StAsciiFont32[95][96] LD_CONST_INTO_FLASH;
// /* 10x16的点阵 */
// extern const uint8_t StAsciiFont16[95][32];

void st7789SpiSendByte(uint8_t bt);
void st7789SpiSendBytes(uint8_t* bts, uint32_t size);
void st7789SpiDMASendDatas(uint8_t* bts, uint32_t size);
void st7789SpiSendCmd(St7786SpiCmd cmd);
void st7789SpiSendData(uint8_t dat);
void st7789SpiSendDatas(uint8_t* dats, uint32_t size);
void st7789SpiRecvByte(uint32_t* ptr);
void st7789SpiShowChar(const unsigned char ch,
                       uint16_t wx,
                       uint16_t hy,
                       St7786Spi4Color18* fg,
                       St7786Spi4Color18* bg,
                       St7786SpiFontSize siz);
void st7789SpiShowStr(const char* ch,
                      int16_t columnSpace,
                      uint16_t wx,
                      uint16_t hy,
                      St7786Spi4Color18* fg,
                      St7786Spi4Color18* bg,
                      St7786SpiFontSize siz);
void st7789Init(St7789InitStruct* stInitStruct);
void st7789SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void st7789SetWinRec(St7786Rect* rct);
void st7789OffBg(void);
void st7789OnBg(void);
void st7789SetLightLv(uint8_t lv);
void st7789FillRect(St7786Rect* rct, St7786Spi4Color18* fg);
void st7789DMAFillRect(St7786Rect* rct, St7786Spi4Color18* fg);
void st7789SetRectFg(St7786Rect* rct, St7786Spi4Color18* fg, St7786Spi4Color18* bg);
void st7789SetRectBg(St7786Rect* rct, St7786Spi4Color18* fg, St7786Spi4Color18* bg);
void st7789HardReset(void);
void st7789SoftReset(void);
void st7789Clear(void);
void st7789ColorMap(uint8_t* buffer, St7786Spi4Color18* color);
void st7789RectSet(St7786Rect* rct, uint32_t xs, uint32_t ys, uint32_t wide, uint32_t height);
void st7789ColorSet(St7786Spi4Color18* color, uint8_t R, uint8_t G, uint8_t B);
void st7789DrawLine(St7786Line* line, uint8_t thickness, St7786Spi4Color18* fg);
void st7789DrawRect(St7786Rect* rct, uint8_t thickness, St7786Spi4Color18* fg);
void st7789DrawPoint(St7786Point* point, uint8_t thickness, St7786Spi4Color18* fg);
void st7789DrawOval(St7786Oval* oval, uint8_t thickness, St7786Spi4Color18* fg);
#ifdef __cplusplus
}
#endif
#endif
