---
title: 简要介绍
description: 基于现代CPP静态多态的嵌入式RTOS
author: YT_Minro
date: 2025-9-24
---
<div align="center">

# StratOS - a RTOS for ARM Cortex-M3 using Template-based Policy Design and GNU Toolchain<br>基于静态模板策略模式和GNU工具链的ARM Cortex-M3实时操作系统 

[![License: LGPL v3.0](https://img.shields.io/badge/License-LGPL_v3.0-blue.svg)](https://www.gnu.org/licenses/lgpl-2.0)
[![GitHub release](https://img.shields.io/github/release/username/repository.svg)](https://github.com/username/repository/releases)

</div>

**介绍**：*厌倦了一大堆宏？被类型安全和内存安全折磨？是时候拥抱现代CPP了！这是一个使用并且基于现代CPP的特性的嵌入式RTOS尝试，目前尝试部署在Cortex-M3架构。
此RTOS旨在让编译器承担我们的相当一部分类型处理工作，使得RTOS变得更加易用、易维护、方便拓展，对于大部分任务减少甚至消除运行时开销。*
<br>**你可以从项目的
`{root}/doc/`下找到详细的文档说明。**
<br>**要获得在线文档，请访问：[*StratOS 在线参考手册*](https://yuriltlef.github.io/StratOS_LiySTD_Documentation/)**

## StratOS 设计宗旨

StratOS关心三个方面：
1. 类型安全
2. 减少运行开销
3. 方便编码

为此，在编写代码是，我们遵循以下约定：
- 安全第一。非必要避免使用动态内存，如有必要使用系统接口。
- 组合优于继承。非必要不要继承。
- 使用RAII管理资源。
- 避免动态类型开销，使用静态类型。
- 不要使用RTTI和异常。

## 编码规范

代码格式使用`{root}/.clang-format`文件设置。命名约定：
### 1. 宏、枚举量、`constexpr`全大写。
```cpp
/* great */
#define SYSTEM_CLOCK_FREQUENCY (uint16_t(0xff))
enum class TaskStatus {
    READY = 0,
    ...
};

/* bad */
#define colock (uint16_t(0xff))
enum class taskstatus {
    r = 0,
    ...
};
```
### 2. 类型、枚举名采用驼峰紧凑写法。
```cpp
/* great */
enum class TaskStatus {
    ...
};

/* bad */
enum class taskstatus {
    ...
};
```
### 3. 函数名采用蛇形命名。
```cpp
/* great */
class TaskManager {
    StatusCode creat_task();
};

/* bad */
class TaskManager {
    StatusCode CreatTask();
};
```
### 4. 命名文档化，简介明了。
```cpp
/* great */
class GPIOServer {
    StatusCode init();
    StatusCode send_byte();
};

/* bad */
class GPIOServer {
    StatusCode i();
    StatusCode s();
};
```

### 5. 注释使用`/**/`，无论几行。
```cpp
/* great */

/**
 * xxx
*/ 
class GPIOServer {
    /* 初始化GPIO外设 */
    StatusCode init();
    /* 发送一个字节 */
    StatusCode send_byte();
};
```

### 6. 头文件模板声明与定义分离为`.hpp`与`.ipp`。

## 项目结构
