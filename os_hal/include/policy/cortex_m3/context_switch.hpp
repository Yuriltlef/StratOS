/**
 * @file context_switch.hpp
 * @author StratOS Team
 * @brief 内置 Cortex-M3 上下文切换策略
 * @version 1.0.0
 * @date 2026-04-01
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件提供了针对 ARM Cortex-M3 内核的上下文切换策略实现，
 * 属于官方内置策略（builtin）。它基于 CMSIS 标准接口，
 * 实现了触发 PendSV、读写 PSP/MSP、切换特权级、内存屏障
 * 以及获取当前异常编号等操作。
 *
 * 该策略满足 strat_os::hal::traits::is_valid_context_switch_policy
 * 的所有要求，可直接与 strat_os::hal::ContextSwitch 适配器配合使用。
 *
 * @note 所有方法均为内联且 noexcept，保证零开销抽象。
 * @note get_current_exception() 为可选扩展方法，返回当前异常编号
 *       （0 表示线程模式）。
 * @warning 多核相关方法（core_id、send_ipi）未实现，因为 Cortex-M3
 *          为单核处理器。SFINAE 会屏蔽这些接口，避免误用。
 */
#pragma once

#ifndef STRATOS_HAL_POLICY_CORTEX_M3_CONTEXT_SWITCH_HPP
#define STRATOS_HAL_POLICY_CORTEX_M3_CONTEXT_SWITCH_HPP

#include "stm32f10x.h" // for SCB, IRQn_Type (not used directly)
#include "core_cm3.h"  // for __get_IPSR(), __get_MSP, __set_MSP, etc.
#include <cstdint>     // for uint32_t

namespace
{
///@warning 仅用于避免 clangd 对未使用 stm32f10x.h 的警告（实际不使用）
using IQRN_TYPE_KEEP_FOR_NO_CLANGD_WARNING_AND_CMSIS = ::IRQn_Type;
};

namespace strat_os::hal::policy::builtin {

/**
 * @brief Cortex-M3 上下文切换内置策略
 *
 * 该结构体封装了 Cortex-M3 上任务切换所需的所有硬件操作，
 * 包括触发 PendSV、栈指针管理、特权级切换和内存屏障。
 * 所有方法均为内联且 noexcept，保证零开销抽象。
 */
struct CortexM3ContextSwitchPolicy {
    /// 栈指针基本类型（32 位无符号整数）
    using word = std::uint32_t;

    // ========== 必需方法 ==========

    /**
     * @brief 触发 PendSV 异常，用于延迟任务切换
     * @note 通过写 SCB->ICSR 的 PENDSVSET 位实现。
     */
    static void trigger_pendsv() noexcept {
        SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
    }

    /**
     * @brief 设置进程栈指针（PSP）
     * @param psp 栈指针值
     */
    static void set_psp(word psp) noexcept {
        __set_PSP(psp);
    }

    /**
     * @brief 获取当前进程栈指针（PSP）
     * @return 当前 PSP 值
     */
    static word get_psp() noexcept {
        return __get_PSP();
    }

    /**
     * @brief 设置主栈指针（MSP）
     * @param msp 栈指针值
     */
    static void set_msp(word msp) noexcept {
        __set_MSP(msp);
    }

    /**
     * @brief 获取当前主栈指针（MSP）
     * @return 当前 MSP 值
     */
    static word get_msp() noexcept {
        return __get_MSP();
    }

    /**
     * @brief 切换到用户模式（非特权级）
     * @note 设置 CONTROL 寄存器的第 0 位，并执行 ISB 指令同步。
     */
    static void switch_to_unprivileged() noexcept {
        __set_CONTROL(__get_CONTROL() | 0x1);
        __ISB();
    }

    /**
     * @brief 切换到特权模式
     * @note 清除 CONTROL 寄存器的第 0 位，并执行 ISB 指令同步。
     */
    static void switch_to_privileged() noexcept {
        __set_CONTROL(__get_CONTROL() & ~0x1);
        __ISB();
    }

    /**
     * @brief 数据内存屏障（DMB）
     */
    static void dmb() noexcept {
        __DMB();
    }

    /**
     * @brief 数据同步屏障（DSB）
     */
    static void dsb() noexcept {
        __DSB();
    }

    /**
     * @brief 指令同步屏障（ISB）
     */
    static void isb() noexcept {
        __ISB();
    }

    // ========== 可选扩展方法 ==========

    /**
     * @brief 获取当前异常编号
     * @return 异常编号（0 表示线程模式，非 0 表示正在处理的中断/异常号）
     * @note 通过读取 IPSR 寄存器实现，仅低 8 位有效。
     *       此方法可用于判断当前是否在中断服务程序中。
     */
    static word get_current_exception() noexcept {
        word ipsr;
        __asm volatile("mrs %0, ipsr" : "=r"(ipsr));
        return static_cast<word>(ipsr & 0xFF);
    }

    // 注意：多核相关方法（core_id、send_ipi）不在此策略中实现，
    // 因为 Cortex-M3 为单核处理器。SFINAE 会自动屏蔽这些接口。
};

} // namespace strat_os::hal::policy::builtin

#endif // STRATOS_HAL_POLICY_CORTEX_M3_CONTEXT_SWITCH_HPP