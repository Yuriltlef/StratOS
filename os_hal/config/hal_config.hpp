/**
 * @file hal_config.hpp
 * @author StratOS Team
 * @brief 硬件抽象层（HAL）策略配置聚合器
 * @version 1.0.0
 * @date 2026-06-07
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件根据编译时定义的芯片型号（如 STM32F10X_MD）选择对应的 HAL 策略类。
 * 它提供了一个统一的配置结构体 `OsHalConfig`，其中包含所有 HAL 组件的策略类型别名。
 *
 * 使用方式：
 * - 在构建系统中定义目标芯片宏（例如 `-DSTM32F10X_MD`）。
 * - 包含本头文件后，通过 `strat_os::hal::config::OsHalConfig` 即可获得当前平台下的
 *   原子操作、上下文切换、调试、MPU、平台上下文、系统控制、系统节拍、中断控制器等策略类型。
 *
 * @note 对于未支持的平台，策略类型会被定义为 `void`，使用时需通过 `std::conditional_t` 检测。
 */

#pragma once

#ifndef STRATOS_HAL_CONFIG_HPP
#define STRATOS_HAL_CONFIG_HPP

#include "os_hal/include/platform/cortex_m3/stm32f1/atomic.hpp"
#include "os_hal/include/platform/cortex_m3/stm32f1/context_switch.hpp"
#include "os_hal/include/platform/cortex_m3/stm32f1/debug.hpp"
#include "os_hal/include/platform/cortex_m3/stm32f1/interrupt.hpp"
#include "os_hal/include/platform/cortex_m3/stm32f1/mpu.hpp"
#include "os_hal/include/platform/cortex_m3/stm32f1/platform_context.hpp"
#include "os_hal/include/platform/cortex_m3/stm32f1/system_control.hpp"
#include "os_hal/include/platform/cortex_m3/stm32f1/system_tick.hpp"
#include <type_traits>

namespace strat_os::hal::config
{

/**
 * @brief 检测是否为 STM32F10x 中等密度系列芯片
 * @details 该常量由编译器预定义宏 `STM32F10X_MD` 决定，若未定义则为 false。
 */
#ifdef STM32F10X_MD
constexpr bool stm32_f10x_md = true;
#else
constexpr bool stm32_f10x_md = false;
#endif

/// 命名空间别名，简化内置策略的访问
namespace os_bp_ = strat_os::hal::policy::builtin;

/**
 * @brief HAL 策略配置结构体
 *
 * 根据 `stm32_f10x_md` 常量选择对应的策略类型。若当前平台不支持某组件，
 * 则对应类型别名为 `void`（使用时可通过 `std::is_same_v` 检测）。
 */
struct OsHalConfig {
    /**
     * @brief 原子操作策略类型
     * @note 当前仅支持 STM32F10x MD 平台。
     */
    using OsAtomicPolicy = std::conditional_t<stm32_f10x_md, os_bp_::CortexM3Stm32F1AtomicPolicy, void>;

    /**
     * @brief 上下文切换策略类型
     * @note 当前仅支持 STM32F10x MD 平台。
     */
    using OsContextSwitchPolicy = std::conditional_t<stm32_f10x_md, os_bp_::CortexM3Stm32F1ContextSwitchPolicy, void>;

    /**
     * @brief 调试策略类型
     * @note 当前仅支持 STM32F10x MD 平台。
     */
    using OsDebugPolicy = std::conditional_t<stm32_f10x_md, os_bp_::CortexM3Stm32F1DebugPolicy, void>;

    /**
     * @brief MPU（内存保护单元）策略类型
     * @note 当前仅支持 STM32F10x MD 平台。
     */
    using OsMPUPolicy = std::conditional_t<stm32_f10x_md, os_bp_::CortexM3Stm32F1MPUPolicy, void>;

    /**
     * @brief 平台上下文策略类型（用于 TCB 扩展）
     * @note 当前仅支持 STM32F10x MD 平台。
     */
    using OsPlatformContextPolicy =
        std::conditional_t<stm32_f10x_md, os_bp_::CortexM3Stm32F1PlatformContextPolicy, void>;

    /**
     * @brief 系统控制策略类型（休眠、复位等）
     * @note 当前仅支持 STM32F10x MD 平台。
     */
    using OsSystemControlPolicy = std::conditional_t<stm32_f10x_md, os_bp_::CortexM3Stm32F1SystemControlPolicy, void>;

    /**
     * @brief 系统节拍定时器策略类型（SysTick）
     * @note 当前仅支持 STM32F10x MD 平台。
     */
    using OsSystemTickPolicy = std::conditional_t<stm32_f10x_md, os_bp_::CortexM3Stm32F1SystemTickPolicy, void>;

    /**
     * @brief 中断控制器策略类型
     * @note 当前仅支持 STM32F10x MD 平台。
     */
    using OsInterruptCtrlPolicy =
        std::conditional_t<stm32_f10x_md, os_bp_::CortexM3Stm32F1InterruptControllerPolicy, void>;
};

} // namespace strat_os::hal::config

#endif // STRATOS_HAL_CONFIG_HPP