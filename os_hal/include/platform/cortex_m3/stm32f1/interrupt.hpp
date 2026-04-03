/**
 * @file interrupt.hpp
 * @author StratOS Team
 * @brief 内置 Cortex-M3 中断控制器策略
 * @version 1.0.0
 * @date 2026-04-01
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件提供了针对 ARM Cortex-M3 内核的中断控制器策略实现，
 * 属于官方内置策略（builtin）。它基于 CMSIS 标准接口，
 * 实现了中断使能/禁用、优先级管理、软件触发、全局中断控制
 * 以及可选的增强功能（中断状态查询、优先级分组等）。
 *
 * 该策略满足 strat_os::hal::traits::is_valid_interrupt_controller
 * 的所有要求，可直接与 strat_os::hal::InterruptController 适配器配合使用。
 *
 * @note 此策略依赖 STM32F10x 标准外设库和 CMSIS-CORE。
 * @warning 所有方法均为 noexcept，适合在实时环境中使用。
 */
#pragma once

#ifndef STRATOS_HAL_POLICY_CORTEX_M3_STM32F1XX_INTERRUPT_HPP
#define STRATOS_HAL_POLICY_CORTEX_M3_STM32F1XX_INTERRUPT_HPP

#include "stm32f10x.h" // for CMSIS
#include <cstdint>     // for std::uint32_t, std::uint8_t, std::int32_t

#ifndef __CM3_CORE_H__
#include "core_cm3.h" // Cortex-M3 内核支持（通常已由 stm32f10x.h 包含）
#endif

namespace strat_os::hal::policy::builtin
{

/**
 * @brief Cortex-M3 中断控制器内置策略
 *
 * 该结构体封装了 Cortex-M3 NVIC 的所有中断控制操作，
 * 提供与 strat_os::hal::InterruptController 适配器完全兼容的静态接口。
 * 所有方法均为内联且 noexcept，保证零开销抽象。
 */
struct CortexM3Stm32F1InterruptControllerPolicy {
    /// 中断号类型，直接使用 CMSIS 定义的 IRQn_Type 枚举
    using IRQn_Type = ::IRQn_Type;
    /// 优先级分组类型（写入 SCB->AIRCR 的值）
    using priority_group_type = std::uint32_t;
    /// 优先级类型（Cortex-M3 使用 4 位优先级，取值范围 0-15）
    using priority_type = std::uint8_t;

    // ========== 必需方法 ==========
    // 以下方法为中断控制器接口的核心功能，所有平台必须实现

    /**
     * @brief 使能指定中断
     * @param irq 中断号（由 CMSIS 定义，如 USART1_IRQn）
     */
    inline static void enable(IRQn_Type irq) noexcept {
        NVIC_EnableIRQ(irq);
    }

    /**
     * @brief 禁用指定中断
     * @param irq 中断号
     */
    inline static void disable(IRQn_Type irq) noexcept {
        NVIC_DisableIRQ(irq);
    }

    /**
     * @brief 设置中断优先级
     * @param irq      中断号
     * @param priority 优先级值（0 为最高，15 为最低，仅低 4 位有效）
     * @note 优先级分组会影响抢占优先级与子优先级的划分，
     *       实际有效位由 set_priority_grouping() 配置。
     */
    inline static void set_priority(IRQn_Type irq, priority_type priority) noexcept {
        NVIC_SetPriority(irq, priority);
    }

    /**
     * @brief 获取中断优先级
     * @param irq 中断号
     * @return 当前优先级值（仅低 4 位有效）
     */
    inline static priority_type get_priority(IRQn_Type irq) noexcept {
        return static_cast<priority_type>(NVIC_GetPriority(irq));
    }

    /**
     * @brief 触发软件中断（设置挂起位）
     * @param irq 中断号
     * @note 可用于实现 PendSV 或任务切换请求。
     */
    inline static void trigger_software(IRQn_Type irq) noexcept {
        NVIC_SetPendingIRQ(irq);
    }

    /**
     * @brief 全局使能所有可屏蔽中断
     * @note 对应 ARM 指令 CPSIE i，清除 PRIMASK 寄存器。
     */
    inline static void global_enable() noexcept {
        __enable_irq();
    }

    /**
     * @brief 全局禁用所有可屏蔽中断
     * @note 对应 ARM 指令 CPSID i，设置 PRIMASK 寄存器。
     */
    inline static void global_disable() noexcept {
        __disable_irq();
    }

    // ========== 可选增强方法 ==========
    // 以下方法为扩展功能，通过 SFINAE 自动暴露，非必需但提供更丰富的接口

    /**
     * @brief 判断当前是否在中断服务程序中
     * @return true 表示当前在中断/异常上下文中，false 表示在任务（线程）模式
     * @note 通过读取 IPSR（中断状态寄存器）实现，非零值表示处于异常状态。
     */
    inline static bool in_isr() noexcept {
        std::uint32_t ipsr;
        __asm volatile("mrs %0, ipsr" : "=r"(ipsr));
        return (ipsr != 0);
    }

    /**
     * @brief 获取当前正在执行的中断号
     * @return 当前异常编号（若为 0 表示线程模式，非 0 表示正在处理的异常/中断号）
     * @warning 返回值不与 CMSIS 的 IRQn_Type 兼容。
     */
    inline static std::uint32_t get_current_irq() noexcept {
        std::uint32_t ipsr;
        __asm volatile("mrs %0, ipsr" : "=r"(ipsr));
        return ipsr & 0xFF;
    }

    /**
     * @brief 设置中断优先级分组
     * @param group 优先级分组值（0-7），决定抢占优先级与子优先级的划分。
     * @note 对应 SCB->AIRCR 的 PRIGROUP 字段。
     *       分组值越低，抢占优先级位数越多，具体映射关系见 ARM 文档。
     */
    inline static void set_priority_grouping(priority_group_type group) noexcept {
        NVIC_SetPriorityGrouping(group);
    }

    /**
     * @brief 获取当前中断优先级分组
     * @return 当前的优先级分组值（0-7）
     */
    inline static priority_group_type get_priority_grouping() noexcept {
        return static_cast<priority_group_type>(NVIC_GetPriorityGrouping());
    }
};

} // namespace strat_os::hal::policy::builtin

#endif // STRATOS_HAL_POLICY_CORTEX_M3_STM32F1XX_INTERRUPT_HPP