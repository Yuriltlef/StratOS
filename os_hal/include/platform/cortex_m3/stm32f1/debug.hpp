/**
 * @file debug.hpp
 * @author StratOS Team
 * @brief 内置 Cortex-M3 STM32F1xx 调试策略
 * @version 1.1.0
 * @date 2026-04-03
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件提供了针对 STM32F1xx 系列（Cortex-M3）的调试策略实现，
 * 属于官方内置策略（builtin）。它基于 CMSIS 内核调试组件，
 * 实现了硬件断点、周期计数器（DWT）以及可选的 ITM 字符输出。
 *
 * 该策略满足 strat_os::hal::traits::is_valid_debug_policy
 * 的所有要求，可直接与 strat_os::hal::Debug 适配器配合使用。
 *
 * @note 周期计数器（CYCCNT）需要先使能 DWT 单元，并在系统启动时调用
 *       enable_cycle_counter() 后才开始计数。
 * @warning ITM 输出需要调试器（如 ST-Link）连接 SWO 引脚并配置合适的
 *          时钟和波特率，否则 send_char 可能无效。
 */
#pragma once

#ifndef STRATOS_POLICY_CORTEX_M3_STM32F1_DEBUG_HPP
#define STRATOS_POLICY_CORTEX_M3_STM32F1_DEBUG_HPP

#include "stm32f10x.h" // for core_cm3.h indirectly
#include <cstddef>     // for std::size_t, std::byte
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
 * @brief STM32F1xx 调试策略（Cortex-M3）
 *
 * 提供硬件断点（BKPT）、周期计数器（DWT）和 ITM 字符输出。
 * 周期计数器需要调用 enable_cycle_counter() 才能开始计数。
 */
struct CortexM3Stm32F1DebugPolicy {
    /// 周期计数器的类型（32 位无符号整数）
    using cycle_counter_size_type = std::uint32_t;

    // ========== 寄存器地址常量（参考 CMSIS 标准） ==========
    /// CoreDebug->DEMCR 寄存器地址
    static constexpr std::uint32_t DEMCR_ADDR = 0xE000EDFCUL;
    /// DWT->CTRL 寄存器地址
    static constexpr std::uint32_t DWT_CTRL_ADDR = 0xE0001000UL;
    /// DWT->CYCCNT 寄存器地址
    static constexpr std::uint32_t DWT_CYCCNT_ADDR = 0xE0001004UL;

    // ========== 位掩码常量 ==========
    /// CoreDebug_DEMCR 的 TRCENA 位（位 24）
    static constexpr std::uint32_t TRCENA_MASK = (1UL << 24);
    /// DWT_CTRL 的 CYCCNTENA 位（位 0）
    static constexpr std::uint32_t CYCCNTENA_MASK = 1UL;

    // ========== 必需方法 ==========

    /**
     * @brief 插入硬件断点
     * @note 执行 BKPT #0 指令，会触发调试监控（如果连接调试器则暂停）。
     */
    inline static void bkpt() noexcept {
        __asm volatile("bkpt #0"); ///< 内联汇编是访问 BKPT 指令的唯一方式
    }

    /**
     * @brief 使能周期计数器（DWT->CYCCNT）
     * @note 需要先使能 DWT 单元的跟踪接口（DEMCR.TRCENA = 1）。
     *       调用后 CYCCNT 开始累加处理器周期。
     * @par MISRA 例外：
     *      MISRA C++ 2023 Rule 5-2-6 建议避免 reinterpret_cast，但在嵌入式
     *      硬件寄存器访问中，将固定地址转换为 volatile 指针是标准做法。
     *      所有地址均为 constexpr 常量，且操作在受控环境下进行，不会引入未定义行为。
     */
    inline static void enable_cycle_counter() noexcept {
        /// 使能 DWT 跟踪单元
        volatile std::uint32_t* demcr = reinterpret_cast<volatile std::uint32_t*>(DEMCR_ADDR);
        *demcr |= TRCENA_MASK;

        /// 复位 CYCCNT
        volatile std::uint32_t* cyccnt = reinterpret_cast<volatile std::uint32_t*>(DWT_CYCCNT_ADDR);
        *cyccnt                        = 0;

        /// 使能 CYCCNT
        volatile std::uint32_t* dwt_ctrl = reinterpret_cast<volatile std::uint32_t*>(DWT_CTRL_ADDR);
        *dwt_ctrl |= CYCCNTENA_MASK;
    }

