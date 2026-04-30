/**
 * @file mode.hpp
 * @author StratOS Team
 * @brief 内存模式策略适配器（Mode）
 * @version 1.1.0
 * @date 2026-04-29
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件定义了内存模式（Mode）的策略适配器。模式策略用于描述内存区域的
 * 分配行为特征，主要区分是否为动态分配模式。
 *
 * 模式策略类必须提供以下静态常量：
 * - is_dynamic : static constexpr bool，表示是否支持动态分配（true 表示动态，false 表示静态）。
 *
 * 适配器模板 `MemoryMode` 会进行编译期验证，并暴露 is_dynamic 常量。
 *
 * 该设计符合 StratOS 的静态策略模式，用户可自定义不同模式
 * （纯静态、混合、完全动态），并保证零开销。
 *
 * @note 分配器类型由具体的池策略（PoolPolicy）决定，不再通过 Mode 传递。
 */
#pragma once

#ifndef STRATOS_KERNEL_MODE_HPP
#define STRATOS_KERNEL_MODE_HPP

#include "os_kernel/include/core/common_traits.hpp"
#include <type_traits>

namespace strat_os::kernel
{

/**
 * @brief 内存模式适配器模板
 * @tparam MemoryModePolicy 具体的模式策略类，必须提供 is_dynamic 常量（bool）
 *
 * 该类将模式策略包装为统一接口，并进行编译期验证。
 * 它仅暴露 is_dynamic 常量。
 *
 * @par 使用示例
 * @code
 * // 纯静态模式
 * struct StaticModePolicy {
 *     static constexpr bool is_dynamic = false;
 * };
 * using StaticMode = MemoryMode<StaticModePolicy>;
 *
 * // 动态模式
 * struct DynamicModePolicy {
 *     static constexpr bool is_dynamic = true;
 * };
 * using DynamicMode = MemoryMode<DynamicModePolicy>;
 *
 * // 使用
 * constexpr bool dyn = DynamicMode::is_dynamic; // true
 * @endcode
 *
 * @note 分配器类型由具体的池策略（如 StaticPoolPolicy 或 DynamicPoolPolicy）
 *       自行定义和实现，模式策略仅提供静态/动态的元信息。
 */
template <typename MemoryModePolicy, typename = std::enable_if_t<traits::is_valid_mode_policy_v<MemoryModePolicy>>>
struct MemoryMode {
    /// 原始模式策略类型
    using Policy = MemoryModePolicy;

    static_assert(traits::has_is_dynamic_v<Policy>, "MemoryModePolicy must provide 'is_dynamic' constant");
    static_assert(traits::is_valid_is_dynamic_type_v<Policy>, "MemoryModePolicy::is_dynamic must be of type bool");

    /// 是否为动态分配模式（true: 动态, false: 静态）
    static constexpr bool is_dynamic = Policy::is_dynamic;
};

} // namespace strat_os::kernel

#endif // STRATOS_KERNEL_MODE_HPP