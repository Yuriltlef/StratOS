/**
 * @file static_pool.hpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2026-04-30
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#ifndef STRATOS_POLICY_KERNEL_STATIC_POOL_HPP
#define STRATOS_POLICY_KERNEL_STATIC_POOL_HPP

#include "os_kernel/include/core/memory/region.hpp"
#include <cstddef> // for std::size_t
#include <cstdint> // for std::uintptr_t

namespace strat_os::kernel::policy::builtin
{
template <typename LayoutPolicy, typename ModePolicy>
struct StaticPoolPolicy {
    using layout_policy                  = LayoutPolicy;
    using mode_policy                    = ModePolicy;
    using region                         = MemoryRegion<layout_policy, mode_policy>;

    static constexpr std::uintptr_t base = region::layout::base;
    static constexpr std::size_t size    = region::layout::size;

    using size_type                      = std::size_t;
    using difference_type                = std::ptrdiff_t;
    using pointer                        = void*;
    using const_pointer                  = const void*;

    static constexpr bool is_dynamic     = false; // 静态池不支持动态分配

    static_assert(!region::mode::is_dynamic,
                  "StaticPoolPolicy requires a non-dynamic region (region::mode::is_dynamic must be false)");

    template <typename T>
    [[nodiscard]] constexpr inline static T* allocate(const std::size_t count = 1) noexcept {
        return nullptr;
    }

    constexpr inline static void* allocate(const std::size_t size) noexcept {
        return nullptr;
    }

    constexpr inline static void deallocate(void* ptr) noexcept {
        return;
    }

    template <typename T>
    [[nodiscard]] constexpr inline static T* deallocate(const std::size_t count = 1) noexcept {
        return nullptr;
    }
};

} // namespace strat_os::kernel::policy::builtin

#endif // STRATOS_POLICY_KERNEL_STATIC_POOL_HPP