/**
 * @file layout.hpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2026-04-14
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#ifndef STRATOS_POLICY_KERNEL_LAYOUT_HPP
#define STRATOS_POLICY_KERNEL_LAYOUT_HPP

#include <cstddef> // for std::size_t
#include <cstdint> // for std::uintptr_t
#include "os_kernel/include/core/common_traits.hpp"

namespace strat_os::kernel::policy::builtin
{

template <typename ParentRegion, std::uintptr_t offset, std::size_t mem_size>
struct RelativeLayoutPolicy {
    static_assert(::strat_os::kernel::traits::is_region_v<ParentRegion>, "ParentRegion must be a valid MemoryRegion");
    using parent_layout = typename ParentRegion::layout;
    static constexpr std::uintptr_t base = parent_layout::base + offset;
    static constexpr std::size_t size    = mem_size;
};

template <std::uintptr_t mem_base, std::size_t mem_size>
struct AbsoluteLayoutPolicy {
    static constexpr std::uintptr_t base = mem_base;
    static constexpr std::size_t size    = mem_size;
};

} // namespace strat_os::kernel::policy::builtin

#endif // STRATOS_POLICY_KERNEL_LAYOUT_HPP