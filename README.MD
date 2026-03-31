
<div align="center">

# StratOS - a RTOS for ARM Cortex-M3 using Template-based Policy Design and GNU Toolchain基于静态模板策略模式和GNU工具链的ARM Cortex-M3实时操作系统

[![License: LGPL v3.0](https://img.shields.io/badge/License-LGPL_v3.0-blue.svg)](https://www.gnu.org/licenses/lgpl-3.0)

</div>

**介绍**：*厌倦了一大堆宏？被类型安全和内存安全折磨？是时候拥抱现代CPP了！这是一个使用并且基于现代CPP的特性的嵌入式RTOS尝试，目前支持部署在Cortex-M3和Cortex-M4架构。
此RTOS旨在让编译器承担我们的相当一部分类型处理工作，使得RTOS变得更加易用、易维护、方便拓展，对于大部分任务减少甚至消除运行时开销。*
**你可以从项目的
`{root}/docs/`下找到详细的文档说明。**

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

代码格式使用`{root}/.clang-format`文件设置。以下是项目的编码约定：

### 1. 命名约定

#### 1.1 命名空间

- 采用蛇形命名法（snake\_case）
- 层次清晰，避免过深嵌套
- 示例：
  ```cpp
  // 正确示例
  namespace strat_os::hal::traits {
      // 代码
  }

  // 错误示例
  namespace StratOS_HAL_Traits {
      // 代码
  }
  ```

#### 1.2 类型和类

- 采用驼峰命名法（PascalCase）
- 示例：
  ```cpp
  // 正确示例
  class BitMap {
      // 代码
  };

  struct SystemControl {
      // 代码
  };

  // 错误示例
  class bit_map {
      // 代码
  };
  ```

#### 1.3 函数

- 采用蛇形命名法（snake\_case）
- 示例：
  ```cpp
  // 正确示例
  void set_vector_table(address_type address) noexcept {
      // 代码
  }

  priority_group_type get_priority_grouping() noexcept {
      // 代码
  }

  // 错误示例
  void SetVectorTable(address_type address) {
      // 代码
  }
  ```

#### 1.4 变量

- 采用蛇形命名法（snake\_case）
- 成员变量与普通变量命名规则一致
- 示例：
  ```cpp
  // 正确示例
  ErrorHandler errorHandler_;
  Storage blocks_;
  uint32_t task_count;

  // 错误示例
  ErrorHandler error_handler;
  Storage Blocks;
  uint32_t taskCount;
  ```

#### 1.5 常量和枚举

- 常量：采用全大写蛇形命名法
- 枚举类名：采用驼峰命名法
- 枚举值：采用驼峰命名法（首字母小写）
- 示例：
  ```cpp
  // 正确示例
  constexpr std::size_t OS_RESERVED_ERROR_HANDLE_COUNT{32};

  enum class ErrorCode {
      None = 0x0000,      // 正确：首字母小写驼峰
      Unknown,            // 正确：首字母小写驼峰
      OutOfBounds = 0x0100, // 正确：首字母小写驼峰
      Full,               // 正确：首字母小写驼峰
      Empty,              // 正确：首字母小写驼峰
      InvalidArgument,    // 正确：首字母小写驼峰
      AllocationFailed,   // 正确：首字母小写驼峰
      NotEnoughSpace,     // 正确：首字母小写驼峰
      OutOfMemory,        // 正确：首字母小写驼峰
      Timeout = 0x0200,   // 正确：首字母小写驼峰
      ResourceBusy,       // 正确：首字母小写驼峰
      InvalidHandle,      // 正确：首字母小写驼峰
      NotSupported,       // 正确：首字母小写驼峰
      UserBase = 0x8000   // 正确：首字母小写驼峰
  };

  // 错误示例
  constexpr std::size_t osReservedErrorHandleCount{32};

  enum class error_code {
      NONE = 0x0000,      // 错误：全大写
      unknown,            // 错误：全小写
  };
  ```

#### 1.6 模板参数

- 采用驼峰命名法
- 示例：
  ```cpp
  // 正确示例
  template <typename AllocPolicy, typename ErrorHandler>
  class BitMap {
      // 代码
  };

  // 错误示例
  template <typename alloc_policy, typename error_handler>
  class BitMap {
      // 代码
  };
  ```

### 2. 注释规范

- 使用 Doxygen 风格的注释（`/** ... */`）
- 包含必要的标签：@file、@author、@brief、@param、@return、@tparam 等
- 函数、类、模板都应有详细的注释
- 复杂逻辑应有内联注释
- 示例：
  ```cpp
  // 正确示例
  /**
   * @file atomic.hpp
   * @author StratOS Team
   * @brief 原子操作实现
   */

  /**
   * @brief 原子加载
   * @param ptr 指向 volatile 内存的指针
   * @return 当前值
   */
  [[nodiscard]] inline static value_type load(volatile value_type* ptr) noexcept {
      return Policy::load(ptr);
  }

  // 错误示例
  // 原子加载
  static value_type load(volatile value_type* ptr) {
      return Policy::load(ptr);
  }
  ```

### 3. 代码风格

- 使用 `[[nodiscard]]` 标记有返回值的函数
- 使用 `inline` 和 `noexcept` 提高性能
- 使用 `static_assert` 进行编译期检查
- 优先使用模板和类型萃取进行编译期多态
- 避免使用 RTTI 和异常
- 使用 `#pragma once` 作为头文件保护
- 命名空间嵌套使用缩进
- 代码块使用大括号，即使只有一行
- 操作符前后有空格
- 函数参数列表换行时，参数对齐
- 示例：
  ```cpp
  // 正确示例
  #pragma once

  namespace strat_os {
  namespace hal {

  template <typename T>
  class Atomic {
  public:
      [[nodiscard]] inline T load() const noexcept {
          return value_;
      }
      
      inline void store(T value) noexcept {
          value_ = value;
      }
  private:
      T value_{};
  };

  } // namespace hal
  } // namespace strat_os

  // 错误示例
  #ifndef STRATOS_HAL_ATOMIC_HPP
  #define STRATOS_HAL_ATOMIC_HPP

  namespace strat_os {
  namespace hal {

  template <typename T>
  class Atomic {
  public:
      T load() const { return value_; }
      void store(T value) { value_=value; }
  private:
      T value_;
  };

  }}

  #endif // STRATOS_HAL_ATOMIC_HPP
  ```

### 4. 宏使用

- 仅用于包含守卫和必要的系统级定义
- 采用全大写命名，单词间用下划线分隔
- 避免滥用宏，优先使用 `constexpr` 和模板
- 示例：
  ```cpp
  // 正确示例
  #pragma once

  // 或使用宏作为包含守卫
  #ifndef STRATOS_HAL_ATOMIC_HPP
  #define STRATOS_HAL_ATOMIC_HPP

  // 错误示例
  #define MAX_SIZE 1024
  #define ADD(a, b) ((a) + (b))
  ```

### 5. 错误处理

- 使用自定义错误处理器
- 错误处理器必须满足特定接口要求
- 越界等错误会调用错误处理器
- 错误处理器应提供清晰的错误信息
- 示例：
  ```cpp
  // 正确示例
  template <typename ErrorHandler>
  class BitMap {
  public:
      void set(std::size_t pos) {
          if (pos >= size) {
              ErrorHandler::handle_error(ErrorCode::OutOfBounds);
              return;
          }
          // 实现
      }
  };

  // 错误示例
  class BitMap {
  public:
      void set(std::size_t pos) {
          // 无边界检查
          // 实现
      }
  };
  ```

### 6. 性能优化

- 内联函数：使用 `inline` 关键字
- 编译期常量：使用 `constexpr`
- 编译器内置函数：如 `__builtin_popcount`、`__builtin_ctz`
- 零开销抽象：通过模板和静态多态实现
- 避免动态内存分配：使用静态内存和内存池
- 减少运行时检查：提供安全版本和不安全版本的接口
- 示例：
  ```cpp
  // 正确示例
  constexpr std::size_t BITS_PER_WORD{sizeof(std::size_t) * 8};

  [[nodiscard]] inline std::size_t find_first_set(std::size_t word) noexcept {
      return __builtin_ctz(word);
  }

  // 安全版本（带边界检查）
  void set(std::size_t pos) {
      if (pos >= size) {
          ErrorHandler::handle_error(ErrorCode::OutOfBounds);
          return;
      }
      unsafe_set(pos);
  }

  // 不安全版本（无边界检查，更高性能）
  void unsafe_set(std::size_t pos) noexcept {
      // 实现
  }

  // 错误示例
  #define BITS_PER_WORD (sizeof(std::size_t) * 8)

  std::size_t find_first_set(std::size_t word) {
      for (std::size_t i = 0; i < BITS_PER_WORD; ++i) {
          if (word & (1 << i)) {
              return i;
          }
      }
      return BITS_PER_WORD;
  }
  ```

### 7. 安全性

- 安全第一，非必要避免使用动态内存
- 使用 RAII 管理资源
- 避免未定义行为
- 进行适当的边界检查
- 禁止拷贝和移动（确保唯一所有权）
- 零初始化保证：默认构造时初始化所有成员
- 示例：
  ```cpp
  // 正确示例
  class BitMap {
  public:
      BitMap() noexcept = default;
      
      // 禁止拷贝和移动
      BitMap(const BitMap&) = delete;
      BitMap& operator=(const BitMap&) = delete;
      BitMap(BitMap&&) = delete;
      BitMap& operator=(BitMap&&) = delete;
      
  private:
      std::size_t blocks_{};
  };

  // 错误示例
  class BitMap {
  public:
      BitMap() {
          // 无初始化
      }
      
  private:
      std::size_t blocks;
  };
  ```

### 8. 文件命名

- 头文件：使用 `.hpp` 扩展名
- 源文件：使用 `.cpp` 扩展名
- 汇编文件：使用 `.s` 扩展名
- 模板实现：与声明分离为 `.hpp` 与 `.ipp`（如需）
- 示例：
  ```
  // 正确示例
  os_hal/include/atomic.hpp
  os_hal/src/atomic.cpp
  os_hal/src/startup.s

  // 错误示例
  os_hal/include/atomic.h
  os_hal/src/atomic.c
  os_hal/src/startup.asm
  ```

### 9. 代码组织

- 按功能模块组织代码
- 头文件包含顺序：标准库、第三方库、项目内部头文件
- 命名空间层次清晰，避免命名冲突
- 使用类型萃取和 SFINAE 实现编译期多态
- 示例：
  ```cpp
  // 正确示例
  #include <cstddef>
  #include <type_traits>

  #include "mu_sstl/errors/basic_error.hpp"

  #include "strat_os/hal/traits/system_traits.hpp"

  namespace strat_os {
  namespace hal {

  // 代码

  } // namespace hal
  } // namespace strat_os

  // 错误示例
  #include "strat_os/hal/traits/system_traits.hpp"
  #include <cstddef>
  #include "mu_sstl/errors/basic_error.hpp"

  namespace strat_os_hal {

  // 代码

  }
  ```

### 10. 最佳实践

- 优先考虑编译期多态而非运行时多态
- 使用 `std::conjunction` 等类型特性进行编译期检查
- 提供详细的静态断言错误信息
- 利用模板特化和偏特化实现不同平台的适配
- 保持接口一致性，便于扩展和维护
- 示例：
  ```cpp
  // 正确示例
  template <typename T, typename = void>
  struct is_atomic : std::false_type {};

  template <typename T>
  struct is_atomic<T, std::void_t<decltype(T::load()), decltype(T::store(std::declval<T>()))>> : std::true_type {};

  template <typename T>
  void process_atomic(T& atomic) {
      static_assert(is_atomic<T>::value, "T must be an atomic type with load() and store() methods");
      // 实现
  }

  // 错误示例
  class AtomicInterface {
  public:
      virtual ~AtomicInterface() = default;
      virtual void store(int value) = 0;
      virtual int load() const = 0;
  };

  class AtomicInt : public AtomicInterface {
  public:
      void store(int value) override { value_ = value; }
      int load() const override { return value_; }
  private:
      int value_{};
  };
  ```

## 项目结构

StratOS 采用模块化设计，主要包含以下目录结构：

| 目录           | 功能描述                        |
| ------------ | --------------------------- |
| LICENSES/    | 项目许可证文件                     |
| cmake/       | CMake 配置文件                  |
| docs/        | 项目文档                        |
| examples/    | 示例代码                        |
| libraries/   | 依赖库，包括 MUSSTL 和 stm32SL     |
| os\_boost/   | 启动代码，针对不同平台的初始化             |
| os\_hal/     | 硬件抽象层，包含不同 Cortex-M 系列的底层实现 |
| os\_kernel/  | 内核实现，包含任务调度、内存管理、队列等核心功能    |
| os\_linker/  | 链接脚本，用于不同平台的内存布局            |
| os\_service/ | 系统服务                        |
| scripts/     | 辅助脚本                        |
| tools/       | 工具目录，包含性能分析、测试和验证工具         |
| user/        | 用户应用代码                      |

### 核心模块详解

#### os\_hal (硬件抽象层)

- 提供针对不同 Cortex-M 系列处理器的底层实现
- 支持 Cortex-M3 和 Cortex-M4 架构
- 包含原子操作、上下文切换、中断处理等核心功能

#### os\_kernel (内核)

- 实现实时操作系统的核心功能
- 包含任务调度、内存管理、队列、信号量、定时器等组件
- 基于现代 C++ 模板策略设计模式

#### libraries

- **MUSSTL**：自定义的 STL 实现，提供容器和工具类
- **stm32SL**：STM32 标准库，包含 CMSIS 和外设驱动

#### user

- 用户应用代码示例
- 包含主程序入口和设备驱动示例

## 技术特性

1. **现代 C++ 特性**：利用模板、静态多态等现代 C++ 特性
2. **零运行时开销**：通过模板元编程和静态多态，将许多运行时检查和决策转移到编译期
3. **类型安全**：强类型设计，减少运行时错误
4. **可扩展性**：基于策略模式的设计，便于扩展和定制
5. **跨平台支持**：支持不同的 Cortex-M 系列处理器

## 快速开始

### 环境要求

- GNU 工具链 (arm-none-eabi-gcc)
- CMake
- 支持 Cortex-M3/M4 的开发板

### 构建步骤

1. 克隆项目到本地
2. 配置 CMake 构建系统
3. 编译项目
4. 烧录到目标设备

## 示例代码

项目提供了丰富的示例代码，位于 `examples/` 目录下，展示如何使用 StratOS 的各种功能。

## 文档

项目文档位于 `docs/` 目录下，包含详细的使用说明和 API 参考。

## 许可证

StratOS 采用 LGPL v3.0 许可证，详见 `LICENSES/` 目录下的文件。
