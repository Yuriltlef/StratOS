/**
 * @file layout.hpp
 * @author StratOS Team
 * @brief 内存布局策略适配器（Layout）
 * @version 1.0.0
 * @date 2026-04-29
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件定义了内存布局（Layout）的策略适配器。布局策略负责描述一块
 * 连续内存区域的物理基址和大小，不关心内存的管理方式（分配算法）。
 *
 * 布局策略类必须提供以下静态常量：
 * - base : std::uintptr_t 类型的起始地址
 * - size : std::size_t 类型的区域大小（字节）
 *
 * 适配器模板 `MemoryLayout` 会进行编译期验证，确保策略满足要求，
 * 并暴露 base 和 size 常量供上层（如 Region）使用。
 *
 * 该设计符合 StratOS 的静态策略模式，用户可定义不同的布局策略
 * （例如绝对地址、相对偏移、由链接脚本符号定义等），并保证零开销。
 */
#pragma once

#ifndef STRATOS_KERNEL_LAYOUT_HPP
#define STRATOS_KERNEL_LAYOUT_HPP

#include <cstdint>
#include <type_traits>
#include "os_kernel/include/core/common_traits.hpp"

namespace strat_os::kernel
{

/**
 * @brief 内存布局适配器模板
 * @tparam MemoryLayoutPolicy 具体的布局策略类，必须提供 base 和 size 静态常量
 *
 * 该类将布局策略包装为统一接口，并进行编译期验证。
 * 它仅暴露 base 和 size 常量，不包含任何运行时方法。
 *
 * @par 使用示例
 * @code
 * struct MyLayoutPolicy {
 *     static constexpr std::uintptr_t base = 0x20000000;
 *     static constexpr std::size_t size = 0x1000;
 * };
 * using MyLayout = MemoryLayout<MyLayoutPolicy>;
 *
 * constexpr auto addr = MyLayout::base;   // 0x20000000
 * constexpr auto len  = MyLayout::size;   // 4096
 * @endcode
 *
 * @note 策略类可以是普通类、结构体，甚至可以是全局常量对象，
 *       但通常定义为纯静态结构体。
 */
template <typename MemoryLayoutPolicy,
          typename = std::enable_if_t<traits::is_valid_layout_policy_v<MemoryLayoutPolicy>>>
struct MemoryLayout {
    /// 原始布局策略类型
    using Policy = MemoryLayoutPolicy;

    static_assert(traits::has_layout_base_v<Policy>, "MemoryLayoutPolicy must provide 'base' constant");
    static_assert(traits::is_valid_layout_base_type_v<Policy>,
                  "MemoryLayoutPolicy::base must be of type std::uintptr_t");
    static_assert(traits::has_layout_size_v<Policy>, "MemoryLayoutPolicy must provide 'size' constant");
    static_assert(traits::is_valid_layout_size_type_v<Policy>, "MemoryLayoutPolicy::size must be of type std::size_t");

    /// 内存区域的起始地址（编译期常量）
    static constexpr std::uintptr_t base = Policy::base;
    /// 内存区域的总大小（字节，编译期常量）
    static constexpr std::size_t size = Policy::size;
};

} // namespace strat_os::kernel

#endif // STRATOS_KERNEL_LAYOUT_HPP