<div align="center">

# StratOS

### *A Real-Time Operating System using Template-based Policy Design and GNU Toolchain*  
### *基于静态模板策略模式和 GNU 工具链的实时操作系统*

[![License: LGPL v3.0](https://img.shields.io/badge/License-LGPL_v3.0-blue.svg)](https://www.gnu.org/licenses/lgpl-3.0)

</div>

## 📖 介绍

> **厌倦了一大堆宏？被类型安全和内存安全折磨？是时候拥抱现代 C++ 了！**

StratOS 是一个使用 **现代 C++ 特性** 构建的嵌入式 RTOS 尝试，目前支持 **Cortex-M3** 和 **Cortex-M4** 架构。  
本系统旨在**让编译器承担相当一部分类型处理工作**，使 RTOS 变得：

- 更易用
- 更易维护
- 更方便扩展
- 对大部分任务减少甚至消除运行时开销

> 📘 详细的文档请见项目根目录下的 [`docs/`](./docs) 文件夹。

---

## 🎯 设计宗旨

StratOS 重点关注三个方面：

1. **类型安全**  
2. **减少运行时开销**  
3. **方便编码**

### 编码约定

- ✅ **安全第一**：非必要避免动态内存，如有必要请使用系统接口。
- ✅ **组合优于继承**：非必要不要使用继承。
- ✅ **RAII 管理资源**。
- ✅ **静态类型优先**：避免动态类型开销。
- ✅ **禁用 RTTI 和异常**。

---

## 🚀 快速开始

> ⚠️ **注意**：当前项目配置设施尚不够全面，仅适配了 **Cortex-M3** 及 **STM32 F1xx** 系列。  
> 若需要移植到其他平台，请修改 `cmake/` 下的配置文件。

### 1️⃣ 环境要求

- **GNU 工具链** (`arm-none-eabi-gcc`) 推荐版本 **15+**
- **CMake** 版本不低于 **3.25**
- 支持 **Cortex-M3/M4** 的开发板
- **Python 3** 环境

### 2️⃣ 开发工具

- **VS Code**（推荐）或任意文本编辑器
- **VS Code Clangd 扩展**（推荐）
- **clangd 语言服务器**（可选但推荐），版本 **不低于 22.0**

### 3️⃣ 构建步骤

#### ① 克隆项目

```bash
git clone https://github.com/Yuriltlef/StratOS.git my_project
```

#### ② 配置 clangd 插件

在项目**根目录**下创建 `clangd_config.json`，模板如下（请根据目标平台修改 `project_definitions`）：

```json
{
    "version": "0.1.2",
    "clangd": {
        "g++_compiler_path": "path/to/your/g++_compiler_executable_file.exe", // 注意需要包含编译器全称（比如： xxx/xxx/arm-none-eabi-g++.exe）
        "toolchain_root_path": "path/to/your/tool-chain/root",
        "project_root": "path/to/your/project/root",
        "project_definitions": [
            "your_definitions_1",
            "your_definitions_2"
        ]
    }
}
```

> 💡 **示例（Cortex-M3）**：  
> `"project_definitions": ["USE_STDPERIPH_DRIVER","STM32F10X_MD","OS_STM_RAM_SIZE=0x5000","__CORTEX_M3","__thumb2__=1","__THUMBEL__=1"]`

随后，进入 Clangd 插件设置界面，在 `Clangd:Arguments` 选项内，添加一项：
```text
--query-driver={g++_compiler_path}
```
> 💡 注意，这里的 `{g++_compiler_path}` 应该同  `clangd_config.json` 的 `g++_compiler_path` 一样且包含 g++ 可执行文件全称，且可执行文件路径不需要带引号。

完成上述配置后，**请按照 ③ 继续配置 CMake**。项目主 `CMakeLists.txt` 会自动执行 `{root}/scripts/cfg_clangd.py` 脚本，生成：

- `.clangd` 文件
- `clangd_fallback_flags.json` 文件

你只需要将 `clangd_fallback_flags.json` 的内容复制到 VS Code 的 clangd 扩展的 `fallbackFlags` 设置中即可。

#### ③ 配置 CMake 构建系统

在项目的 **`cmake/`** 目录下，创建 **`private.cmake`** 文件，内部应该创建一个名为 **`__ARM_TOOLCHAIN_PATH__`** 的变量，此变量为指向你的工具链根目录的字符串。一个文件可能长这样：
```cmake
set(__ARM_TOOLCHAIN_PATH__ "F:/arm/mingw-w64-arm-none-eabi-15.2/")
```
> ⚠️ **注意**：此路径应该全局统一，也必须和 `clangd_config.json` 文件内的工具链路径保持一致。  

之后就可以开始正常配置了：

- 若使用 **VS Code**：只需设置根目录并选择工具链。
- 其他平台：遵循常规 CMake 构建流程。

#### ④ 编译项目

添加你自己的代码和目标，然后编译。

#### ⑤ 烧录到目标设备

选择你喜欢的烧录方式（如 J-Link、ST-Link 等）。

---

## 📝 编码规范

代码格式由 `{root}/.clang-format` 文件定义。以下是项目的核心编码约定。

### 1. 命名约定

| 类型         | 规范                       | 示例                                                      |
| ------------ | -------------------------- | --------------------------------------------------------- |
| 命名空间     | `snake_case`               | `strat_os::hal::traits`                                   |
| 类型/类      | `PascalCase`               | `class BitMap {};`                                        |
| 函数         | `snake_case`               | `void set_vector_table() noexcept;`                       |
| 变量         | `snake_case`               | `uint32_t task_count;`<br>`ErrorHandler error_handler_;`  |
| 常量         | `UPPER_SNAKE_CASE`         | `constexpr std::size_t OS_RESERVED_ERROR_HANDLE_COUNT{32};` |
| 枚举类名     | `PascalCase`               | `enum class ErrorCode {};`                                |
| 枚举值       | `camelCase`（首字母小写）   | `ErrorCode::outOfBounds`                                  |
| 模板参数     | `PascalCase`               | `template <typename AllocPolicy>`                         |

### 2. 注释规范

- 使用 **Doxygen** 风格：`/** ... */`
- 必须包含的标签：`@file`, `@author`, `@brief`, `@param`, `@return`, `@tparam` 等
- 函数、类、模板都需要详细注释
- 复杂逻辑应有内联注释

```cpp
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
[[nodiscard]] inline static value_type load(volatile value_type* ptr) noexcept;
```

### 3. 代码风格

- 使用 `[[nodiscard]]` 标记有返回值的函数
- 使用 `inline` 和 `noexcept` 提高性能
- 使用 `static_assert` 进行编译期检查
- 优先使用模板和类型萃取实现编译期多态
- **禁止 RTTI 和异常**
- 头文件保护统一使用 `#pragma once`
- 命名空间嵌套使用缩进
- 即使只有一行代码，也使用大括号 `{}`
- 操作符前后加空格
- 函数参数换行时对齐

```cpp
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
```

### 4. 宏使用

- **仅用于包含守卫和必要的系统级定义**
- 全大写 + 下划线分隔
- **避免滥用宏**：优先使用 `constexpr` 和模板

```cpp
// 可以接受（包含守卫）
#ifndef STRATOS_HAL_ATOMIC_HPP
#define STRATOS_HAL_ATOMIC_HPP
// ...
#endif

// 错误示例
#define MAX_SIZE 1024
```

### 5. 错误处理

- 使用自定义错误处理器（需满足特定接口）
- 越界等错误必须调用错误处理器
- 提供清晰的错误信息

```cpp
template <typename ErrorHandler>
class BitMap {
public:
    void set(std::size_t pos) {
        if (pos >= size) {
            ErrorHandler::handle_error(ErrorCode::outOfBounds);
            return;
        }
        // 正常实现
    }
};
```

### 6. 性能优化

- 使用 `inline` 内联函数
- 使用 `constexpr` 编译期常量
- 利用编译器内置函数（如 `__builtin_popcount`, `__builtin_ctz`）
- 零开销抽象：模板 + 静态多态
- **避免动态内存分配**：使用静态内存或内存池
- 提供安全版本（带检查）和不安全版本（高性能）

```cpp
constexpr std::size_t BITS_PER_WORD{sizeof(std::size_t) * 8};

[[nodiscard]] inline std::size_t find_first_set(std::size_t word) noexcept {
    return __builtin_ctz(word);
}

// 安全版本
void set(std::size_t pos) {
    if (pos >= size) {
        ErrorHandler::handle_error(ErrorCode::outOfBounds);
        return;
    }
    unsafe_set(pos);
}

// 不安全版本（高性能）
void unsafe_set(std::size_t pos) noexcept { /* 实现 */ }
```

### 7. 安全性

- **安全第一**
- 使用 RAII 管理资源
- 避免未定义行为
- 执行适当的边界检查
- 禁止拷贝和移动（确保唯一所有权）
- **零初始化保证**：默认构造时初始化所有成员

```cpp
class BitMap {
public:
    BitMap() noexcept = default;

    // 禁止拷贝和移动
    BitMap(const BitMap&) = delete;
    BitMap& operator=(const BitMap&) = delete;
    BitMap(BitMap&&) = delete;
    BitMap& operator=(BitMap&&) = delete;

private:
    std::size_t blocks_{};   // 零初始化
};
```

### 8. 文件命名

| 文件类型     | 扩展名  | 示例                     |
| ------------ | ------- | ------------------------ |
| 头文件       | `.hpp`  | `atomic.hpp`             |
| 源文件       | `.cpp`  | `atomic.cpp`             |
| 汇编文件     | `.s`    | `startup.s`              |
| 模板实现分离 | `.ipp`  | `atomic_impl.ipp`（可选） |

### 9. 代码组织

- 按功能模块组织
- 头文件包含顺序：**标准库 → 第三方库 → 项目内部头文件**
- 命名空间层次清晰，避免冲突
- 使用类型萃取和 SFINAE 实现编译期多态

```cpp
#include <cstddef>
#include <type_traits>

#include "mu_sstl/errors/basic_error.hpp"
#include "strat_os/hal/traits/system_traits.hpp"

namespace strat_os {
namespace hal {
    // 代码
} // namespace hal
} // namespace strat_os
```

### 10. 最佳实践

- **编译期多态优先于运行时多态**
- 使用 `std::conjunction` 等类型特性进行编译期检查
- 提供详细的静态断言错误信息
- 利用模板特化和偏特化实现平台适配
- 保持接口一致性，便于扩展

```cpp
template <typename T, typename = void>
struct is_atomic : std::false_type {};

template <typename T>
struct is_atomic<T, std::void_t<decltype(T::load()), decltype(T::store(std::declval<T>()))>>
    : std::true_type {};

template <typename T>
void process_atomic(T& atomic) {
    static_assert(is_atomic<T>::value,
                  "T must be an atomic type with load() and store() methods");
    // 实现
}
```

---

## 📂 项目结构

StratOS 采用模块化设计，主要目录如下：

| 目录           | 功能描述                                   |
| -------------- | ------------------------------------------ |
| `LICENSES/`    | 项目许可证文件                             |
| `cmake/`       | CMake 配置文件                             |
| `docs/`        | 项目文档                                   |
| `examples/`    | 示例代码                                   |
| `libraries/`   | 依赖库（MUSSTL、stm32SL）                  |
| `os_boost/`    | 启动代码（不同平台的初始化）               |
| `os_hal/`      | 硬件抽象层（底层实现，支持 Cortex-M3/M4）  |
| `os_kernel/`   | 内核实现（任务调度、内存管理、队列等）     |
| `os_linker/`   | 链接脚本（不同平台的内存布局）             |
| `os_service/`  | 系统服务                                   |
| `scripts/`     | 辅助脚本                                   |
| `tools/`       | 工具（性能分析、测试、验证）               |
| `user/`        | 用户应用代码                               |

### 核心模块说明

#### `os_hal` (硬件抽象层)
- 提供不同架构的底层实现
- 支持 Cortex-M3、Cortex-M4
- 包含原子操作、上下文切换、中断处理等

#### `os_kernel` (内核)
- RTOS 核心功能
- 任务调度、内存管理、队列、信号量、定时器
- 基于现代 C++ 模板策略设计模式

#### `libraries`
- **MUSSTL**：自定义 STL 实现（容器、工具类）
- **stm32SL**：STM32 标准库（CMSIS + 外设驱动）

#### `user`
- 用户应用代码示例
- 主程序入口、设备驱动示例

---

## ✨ 技术特性

1. **现代 C++ 特性**：模板、静态多态等
2. **零运行时开销**：编译期完成大部分检查和决策
3. **类型安全**：强类型设计，减少运行时错误
4. **可扩展性**：基于策略模式的设计，便于扩展与定制
5. **跨平台支持**：支持不同架构处理器

---

## 📌 示例代码

项目提供了丰富的示例代码，位于 [`examples/`](./examples) 目录下，展示如何使用 StratOS 的各种功能。

---

## 📚 文档

详细文档位于 [`docs/`](./docs) 目录下，包含使用说明和 API 参考。

---

## ⚖️ 许可证

StratOS 采用 **LGPL v3.0** 许可证，详见 [`LICENSES/`](./LICENSES) 目录下的文件。
