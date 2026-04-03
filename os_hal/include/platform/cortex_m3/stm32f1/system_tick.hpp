/**
 * @file system_tick.hpp
 * @author StratOS Team
 * @brief 内置 Cortex-M3 系统节拍定时器（SysTick）策略
 * @version 1.0.0
 * @date 2026-04-02
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件提供了针对 ARM Cortex-M3 内核的 SysTick 定时器策略实现，
 * 属于官方内置策略（builtin）。它基于 CMSIS 标准接口，
 * 实现了初始化、使能/禁用、中断控制、计数值读取和溢出标志检查。
 *
 * 时钟源可选处理器时钟（AHB）或 AHB/8，通过枚举配置。
 * 所有方法均为内联且 noexcept，保证零开销抽象。
 */
#pragma once

#ifndef STRATOS_HAL_POLICY_CORTEX_M3_STM32F1XX_SYSTEM_TICK_HPP
#define STRATOS_HAL_POLICY_CORTEX_M3_STM32F1XX_SYSTEM_TICK_HPP

#include "stm32f10x.h" // for CMSIS
#include <cstdint>     // for uint32_t

#ifndef __CM3_CORE_H__
#include "core_cm3.h"
#endif

namespace
{
// 避免 clangd 对未使用 stm32f10x.h 的警告
using IQRN_TYPE_KEEP_FOR_NO_CLANGD_WARNING_AND_CMSIS = ::IRQn_Type;
} // namespace

namespace strat_os::hal::policy::builtin
{

/**
 * @brief Cortex-M3 SysTick 时钟源枚举
 *
 * 用于选择 SysTick 定时器的时钟源：
 * - AHBClock：处理器时钟（AHB 时钟频率）
 * - AHBDiv8：处理器时钟的 8 分频（AHB/8）
 */
enum class CortexM3Stm32F1SystemTickClockSource : std::uint8_t {
    AHBClock = 1, ///< AHB 时钟（对应 CLKSOURCE 位置 1）
    AHBDiv8  = 0  ///< AHB/8 时钟（对应 CLKSOURCE 位清 0）
};

/**
 * @brief Cortex-M3 系统节拍定时器内置策略
 *
 * 该结构体封装了 Cortex-M3 SysTick 定时器的所有操作，
 * 提供与 strat_os::hal::SystemTick 适配器完全兼容的静态接口。
 * 所有方法均为内联且 noexcept，保证零开销抽象。
 */
struct CortexM3Stm32F1SystemTickPolicy {
    /// 重装载值类型（24 位有效，使用 uint32_t 存储）
    using reload_type = std::uint32_t;
    /// 时钟源类型（使用上述枚举）
    using clock_source_type = CortexM3Stm32F1SystemTickClockSource;

    /**
     * @brief 初始化 SysTick 定时器
     * @param reload_value 重装载值（仅低 24 位有效）
     * @param clock_source 时钟源（AHBClock 或 AHBDiv8）对于stm32f10x系列，FCLK来自AHB
     * @note 此函数配置重装载寄存器和时钟源，并清零当前计数器，
     *       但不使能定时器或中断，需单独调用 enable() 和 enable_irq()。
     */
    inline static void init(reload_type reload_value, clock_source_type clock_source) noexcept {
        /// 先禁用定时器和中断（清除 ENABLE 和 TICKINT 位）
        SysTick->CTRL &= ~(SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk);
        /// 配置时钟源
        if (clock_source == clock_source_type::AHBClock) {
            SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk; // 使用 AHB 时钟
        } else {
            SysTick->CTRL &= ~SysTick_CTRL_CLKSOURCE_Msk; // 使用 AHB/8
        }
        /// 设置重装载值（仅低 24 位有效，硬件自动忽略高位）
        SysTick->LOAD = reload_value & 0xFFFFFF;
        /// 清空当前计数值
        SysTick->VAL = 0;
    }

    /**
     * @brief 使能 SysTick 计数器
     */
    inline static void enable() noexcept {
        SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
    }

    /**
     * @brief 禁用 SysTick 计数器
     */
    inline static void disable() noexcept {
        SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    }

    /**
     * @brief 使能 SysTick 中断
     * @note 计数器减到 0 时触发 SysTick 异常
     */
    inline static void enable_irq() noexcept {
        SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
    }

    /**
     * @brief 禁用 SysTick 中断
     */
    inline static void disable_irq() noexcept {
        SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
    }

    /**
     * @brief 获取当前计数值
     * @return 当前递减计数值（仅低 24 位有效）
     */
    inline static reload_type get_value() noexcept {
        return static_cast<reload_type>(SysTick->VAL & 0xFFFFFF);
    }

    /**
     * @brief 检查是否发生溢出
     * @return true 如果计数器从 1 减到 0（即发生溢出），否则 false
     * @note 读取该标志后，硬件会自动清零。
     */
    inline static bool is_overflow() noexcept {
        return (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) != 0;
    }
};

} // namespace strat_os::hal::policy::builtin

#endif // STRATOS_HAL_POLICY_CORTEX_M3_STM32F1XX_SYSTEM_TICK_HPP