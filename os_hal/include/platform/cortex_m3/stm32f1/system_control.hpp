/**
 * @file system_control.hpp
 * @author StratOS Team
 * @brief 内置 Cortex-M3 STM32F1xx 系统控制策略
 * @version 1.0.0
 * @date 2026-04-03
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件提供了针对 STM32F1xx 系列（Cortex-M3）的系统控制策略实现，
 * 属于官方内置策略（builtin）。它基于 CMSIS 标准接口，
 * 实现了系统复位、向量表重定位、优先级分组、睡眠模式、
 * 异常优先级管理以及可选的故障处理。
 *
 * 该策略满足 strat_os::hal::traits::is_valid_system_control_policy
 * 的所有要求，可直接与 strat_os::hal::SystemControl 适配器配合使用。
 *
 * @note 所有方法均为内联且 noexcept，保证零开销抽象。
 * @warning 设置向量表时，基地址必须按平台要求对齐（通常为 256 字节）。
 */
#pragma once

#ifndef STRATOS_POLICY_CORTEX_M3_STM32F1XX_HAL_SYSTEM_CONTROL_HPP
#define STRATOS_POLICY_CORTEX_M3_STM32F1XX_HAL_SYSTEM_CONTROL_HPP

#include "stm32f10x.h" // for SCB, NVIC, SysTick definitions
#include <cstdint>     // for uint32_t, uint8_t

#ifndef __CM3_CORE_H__
#include "core_cm3.h"
#endif

namespace strat_os::hal::policy::builtin
{

/**
 * @brief STM32F1xx 系统控制内置策略
 *
 * 该结构体封装了 Cortex-M3 系统控制的所有操作，
 * 包括复位、向量表、优先级分组、睡眠、异常优先级及故障处理。
 * 所有方法均为内联且 noexcept，保证零开销抽象。
 */
struct CortexM3Stm32F1SystemControlPolicy {
    /// 异常编号类型（使用 CMSIS 的 IRQn_Type）
    using exception_type = ::IRQn_Type;
    /// 优先级类型（8 位无符号整数）
    using priority_type = std::uint8_t;
    /// 地址类型（32 位无符号整数）
    using address_type = std::uint32_t;
    /// 优先级分组类型（32 位无符号整数）
    using priority_group_type = std::uint32_t;
    /// 故障掩码类型（32 位无符号整数）
    using fault_mask_type = std::uint32_t;

    // ========== 必需方法 ==========

    /**
     * @brief 触发系统复位
     * @note 调用 CMSIS 函数 NVIC_SystemReset()，写入 AIRCR 寄存器。
     */
    inline static void system_reset() noexcept {
        NVIC_SystemReset();
    }

    /**
     * @brief 设置中断向量表基址
     * @param base_address 向量表起始地址（必须对齐到 256 字节）
     * @note 写 SCB->VTOR 寄存器。
     */
    inline static void set_vector_table(address_type base_address) noexcept {
        SCB->VTOR = base_address;
    }

    /**
     * @brief 设置中断优先级分组
     * @param group 优先级分组值（0~7），决定抢占优先级与子优先级的划分
     * @note 调用 CMSIS 函数 NVIC_SetPriorityGrouping()。
     */
    static void set_priority_grouping(priority_group_type group) noexcept {
        NVIC_SetPriorityGrouping(group);
    }

    /**
     * @brief 获取当前中断优先级分组
     * @return 优先级分组值（0~7）
     */
    inline static priority_group_type get_priority_grouping() noexcept {
        return static_cast<priority_group_type>(NVIC_GetPriorityGrouping());
    }

    /**
     * @brief 进入普通睡眠模式（WFI）
     * @note 清除 SLEEPDEEP 位，然后执行 WFI 指令。
     */
    inline static void sleep() noexcept {
        /// 清除 SLEEPDEEP 位，进入普通睡眠
        SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
        __WFI();
    }

    /**
     * @brief 进入深度睡眠模式（WFI）
     * @note 设置 SLEEPDEEP 位，然后执行 WFI 指令。
     *       实际功耗模式取决于电源管理单元的配置。
     */
    inline static void deep_sleep() noexcept {
        /// 设置 SLEEPDEEP 位，进入深度睡眠
        SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
        __WFI();
    }

    /**
     * @brief 设置中断返回后自动进入睡眠模式
     * @param enable true 使能 SLEEPONEXIT，false 禁用
     * @note 写 SCB->SCR 的 SLEEPONEXIT 位。
     */
    inline static void set_sleep_on_exit(bool enable) noexcept {
        if (enable) {
            SCB->SCR |= SCB_SCR_SLEEPONEXIT_Msk;
        } else {
            SCB->SCR &= ~SCB_SCR_SLEEPONEXIT_Msk;
        }
    }

    /**
     * @brief 设置系统异常优先级
     * @param ex   异常编号（如 PendSV_IRQn）
     * @param prio 优先级值（数值越小优先级越高）
     * @note 调用 CMSIS 函数 NVIC_SetPriority()，支持系统异常（负数编号）。
     */
    inline static void set_exception_priority(exception_type ex, priority_type prio) noexcept {
        NVIC_SetPriority(ex, prio);
    }

    /**
     * @brief 获取系统异常优先级
     * @param ex 异常编号
     * @return 优先级值
     */
    inline static priority_type get_exception_priority(exception_type ex) noexcept {
        return static_cast<priority_type>(NVIC_GetPriority(ex));
    }

    // ========== 可选扩展方法（故障处理） ==========

    /**
     * @brief 使能指定的可配置故障异常
     * @param faults 故障掩码（例如 SCB_SHCSR_MEMFAULTENA_Msk）
     * @note 写 SCB->SHCSR 寄存器。
     */
    static void enable_faults(fault_mask_type faults) noexcept {
        SCB->SHCSR |= faults;
    }

    /**
     * @brief 禁用指定的可配置故障异常
     * @param faults 故障掩码
     */
    static void disable_faults(fault_mask_type faults) noexcept {
        SCB->SHCSR &= ~faults;
    }

    /**
     * @brief 获取故障信息
     * @return CFSR（可配置故障状态寄存器）的值
     * @note CFSR 包含 UFSR、BFSR、MMFSR，用于诊断内存管理、总线、用法故障。
     */
    static std::uint32_t get_fault_info() noexcept {
        /// 返回 CFSR 寄存器值
        return static_cast<std::uint32_t>(SCB->CFSR);
    }
};

} // namespace strat_os::hal::policy::builtin

#endif // STRATOS_POLICY_CORTEX_M3_STM32F1XX_HAL_SYSTEM_CONTROL_HPP