    /**
     * @brief 禁用周期计数器
     * @note 清除 DWT->CTRL 中的 CYCCNTENA 位。
     * @par MISRA 例外：
     *      MISRA C++ 2023 Rule 5-2-6 建议避免 reinterpret_cast，但在嵌入式
     *      硬件寄存器访问中，将固定地址转换为 volatile 指针是标准做法。
     *      所有地址均为 constexpr 常量，且操作在受控环境下进行，不会引入未定义行为。
     */
    inline static void disable_cycle_counter() noexcept {
        volatile std::uint32_t* dwt_ctrl = reinterpret_cast<volatile std::uint32_t*>(DWT_CTRL_ADDR);
        *dwt_ctrl &= ~CYCCNTENA_MASK;
    }

    /**
     * @brief 获取当前周期计数值
     * @return CYCCNT 寄存器值（32 位，溢出后自动回绕）
     * @par MISRA 例外：
     *      MISRA C++ 2023 Rule 5-2-6 建议避免 reinterpret_cast，但在嵌入式
     *      硬件寄存器访问中，将固定地址转换为 volatile 指针是标准做法。
     *      所有地址均为 constexpr 常量，且操作在受控环境下进行，不会引入未定义行为。
     */
    inline static cycle_counter_size_type get_cycle_counter() noexcept {
        volatile std::uint32_t* cyccnt = reinterpret_cast<volatile std::uint32_t*>(DWT_CYCCNT_ADDR);
        return static_cast<cycle_counter_size_type>(*cyccnt);
    }

    /**
     * @brief 查询周期计数器是否已使能
     * @return true 如果 CYCCNTENA 位被设置，否则 false
     * @par MISRA 例外：
     *      MISRA C++ 2023 Rule 5-2-6 建议避免 reinterpret_cast，但在嵌入式
     *      硬件寄存器访问中，将固定地址转换为 volatile 指针是标准做法。
     *      所有地址均为 constexpr 常量，且操作在受控环境下进行，不会引入未定义行为。
     */
    inline static bool is_cycle_counter_enabled() noexcept {
        volatile std::uint32_t* dwt_ctrl = reinterpret_cast<volatile std::uint32_t*>(DWT_CTRL_ADDR);
        return (*dwt_ctrl & CYCCNTENA_MASK) != 0;
    }

    // ========== 可选方法（ITM 软件跟踪） ==========

    /**
     * @brief 通过 ITM 发送一个字符（单字节）
     * @param c 要发送的字符
     * @note 仅当 ITM 端口 0 使能且调试器连接时才有效。
     */
    static void send_char(char c) noexcept {
        ITM_SendChar(static_cast<std::uint32_t>(c));
    }

    /// ITM 块传输使用的大小类型（32 位无符号整数）
    using size_type = std::uint32_t;

    /**
     * @brief 通过 ITM 发送一块数据
     * @param data 数据指针
     * @param size 字节数
     * @note 逐个字节发送，效率较低，但简单可靠。
     */
    static void send_block(const std::byte* data, size_type size) noexcept {
        for (size_type i = 0; i < size; ++i) {
            ITM_SendChar(static_cast<std::uint32_t>(data[i]));
        }
    }

    /**
     * @brief 查询 ITM 是否就绪（端口 0 可发送）
     * @return true 如果端口 0 空闲，否则 false
     */
    static bool is_ready() noexcept {
        return (ITM->PORT[0].u32 & 1) != 0;
    }
};

} // namespace strat_os::hal::policy::builtin

#endif // STRATOS_POLICY_CORTEX_M3_STM32F1_DEBUG_HPP