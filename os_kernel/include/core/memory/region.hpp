/**
 * @file region.hpp
 * @author StratOS Team
 * @brief 内存区域适配器（Region）
 * @version 1.0.0
 * @date 2026-04-29
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件定义了内存区域（Region）的策略适配器。区域将一个布局（Layout）
 * 和一个模式（Mode）组合在一起，形成一个完整的内存区域描述。
 *
 * Region 本身不提供分配操作，而是作为元数据容器，供上层内存池使用。
 * 它可以用于定义系统中的各个独立内存域（例如内核专用区、用户堆区、DMA 区）。
 *
 * 适配器模板 `MemoryRegion` 接受布局策略和模式策略，并验证两者的有效性，
 * 然后提供 layout 和 mode 两个嵌套类型。
 *
 * 用户也可以通过继承或组合使用 Region，获得编译期类型安全。
 *
 * @par 使用示例
 * @code
 * struct MyLayout {
 *     static constexpr uintptr_t base = 0x20000000;
 *     static constexpr size_t size = 0x2000;
 * };
 * struct MyMode {
 *     static constexpr bool is_dynamic = false;
 *     using allocator_type = StaticAllocator;
 * };
 *
 * using MyRegion = MemoryRegion<MyLayout, MyMode>;
 *
 * constexpr auto base = MyRegion::layout::base;
 * constexpr bool dyn = MyRegion::mode::is_dynamic;
 * @endcode
 *
 * @note Region 本身不要求布局和模式策略来自 `MemoryLayout` 和 `MemoryMode` 适配器，
 *       只要是合法的布局策略和模式策略即可。但通常推荐使用上述适配器来包装原始策略。
 */
#pragma once

#ifndef STRATOS_KERNEL_REGION_HPP
#define STRATOS_KERNEL_REGION_HPP

#include "os_kernel/include/core/memory/layout.hpp"
#include "os_kernel/include/core/memory/mode.hpp"

namespace strat_os::kernel
{

/**
 * @brief 内存区域适配器模板
 * @tparam LayoutPolicy 布局策略类（必须提供 base 和 size）
 * @tparam ModePolicy    模式策略类（必须提供 is_dynamic，可选 allocator_type）
 *
 * 该类将布局和模式组合为一个独立的类型，并通过静态断言验证两者的有效性。
 * 它对外暴露 `layout` 和 `mode` 两个嵌套类型，便于上层查询元数据。
 *
 * @note 该模板不进行任何运行时操作，只用于编译期类型组合。
 *       所有成员均为类型别名或静态常量，零开销。
 */
template <typename LayoutPolicy, typename ModePolicy>
struct MemoryRegion {
    /// 布局策略
    using layout_policy = LayoutPolicy;
    /// 模式策略
    using mode_policy = ModePolicy;
    /// 布局
    using layout = MemoryLayout<LayoutPolicy>;
    /// 模式
    using mode = MemoryMode<ModePolicy>;
};

} // namespace strat_os::kernel

#endif // STRATOS_KERNEL_REGION_HPP