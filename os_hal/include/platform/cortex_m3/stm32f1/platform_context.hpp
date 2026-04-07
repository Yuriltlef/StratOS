/**
 * @file platform_context.hpp
 * @author StratOS Team
 * @brief 内置 Cortex-M3 STM32F1xx 平台上下文策略（无硬件上下文）
 * @version 1.0.0
 * @date 2026-04-07
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件提供了针对 STM32F1xx 系列（Cortex-M3）的平台上下文策略实现，
 * 属于官方内置策略（builtin）。该平台没有额外的硬件上下文需要保存/恢复
 * （如 FPU 寄存器、MPU 配置等），因此 supports_platform_context 为 false。
 *
 * 该策略满足 strat_os::hal::traits::is_valid_platform_context_policy_v 的要求，
 * 可直接与 strat_os::hal::PlatformContext 适配器配合使用。
 *
 * @note 当 supports_platform_context 为 false 时，适配器不会暴露 save/restore 方法，
 *       也不会在 TCB 中包含额外的平台上下文成员。
 */
#pragma once

#ifndef STRATOS_POLICY_CORTEX_M3_STM32F1_PLATFORM_CONTEXT_HPP
#define STRATOS_POLICY_CORTEX_M3_STM32F1_PLATFORM_CONTEXT_HPP

namespace strat_os::hal::policy::builtin
{

/**
 * @brief Cortex-M3 STM32F1xx 平台上下文策略（无硬件上下文）
 *
 * 该策略声明 STM32F1xx 系列芯片不需要保存/恢复额外的硬件上下文。
 * 因此，supports_platform_context 为 false，但需要定义 platform_context_type
 * 不需要save、restore 等成员。
 *
 * 调度器在任务切换时会通过 `if constexpr (!supports_platform_context)` 跳过
 * 平台上下文的保存/恢复操作，从而不产生任何代码开销。
 */
struct CortexM3Stm32F1PlatformContextPolicy {
    /// 表示该平台不支持额外的硬件上下文
    static constexpr bool supports_platform_context = false;

    /// 空基类占位符
    struct platform_context_type {};
};

} // namespace strat_os::hal::policy::builtin

#endif // STRATOS_POLICY_CORTEX_M3_STM32F1_PLATFORM_CONTEXT_HPP