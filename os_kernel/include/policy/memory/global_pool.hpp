/**
 * @file global_pool.hpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2026-05-07
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#ifndef STRATOS_POLICY_KERNEL_GLOBAL_POOL_HPP
#define STRATOS_POLICY_KERNEL_GLOBAL_POOL_HPP

#include "os_kernel/include/core/memory/region.hpp"
#include "os_kernel/include/policy/memory/layout.hpp"

#include <cstdint> // for std::uintptr_t, std::uint8_t

namespace strat_os::kernel::policy::builtin
{
template <std::uintptr_t GlobalBase, std::size_t GlobalSize>
using GlobalMemoryLayoutPolicy = AbsoluteLayoutPolicy<GlobalBase, GlobalSize>;

template <bool IsDynamic>
struct GlobalMemoryModePolicy {
    static constexpr bool is_dynamic = IsDynamic;
};

template <std::uintptr_t GlobalBase, std::size_t GlobalSize, bool IsDynamic = false>
using GlobalRegion = MemoryRegion<GlobalMemoryLayoutPolicy<GlobalBase, GlobalSize>, GlobalMemoryModePolicy<IsDynamic>>;

template <std::uintptr_t GlobalBase, std::size_t GlobalSize, bool IsDynamic = false>
struct GlobalPoolPolicy {
    using region = GlobalRegion<GlobalBase, GlobalSize, IsDynamic>;

    static constexpr bool is_dynamic = region::mode::is_dynamic;

    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using pointer         = void*;
    using const_pointer   = const void*;

    alignas(std::max_align_t) static std::uint8_t pool[GlobalSize]{};

    inline static void* allocate(std::size_t size) noexcept {
        if constexpr (is_dynamic) {
            return nullptr; // 这里需要实现对应的分配逻辑，返回指向可用内存块的指针
        }
        return nullptr; // 返回 nullptr 表示分配失败
    }

    inline static void deallocate(void* ptr) noexcept {
        if constexpr (is_dynamic) {
            return; // 这里需要实现对应的释放逻辑，标记该块为可用
        }
        return; // 这里需要实现对应的释放逻辑，标记该块为可用
    }
};

} // namespace strat_os::kernel::policy::builtin

#endif // STRATOS_POLICY_KERNEL_GLOBAL_POOL_HPP