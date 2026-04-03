/**
 * @file system_control.hpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2026-04-03
 *
 * @copyright Copyright (c) 2026
 *
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
struct CortexM3Stm32F1SystemControlPolicy {
    using exception_type      = ::IRQn_Type;
    using priority_type       = std::uint8_t;
    using address_type        = std::uint32_t;
    using priority_group_type = std::uint32_t;
    using fault_mask_type     = std::uint32_t;

    inline static void system_reset() noexcept {
        NVIC_SystemReset();
    }

    inline static void set_vector_table(address_type base_address) noexcept {
        SCB->VTOR = base_address;
    }

    static void set_priority_grouping(priority_group_type group) noexcept {
        NVIC_SetPriorityGrouping(group);
    }

    inline static priority_group_type get_priority_grouping() noexcept {
        return static_cast<priority_group_type>(NVIC_GetPriorityGrouping());
    }

    inline static void sleep() noexcept {
        /// 清除 SLEEPDEEP 位，进入普通睡眠
        SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
        __WFI();
    }

    inline static void deep_sleep() noexcept {
        /// 设置 SLEEPDEEP 位，进入深度睡眠
        SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
        __WFI();
    }

    inline static void set_sleep_on_exit(bool enable) noexcept {
        if (enable) {
            SCB->SCR |= SCB_SCR_SLEEPONEXIT_Msk;
        } else {
            SCB->SCR &= ~SCB_SCR_SLEEPONEXIT_Msk;
        }
    }
    inline static void set_exception_priority(exception_type ex, priority_type prio) noexcept {
        NVIC_SetPriority(ex, prio);
    }

    inline static priority_type get_exception_priority(exception_type ex) noexcept {
        return static_cast<priority_type>(NVIC_GetPriority(ex));
    }

    static void enable_faults(fault_mask_type faults) noexcept {
        SCB->SHCSR |= faults;
    }

    static void disable_faults(fault_mask_type faults) noexcept {
        SCB->SHCSR &= ~faults;
    }

    static std::uint32_t get_fault_info() noexcept {
        /// 返回 CFSR 寄存器值
        return static_cast<std::uint32_t>(SCB->CFSR);
    }
};
} // namespace strat_os::hal::policy::builtin

#endif // STRATOS_POLICY_CORTEX_M3_STM32F1XX_HAL_SYSTEM_CONTROL_HPP